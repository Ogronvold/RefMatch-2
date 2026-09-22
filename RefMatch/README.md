# 0.5.0 implementation notes

The confirmed 0.4.4 system-media transport and MIX fade are retained. Explicit A/B
selection is idempotent and uses that coordinator. Capture and looping do not gate
transport. Editor closure does not stop processor-owned learning or loop timers.
Removing the plugin still cannot recall an already-dispatched native media command.

LearnCapture accumulates linear power independently per stereo channel before
averaging channels. Opposite-phase stereo therefore does not cancel. Only one side
records at a time; previously captured opposite-side data remains frozen. Silent
FFT frames are excluded. UI starts/stops are communicated atomically, and publishing
uses a try-lock so the learning code never waits for the UI in the audio callback.
The stored profiles are serialised as JSON strings in the APVTS XML state alongside
sample rate and duration. Profiles are interpolated if sample rates differ at MATCH.

MatchEQ replaces live-spectrum matching and per-block allocated JUCE filter objects.
EQDesign fits a gain-normalised, smoothed spectral difference using a fixed 20-band
peaking bank. Amount scales learned gains; Limit scales overall response. Filters
use fixed storage, coefficient design happens outside the audio callback and changes
are interpolated. Filter state is audio-owned; prepare does not erase learned gains.
EQ keeps processing while bypassed to avoid stale filter history and uses a 20 ms
wet transition. The gain/coefficients are retained across host prepare calls and
session restore. The response graph uses those same coefficient equations.

This is a broad tonal match, not a clone of Logic's Match EQ algorithm. Low-frequency
resolution is limited by the 4096-sample FFT. Unrelated system sounds can contaminate
REF; mute notifications/other sources during capture. For meaningful comparison,
record similar musical sections. REF measurement still consumes audio on Logic's
processBlock cadence; stopped/suspended processing does not learn new material.

ReferenceLoop dynamically resolves optional MRMediaRemoteGetNowPlayingInfo and
MRMediaRemoteSetElapsedTime. Metadata requests time out after one second and do not
use the A/B transport busy flag. Verified loop checks track identity and seek response;
TIMED mode uses the entered interval and deliberately labels seeking unverified.
System APIs are private and can change. An API command being delivered does not prove
that a player acted. No private adapter/helper or entitlement workaround is bundled.

Primary references for behavior/API investigation:
- https://support.apple.com/en-au/guide/logicpro/lgcef1edc5c5/mac
- https://github.com/theos/headers/blob/master/MediaRemote/MediaRemote.h
- https://github.com/ungive/mediaremote-adapter

No Stream AB implementation or assets are copied. Source is based on RefMatch 0.4.4.
