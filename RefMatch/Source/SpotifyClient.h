#pragma once

#include <JuceHeader.h>
#include <functional>

struct SpotifyTrackInfo
{
    juce::String name;
    juce::String artist;
    juce::String uri;
    juce::String album;
    juce::String imageUrl;
};

struct SpotifyPlaybackInfo
{
    bool isPlaying = false;
    juce::String deviceName;
    juce::String trackName;
    juce::String artistName;

    int progressMs = 0;
    int durationMs = 0;
};

class SpotifyClient
{
public:
    SpotifyClient() = default;

    void setAccessToken(const juce::String& token);

    void getPlaybackState(
        std::function<void(bool, SpotifyPlaybackInfo)> callback);

    void pause(
        std::function<void(bool)> callback);

    void resume(
        std::function<void(bool)> callback);

    void playTrack(
        const juce::String& trackUri,
        std::function<void(bool)> callback);

    void searchTracks(
        const juce::String& query,
        std::function<void(bool, std::vector<SpotifyTrackInfo>)> callback);

private:
    juce::String accessToken;

    std::unique_ptr<juce::InputStream> createRequest(
        const juce::String& endpoint,
        const juce::String& method = "GET",
        const juce::String& body = {});

    juce::String makeAuthHeader() const;
};
