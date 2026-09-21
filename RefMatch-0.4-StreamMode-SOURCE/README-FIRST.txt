RefMatch 0.4 — STREAM MODE

This build intentionally removes Spotify OAuth/login.

Expected first-run flow in Logic:
1. Open Spotify desktop and have a track ready.
2. Load RefMatch on the master.
3. Click ENABLE SPOTIFY CONTROL and allow Logic to control Spotify if macOS asks.
4. Click SWITCH TO REF.
5. macOS may ask for Screen & System Audio Recording permission. Allow Logic, then reopen Logic if requested.
6. SWITCH toggles between the DAW mix and Spotify. The Spotify signal is heard directly from Spotify; captured system audio is used only for meters/spectrum/difference analysis.

This is deliberately closer to the Stream AB architecture: no login, external reference playback, plugin mutes/unmutes the DAW, and system audio capture is measurement-only.
