#include "PluginProcessor.h"
#include "PluginEditor.h"

RefMatchAudioProcessor::RefMatchAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParams())
{
    startTimerHz(50);
}

RefMatchAudioProcessor::~RefMatchAudioProcessor()
{
    stopTimer();
    // The controller cancels delivery to this processor on destruction.
    // An already-dispatched OS command cannot be recalled during plugin unload.
}

void RefMatchAudioProcessor::selectSource(bool reference)
{
    if (reference == isReferenceSelected()) return;
    if (reference && isTransportPending()) return;
    switchWithSystemMedia();
}

void RefMatchAudioProcessor::recordProfile(LearnCapture::Side side)
{
    if (learning.active()==side) { learning.stop(); learningStatus="Profile captured"; return; }
    if (side==LearnCapture::reference && !isReferenceCaptureRunning()) startReferenceCapture();
    selectSource(side==LearnCapture::reference);
    learning.start(side);
    learningStatus=side==LearnCapture::mix ? "Recording MIX - play your mix" : "Recording REF - play your reference";
}

void RefMatchAudioProcessor::switchWithSystemMedia()
{
    using Phase = TransportSwitch::Phase;
    // MIX is accepted even while a transport request is outstanding.
    if (isReferenceSelected() || transportSwitch.pending()) {
        if (transportSwitch.getPhase() == Phase::playing) {
            transportSwitch.requestMix();
            setReferenceSelected(false);
            return; // Completion queues PAUSE after PLAY, never concurrently.
        }
        if (transportSwitch.getPhase() == Phase::fadingOut) {
            transportSwitch.reset();
            setReferenceSelected(false);
            return;
        }
        if (transportSwitch.getPhase() == Phase::pausing) { setReferenceSelected(false); return; }
        if (mediaController.isBusy()) {
            setReferenceSelected(false);
            transportSwitch.requestMix(); // Timer sends PAUSE after status returns.
            return;
        }
        pauseMediaAndRestore();
        return;
    }
    transportError.clear();
    if (mediaController.isBusy()) { transportError = "Media player is busy. Try SWITCH again."; return; }
    transportSwitch.beginReference();
    setReferenceSelected(true);
    fadeDeadline = juce::Time::getMillisecondCounterHiRes() + 1000.0;
}

void RefMatchAudioProcessor::pauseMediaAndRestore()
{
    transportSwitch.requestMix();
    fadeDeadline = juce::Time::getMillisecondCounterHiRes() + 2000.0;
    mediaController.request(SystemMediaController::Command::pause,
        [this](bool ok, SystemMediaInfo, juce::String error) {
            setReferenceSelected(false);
            mediaStarted = false;
            transportSwitch.reset();
            if (!ok) transportError = error + " Pause Media player manually if it is still audible.";
        });
}

void RefMatchAudioProcessor::timerCallback()
{
    referenceLoop.setAuditioning(isReferenceSelected());
    const float amount=apvts.getRawParameterValue("matchamount")->load()/100.f;
    const float limit=apvts.getRawParameterValue("maxcorrection")->load();
    if(amount!=lastAmount || limit!=lastLimit || lastEQRate!=currentSampleRate) {
        lastAmount=amount;lastLimit=limit;lastEQRate=currentSampleRate;
        matchEQ.setAmount(amount);matchEQ.setMaxCorrectionDb(limit);matchEQ.refresh();
    }
    using Phase = TransportSwitch::Phase;
    if ((transportSwitch.getPhase() == Phase::playing || transportSwitch.getPhase() == Phase::pausing)
        && juce::Time::getMillisecondCounterHiRes() > fadeDeadline) {
        setReferenceSelected(false);
        if (transportSwitch.getPhase() == Phase::playing) transportSwitch.requestMix();
        transportError = "Media command delayed. MIX restored; pause the source manually if needed.";
    }
    if (!isReferenceSelected() && mediaStarted && !transportSwitch.pending())
        transportSwitch.requestMix();
    if (transportSwitch.getPhase() == Phase::pausing && !mediaController.isBusy()) {
        pauseMediaAndRestore();
        return;
    }
    if (transportSwitch.getPhase() != Phase::fadingOut) return;
    if (!isReferenceSelected()) { transportSwitch.reset(); return; }
    if (juce::Time::getMillisecondCounterHiRes() > fadeDeadline) {
        setReferenceSelected(false);
        transportSwitch.reset();
        transportError = "MIX restored: Logic did not finish the fade. Retry SWITCH.";
        return;
    }
    const bool audioInactive = juce::Time::getMillisecondCounterHiRes() - lastAudioCallbackMs.load() > 100.0;
    if (audioInactive) muteOnResume.store(true);
    if (!transportSwitch.fadeComplete(isMixMuted(), audioInactive)) return;
    fadeDeadline = juce::Time::getMillisecondCounterHiRes() + 2000.0;
    mediaController.request(SystemMediaController::Command::play,
        [this](bool ok, SystemMediaInfo, juce::String error) {
            const bool keepReference = transportSwitch.playComplete(ok,
                isReferenceSelected());
            mediaStarted = ok;
            if (!keepReference) {
                setReferenceSelected(false);
                if (!ok) transportError = error;
                pauseMediaAndRestore();
            }
        });
}

juce::AudioProcessorValueTreeState::ParameterLayout RefMatchAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterBool>("reference", "Reference", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sourcegain", "Mix Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("matchenabled", "Match EQ", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "matchamount", "Match Amount", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "maxcorrection", "Max Correction", juce::NormalisableRange<float>(0.5f, 12.0f, 0.1f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    return { params.begin(), params.end() };
}

void RefMatchAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int channels = juce::jmax(1, getTotalNumOutputChannels());
    referenceBuffer.setSize(channels, samplesPerBlock);
    dryMixBuffer.setSize(channels, samplesPerBlock);
    eqBuffer.setSize(channels,samplesPerBlock);
    eqWet.reset(sampleRate,.020);
    eqWet.setCurrentAndTargetValue(apvts.getRawParameterValue("matchenabled")->load());
    sourceAnalyser.reset();
    referenceAnalyser.reset();
    matchEQ.prepare(sampleRate, samplesPerBlock, channels);
    learning.prepare();
    sourcePeakSmooth.store(0.0f);
    referencePeakSmooth.store(0.0f);
    sourceRmsSmooth.store(0.0f);
    referenceRmsSmooth.store(0.0f);
    referenceAudioPresent.store(false);
    effectiveReference.store(false);
    referenceFade.prepare(sampleRate, isReferenceSelected());
}

void RefMatchAudioProcessor::releaseResources()
{
    stopReferenceCapture();
}

bool RefMatchAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != output) return false;
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

void RefMatchAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    lastAudioCallbackMs.store(juce::Time::getMillisecondCounterHiRes());
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0) return;

    dryMixBuffer.setSize(numChannels, numSamples, false, false, true);
    dryMixBuffer.makeCopyOf(buffer, true);
    sourceAnalyser.pushBlock(dryMixBuffer);

    float sourcePeak = 0.0f, sourceRms = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        sourcePeak = juce::jmax(sourcePeak, dryMixBuffer.getMagnitude(ch, 0, numSamples));
        sourceRms += dryMixBuffer.getRMSLevel(ch, 0, numSamples);
    }
    sourceRms /= (float)numChannels;
    sourcePeakSmooth.store(juce::jmax(sourcePeak, sourcePeakSmooth.load() * 0.93f));
    sourceRmsSmooth.store(0.985f * sourceRmsSmooth.load() + 0.015f * sourceRms);

    referenceBuffer.setSize(numChannels, numSamples, false, false, true);
    const bool gotReference = referenceCapture.pullAudio(referenceBuffer, currentSampleRate);
    referenceAudioPresent.store(gotReference);
    learning.push(dryMixBuffer,referenceBuffer,gotReference,currentSampleRate);

    if (gotReference)
    {
        referenceAnalyser.pushBlock(referenceBuffer);
        float refPeak = 0.0f, refRms = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            refPeak = juce::jmax(refPeak, referenceBuffer.getMagnitude(ch, 0, numSamples));
            refRms += referenceBuffer.getRMSLevel(ch, 0, numSamples);
        }
        refRms /= (float)numChannels;
        referencePeakSmooth.store(juce::jmax(refPeak, referencePeakSmooth.load() * 0.93f));
        referenceRmsSmooth.store(0.985f * referenceRmsSmooth.load() + 0.015f * refRms);
    }
    else
    {
        referencePeakSmooth.store(referencePeakSmooth.load() * 0.90f);
        referenceRmsSmooth.store(referenceRmsSmooth.load() * 0.98f);
    }

    const bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    if (!bypass)
    {
        const float sourceGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("sourcegain")->load());
        buffer.applyGain(sourceGain);

    }
    // Keep filter state warm and ramp the EQ bypass, avoiding a hard gain jump.
    eqBuffer.makeCopyOf(buffer,true);
    matchEQ.process(eqBuffer);
    eqWet.setTargetValue(!bypass && apvts.getRawParameterValue("matchenabled")->load()>.5f ? 1.f:0.f);
    for(int i=0;i<numSamples;++i) {
        const float wet=eqWet.getNextValue();
        for(int ch=0;ch<numChannels;++ch) {
            const float dry=buffer.getSample(ch,i);
            buffer.setSample(ch,i,dry+wet*(eqBuffer.getSample(ch,i)-dry));
        }
    }

    // Stream-AB style switching: B is heard directly from the external app.
    // RefMatch only fades/mutes the DAW signal. Captured system audio is used
    // exclusively for analysis and is never routed through the plugin output.
    const bool reference = isReferenceSelected();
    if (muteOnResume.exchange(false) && reference)
        referenceFade.prepare(currentSampleRate, true);
    effectiveReference.store(reference);
    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = referenceFade.next(reference);
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
    }
    mixMuted.store(referenceFade.muted());

}

void RefMatchAudioProcessor::startReferenceCapture() { referenceCapture.start(); }
void RefMatchAudioProcessor::stopReferenceCapture() { referenceCapture.stop(); }
bool RefMatchAudioProcessor::isReferenceCaptureRunning() const { return referenceCapture.isRunning(); }
bool RefMatchAudioProcessor::isReferenceCaptureStarting() const { return referenceCapture.isStarting(); }
juce::String RefMatchAudioProcessor::getReferenceCaptureStatus() const { return referenceCapture.getStatusText(); }

void RefMatchAudioProcessor::setReferenceSelected(bool shouldSelectReference)
{
    if (shouldSelectReference) {
        referenceFailure.store(false);
        mixMuted.store(false); // Require a fresh audio callback before issuing PLAY.
        muteOnResume.store(false);
    }
    if (auto* p = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("reference")))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(shouldSelectReference ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

void RefMatchAudioProcessor::toggleSource() { setReferenceSelected(!isReferenceSelected()); }

bool RefMatchAudioProcessor::isReferenceSelected() const
{
    // Control state must remain switchable when Logic suspends audio callbacks.
    // effectiveReference is an audio-thread result, not the user selection.
    return apvts.getRawParameterValue("reference")->load() > 0.5f;
}

void RefMatchAudioProcessor::autoGainMatch()
{
    const float src = sourceRmsSmooth.load();
    const float ref = referenceRmsSmooth.load();
    if (src <= 1.0e-6f || ref <= 1.0e-6f) return;

    const float db = juce::jlimit(-24.0f, 24.0f, juce::Decibels::gainToDecibels(ref / src, -24.0f));
    if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("sourcegain")))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(db));
        p->endChangeGesture();
    }
}

void RefMatchAudioProcessor::learnMatch()
{
    learning.stop();
    const auto a=learning.get(LearnCapture::mix),b=learning.get(LearnCapture::reference);
    if(!a.ready || !b.ready) { learningStatus="Record both MIX and REF first (at least 0.5 s of audio).";return; }
    auto remap=[this](const LearnCapture::Profile& p) {
        std::array<float,SpectrumAnalyser::bins> result{};
        for(int i=0;i<SpectrumAnalyser::bins;++i) {
            const double source=i*currentSampleRate/p.sampleRate;
            const int j=std::clamp(int(source),0,SpectrumAnalyser::bins-1),k=std::min(j+1,SpectrumAnalyser::bins-1);
            result[i]=p.db[j]+float(std::clamp(source-j,0.,1.))*(p.db[k]-p.db[j]);
        }return result;
    };
    matchEQ.setAmount(apvts.getRawParameterValue("matchamount")->load()/100.f);
    matchEQ.setMaxCorrectionDb(apvts.getRawParameterValue("maxcorrection")->load());
    matchEQ.learn(remap(a),remap(b),currentSampleRate);
    apvts.state.setProperty("hasLearnedMatch",true,nullptr);
    apvts.getParameter("matchenabled")->setValueNotifyingHost(1.f);
    learningStatus="Match applied to MIX";
}

void RefMatchAudioProcessor::clearMatch()
{
    matchEQ.reset();
    learningStatus="EQ reset";
    apvts.getParameter("matchenabled")->setValueNotifyingHost(0.f);
    apvts.state.setProperty("hasLearnedMatch", false, nullptr);
}

std::vector<float> RefMatchAudioProcessor::getMatchCurveDb() const { return matchEQ.getCurveDb(); }
std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getSourceSpectrum() const { return sourceAnalyser.getAveragedMagnitudes(); }
std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getReferenceSpectrum() const { return referenceAnalyser.getAveragedMagnitudes(); }

std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getDifferenceSpectrum() const
{
    auto a = sourceAnalyser.getAveragedMagnitudes();
    auto b = referenceAnalyser.getAveragedMagnitudes();
    std::array<float, SpectrumAnalyser::bins> d {};
    for (size_t i = 0; i < d.size(); ++i)
        d[i] = juce::jlimit(-18.0f, 18.0f, b[i] - a[i]);
    return d;
}

float RefMatchAudioProcessor::getSourcePeakDb() const { return juce::Decibels::gainToDecibels(sourcePeakSmooth.load(), -100.0f); }
float RefMatchAudioProcessor::getReferencePeakDb() const { return juce::Decibels::gainToDecibels(referencePeakSmooth.load(), -100.0f); }
float RefMatchAudioProcessor::getSourceGainDb() const { return apvts.getRawParameterValue("sourcegain")->load(); }

void RefMatchAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    juce::Array<juce::var> gains;
    for(auto gain:matchEQ.getGains())gains.add(gain);
    state.setProperty("eqGains",juce::JSON::toString(juce::var(gains)),nullptr);
    for(auto side:{LearnCapture::mix,LearnCapture::reference}) {
        const auto p=learning.get(side);const juce::String prefix=side==LearnCapture::mix?"mixProfile":"refProfile";
        juce::Array<juce::var> spectrum;for(auto db:p.db)spectrum.add(db);
        state.setProperty(prefix,juce::JSON::toString(juce::var(spectrum)),nullptr);
        state.setProperty(prefix+"Rate",p.sampleRate,nullptr);
        state.setProperty(prefix+"Seconds",p.seconds,nullptr);
    }
    for (auto child : state)
        if (child.getProperty("id").toString() == "reference") child.setProperty("value", 0.0f, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void RefMatchAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) {
        auto state = juce::ValueTree::fromXml(*xml);
        for (auto child : state)
            if (child.getProperty("id").toString() == "reference") child.setProperty("value", 0.0f, nullptr);
        apvts.replaceState(state);
        EQDesign::Gains gains{};
        const auto value=juce::JSON::parse(state.getProperty("eqGains").toString());
        if(const auto* array=value.getArray())for(int i=0;i<std::min(array->size(),EQDesign::bands);++i) {
            const double v=double((*array)[i]);gains[i]=std::isfinite(v)?std::clamp(v,-12.,12.):0;
        }
        matchEQ.restoreGains(gains);
        learning.stop();
        for(auto side:{LearnCapture::mix,LearnCapture::reference}) {
            LearnCapture::Profile p;const juce::String prefix=side==LearnCapture::mix?"mixProfile":"refProfile";
            const auto value=juce::JSON::parse(state.getProperty(prefix).toString());
            if(const auto* array=value.getArray();array && array->size()==SpectrumAnalyser::bins) {
                for(int i=0;i<SpectrumAnalyser::bins;++i) {const float v=float((*array)[i]);p.db[i]=std::isfinite(v)?std::clamp(v,-120.f,30.f):-120.f;}
                p.sampleRate=std::clamp(double(state.getProperty(prefix+"Rate",48000.)),8000.,384000.);
                p.seconds=std::max(0.,double(state.getProperty(prefix+"Seconds",0.)));p.ready=p.seconds>=.5;
            }
            learning.restore(side,p);
        }
    }
}

juce::AudioProcessorEditor* RefMatchAudioProcessor::createEditor() { return new RefMatchAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RefMatchAudioProcessor(); }
