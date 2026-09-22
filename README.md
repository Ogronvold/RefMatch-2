# RefMatch 0.5.1 — Compact / Learn / Loop

Source candidate based on the user's confirmed working 0.4.4 system-media A/B.
AU and VST3 only. No standalone app, no helper, no bundled third-party framework.

## Interface

Black 640 × 550 Match EQ view and 640 × 390 Loop view, plus the host toolbar.
A, B and SWITCH remain visible. Subtle input meters show incoming MIX and system REF.
Search is always beside reference; it opens Spotify. Track and artist appear when
macOS provides metadata. PLAY/PAUSE lights only with reported playback, with an
explicit unknown state otherwise. Capture starts when the editor opens and can
request macOS screen/system-audio permission.

## Match EQ

1. Open Match EQ. Play the Logic mix, press RECORD MIX, then STOP MIX.
2. Press RECORD REF. This selects B, starts reference capture if necessary, and
   learns the external source (Spotify or another system player). Permit capture.
   Press STOP REF after the desired section.
3. Press MATCH. EQ enables automatically on the MIX signal. Select A to hear it.
4. Use Amount and EQ ON to compare. RESET flattens EQ; profiles are retained.

Record several seconds of representative material. At least 0.5 seconds of non-silent
FFT frames is required per profile. As in the earlier capture implementation, Logic
must process audio for learning/meters; this does not restrict A/B switching.

The profiles are separately accumulated stereo-power spectra, not snapshots of a
moving display. They are frozen after recording and saved with the project. The
EQ fits 20 broad peaking bands after removing overall gain difference. This follows
the learn-current/learn-reference/match workflow, not Apple's proprietary filter
algorithm. The separate graph displays the filter response at the selected Amount
only. 100% applies the full fitted correction; the legacy Limit is ignored.
This matches broad spectral balance, not instruments, dynamics or an identical
waveform. Separate live/frozen profile plots and recording/captured labels show progress.
EQ enable/bypass and coefficient changes ramp over approximately 20 ms.
A reference that is simply louder should generate a flat tonal correction.

## Loop

Enter In and Out (seconds, m:ss, or h:mm:ss), or SET IN/SET OUT from the current
position, then enable LOOP. Choose B to audition. Verified mode reads the active
player's position, seeks back at Out, checks the resulting position and disables on
track change, missing data lasting 3 seconds or unconfirmed seek after 3 seconds. It follows the system media target, not
an authenticated Spotify track. Seek/position access depends on macOS and the player.

TIMED is an explicit fallback when position access is unavailable: it issues seek to
In and repeats at the entered interval. It cannot verify seek, detect a track change
without metadata, or account for external pauses; turn it off before changing tracks
or pausing outside RefMatch. Neither loop mode is sample-accurate. Loop starts off
when the plugin is created; ranges/loop state are not restored with the session.

## Build

Upload the extracted RefMatch folder and replace .github/workflows/main.yml at the
repository root. Do not upload the enclosing version directory. GitHub Actions
builds universal AU/VST3 and runs transport/math tests plus JUCE DSP signal tests.

0.5.1 has not been compiled or tested in Logic locally: this Mac has no Apple
developer toolchain. See BUILD-NOTES.md for the exact verification performed.
