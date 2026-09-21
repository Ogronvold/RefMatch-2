#include "MatchEQ.h"

void MatchEQ::prepare(double sampleRate, int, int channels)
{
    sr = sampleRate;
    filters.clear();
    filters.resize((size_t) channels);
    reset();
}

void MatchEQ::reset()
{
    bandGainsDb.fill(0.0f);
    for (auto& channel : filters)
        for (auto& f : channel) f.reset();
    updateFilters();
}

void MatchEQ::setZones(std::vector<Zone> z)
{
    zones = std::move(z);
    updateFilters();
}

bool MatchEQ::frequencyAllowed(float hz) const
{
    for (const auto& z : zones)
        if (z.enabled && hz >= z.lowHz && hz <= z.highHz) return true;
    return false;
}

void MatchEQ::learn(const std::array<float, SpectrumAnalyser::bins>& source,
                    const std::array<float, SpectrumAnalyser::bins>& reference,
                    double sampleRate)
{
    curveDb.assign(SpectrumAnalyser::bins, 0.0f);
    const float maxDb = maxCorrectionDb.load();

    for (int i = 1; i < SpectrumAnalyser::bins; ++i)
    {
        const float hz = (float) i * (float) sampleRate / (float) SpectrumAnalyser::fftSize;
        float diff = reference[(size_t) i] - source[(size_t) i];
        diff = juce::jlimit(-maxDb, maxDb, diff);
        curveDb[(size_t) i] = frequencyAllowed(hz) ? diff : 0.0f;
    }

    // Broad-band musical approximation of the learned difference curve.
    for (int b = 0; b < numBands; ++b)
    {
        const float centre = centres[(size_t) b];
        const float lo = centre / 1.45f, hi = centre * 1.45f;
        float sum = 0.0f; int count = 0;
        for (int i = 1; i < SpectrumAnalyser::bins; ++i)
        {
            const float hz = (float) i * (float) sampleRate / (float) SpectrumAnalyser::fftSize;
            if (hz >= lo && hz <= hi) { sum += curveDb[(size_t) i]; ++count; }
        }
        bandGainsDb[(size_t) b] = count > 0 ? sum / (float) count : 0.0f;
    }
    updateFilters();
}

void MatchEQ::updateFilters()
{
    const float amt = amount.load();
    for (size_t ch = 0; ch < filters.size(); ++ch)
    {
        for (int b = 0; b < numBands; ++b)
        {
            const float hz = centres[(size_t) b];
            const float db = frequencyAllowed(hz) ? bandGainsDb[(size_t) b] * amt : 0.0f;
            const float gain = juce::Decibels::decibelsToGain(db);
            filters[ch][(size_t) b].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, hz, 0.8f, gain);
        }
    }
}

void MatchEQ::process(juce::AudioBuffer<float>& buffer)
{
    updateFilters();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), (int) filters.size()); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            for (auto& f : filters[(size_t) ch]) x = f.processSample(x);
            data[i] = x;
        }
    }
}
