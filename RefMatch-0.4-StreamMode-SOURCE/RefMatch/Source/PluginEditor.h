#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpotifyDesktopController.h"
#include "SwitchState.h"

class RefMatchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RefMatchLookAndFeel();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider&) override;
};

class RefMatchAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit RefMatchAudioProcessorEditor(RefMatchAudioProcessor&);
    ~RefMatchAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void switchSource();
    void enableSpotifyControl();
    void openSpotifySearch();
    void updateSpotifyInfo();
    void runSpotifyCommand(SpotifyDesktopController::Command, bool switching = false);
    void applySpotifyInfo(const SpotifyDesktopInfo&);
    void showSpotifyError(const juce::String&);

    void updateSourceState();
    void drawSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void drawPeakMeter(juce::Graphics&, juce::Rectangle<float>, float, const juce::String&, juce::Colour);
    void drawPill(juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour, bool);

    RefMatchAudioProcessor& audioProcessor;
    RefMatchLookAndFeel lookAndFeel;
    SpotifyDesktopController spotify;
    SpotifyDesktopInfo spotifyInfo;

    juce::Label logoLabel;
    juce::Label versionLabel;
    juce::Label straplineLabel;

    juce::Label trackLabel;
    juce::Label artistLabel;
    juce::Label spotifyStateLabel;
    juce::Label captureStateLabel;
    juce::TextButton enableSpotifyButton { "CONNECT SPOTIFY" };
    juce::TextButton permissionButton { "PERMISSIONS" };
    juce::TextButton playPauseButton { "PLAY / PAUSE" };
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextEditor searchBox;
    juce::TextButton searchButton { "OPEN IN SPOTIFY" };

    juce::TextButton switchButton { "SWITCH" };
    juce::Label listeningLabel;
    juce::TextButton autoGainButton { "AUTO MATCH" };
    juce::Label gainValueLabel;

    juce::Label matchTitleLabel;
    juce::TextButton learnButton { "LEARN" };
    juce::TextButton resetButton { "RESET" };
    juce::ToggleButton matchToggle { "MATCH EQ" };
    juce::ToggleButton bypassToggle { "BYPASS" };
    juce::Slider sourceGainSlider;
    juce::Slider matchAmountSlider;
    juce::Slider maxCorrectionSlider;
    juce::Label sourceGainLabel;
    juce::Label matchAmountLabel;
    juce::Label maxCorrectionLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sourceGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> matchEnabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> matchAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxCorrectionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    int timerTicks = 0;
    bool spotifyControlReady = false;
    bool spotifyError = false;
    SwitchState switchState;
    double fadeStarted = 0.0;
    juce::TooltipWindow tooltips { this, 650 };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefMatchAudioProcessorEditor)
};
