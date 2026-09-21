# 0.4.4 validation

## Implemented

System media-session transport replaces Spotify Apple Events. Capture is optional
for A/B. Stopped-host switching has a mute-on-resume path. Existing 20 ms fade,
transport cancellation and late-response cleanup remain. Two-second pending-command
watchdog restores MIX without requiring the native operation to return.

## Actually checked locally

- Loaded the system MediaRemote framework and resolved MRMediaRemoteSendCommand.
  No command was sent; this is a symbol check, not a transport/Logic test.
- Source UTF-8, delimiter balance, CMake source paths and plugin-only formats.
- Workflow shell syntax and corrected lipo argument order.
- ZIP integrity and required source/test/workflow files; no helper/app bundles.

## Not tested

No active Apple developer directory exists on this Mac. 0.4.4 has not compiled,
its expanded C++ tests have not executed, and Logic/native transport has not been
tested. Earlier 0.4.2/0.4.3 build results do not certify 0.4.4.
The C++ test suite covers fade continuity, explicit play/pause phase sequencing,
duplicate clicks, cancellation, late PLAY cleanup, failed PLAY, stopped-host
transition, and audio resuming in REF. Run it using the included GitHub workflow.

## Required acceptance test

Build and CTest green; version 0.4.4 visible; no helper; Spotify source primed then
paused; A/B both ways with Logic running and stopped; browser and Music source;
capture disabled/denied while A/B still works; rapid reversal; editor close during
switch; audio resumes muted in REF; multiple block sizes/sample rates; unavailable
MediaRemote and failed command fallback; changed system media target.

## Material limits

Private API stability and host access are not guaranteed. Accepted commands may be
ignored by the target. We intentionally do not block A/B on restricted metadata or
capture availability. This trades verified player-state confirmation for generic
media-session control. A source must previously have established a media session.
Only one RefMatch instance should control it. Plugin unload cannot recall a pending
OS command. External audio is never routed through RefMatch, so it cannot be muted
by the plugin if the player ignores PAUSE. Source output and DAW output must use the
intended listening device. Unrelated system sounds can enter the optional analyser.
