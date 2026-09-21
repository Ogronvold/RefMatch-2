#include "PluginProcessor.h"
#include "PluginEditor.h"

RefMatchAudioProcessor::RefMatchAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParams())
{
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
    sourceAnalyser.reset();
    referenceAnalyser.reset();
    matchEQ.prepare(sampleRate, samplesPerBlock, channels);
    sourcePeakSmooth.store(0.0f);
    referencePeakSmooth.store(0.0f);
    sourceRmsSmooth.store(0.0f);
    referenceRmsSmooth.store(0.0f);
    referenceAudioPresent.store(false);
    muteCrossfade = apvts.getRawParameterValue("reference")->load() > 0.5f ? 1.0f : 0.0f;
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

        if (apvts.getRawParameterValue("matchenabled")->load() > 0.5f)
        {
            matchEQ.setAmount(apvts.getRawParameterValue("matchamount")->load() / 100.0f);
            matchEQ.setMaxCorrectionDb(apvts.getRawParameterValue("maxcorrection")->load());
            matchEQ.process(buffer);
        }
    }

    // Stream-AB style switching: B is heard directly from the external app.
    // RefMatch only fades/mutes the DAW signal. Captured system audio is used
    // exclusively for analysis and is never routed through the plugin output.
    const float target = apvts.getRawParameterValue("reference")->load() > 0.5f ? 1.0f : 0.0f;
    const float fadeSamples = (float)juce::jmax(1.0, currentSampleRate * 0.020);
    const float step = 1.0f / fadeSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        if (muteCrossfade < target) muteCrossfade = juce::jmin(target, muteCrossfade + step);
        else if (muteCrossfade > target) muteCrossfade = juce::jmax(target, muteCrossfade - step);

        const float mixGain = std::cos(muteCrossfade * juce::MathConstants<float>::halfPi);
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * mixGain);
    }
}

void RefMatchAudioProcessor::startReferenceCapture() { referenceCapture.start(); }
void RefMatchAudioProcessor::stopReferenceCapture() { referenceCapture.stop(); }
bool RefMatchAudioProcessor::isReferenceCaptureRunning() const { return referenceCapture.isRunning(); }
bool RefMatchAudioProcessor::isReferenceCaptureStarting() const { return referenceCapture.isStarting(); }
juce::String RefMatchAudioProcessor::getReferenceCaptureStatus() const { return referenceCapture.getStatusText(); }

void RefMatchAudioProcessor::setReferenceSelected(bool shouldSelectReference)
{
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
    matchEQ.setMaxCorrectionDb(apvts.getRawParameterValue("maxcorrection")->load());
    matchEQ.learn(sourceAnalyser.getAveragedMagnitudes(), referenceAnalyser.getAveragedMagnitudes(), currentSampleRate);
    apvts.state.setProperty("hasLearnedMatch", true, nullptr);
}

void RefMatchAudioProcessor::clearMatch()
{
    matchEQ.reset();
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
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void RefMatchAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* RefMatchAudioProcessor::createEditor() { return new RefMatchAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RefMatchAudioProcessor(); }
