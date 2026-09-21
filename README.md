# RefMatch 0.4.3 — Spotify A/B, plugin only

Source candidate: SWITCH now sequences DAW mute with Spotify transport.
AU and VST3 only. No Control application, standalone application or helper.

1. Open Spotify and select a playable track.
2. In RefMatch, CONNECT SPOTIFY tests playback-state access. Grant host Automation
   permission if macOS permits a prompt.
3. ENABLE SYSTEM AUDIO starts ScreenCaptureKit. Permit capture for the actual host.
4. With Logic processing audio, SWITCH TO REF fades MIX out over 20 ms, sends PLAY
   and verifies Spotify's playing state. SWITCH TO MIX sends PAUSE, checks state,
   then restores MIX through the existing fade.
5. Errors restore MIX. If macOS prevents PAUSE as well, Spotify may still be audible;
   the plugin reports this and asks you to pause Spotify manually.

This replaces script-based transport with native Apple Events using the codes in
Spotify's installed scripting dictionary. Track metadata is deliberately not read:
Spotify assigns it a different access group from playback. This removes unnecessary
metadata/dictionary dependencies, but does not bypass or grant host permissions.
The reported privilege violation may still occur if Logic/AUHostingService lacks
transport permission. This must be tested in Logic; no claim of a confirmed fix.

See BUILD-NOTES.md and INSTALL-DA.txt. The previous 0.4.2 built and passed its tests
on GitHub. These 0.4.3 changes have not yet been compiled or tested in Logic.

## GitHub update

Extract this ZIP. Upload the RefMatch folder into the repository root, replacing
matching files. Replace .github/workflows/main.yml with the included version to
name the new artifact 0.4.3. Do not nest the entire extracted version folder.
The workflow builds universal AU/VST3 and runs the expanded C++ tests.
