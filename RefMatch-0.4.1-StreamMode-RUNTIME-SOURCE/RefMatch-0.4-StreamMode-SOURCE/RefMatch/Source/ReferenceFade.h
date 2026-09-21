#pragma once
#include <algorithm>
#include <cmath>

// The reference stays outside the plugin. Only the DAW signal is faded;
// this is deliberately not a claim of a sample-synchronised Spotify crossfade.
class ReferenceFade
{
public:
    void prepare(double sampleRate, bool reference)
    {
        step = 1.0f / static_cast<float>(std::max(1.0, sampleRate * 0.020));
        position = reference ? 1.0f : 0.0f;
    }
    float next(bool reference)
    {
        position = reference ? std::min(1.0f, position + step)
                             : std::max(0.0f, position - step);
        if (position >= 1.0f) return 0.0f;
        if (position <= 0.0f) return 1.0f;
        return std::cos(position * 1.57079632679489661923f);
    }
    bool muted() const { return position >= 1.0f; }
private:
    float position = 0.0f, step = 1.0f;
};
