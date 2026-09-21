#pragma once

#include <JuceHeader.h>
#include "SpectrumAnalyser.h"
#include "MatchEQ.h"
#include "SystemAudioCapture.h"
#include "ReferenceFade.h"
#include "SystemMediaController.h"
#include "TransportSwitch.h"

class RefMatchAudioProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    RefMatchAudioProcessor();
    ~RefMatchAudioProcessor() override;

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
    void switchWithSystemMedia();
    bool isTransportPending() const { return transportSwitch.pending(); }
    juce::String getTransportError() const { return transportError; }
    SystemMediaController& getMediaController() { return mediaController; }
    bool isMixMuted() const { return mixMuted.load(); }
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
    bool hasReferenceFailure() const { return referenceFailure.load(); }
    bool hasReferenceAudio() const { return referenceAudioPresent.load(); }
    double getSampleRateForDisplay() const { return currentSampleRate; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    void timerCallback() override;
    void pauseMediaAndRestore();
    SystemMediaController mediaController;
    TransportSwitch transportSwitch;
    juce::String transportError;
    double fadeDeadline = 0;
    bool mediaStarted = false;
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
    ReferenceFade referenceFade;
    std::atomic<bool> referenceFailure { false };
    std::atomic<double> lastAudioCallbackMs { 0.0 };
    std::atomic<bool> muteOnResume { false };
    std::atomic<bool> effectiveReference { false };
    std::atomic<bool> mixMuted { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefMatchAudioProcessor)
};
