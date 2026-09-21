#include "ReferenceFade.h"
#include "ReferenceSafety.h"
#include <cstdlib>
#include <iostream>
#include <vector>

void check(bool condition, const char* message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        ReferenceFade fade;
        fade.prepare(rate, false);
        float last = 1.0f;
        const int frames = static_cast<int>(rate * .020) + 2;
        for (int i = 0; i < frames; ++i)
        {
            const float gain = fade.next(true);
            check(std::isfinite(gain) && gain >= 0 && gain <= 1, "finite normalised fade");
            check(gain <= last, "fade out is monotonic");
            check(std::abs(gain - last) < .003f, "no abrupt fade step");
            last = gain;
        }
        check(fade.muted() && last == 0, "reference ends at exact silence");
        for (int i = 0; i < frames; ++i)
        {
            const float gain = fade.next(false);
            check(gain >= last && gain <= 1, "fade in is monotonic");
            last = gain;
        }
        check(last == 1, "mix restored to unity");
        for (int i = 0; i < frames / 2; ++i) last = fade.next(true);
        check(std::abs(fade.next(false) - last) < .003f, "mid-fade reversal is continuous");
    }
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0}) {
        ReferenceSafety safety;
        check(!safety.update(true, false, false, 512, rate), "stopped capture must preserve MIX");
        check(!safety.update(true, true, true, 512, rate), "failure stays latched after capture recovers");
        safety.reset();
        check(safety.update(true, true, true, 512, rate), "explicit retry with capture enters REF");
        check(safety.update(true, true, false, 512, rate), "short packet gap tolerates scheduling jitter");
        for (int i = 0; i < static_cast<int>(rate / 512) + 2; ++i)
            safety.update(true, true, false, 512, rate);
        check(!safety.update(true, true, true, 512, rate), "one second starvation latches MIX");
        check(!safety.update(false, true, true, 512, rate), "MIX always available");
        check(safety.update(true, true, true, 512, rate), "MIX rearms the next reference selection");
        for (int i = 0; i < 1000; ++i)
            check(safety.update(true, true, true, 512, rate), "silent valid packets do not trigger failure");
    }
    std::cout << "PASS: fade endpoints, continuity, sample rates, reversal, capture failure and recovery\n";
}
