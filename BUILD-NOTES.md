# RefMatch 0.4.2 validation — 2026-09-22

## Source provenance

Based on the preserved 0.4.1 source at:
`2026-09-21/referenced-chatgpt-conversation-this-is-an-2/work/runtime-fix/RefMatch-0.4-StreamMode-SOURCE/RefMatch`.
Its CMake project declares 0.4.1 and contains the Control helper and runtime tests.
The live GitHub branch was not fetched or compared; this package is based on that
local source, not a claim that later remote commits were incorporated.

## Completed locally

- Checked all source files decode strictly as UTF-8, with no replacement characters.
- Checked balanced source delimiters after stripping comments and strings.
- Checked CMake formats contain only AU and VST3.
- Checked helper source directory, launch API, distributed notifications and old
  Spotify-dependent switching state machine are absent.
- Reviewed processor-owned capture failure fallback, frame-based one-second
  starvation threshold, explicit retry, and 20 ms gain ramp.
- Checked repository ZIP integrity and required source/workflow/test entries.

These are source/package checks, not compilation or runtime validation.

## Not run locally

This Mac has neither an active Apple developer directory nor usable CMake/clang.
`xcode-select -p` reports no active developer directory. The system Python launcher
also requires the missing tools; file checks used the bundled Python runtime.
No AU/VST3 binary was built, no C++ test executable ran, no GitHub workflow was
triggered, and no Logic capture, transport, audio or UI test was performed.
The existing UI styling/layout was retained but not visually rendered locally.

## Included automated validation

GitHub Actions configures JUCE 9.0.2, builds universal AU/VST3 and runs CTest.
RuntimeTests checks fade endpoints/monotonicity/continuity/reversal at 44.1, 48,
96 and 192 kHz, capture denial, brief packet gaps, starvation latch, explicit
recovery, and valid silent packets. The package stage rejects app bundles and
verifies plugin signatures plus both CPU architectures.

## Required Logic acceptance checks

1. Build workflow must pass before installation. Install only the new component.
2. Confirm component validation, opening/closing the editor, and session reload
   (MIX default). Confirm no app, menu-bar helper, Dock item or extra window.
3. Deny capture: MIX remains audible. Grant the actual macOS host prompt, reopen
   the host if required, and confirm capture with Spotify, Apple Music and browser.
4. With Logic alone playing, verify REF stays quiet, including Logic's out-of-process
   hosting configuration. Other DAWs need their own exclusion verification.
5. Capture known reference audio at 44.1/48/96 kHz and varying DAW block sizes.
   Check REF meters/spectrum; listen for clicks through repeated/reversed fades.
6. Stop/revoke capture with editor open and closed. MIX should restore through
   the 20 ms fade. Missing packets should restore MIX after about one second.
   Silent valid packets should leave REF selected. Explicit retry should recover.
7. Pause the external source before MIX: verify isolated listening. If it keeps
   playing, both sources are audible; automatic external transport is not supplied.
8. Test optional Spotify CONNECT with granted/denied/missing host Automation
   capabilities; Unicode track names; source quit; editor close during a request.
   Generic switching must stay responsive and independent of these operations.
9. Test stopped/suspended Logic transport, bypass, automation, session reload,
   multiple instances, capture-start cancellation and repeated enable attempts.

## Remaining limits

No claim of universal automatic external transport. No notarization. Capture
analysis depends on the host running audio callbacks. The source application's
normal output cannot be muted through ScreenCaptureKit. Capture includes other
system sounds. Existing EQ/analyser synchronization and oversized host-buffer
allocation behavior were retained from 0.4.1 and are not newly certified realtime-safe.

## Build #21 correction

The supplied GitHub log compiled the project until SystemAudioCapture.mm:222.
The Objective-C selector incorrectly used the Swift-style name. Corrected to
getShareableContentExcludingDesktopWindows:onScreenWindowsOnly:completionHandler:,
as documented by Apple. Only that invocation changed in the source.
Rebuild required; successful compilation is not yet verified.

## GitHub build log at 22:49 UTC

AU and VST3 compilation completed. CTest passed 1/1 tests. Packaging failed
because lipo received the input filename after the variable-length architecture
list. Moved the filename before -verify_arch, matching the tool usage in the log.
Architecture/signature checks remain enabled. Final packaging and Logic tests
still require verification; this correction has not yet run on GitHub.

## Logic report: switch direction and Spotify privilege failure

Fixed selection getter to read the host parameter, rather than the last audio
callback result. The button now cancels REF immediately even while Logic has
suspended callbacks; the audio fade runs when processing resumes. Capture-failure
protection remains unchanged. After failure the button permits explicit MIX reset.
Spotify command failures invalidate control readiness even if permission preflight
succeeded. The actual AppleScript error number is shown. This does not grant
missing host permissions or claim that Spotify control has been repaired.
Moved the external-playback reminder below the gain controls to avoid overlap.
These changes have source-level verification only; rebuild and Logic test required.

## 0.4.3 Spotify A/B implementation (latest candidate)

Supersedes earlier generic-only switching notes. Processor-owned switch coordination,
native Apple Events and playback-only verification replace script/metadata coupling.
Expanded C++ tests cover fade-before-play, duplicate requests, cancellation before
and after dispatch, failed PLAY, normal return, and capture loss during PLAY.
Native codes checked against installed Spotify.sdef. Objective-C descriptor API and
AESendMessage signature checked against Apple's documentation.

Local validation: source/ZIP structure, UTF-8, delimiter balance and extracted workflow
shell syntax only. Apple toolchain is still unavailable (xcode-select has no active
developer directory). New C++ tests have NOT run. Native Objective-C has NOT compiled.
Do not apply prior 0.4.2 build/test success to this new candidate.

Required tests: compile and CTest on GitHub; Logic CONNECT success or exact native
failure code; PLAY after completed MIX fade; PAUSE before fade-in; rapid reverse
clicks; capture denial and starvation; Spotify quit/unresponsive; window close during
PLAY; no helper process. Test permission-denied behavior and ensure MIX returns even
when PAUSE is denied. Test cold/paused player, advertisements, and Spotify Connect
remote playback; successful state replies alone cannot prove local audibility.
