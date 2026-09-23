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
    if (recording()==side) { learning.stop(); referenceAnalysis.learning.stop(); learningStatus="Profile captured"; return; }
    if (side==LearnCapture::reference && !isReferenceCaptureRunning()) startReferenceCapture();
    selectSource(side==LearnCapture::reference);
    learning.stop();referenceAnalysis.learning.stop();
    if(side==LearnCapture::reference)referenceAnalysis.learning.start(side);else learning.start(side);
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
    const float smooth=apvts.getRawParameterValue("smooth")->load()/100.f;
    if(smooth!=lastSmooth && recording()==LearnCapture::none) {
        lastSmooth=smooth;matchEQ.setSmoothing(smooth);
        if(hasMatch() && recording()==LearnCapture::none)recalculateMatch();
    }
    bool toneChanged=false;std::array<float,6> tone{};
    for(int i=0;i<3;++i) {
        const juce::String id="tone"+juce::String(i);
        tone[2*i]=apvts.getRawParameterValue(id+"gain")->load();tone[2*i+1]=apvts.getRawParameterValue(id+"freq")->load();
    }
    if(tone!=lastTone){lastTone=tone;toneChanged=true;matchEQ.setTone(tone);}
    const bool toneEnabled=apvts.getRawParameterValue("toneenabled")->load()>.5f;
    if(toneEnabled!=lastToneEnabled){lastToneEnabled=toneEnabled;toneChanged=true;matchEQ.setToneEnabled(toneEnabled);}
    const bool lowShelf=apvts.getRawParameterValue("tone0shelf")->load()>.5f;
    const bool highShelf=apvts.getRawParameterValue("tone2shelf")->load()>.5f;
    const float midQ=apvts.getRawParameterValue("tone1q")->load();
    if(lowShelf!=lastLowShelf || highShelf!=lastHighShelf){lastLowShelf=lowShelf;lastHighShelf=highShelf;toneChanged=true;matchEQ.setToneTypes(lowShelf,highShelf);}
    if(midQ!=lastMidQ){lastMidQ=midQ;toneChanged=true;matchEQ.setMidQ(midQ);}
    const float matchLow=apvts.getRawParameterValue("matchlow")->load();
    const float matchHigh=apvts.getRawParameterValue("matchhigh")->load();
    if(matchLow!=lastMatchLow || matchHigh!=lastMatchHigh){lastMatchLow=matchLow;lastMatchHigh=matchHigh;toneChanged=true;matchEQ.setMatchRange(matchLow,matchHigh);}
    if(toneChanged)matchEQ.refresh();
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
        "matchamount", "Match Amount", juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "maxcorrection", "Max Correction", juce::NormalisableRange<float>(0.5f, 12.0f, 0.1f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("smooth","Smooth",juce::NormalisableRange<float>(0,100,.1f),35));
    const std::array<std::pair<float,float>,3> toneRanges{{{30.f,300.f},{200.f,6000.f},{3000.f,20000.f}}};
    const std::array<float,3> toneDefaults{{120.f,1000.f,8000.f}};
    for(int i=0;i<3;++i) {
        const juce::String id="tone"+juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id+"gain","Tone "+juce::String(i+1)+" Gain",juce::NormalisableRange<float>(-6,6,.1f),0));
        juce::NormalisableRange<float> range(toneRanges[i].first,toneRanges[i].second,1);range.setSkewForCentre(toneDefaults[i]);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id+"freq","Tone "+juce::String(i+1)+" Frequency",range,toneDefaults[i]));
    }
    params.push_back(std::make_unique<juce::AudioParameterBool>("tone0shelf","Low Shelf",true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone1q","Mid Q",juce::NormalisableRange<float>(0.3f,4.0f,0.01f,0.45f),0.75f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("tone2shelf","High Shelf",true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("toneenabled","Tone EQ Enabled",true));
    juce::NormalisableRange<float> lowRange(20.f,1000.f,0.1f);lowRange.setSkewForCentre(120.f);
    juce::NormalisableRange<float> highRange(1000.f,20000.f,1.f);highRange.setSkewForCentre(12000.f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("matchlow","Match Low",lowRange,20.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("matchhigh","Match High",highRange,20000.f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("processingafter","Before After",true));
    return { params.begin(), params.end() };
}

void RefMatchAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int channels = juce::jmax(1, getTotalNumOutputChannels());
    dryMixBuffer.setSize(channels, samplesPerBlock);
    eqBuffer.setSize(channels,samplesPerBlock);
    eqWet.reset(sampleRate,.020);
    eqWet.setCurrentAndTargetValue(apvts.getRawParameterValue("matchenabled")->load());
    sourceAnalyser.reset();
    beforeEffectAnalyser.reset();
    afterEffectAnalyser.reset();
    matchEQ.prepare(sampleRate, samplesPerBlock, channels);
    learning.prepare();
    sourcePeakSmooth.store(0.0f);
    sourceRmsSmooth.store(0.0f);
    effectiveReference.store(false);
    referenceFade.prepare(sampleRate, isReferenceSelected());
}

void RefMatchAudioProcessor::releaseResources()
{
    // Keep external reference analysis alive while the host suspends processing.
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

    learning.push(dryMixBuffer,dryMixBuffer,false,currentSampleRate);

    const bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    if (!bypass)
    {
        const float sourceGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("sourcegain")->load());
        buffer.applyGain(sourceGain);

    }
    beforeEffectAnalyser.pushBlock(buffer);
    // Keep filter state warm and ramp the EQ bypass, avoiding a hard gain jump.
    eqBuffer.makeCopyOf(buffer,true);
    matchEQ.process(eqBuffer);
    eqWet.setTargetValue(!bypass && apvts.getRawParameterValue("processingafter")->load()>.5f && apvts.getRawParameterValue("matchenabled")->load()>.5f ? 1.f:0.f);
    for(int i=0;i<numSamples;++i) {
        const float wet=eqWet.getNextValue();
        for(int ch=0;ch<numChannels;++ch) {
            const float dry=buffer.getSample(ch,i);
            buffer.setSample(ch,i,dry+wet*(eqBuffer.getSample(ch,i)-dry));
        }
    }
    afterEffectAnalyser.pushBlock(buffer);

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

void RefMatchAudioProcessor::startReferenceCapture() { referenceAnalysis.capture.start(); }
void RefMatchAudioProcessor::stopReferenceCapture() { referenceAnalysis.capture.stop(); }
bool RefMatchAudioProcessor::isReferenceCaptureRunning() const { return referenceAnalysis.capture.isRunning(); }
bool RefMatchAudioProcessor::isReferenceCaptureStarting() const { return referenceAnalysis.capture.isStarting(); }
juce::String RefMatchAudioProcessor::getReferenceCaptureStatus() const { return referenceAnalysis.capture.getStatusText(); }

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
    const float ref = referenceAnalysis.rms.load();
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
    learning.stop();referenceAnalysis.learning.stop();
    const auto a=profile(LearnCapture::mix),b=profile(LearnCapture::reference);
    if(!a.ready || !b.ready) { learningStatus="Record both MIX and REF first. Recommended: at least 8 s of representative audio.";return; }
    recalculateMatch();
    apvts.state.setProperty("hasLearnedMatch",true,nullptr);
    apvts.getParameter("matchenabled")->setValueNotifyingHost(1.f);
    learningStatus="Match applied to MIX";
}

void RefMatchAudioProcessor::recalculateMatch()
{
    const auto a=profile(LearnCapture::mix),b=profile(LearnCapture::reference);
    if(!a.ready || !b.ready)return;
    auto remap=[this](const LearnCapture::Profile& p) {
        std::array<float,SpectrumAnalyser::bins> result{};
        for(int i=0;i<SpectrumAnalyser::bins;++i) {
            const double source=i*currentSampleRate/p.sampleRate;
            const int j=std::clamp(int(source),0,SpectrumAnalyser::bins-1),k=std::min(j+1,SpectrumAnalyser::bins-1);
            result[i]=p.db[j]+float(std::clamp(source-j,0.,1.))*(p.db[k]-p.db[j]);
        }return result;
    };
    matchEQ.setSmoothing(apvts.getRawParameterValue("smooth")->load()/100.f);
    matchEQ.setAmount(apvts.getRawParameterValue("matchamount")->load()/100.f);
    matchEQ.setMaxCorrectionDb(apvts.getRawParameterValue("maxcorrection")->load());
    matchEQ.setMatchRange(apvts.getRawParameterValue("matchlow")->load(),apvts.getRawParameterValue("matchhigh")->load());
    matchEQ.setToneTypes(apvts.getRawParameterValue("tone0shelf")->load()>.5f,apvts.getRawParameterValue("tone2shelf")->load()>.5f);
    matchEQ.setMidQ(apvts.getRawParameterValue("tone1q")->load());
    matchEQ.learn(remap(a),remap(b),currentSampleRate);
}

void RefMatchAudioProcessor::clearMatch()
{
    matchEQ.reset();
    learningStatus="EQ reset";
    apvts.getParameter("matchenabled")->setValueNotifyingHost(0.f);
    apvts.state.setProperty("hasLearnedMatch", false, nullptr);
}

std::vector<float> RefMatchAudioProcessor::getMatchCurveDb() const { return matchEQ.getCurveDb(); }
std::vector<float> RefMatchAudioProcessor::getMatchCurveDbAtAmount(float amount) const { return matchEQ.getCurveDb(amount); }
std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getSourceSpectrum() const { auto values=sourceAnalyser.getAveragedMagnitudes();if(juce::Time::getMillisecondCounterHiRes()-lastAudioCallbackMs.load()>200)values.fill(-100.f);return values; }
std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getReferenceSpectrum() const { auto values=referenceAnalysis.analyser.getAveragedMagnitudes();if(!referenceAnalysis.present.load())values.fill(-100.f);return values; }
std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getBeforeEffectSpectrum() const { auto values=beforeEffectAnalyser.getAveragedMagnitudes();if(juce::Time::getMillisecondCounterHiRes()-lastAudioCallbackMs.load()>200)values.fill(-100.f);return values; }
std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getAfterEffectSpectrum() const { auto values=afterEffectAnalyser.getAveragedMagnitudes();if(juce::Time::getMillisecondCounterHiRes()-lastAudioCallbackMs.load()>200)values.fill(-100.f);return values; }

std::array<float, SpectrumAnalyser::bins> RefMatchAudioProcessor::getDifferenceSpectrum() const
{
    auto a = sourceAnalyser.getAveragedMagnitudes();
    auto b = referenceAnalysis.analyser.getAveragedMagnitudes();
    std::array<float, SpectrumAnalyser::bins> d {};
    for (size_t i = 0; i < d.size(); ++i) {
        const int index=std::clamp(int(i*currentSampleRate.load()/ReferenceAnalysis::sampleRate),0,SpectrumAnalyser::bins-1);
        d[i] = juce::jlimit(-18.0f, 18.0f, b[index] - a[i]);
    }
    return d;
}

float RefMatchAudioProcessor::getSourcePeakDb() const { return juce::Decibels::gainToDecibels(juce::Time::getMillisecondCounterHiRes()-lastAudioCallbackMs.load()<200?sourcePeakSmooth.load():0.f, -100.0f); }
float RefMatchAudioProcessor::getReferencePeakDb() const { return juce::Decibels::gainToDecibels(referenceAnalysis.peak.load(), -100.0f); }
float RefMatchAudioProcessor::getSourceGainDb() const { return apvts.getRawParameterValue("sourcegain")->load(); }

void RefMatchAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    juce::Array<juce::var> gains;
    for(auto gain:matchEQ.getGains())gains.add(gain);
    state.setProperty("eqGains",juce::JSON::toString(juce::var(gains)),nullptr);
    for(auto side:{LearnCapture::mix,LearnCapture::reference}) {
        const auto p=profile(side);const juce::String prefix=side==LearnCapture::mix?"mixProfile":"refProfile";
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
        learning.stop();referenceAnalysis.learning.stop();
        for(auto side:{LearnCapture::mix,LearnCapture::reference}) {
            LearnCapture::Profile p;const juce::String prefix=side==LearnCapture::mix?"mixProfile":"refProfile";
            const auto value=juce::JSON::parse(state.getProperty(prefix).toString());
            if(const auto* array=value.getArray();array && array->size()==SpectrumAnalyser::bins) {
                for(int i=0;i<SpectrumAnalyser::bins;++i) {const float v=float((*array)[i]);p.db[i]=std::isfinite(v)?std::clamp(v,-120.f,30.f):-120.f;}
                p.sampleRate=std::clamp(double(state.getProperty(prefix+"Rate",48000.)),8000.,384000.);
                p.seconds=std::max(0.,double(state.getProperty(prefix+"Seconds",0.)));p.ready=p.seconds>=.5;
            }
            if(side==LearnCapture::reference)referenceAnalysis.learning.restore(side,p);else learning.restore(side,p);
        }
    }
}

juce::AudioProcessorEditor* RefMatchAudioProcessor::createEditor() { return new RefMatchAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RefMatchAudioProcessor(); }
