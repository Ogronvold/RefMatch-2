#include "SpectrumAnalyser.h"

SpectrumAnalyser::SpectrumAnalyser() { reset(); }

void SpectrumAnalyser::reset()
{
    const juce::SpinLock::ScopedLockType sl(lock);
    fftData.fill(0.0f);
    smoothed.fill(-100.0f);
    fifoIndex = 0;
}

void SpectrumAnalyser::pushBlock(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() == 0) return;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            mono += buffer.getSample(ch, i);
        mono /= (float) buffer.getNumChannels();

        fftData[(size_t) fifoIndex++] = mono;
        if (fifoIndex >= fftSize)
        {
            runFFT();
            fifoIndex = 0;
        }
    }
}

void SpectrumAnalyser::runFFT()
{
    auto local = fftData;
    window.multiplyWithWindowingTable(local.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(local.data());

    const juce::SpinLock::ScopedLockType sl(lock);
    for (int i = 0; i < bins; ++i)
    {
        const auto mag = juce::jmax(local[(size_t) i] / (float) fftSize, 1.0e-9f);
        const auto db = juce::Decibels::gainToDecibels(mag);
        smoothed[(size_t) i] = 0.85f * smoothed[(size_t) i] + 0.15f * db;
    }
}

std::array<float, SpectrumAnalyser::bins> SpectrumAnalyser::getAveragedMagnitudes() const
{
    const juce::SpinLock::ScopedLockType sl(const_cast<juce::SpinLock&>(lock));
    return smoothed;
}
