#pragma once

#include <JuceHeader.h>
#include "SpectrumAnalyser.h"
#include "MatchEQ.h"
#include "LearnCapture.h"
#include "SystemAudioCapture.h"
#include "ReferenceFade.h"
#include "SystemMediaController.h"
#include "TransportSwitch.h"
#include "ReferenceLoop.h"
#include "ReferenceAnalysis.h"

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

    void selectSource(bool reference);
    void recordProfile(LearnCapture::Side side);
    LearnCapture::Side recording() const { return referenceAnalysis.learning.active()==LearnCapture::reference?LearnCapture::reference:learning.active(); }
    LearnCapture::Profile profile(LearnCapture::Side side) const { return side==LearnCapture::reference?referenceAnalysis.learning.get(side):learning.get(side); }
    juce::String getLearningStatus() const { return learningStatus; }
    bool hasMatch() const { return apvts.state.getProperty("hasLearnedMatch",false); }
    ReferenceLoop& getLoop() { return referenceLoop; }
    void autoGainMatch();
    void learnMatch();
    void clearMatch();
    std::vector<float> getToneCurveDb() const { return matchEQ.getCurveDb(0.f); }
    std::vector<float> getFullMatchCurveDb() const { return matchEQ.getCurveDb(1.f); }
    std::vector<float> getMatchCurveDb() const;

    std::array<float, SpectrumAnalyser::bins> getSourceSpectrum() const;
    std::array<float, SpectrumAnalyser::bins> getReferenceSpectrum() const;
    std::array<float, SpectrumAnalyser::bins> getDifferenceSpectrum() const;

    float getSourcePeakDb() const;
    float getReferencePeakDb() const;
    float getSourceGainDb() const;
    bool hasReferenceFailure() const { return referenceFailure.load(); }
    bool hasReferenceAudio() const { return referenceAnalysis.present.load(); }
    double getSampleRateForDisplay() const { return currentSampleRate; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    void timerCallback() override;
    void pauseMediaAndRestore();
    void recalculateMatch();
    SystemMediaController mediaController;
    ReferenceLoop referenceLoop {mediaController};
    TransportSwitch transportSwitch;
    juce::String transportError;
    double fadeDeadline = 0;
    bool mediaStarted = false;
    ReferenceAnalysis referenceAnalysis;
    juce::AudioBuffer<float> dryMixBuffer;
    juce::AudioBuffer<float> eqBuffer;
    juce::SmoothedValue<float> eqWet;
    SpectrumAnalyser sourceAnalyser;
    MatchEQ matchEQ;
    LearnCapture learning;
    juce::String learningStatus {"Record MIX and REF, then press MATCH"};
    float lastAmount=-1,lastLimit=-1,lastSmooth=-1;
    std::array<float,6> lastTone{{-999,-999,-999,-999,-999,-999}};
    double lastEQRate=0;

    std::atomic<float> sourcePeakSmooth { 0.0f };
    std::atomic<float> sourceRmsSmooth { 0.0f };

    std::atomic<double> currentSampleRate {48000.0};
    ReferenceFade referenceFade;
    std::atomic<bool> referenceFailure { false };
    std::atomic<double> lastAudioCallbackMs { 0.0 };
    std::atomic<bool> muteOnResume { false };
    std::atomic<bool> effectiveReference { false };
    std::atomic<bool> mixMuted { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefMatchAudioProcessor)
};
