# Architecture — 0.4.4

Original implementation; no Stream AB source, binaries, scripts, adapters or helper
bundles are included. SystemMediaController resolves MediaRemote dynamically and
runs transport on a serial queue. The signatures and command IDs were checked
against independently published interface descriptions:
- https://github.com/theos/headers/blob/master/MediaRemote/MediaRemote.h
- https://github.com/ungive/mediaremote-adapter

Only ABI declarations/IDs are used, not either project's implementation. PLAY=0,
PAUSE=1, toggle=2, next=4, previous=5. SWITCH uses explicit PLAY and PAUSE. No Apple
Events, AppleScript, osascript, accessibility injection or bundled helper is used.
Spotify search remains a user-triggered URI that opens in Spotify.

A processor-owned timer sequences the fade and transport; editor closure does not
interrupt it. Active audio must acknowledge mute. After 100 ms without audio callbacks,
PLAY can proceed and the next callback initializes the gain at silence if REF is
still selected. No audio callback and no reference capture are required to choose REF.
Capture errors affect meters only. Requested state drives MIX/REF, not last callback
state. Transport acceptance failures restore MIX and attempt PAUSE. A two-second
watchdog restores MIX if a native request stalls; it cannot kill that native call,
and a late PLAY response is followed by cleanup PAUSE. Click during pending PLAY
cancels REF. A further click during PAUSE can restore MIX immediately.

This is a private, unsupported macOS API, so symbol presence does not prove host
permission or playback behavior. The return Boolean is command acceptance, not
verified playback. There is no mandatory now-playing metadata lookup. The UI says
system media is available, not that Spotify is connected. One-time source playback
is necessary for macOS to establish its media target. Commands follow the system's
current target; they are not pinned to a specific player or process.

ScreenCaptureKit remains analysis-only with explicit Logic/current-process exclusions.
Reference meters depend on active host audio callbacks. A/B does not. Gain/EQ/aurora
styling remain from the earlier versions. Session restoration starts in MIX.
Use one controlling instance; multiple instances may issue conflicting commands.
Closing the editor is supported. Before deleting the plugin or quitting Logic,
pause the source: an already-dispatched command cannot be recalled during unload.
Host parameter automation still controls gain selection rather than independently
issuing PLAY. UI SWITCH drives the full transport sequence.
