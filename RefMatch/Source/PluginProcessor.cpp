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

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ab", "A/B", juce::StringArray { "A - Mix", "B - Reference" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "levelmatch", "Level Match", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "matchenabled", "Match EQ", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "matchamount", "Match Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "maxcorrection", "Max Correction",
        juce::NormalisableRange<float>(0.5f, 12.0f, 0.1f), 4.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "bypass", "Bypass", false));

    return { params.begin(), params.end() };
}

void RefMatchAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    reference.prepare(sampleRate, samplesPerBlock);
    referenceBuffer.setSize(juce::jmax(2, getTotalNumOutputChannels()), samplesPerBlock);

    sourceAnalyser.reset();
    referenceAnalyser.reset();
    matchEQ.prepare(sampleRate, samplesPerBlock, juce::jmax(1, getTotalNumOutputChannels()));
    sourceRmsSmooth.store(0.0f);
    referenceRmsSmooth.store(0.0f);
}

void RefMatchAudioProcessor::releaseResources()
{
    reference.release();
    referencePlaying.store(false);
}

bool RefMatchAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void RefMatchAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0)
        return;

    referenceBuffer.setSize(numChannels, numSamples, false, false, true);
    reference.getNextAudioBlock(referenceBuffer);

    sourceAnalyser.pushBlock(buffer);
    referenceAnalyser.pushBlock(referenceBuffer);

    float sourceRms = 0.0f;
    float refRms = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        sourceRms += buffer.getRMSLevel(ch, 0, numSamples);
        refRms += referenceBuffer.getRMSLevel(ch, 0, numSamples);
    }

    sourceRms /= (float) numChannels;
    refRms /= (float) numChannels;

    sourceRmsSmooth.store(0.98f * sourceRmsSmooth.load() + 0.02f * sourceRms);
    referenceRmsSmooth.store(0.98f * referenceRmsSmooth.load() + 0.02f * refRms);

    const bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    const bool useReference = apvts.getRawParameterValue("ab")->load() > 0.5f;

    if (useReference)
    {
        if (apvts.getRawParameterValue("levelmatch")->load() > 0.5f)
        {
            const float smoothedRef = referenceRmsSmooth.load();
            if (smoothedRef > 1.0e-6f)
            {
                const float gain = juce::jlimit(0.1f, 10.0f,
                    sourceRmsSmooth.load() / smoothedRef);
                referenceBuffer.applyGain(gain);
            }
        }

        buffer.makeCopyOf(referenceBuffer, true);
        return;
    }

    if (!bypass && apvts.getRawParameterValue("matchenabled")->load() > 0.5f)
    {
        matchEQ.setAmount(apvts.getRawParameterValue("matchamount")->load() / 100.0f);
        matchEQ.setMaxCorrectionDb(apvts.getRawParameterValue("maxcorrection")->load());
        matchEQ.process(buffer);
    }
}

bool RefMatchAudioProcessor::loadReference(const juce::File& file)
{
    if (!reference.loadFile(file))
        return false;

    apvts.state.setProperty("referencePath", file.getFullPathName(), nullptr);
    referenceAnalyser.reset();
    return true;
}

void RefMatchAudioProcessor::setReferencePlaying(bool playing)
{
    reference.setPlaying(playing);
    referencePlaying.store(playing);
}

juce::String RefMatchAudioProcessor::getReferenceName() const
{
    return reference.getLoadedFileName();
}

double RefMatchAudioProcessor::getReferencePositionSeconds() const
{
    return reference.getPositionSeconds();
}

double RefMatchAudioProcessor::getReferenceLengthSeconds() const
{
    return reference.getLengthSeconds();
}

void RefMatchAudioProcessor::setReferencePositionSeconds(double seconds)
{
    reference.setPositionSeconds(seconds);
}

void RefMatchAudioProcessor::learnMatch()
{
    matchEQ.setMaxCorrectionDb(apvts.getRawParameterValue("maxcorrection")->load());
    matchEQ.learn(sourceAnalyser.getAveragedMagnitudes(),
                  referenceAnalyser.getAveragedMagnitudes(),
                  currentSampleRate);
    apvts.state.setProperty("hasLearnedMatch", true, nullptr);
}

void RefMatchAudioProcessor::clearMatch()
{
    matchEQ.reset();
    apvts.state.setProperty("hasLearnedMatch", false, nullptr);
}

std::vector<float> RefMatchAudioProcessor::getMatchCurveDb() const
{
    return matchEQ.getCurveDb();
}

float RefMatchAudioProcessor::getSourceLevelDb() const
{
    return juce::Decibels::gainToDecibels(sourceRmsSmooth.load(), -100.0f);
}

float RefMatchAudioProcessor::getReferenceLevelDb() const
{
    return juce::Decibels::gainToDecibels(referenceRmsSmooth.load(), -100.0f);
}

void RefMatchAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void RefMatchAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

        const auto path = apvts.state.getProperty("referencePath").toString();
        if (path.isNotEmpty())
        {
            const juce::File file(path);
            if (file.existsAsFile())
                reference.loadFile(file);
        }
    }
}

juce::AudioProcessorEditor* RefMatchAudioProcessor::createEditor()
{
    return new RefMatchAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RefMatchAudioProcessor();
}
