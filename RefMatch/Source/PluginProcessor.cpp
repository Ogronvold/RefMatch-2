#include "PluginProcessor.h"
#include "PluginEditor.h"

RefMatchAudioProcessor::RefMatchAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParams())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
RefMatchAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(
        std::make_unique<juce::AudioParameterChoice>(
            "ab",
            "A/B",
            juce::StringArray { "A - Mix", "B - Reference" },
            0));

    params.push_back(
        std::make_unique<juce::AudioParameterBool>(
            "levelmatch",
            "Level Match",
            true));

    return { params.begin(), params.end() };
}

void RefMatchAudioProcessor::prepareToPlay(double sampleRate,
                                           int samplesPerBlock)
{
    reference.prepare(sampleRate, samplesPerBlock);
    referenceBuffer.setSize(2, samplesPerBlock);

    sourceRmsSmooth = 0.0f;
    referenceRmsSmooth = 0.0f;
}

void RefMatchAudioProcessor::releaseResources()
{
    reference.release();
}

bool RefMatchAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void RefMatchAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    referenceBuffer.setSize(
        buffer.getNumChannels(),
        numSamples,
        false,
        false,
        true);

    reference.getNextAudioBlock(referenceBuffer);

    float sourceRms = 0.0f;
    float referenceRms = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        sourceRms += buffer.getRMSLevel(ch, 0, numSamples);
        referenceRms += referenceBuffer.getRMSLevel(ch, 0, numSamples);
    }

    if (buffer.getNumChannels() > 0)
    {
        sourceRms /= (float) buffer.getNumChannels();
        referenceRms /= (float) buffer.getNumChannels();
    }

    sourceRmsSmooth =
        0.98f * sourceRmsSmooth + 0.02f * sourceRms;

    referenceRmsSmooth =
        0.98f * referenceRmsSmooth + 0.02f * referenceRms;

    const bool useReference =
        apvts.getRawParameterValue("ab")->load() > 0.5f;

    if (useReference)
    {
        const bool levelMatch =
            apvts.getRawParameterValue("levelmatch")->load() > 0.5f;

        if (levelMatch && referenceRmsSmooth > 1.0e-6f)
        {
            const float gain =
                juce::jlimit(
                    0.1f,
                    10.0f,
                    sourceRmsSmooth / referenceRmsSmooth);

            referenceBuffer.applyGain(gain);
        }

        buffer.makeCopyOf(referenceBuffer, true);
    }
}

bool RefMatchAudioProcessor::loadReference(const juce::File& file)
{
    return reference.loadFile(file);
}

void RefMatchAudioProcessor::setReferencePlaying(bool playing)
{
    reference.setPlaying(playing);
}

juce::String RefMatchAudioProcessor::getReferenceName() const
{
    return reference.getLoadedFileName();
}

void RefMatchAudioProcessor::getStateInformation(
    juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void RefMatchAudioProcessor::setStateInformation(
    const void* data,
    int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        apvts.replaceState(
            juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor*
RefMatchAudioProcessor::createEditor()
{
    return new RefMatchAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RefMatchAudioProcessor();
}
