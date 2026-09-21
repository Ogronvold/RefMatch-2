# RefMatch prototype

A JUCE 9.0.2 VST3/AU/Standalone reference and Match-EQ prototype.

## Current prototype
- Load WAV/AIFF/MP3/FLAC reference audio
- Instant A/B between DAW input and reference
- Automatic running RMS level match on reference playback
- Dual FFT spectrum display
- Learn tonal difference between the current mix and reference
- Match EQ with amount and maximum-correction controls
- User-selectable active Match EQ range (low/high), acting as a protected-range system

## Planned next passes
1. True ITU-R BS.1770 / EBU R128 LUFS + true peak
2. Multiple independent Match Zones / brush-to-protect UI
3. Smoothing control and minimum-phase / linear-phase Match EQ modes
4. Capture windows (Verse / Chorus / Drop / custom loops)
5. Stereo width, correlation, crest factor and low-end metrics
6. Blind X/Y testing and session statistics
7. Spotify/YouTube source adapters as separate modules
8. Reference library and per-project snapshots

## Build on macOS
Requires CMake 3.22+, Xcode and Git.

```bash
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

CMake fetches JUCE 9.0.2 automatically.

> Note: this is an engineering prototype, not a production-ready release. Loudness matching is currently RMS-based; the next DSP pass should replace it with BS.1770-4/R128 loudness matching.

## Current RefMatch build

The plugin now contains:
- Local reference-file playback with seek position.
- A/B switching between the DAW mix and the local reference.
- Optional RMS level matching for the reference.
- Spectrum capture for source and local reference.
- 12-band Match EQ approximation with Learn, Amount, Max Correction, enable, reset and bypass controls.
- APVTS state persistence for parameters and the local reference path.
- Spotify OAuth/PKCE connection, current playback/device metadata, play/pause, track search and play-selected controls.

Spotify Web API is used only for metadata and remote playback control. Spotify PCM audio is not routed into RefMatch DSP. Match EQ and true in-plugin A/B therefore use a local audio reference file.
