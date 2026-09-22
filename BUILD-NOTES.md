# RefMatch 0.5.3 validation

## Architecture change

ReferenceAnalysis owns the only system-audio FIFO consumer, on an internal JUCE thread.
It pulls complete stereo blocks at 48 kHz for reference spectrum, peak and learning.
processBlock now only measures/learns MIX and processes output audio. releaseResources
no longer stops external capture. Thread shutdown is joined before capture destruction.
Capture buffer fill is checked before consumption; partial packets are not padded into
learned profiles. REF spectra are displayed and remapped using their actual 48 kHz rate.
MIX and REF have separate LearnCapture accumulators and share no FFT working storage.

Smooth uses a continuous Gaussian target blur before fitting. Three broad parametric
stages follow the 20-band match bank; coefficient changes retain the 20 ms ramp.
All controls are appended to the parameter list for existing-session compatibility.
Tone gains/frequencies are APVTS parameters. Old sessions get neutral Tone defaults.

## Local validation

UTF-8 and delimiter checks, CMake source/test path checks and workflow shell syntax.
Source review of the single FIFO consumer, independent analysis lifecycle, safe PLAY
routing and seek-on-release. Layout sketch/coordinate check, not a native screenshot.
ZIP integrity and required source/workflow/test checks.

No Apple developer toolchain is installed. C++ tests, native build, Logic playback,
ScreenCaptureKit on this host and actual new UI rendering have NOT been executed locally.
The new analysis ownership is a material change that requires the acceptance tests below.

## Added automated regressions (GitHub Actions)

- ReferenceAnalysisState records REF and updates meters with no host processBlock calls.
- Smooth reduces alternating narrow correction changes.
- Manual Tone remains active at zero Match Amount and produces the expected 3 dB sine boost.
- Playhead hit testing takes priority over coincident loop edges; both edges remain reachable.
- Existing fade ordering, transport cancellation, playback feedback, captures, stereo power,
  transparent neutral EQ, full-range Amount response and timeline range tests are retained.

## Logic acceptance

1. While A plays MIX, press PLAY: the mix fades out before the reference starts; B is selected.
   Repeat while Logic is stopped, then resume Logic. There must be no simultaneous MIX output.
2. Stop Logic while Spotify plays. REF peak, spectrum and RECORD REF must continue. MIX clears
   when no callback/input is available. Test 44.1/48/96 kHz host sample rates.
3. Capture MIX and REF, stop them, MATCH, change Smooth from Fine to Broad; no re-recording.
4. Open Tone, change all frequencies/gains, audition EQ ON/OFF, Amount 0/100, RESET TONE.
   Confirm actual audible response, bypass ramp and persistence on reopening the project.
5. Drag white playhead, then release; check no region edit. Resize loop edges and draw a new
   region. Seek/skip outside an active loop turns it off. Test short selections and track changes.
6. Deny capture permission, quit source, pause source, suspend/resume audio and remove plugin
   while REF recording. No crash, stale meter, shared FIFO consumer or A/B dependency on meters.
7. Verify typography, orange/purple palette and expanded/collapsed Tone heights in Logic.

Private MediaRemote APIs remain optional and fallible. No helpers, subprocesses, or new
external audio routes are introduced. Existing live-analyser audio-thread locks remain for
MIX; this version does not claim hard realtime certification or sample-accurate Spotify seek.
