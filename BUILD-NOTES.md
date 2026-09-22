# 0.5.2 validation

## Implemented

Immediate requested-state playback feedback, separate MediaRemote playback polling,
optional artwork-data decoding/cache, timeline selection and handles, manual seeking
and +/-5 seconds, removal of search/TIMED UI, aligned meters, shorter empty-profile labels,
and a full-correction-based EQ plot scale independent of the Amount slider.

MediaRemote symbols are dynamically resolved. Artwork support follows the published
community declarations at https://github.com/theos/headers/blob/master/MediaRemote/MediaRemote.h.
No third-party code or framework binaries were bundled.

## Local checks

Source UTF-8 and delimiter checks, CMake source-path checks and workflow shell syntax.
Archive integrity and required-file checks. Static UI coordinate overlap check.
A layout sketch is only a coordinate review, not native plugin rendering.

No C++ compilation, C++ test execution or native Logic/Spotify audition was possible:
xcode-select reports no active developer directory. A green earlier version does not
validate this candidate. GitHub Actions must build and run the updated tests.

## Regression tests included

Existing fade, transport, capture and EQ signal tests, plus:
- PLAY/PAUSE requested state is immediate; stale metadata cannot immediately undo it.
- Confirmation clears pending state, rejection rolls back, stale requests/state expire.
- Timeline reverse drag, minimum range at track end, bounds and coordinate mapping.
- EQ response continues increasing from 50% through 75% to 100%; its full-scale
  reference curve remains unchanged by Amount.

## Required in Logic

1. Both tests/build pass; install 0.5.2. Verify A/B/SWITCH still mute/start and pause/restore.
2. Click PLAY/PAUSE, including rapid changes; check prompt label response and reconciliation.
   Also pause in Spotify and check reported state. Missing status must never display PLAY/PAUSE.
3. Confirm title/artist, actual supplied artwork, placeholder when unavailable and no old
   cover after switching tracks. Cover decoding is cached, not performed per audio block.
4. Timeline: drag both directions, resize each edge, select near the end, click to seek,
   skip repeatedly, enable/disable, switch A/B, change song, and temporarily lose metadata.
5. Confirm loop handles and numeric times agree, including fractional seconds. Loop selection
   is at least 0.5 s; playback seek is not sample-accurate and depends on the external player.
6. Record MIX/REF and MATCH; move Amount from 0 to 100% with a strong correction. Scale stays
   fixed for that correction and the graph continues moving beyond 50%. Verify audible EQ.
7. Save/reopen profiles and test sample-rate changes. Native UI size and typography need testing.

## Boundaries

Transport controls the current system media player, not an authenticated Spotify session.
Playback feedback is initially requested state, not proof of sound; tooltip identifies it.
Private API metadata/cover/seek may be absent or delayed. Timeline is a time ruler, not a
waveform of the full Spotify track. Loop is processor-owned but not sample-accurate;
state/ranges are not persisted across plugin recreation. One controlling instance.
MIX/REF learning depends on host audio callbacks; existing analyser realtime limitations
remain. Broad 20-band EQ matches tonal balance, not identical sound or Apple's exact algorithm.
