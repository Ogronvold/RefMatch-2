#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>

struct SystemMediaInfo
{
    bool installed = false, running = false, playing = false, authorised = false;
    juce::String track, artist;
    double positionSeconds = 0.0, durationSeconds = 0.0;
};

// System media-session commands; serial worker, never the audio thread.
class SystemMediaController
{
public:
    enum class Command { authorise, status, play, pause, playPause, next, previous };
    using Completion = std::function<void(bool, SystemMediaInfo, juce::String)>;
    SystemMediaController();
    ~SystemMediaController();
    void request(Command, Completion);
    bool isBusy() const;
    bool openSearch(const juce::String&, juce::String& error);
    void openAutomationSettings();
private:
    struct Impl;
    std::shared_ptr<Impl> impl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemMediaController)
};
