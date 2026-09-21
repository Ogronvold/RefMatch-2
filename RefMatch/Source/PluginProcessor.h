#pragma once

#include <JuceHeader.h>
#include "ReferenceEngine.h"
#include "SpectrumAnalyser.h"
#include "MatchEQ.h"

class RefMatchAudioProcessor : public juce::AudioProcessor
{
public:
    RefMatchAudioProcessor();
    ~RefMatchAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool loadReference(const juce::File&);
    void setReferencePlaying(bool);
    bool isReferencePlaying() const { return referencePlaying.load(); }
    juce::String getReferenceName() const;
    double getReferencePositionSeconds() const;
    double getReferenceLengthSeconds() const;
    void setReferencePositionSeconds(double seconds);

    void learnMatch();
    void clearMatch();
    std::vector<float> getMatchCurveDb() const;

    float getSourceLevelDb() const;
    float getReferenceLevelDb() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    ReferenceEngine reference;
    juce::AudioBuffer<float> referenceBuffer;
    SpectrumAnalyser sourceAnalyser;
    SpectrumAnalyser referenceAnalyser;
    MatchEQ matchEQ;

    std::atomic<bool> referencePlaying { false };
    std::atomic<float> sourceRmsSmooth { 0.0f };
    std::atomic<float> referenceRmsSmooth { 0.0f };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefMatchAudioProcessor)
};
