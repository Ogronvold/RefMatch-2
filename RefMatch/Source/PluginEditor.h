#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpotifyAuth.h"
#include "SpotifyClient.h"

class AuroraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuroraLookAndFeel();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
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
    void setABMode(int mode);
    void refreshSpotifyPlayback();
    void runSpotifySearch();
    void updateReferenceTransportText();
    static juce::String formatTime(double seconds);

    RefMatchAudioProcessor& audioProcessor;
    AuroraLookAndFeel lookAndFeel;

    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::Label buildLabel;

    juce::Label localTitle;
    juce::Label referenceName;
    juce::TextButton loadButton { "LOAD FILE" };
    juce::TextButton localPlayButton { "PLAY" };
    juce::Slider referencePosition;
    juce::Label referenceTime;

    juce::TextButton mixButton { "A   YOUR MIX" };
    juce::TextButton referenceButton { "B   REFERENCE" };
    juce::ToggleButton levelMatch { "LEVEL MATCH" };

    juce::Label matchTitle;
    juce::TextButton learnButton { "LEARN" };
    juce::ToggleButton matchEnabled { "MATCH EQ" };
    juce::Slider matchAmount;
    juce::Label matchAmountLabel;
    juce::Slider maxCorrection;
    juce::Label maxCorrectionLabel;
    juce::TextButton resetMatchButton { "RESET" };
    juce::ToggleButton bypassButton { "BYPASS" };
    juce::Label metersLabel;

    juce::Label spotifyTitle;
    juce::TextButton spotifyButton { "CONNECT SPOTIFY" };
    juce::Label spotifyStatus;
    juce::Label spotifyDevice;
    juce::Label spotifyNowPlaying;
    juce::TextButton spotifyPlayPause { "PLAY" };
    juce::TextEditor spotifySearch;
    juce::TextButton spotifySearchButton { "SEARCH" };
    juce::ComboBox spotifyResults;
    juce::TextButton spotifyPlaySelected { "PLAY SELECTED" };
    juce::Label spotifyNote;

    SpotifyAuth spotifyAuth;
    SpotifyClient spotifyClient;
    std::vector<SpotifyTrackInfo> spotifyTracks;
    bool spotifyIsPlaying = false;
    int currentAB = 0;
    bool draggingReferencePosition = false;

    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> levelMatchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> matchEnabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> matchAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxCorrectionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefMatchAudioProcessorEditor)
};
