#pragma once

#include <JuceHeader.h>
#include "SpectrumAnalyser.h"
#include "MatchEQ.h"
#include "SystemAudioCapture.h"

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

    void startReferenceCapture();
    void stopReferenceCapture();
    bool isReferenceCaptureRunning() const;
    bool isReferenceCaptureStarting() const;
    juce::String getReferenceCaptureStatus() const;

    void setReferenceSelected(bool);
    void toggleSource();
    bool isReferenceSelected() const;

    void autoGainMatch();
    void learnMatch();
    void clearMatch();
    std::vector<float> getMatchCurveDb() const;

    std::array<float, SpectrumAnalyser::bins> getSourceSpectrum() const;
    std::array<float, SpectrumAnalyser::bins> getReferenceSpectrum() const;
    std::array<float, SpectrumAnalyser::bins> getDifferenceSpectrum() const;

    float getSourcePeakDb() const;
    float getReferencePeakDb() const;
    float getSourceGainDb() const;
    bool hasReferenceAudio() const { return referenceAudioPresent.load(); }
    double getSampleRateForDisplay() const { return currentSampleRate; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    SystemAudioCapture referenceCapture;
    juce::AudioBuffer<float> referenceBuffer;
    juce::AudioBuffer<float> dryMixBuffer;
    SpectrumAnalyser sourceAnalyser;
    SpectrumAnalyser referenceAnalyser;
    MatchEQ matchEQ;

    std::atomic<float> sourcePeakSmooth { 0.0f };
    std::atomic<float> referencePeakSmooth { 0.0f };
    std::atomic<float> sourceRmsSmooth { 0.0f };
    std::atomic<float> referenceRmsSmooth { 0.0f };
    std::atomic<bool> referenceAudioPresent { false };

    double currentSampleRate = 48000.0;
    float muteCrossfade = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefMatchAudioProcessor)
};
