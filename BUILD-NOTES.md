# 0.5.0 validation and remaining checks

## Basis

The user confirmed that 0.4.4 switches Spotify and MIX correctly. That transport is
retained; new A/B buttons select its requested state. No mute regression was inferred
from the earlier report after the user identified host bypass as the cause.

## Completed locally

- Reviewed source for thread ownership of learning, EQ coefficients and filter state.
- Checked all source text as UTF-8, with balanced delimiters and existing CMake paths.
- Checked the extracted workflow shell with bash syntax validation.
- Inspected a rendered layout sketch at the new control coordinates. It is a design
  preview, not a screenshot of a running native plugin. Native UI remains untested.
- Probed MRMediaRemoteGetNowPlayingInfo from the bundled Python process: callback
  returned no dictionary. This does not establish what Logic will return, but supports
  exposing metadata failure and a clearly labelled opt-in Timed-loop fallback.
- Checked final ZIP integrity, required sources/tests/workflow, and no helper bundles.

## Not run

This Mac still lacks an Apple developer toolchain. 0.5.0 was not compiled locally.
The C++ suites below were added but have not executed. No native UI rendering,
Logic EQ audition, profile save/reload or real Spotify looping was performed.
Earlier green builds do not validate these new changes.

## Automated checks included in GitHub Actions

RefMatchRuntime: existing 20 ms fade/transport tests, level-normalisation producing
flat EQ, bounded finite EQ response.

RefMatchDSP: JUCE-based signal test that records separate MIX/REF spectra, verifies
opposite-phase stereo capture and silence rejection, checks frozen profile retention,
preserves gains through prepare, measures actual 3 dB filter gain on a sine signal,
and confirms zero Amount passes signal transparently.

## Required Logic acceptance

1. Build and both tests pass. Confirm 0.5.0 and compact tab heights on a real display.
2. A, B and SWITCH both ways, playing/stopped host, editor close, optional meters off.
3. RECORD MIX/STOP and RECORD REF/STOP from Spotify. Both frozen profiles should
   remain available. Confirm the count advances only while audio processing occurs.
4. MATCH automatically enables EQ. On A, compare EQ ON/OFF and Amount 0/100%; graph
   should correspond to tonal change, and Limit should constrain the curve.
5. Save/reopen project, reopen editor and change sample rate: captures/gains persist.
   Recording is stopped on session load; MIX is restored. No unintentional Spotify PLAY.
6. Test no audio, denied capture, source quit, very quiet material, mono and stereo.
7. Loop known In/Out in Spotify. Verify mode must disable on missing positions, track
   change or failed seek. Timed mode is explicitly unverified and can drift if the
   source buffers, pauses externally or ignores seek. It must not block A/B.
8. Loop disabled before track switch/removing plugin; test return to A and back to B.

## Limits

Broad 20-band tonal matching is not algorithmically identical to Apple's Match EQ.
The FFT limits bass resolution. Existing live analyser locks and oversized host-buffer
allocation behavior are retained; the full processor is not certified hard realtime.
Ref learning depends on host audio callbacks. Private MediaRemote position/seek may
be unavailable or ignored. Timed loop cannot guarantee the right section without
successful seek and should not be presented as verified. One controlling instance.
