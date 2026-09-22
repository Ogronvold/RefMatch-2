# RefMatch 0.5.2 — transport / timeline / EQ display

Plugin-only AU/VST3 for macOS. Keeps the working system-media A/B transport,
20 ms mix fade and ScreenCaptureKit reference analysis. No helper or standalone app.

## What's changed

- PLAY or PAUSE only. Clicking updates the requested playback state immediately;
  independent player-state polling reconciles it. Tooltip distinguishes a pending
  request from confirmed playback. Rejection rolls back; unconfirmed requests expire.
- Search and TIMED removed from the interface. A/B input meters share the same baseline.
- Optional cover art beside the reference title and artist, cached per track.
  A neutral placeholder appears if the system supplies no image.
- -5 s / +5 s transport buttons, enabled only with a valid player position.
- Loop timeline: drag a selection, adjust either edge, or click away from the handles
  to seek. Dragging a selection enables Loop; select B to audition. In/Out fields
  remain available for precise times. Minimum selection is 0.5 s.
- A manual seek outside an enabled loop turns that loop off. Track change,
  sustained missing metadata or an unconfirmed loop seek disables looping safely.
- Empty capture labels read MIX and REF. Recording and captured states remain explicit.
- EQ graph scale is derived from the full 100% correction and held constant as Amount
  moves. It no longer zooms out above 50%. This is a display fix, not an EQ strength change.

## Use

Put RefMatch on the Logic output you want to compare. Play the reference app once so
macOS knows the active player. A selects MIX, B selects the external player, SWITCH
alternates. The plugin must not be bypassed in Logic.

RECORD MIX -> STOP MIX, then RECORD REF -> STOP REF, then MATCH. MATCH enables the
learned correction. Amount 100% applies the whole fitted tonal correction; EQ cannot
make different recordings identical in instruments, dynamics or stereo image.

The editor starts optional system-audio capture for meters. Grant permission when
macOS asks. Learning and meters require Logic to process audio. A/B works independently.
Match EQ view is 640x550, Loop is 640x430, plus Logic's own toolbar.

## Build / installation

Upload the extracted RefMatch folder over the existing repository folder, plus the
root documents and .github/workflows/main.yml. No need to delete the folder first.
GitHub Actions builds universal AU/VST3, runs regression tests and packages plugins only.
See INSTALL-DA.txt for installation and the signing commands.

This source candidate has not been compiled or run in Logic locally. Apple developer
tools are absent on this Mac. See BUILD-NOTES.md for validation and required checks.
