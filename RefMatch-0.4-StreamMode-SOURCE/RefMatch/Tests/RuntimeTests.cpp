#include "ReferenceFade.h"
#include "SwitchState.h"
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
    SwitchState state;
    check(!state.begin(false, false), "unapproved Spotify never mutes mix");
    check(state.begin(false, true), "authorised reference request");
    check(!state.begin(false, true), "rapid duplicate clicks ignored");
    check(!state.fadeReady(false, false), "wait for mix fade");
    check(state.fadeReady(true, false), "muted mix allows Spotify play");
    check(!state.complete(false) && !state.pending(), "denial or timeout restores mix");
    check(state.begin(false, true) && state.fadeReady(false, true), "stopped DAW cannot stall switch");
    check(state.complete(true), "successful play commits reference");
    check(state.begin(true, false), "return to MIX remains possible after permission loss");
    check(!state.complete(false), "pause failure still restores MIX");
    check(state.begin(true, true) && !state.complete(true), "successful pause restores MIX");
    std::cout << "PASS: fade endpoints, continuity, sample rates, reversal, permissions, failure recovery and duplicate clicks\n";
}
