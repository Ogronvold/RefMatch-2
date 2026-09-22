# RefMatch 0.5.3 — independent reference analysis / Tone EQ

AU/VST3 plugin only; no helper or standalone app. Uses the working system-media
transport and 20 ms mix fade, with orange MIX and purple REFERENCE accents.

## New in this version

- PLAY from A uses the B-switch sequence: mute MIX, then start reference. It cannot
  intentionally start the reference while leaving the plugin's MIX path audible.
- Reference meters, spectrum and RECORD REF use their own internal analysis thread.
  They continue when Logic's transport stops or its audio callback is suspended.
  MIX still needs host input; its live display clears when host callbacks stop.
- Drag the white timeline playhead to select a seek position; release to send it.
  Region selection and loop-edge dragging are retained, as are +/-5 second buttons.
- AUTO GAIN and METERS buttons removed. Capture starts when opening the editor or
  recording REF, and ends when the plugin is destroyed (not when Logic stops).
- Smooth continuously changes the breadth of the match, recomputing from the stored
  MIX/REF profiles. No re-recording required. Fine retains more narrow detail.
- Expand TONE EQ for three broad parametric bands, each with gain and frequency.
  These process after the matched EQ and are independent of Match Amount. EQ ON
  bypasses both. RESET TONE zeroes these gains; main RESET clears/bypasses the match.
- Orange/purple gradients, cleaner labels, aligned signal indicators and accurate
  frequency tick positions. Cover art and metadata are retained when supplied by macOS.

Match EQ view is 640x580; expanded Tone is 640x660; Loop is 640x430, plus host toolbar.

## Match workflow

RECORD MIX -> STOP MIX, RECORD REF -> STOP REF, MATCH. Choose A to audition EQ.
Capture representative sections. At least 0.5 s of non-silent data is required.
Amount scales the learned correction. Smooth adjusts detail. Tone is an additional
three-band correction, so Amount 0% can still alter sound if Tone gains are nonzero.
Project state stores profiles, learned EQ and all Smooth/Tone parameters.

## Build

Upload the extracted RefMatch folder over the existing folder, plus the root documents
and .github/workflows/main.yml. Build in GitHub Actions, then follow INSTALL-DA.txt,
including signing commands. This is a source candidate, not a locally tested binary.

## Boundaries

System-media transport and seek target the active player, not a Spotify account.
Metadata, position and artwork depend on macOS. Seeking happens on mouse release;
the time ruler is not an audio waveform or sample-accurate DAW scrubber.
EQ matches broad spectral balance, not instruments, dynamics or identical audio.
MIX measurement cannot invent audio when Logic sends none. REF requires capture permission.
