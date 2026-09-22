#include "ReferenceFade.h"
#include "TransportSwitch.h"
#include "EQDesign.h"
#include "LoopTiming.h"
#include <cstdlib>
#include <iostream>
#include <vector>

void check(bool condition, const char* message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

int main()
{
    check(!LoopTiming::missingExpired(1200,1000), "one missing callback does not disable loop");
    check(LoopTiming::missingExpired(4000,1000), "sustained missing metadata expires");
    check(LoopTiming::confirmsSeek(101.8,100,104,1800), "seek confirmation allows playback during delayed metadata");
    check(!LoopTiming::confirmsSeek(104.1,100,104,1800), "position beyond Out cannot confirm seek");
    check(!LoopTiming::confirmsSeek(99,100,104,1800), "position before In cannot confirm seek");
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
    TransportSwitch transport;
    check(transport.beginReference(), "begin Spotify REF");
    check(!transport.beginReference(), "duplicate REF does not issue extra PLAY");
    check(!transport.fadeComplete(false), "PLAY cannot precede completed mute");
    transport.requestMix();
    check(!transport.pending(), "cancel during fade sends no PLAY");
    check(transport.beginReference() && transport.fadeComplete(true), "mute authorises one PLAY");
    transport.requestMix();
    check(!transport.playComplete(true, true), "late successful PLAY after cancellation requires PAUSE");
    check(transport.getPhase() == TransportSwitch::Phase::pausing, "cancelled PLAY is cleaned up");
    transport.reset();
    check(transport.beginReference() && transport.fadeComplete(true), "retry after cleanup");
    check(!transport.playComplete(false, true), "failed PLAY never commits REF");
    transport.reset();
    check(transport.beginReference() && transport.fadeComplete(true), "new REF request");
    check(transport.playComplete(true, true) && !transport.pending(), "confirmed PLAY commits REF");
    transport.requestMix();
    check(transport.getPhase() == TransportSwitch::Phase::pausing, "return to MIX first requests PAUSE");
    transport.reset();
    check(transport.beginReference() && transport.fadeComplete(true), "cancelled-selection scenario");
    check(!transport.playComplete(true, false), "cancelled selection during PLAY must clean up media");
    transport.reset();
    check(transport.beginReference() && transport.fadeComplete(false, true),
          "stopped host can PLAY without audio callback acknowledgement");
    check(transport.playComplete(true, true), "stopped-host REF can commit");
    transport.requestMix();
    check(transport.getPhase() == TransportSwitch::Phase::pausing, "stopped host can return to MIX");
    ReferenceFade resumed;
    resumed.prepare(48000, true);
    check(resumed.next(true) == 0, "resuming audio in REF starts muted");
    check(resumed.next(false) > 0, "resumed MIX fades back up");
    EQDesign::Gains source{}, reference{};
    for(int i=0;i<EQDesign::bands;++i){source[i]=-40+i;reference[i]=source[i]+12;}
    const auto flat=EQDesign::fit(source,reference,48000,.3);
    for(auto gain:flat)check(std::abs(gain)<1.e-8,"level-only difference produces flat EQ");
    for(int i=0;i<EQDesign::bands;++i)reference[i]=source[i]+(i<EQDesign::bands/2?-3:3);
    const auto fitted=EQDesign::fit(source,reference,48000,0);
    const auto limited=EQDesign::scaled(fitted,1,4,48000);
    for(int i=0;i<180;++i){const double hz=20*std::pow(1000.,i/179.);double db=0;
        for(int b=0;b<EQDesign::bands;++b)db+=EQDesign::response(EQDesign::peak(48000,EQDesign::centre(b),limited[b]),hz,48000);
        check(std::isfinite(db)&&std::abs(db)<4.3,"bounded finite applied EQ response");}
    std::cout << "PASS: fade endpoints, continuity, sample rates, reversal, capture failure and recovery\n";
}
