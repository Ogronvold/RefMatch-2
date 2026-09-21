# RefMatch 0.4.4 — System Media A/B

AU/VST3 plugin only. SWITCH uses the active macOS media session instead of Spotify
Automation. There is no CONNECT requirement and no helper or standalone app.

- REF: fade MIX out over 20 ms and send system PLAY.
- MIX: send system PAUSE and fade MIX back in.
- Stopped host: allow REF without waiting for an audio callback; resumed audio
  starts muted if REF is still selected.
- Metering: ENABLE METERS starts optional ScreenCaptureKit analysis. Denied or
  unavailable capture does not block A/B.

Play the desired source once in Spotify, Music or the browser so macOS knows which
media session should receive commands. Then pause it and use SWITCH. System media
routing is controlled by macOS; switching to another player can change the target.

MediaRemote is a private API. This implementation dynamically resolves
MRMediaRemoteSendCommand and checks its return value. Acceptance is not an
acknowledgement of audible playback. A detected failure restores MIX; an accepted
but ignored command cannot be identified without separate player-state information.
No claim of sample-synchronous external switching is made.

## Update

Extract this ZIP. Upload its RefMatch folder to the repository root, replacing
matching files. Replace .github/workflows/main.yml with the included workflow.
Do not upload the enclosing version folder. Build with GitHub Actions, then install
the 0.4.4 component after closing Logic.

See BUILD-NOTES.md for validation and INSTALL-DA.txt for the Logic test.
