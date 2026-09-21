# RefMatch 0.4.2 — Stream Mode, plugin only

Source candidate based on the locally preserved 0.4.1 runtime-fix source.
Only AU and VST3 ship. There is no executable helper, standalone application,
IPC service, Dock item or auxiliary window. JUCE remains pinned to 9.0.2.
The existing dark aurora layout, UTF-8 strings, Match EQ and meters are retained.

## Switching and capture

SWITCH controls a 20 ms continuous DAW fade, independently of Spotify. External
reference plays through the source application's normal output. ScreenCaptureKit
feeds reference meters, spectrum and matching only; captured sound never enters
the plugin output. Enable capture first, then select REF. MIX is always available.

Capture failure forces a fade back to MIX in the audio processor, even without an
editor. Missing packets for more than one second also latch MIX; valid silence does
not. An explicit UI retry or selecting MIX rearms REF. Saved/restored sessions start
in MIX. Capture reads use a try-lock and persistent fractional resampling position;
they allocate no temporary vectors and discard excessive analysis backlog.

Logic and AUHostingService are explicitly excluded in addition to the current
process. Other DAWs that host plugins out of process need host-specific exclusion
validation; do not assume their mix is excluded. Notifications and unrelated system
sounds are part of generic system reference. Only the bus containing RefMatch is muted.

## Concrete transport and permission limits

This version does **not** promise one-click external play/pause for arbitrary media
apps. Generic SWITCH mutes/restores the DAW only. Pause the external player before
returning to MIX, otherwise both are audible. ScreenCaptureKit does not mute its
source's normal output. A helper would not by itself solve universal transport.
No private MediaRemote API or unacknowledged media-key toggle is used.

Optional Spotify controls use in-process Apple Events/AppleScript on a serial
worker queue. No helper is launched. Only explicit CONNECT may prompt; it first
checks that the main host bundle has NSAppleEventsUsageDescription. Existing
authorization can be used without prompting. Commands use a three-second Apple
Event timeout; an interactive permission dialog may remain pending until answered.
SWITCH never waits for these commands, and metadata errors cannot mute the DAW.
Search is an explicit user action that opens the Spotify URI.

Apple documents the usage-description and executable entitlement requirements:
- https://developer.apple.com/documentation/bundleresources/information-property-list/nsappleeventsusagedescription
- https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.automation.apple-events
- https://developer.apple.com/library/archive/technotes/tn2312/_index.html
- https://developer.apple.com/documentation/screencapturekit/scstreamconfiguration/excludescurrentprocessaudio

The host-specific consequence is an architectural inference: adding keys or
entitlements to an AU cannot modify the permissions/signature of Logic or its AU
hosting process. We include plugin purpose strings, do not advertise sandbox safety,
and do not fabricate host entitlements. Host capture authorization still must be
tested in Logic. A host denial leaves MIX available. No host bundle is modified.

## Build and validation

From repository root on a Mac with Xcode/Command Line Tools and CMake:

```sh
cmake -S RefMatch -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
```

The root GitHub workflow builds universal AU/VST3, runs C++ fade/safety tests,
verifies signatures and architecture, and rejects any app bundle in the artifact.
JUCE is fetched during configuration; this is not an offline dependency archive.
See ../BUILD-NOTES.md for actual validation and remaining manual tests.
