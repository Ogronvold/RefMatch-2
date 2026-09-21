# RefMatch 0.4.1 — Stream Mode

RefMatch is a JUCE AU/VST3 reference plugin for macOS.

## How this build works

- **A / Your Mix:** audio from the DAW passes through RefMatch.
- **B / Reference:** RefMatch fades the DAW signal to silence and asks the local Spotify desktop app to play. Spotify remains an external system-audio source; RefMatch does **not** route or re-encode Spotify audio through the plugin output.
- **System reference analysis:** ScreenCaptureKit captures system audio for analysis only. The current DAW process is excluded from capture, so the mix does not leak into the reference measurement.
- **No Spotify login / OAuth:** local Spotify control is performed with macOS Automation / Apple Events.

## macOS permissions

Spotify control and system capture use separate macOS permissions:

1. **Automation / control Spotify** — press `CONNECT SPOTIFY` and allow **RefMatch Control** to control Spotify. The helper is embedded in the plugin and launched as a separate application. Use `PERMISSIONS` to recover a denied grant; do not modify Logic or its signature.
2. **Screen & System Audio Recording** — first switch to REF starts reference metering. Allow your DAW under System Settings → Privacy & Security → Screen & System Audio Recording, then reopen the DAW if macOS asks.

## Features in 0.4.1

- One-click `SWITCH TO REF / SWITCH TO MIX`
- 20 ms click-free DAW fade when switching
- Spotify desktop play/pause/previous/next
- Spotify current track and artist via local app control
- Spotify search field that opens the query in the installed Spotify app
- System-audio reference capture for analysis only
- Mix and reference peak meters
- Live tonal/spectrum comparison
- Difference curve (`REF - MIX`)
- Auto Match adjusts **your mix gain**, never the reference
- Match EQ Learn / Amount / Max correction / Reset / Bypass
- State saving for plugin parameters

## Important current limitation

This first no-login build opens searches in the Spotify desktop app instead of returning an in-plugin Spotify result list. That avoids OAuth and keeps the local-control architecture clean. The A/B switching and reference metering do not depend on Spotify search.

## Build status

This source candidate has not been compiled or tested in Logic locally because the Mac lacks CMake and Apple developer tools. The included GitHub workflow builds all formats, runs C++ fade/switch tests, and verifies the embedded helper and its entitlements. Replace the root workflow as well as the source folder. See ../INSTALL-DA.txt and ../BUILD-NOTES.txt.
