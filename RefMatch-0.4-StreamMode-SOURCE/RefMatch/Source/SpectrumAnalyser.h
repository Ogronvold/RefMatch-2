#pragma once
#include <JuceHeader.h>

class SpectrumAnalyser
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int bins     = fftSize / 2;

    SpectrumAnalyser();
    void reset();
    void pushBlock(const juce::AudioBuffer<float>& buffer);
    std::array<float, bins> getAveragedMagnitudes() const;

private:
    void runFFT();

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann, true };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, bins> smoothed {};
    int fifoIndex = 0;
    juce::SpinLock lock;
};
