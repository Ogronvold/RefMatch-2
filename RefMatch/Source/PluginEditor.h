#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpotifyAuth.h"
#include "SpotifyClient.h"

class AuroraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuroraLookAndFeel();

    void drawButtonBackground(
        juce::Graphics&,
        juce::Button&,
        const juce::Colour&,
        bool,
        bool) override;

    void drawToggleButton(
        juce::Graphics&,
        juce::ToggleButton&,
        bool,
        bool) override;
};

class RefMatchAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit RefMatchAudioProcessorEditor(
        RefMatchAudioProcessor&);

    ~RefMatchAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void refreshSpotifyPlayback();
    void timerCallback() override;
    void setABMode(int mode);

    RefMatchAudioProcessor& audioProcessor;
    AuroraLookAndFeel lookAndFeel;

    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::Label referenceTitle;
    juce::Label referenceName;

    juce::TextButton spotifyButton { "CONNECT SPOTIFY" };
    juce::Label spotifyStatus;

    juce::Label spotifyDevice;
    juce::Label spotifyNowPlaying;
    juce::TextButton spotifyPlayPause { "PLAY / PAUSE" };

    juce::TextButton loadButton { "LOAD REFERENCE" };
    juce::TextButton playButton { "PLAY" };

    juce::TextButton mixButton { "A  YOUR MIX" };
    juce::TextButton referenceButton { "B  REFERENCE" };

    juce::ToggleButton levelMatch { "LEVEL MATCH" };

    SpotifyAuth spotifyAuth;
    SpotifyClient spotifyClient;

    bool referencePlaying = false;
    int currentAB = 0;

    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        levelMatchAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        RefMatchAudioProcessorEditor)
};
