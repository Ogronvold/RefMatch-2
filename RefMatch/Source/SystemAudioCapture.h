#pragma once
#include <JuceHeader.h>
#include <memory>

class SystemAudioCapture
{
public:
    SystemAudioCapture();
    ~SystemAudioCapture();

    void start();
    void stop();
    bool isRunning() const;
    bool isStarting() const;
    juce::String getStatusText() const;
    juce::String getLastError() const;

    // Pulls system audio into dest for analysis only. The captured signal is never
    // routed to the plugin output.
    bool pullAudio(juce::AudioBuffer<float>& dest, double hostSampleRate);

    struct Impl;

private:
    std::shared_ptr<Impl> impl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemAudioCapture)
};
