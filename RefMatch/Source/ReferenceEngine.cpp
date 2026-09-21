#include "ReferenceEngine.h"

ReferenceEngine::ReferenceEngine() { formats.registerBasicFormats(); }
ReferenceEngine::~ReferenceEngine() { release(); }

void ReferenceEngine::prepare(double sampleRate, int blockSize)
{
    transport.prepareToPlay(blockSize, sampleRate);
}

void ReferenceEngine::release()
{
    transport.stop();
    transport.setSource(nullptr);
    readerSource.reset();
    transport.releaseResources();
}

bool ReferenceEngine::loadFile(const juce::File& file)
{
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file));
    if (!reader) return false;

    transport.stop();
    transport.setSource(nullptr);
    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    transport.setSource(readerSource.get(), 0, nullptr, readerSource->getAudioFormatReader()->sampleRate);
    fileName = file.getFileName();
    return true;
}

void ReferenceEngine::setPlaying(bool shouldPlay) { shouldPlay ? transport.start() : transport.stop(); }
void ReferenceEngine::setPositionSeconds(double seconds) { transport.setPosition(seconds); }
double ReferenceEngine::getPositionSeconds() const { return transport.getCurrentPosition(); }
double ReferenceEngine::getLengthSeconds() const { return transport.getLengthInSeconds(); }
juce::String ReferenceEngine::getLoadedFileName() const { return fileName; }

void ReferenceEngine::getNextAudioBlock(juce::AudioBuffer<float>& dst)
{
    dst.clear();
    juce::AudioSourceChannelInfo info(&dst, 0, dst.getNumSamples());
    transport.getNextAudioBlock(info);
}
