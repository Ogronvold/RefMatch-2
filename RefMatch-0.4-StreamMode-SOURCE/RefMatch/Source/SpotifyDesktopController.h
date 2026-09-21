#pragma once
#include <JuceHeader.h>

struct SpotifyDesktopInfo
{
    bool installed = false;
    bool running = false;
    bool playing = false;
    juce::String track;
    juce::String artist;
    double positionSeconds = 0.0;
    double durationSeconds = 0.0;
};

class SpotifyDesktopController
{
public:
    SpotifyDesktopController() = default;
    ~SpotifyDesktopController() = default;

    bool requestControlPermission(juce::String& errorMessage);
    bool play(juce::String& errorMessage);
    bool pause(juce::String& errorMessage);
    bool playPause(juce::String& errorMessage);
    bool next(juce::String& errorMessage);
    bool previous(juce::String& errorMessage);
    SpotifyDesktopInfo getInfo(juce::String& errorMessage);
    bool openSearch(const juce::String& query, juce::String& errorMessage);

private:
    bool runCommand(const juce::String& body, juce::String* result, juce::String& errorMessage);
};
