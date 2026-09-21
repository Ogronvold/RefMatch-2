#pragma once
#include <JuceHeader.h>

class ReferenceEngine
{
public:
    ReferenceEngine();
    ~ReferenceEngine();

    void prepare(double sampleRate, int blockSize);
    void release();
    bool loadFile(const juce::File& file);
    void setPlaying(bool shouldPlay);
    void setPositionSeconds(double seconds);
    double getPositionSeconds() const;
    double getLengthSeconds() const;
    void getNextAudioBlock(juce::AudioBuffer<float>& dst);
    juce::String getLoadedFileName() const;

private:
    juce::AudioFormatManager formats;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;
    juce::String fileName;
};
