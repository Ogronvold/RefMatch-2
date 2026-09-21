# RefMatch 0.4.3 implementation

The processor owns the Spotify controller and 50 Hz message-thread coordinator;
closing the editor does not cancel a switch. The audio thread only performs the
existing 20 ms ramp and capture safety gate. It never sends Apple Events.

The coordinator waits for a fresh muted acknowledgement before PLAY. No audio
callbacks within one second cancels the request and restores MIX. A second click
while PLAY is outstanding cancels REF and queues PAUSE after completion. Normal
return to MIX pauses before fading up. Capture errors restore MIX immediately
and schedule a best-effort PAUSE. Successful PLAY is accepted only if REF is still
requested and capture is healthy. Metadata polling cannot overlap a switch.

Native Spotify transport uses spfy/Play, spfy/Paus, spfy/PlPs, spfy/Next and spfy/Prev.
Playback verification uses core/getd with pPlS. These are taken from the locally
installed Spotify.app/Contents/Resources/Spotify.sdef. The application/playback
access group is com.spotify.playback; track properties use com.spotify.library.
Only playback is requested in 0.4.3. The track/artist panel displays connection
status rather than claiming to show live metadata.

Every event has a three-second reply timeout. PLAY plus state check plus failed-PLAY
cleanup can take up to nine seconds, excluding permission-system delays. An explicit
CONNECT may request consent only when the host bundle supplies a usage description.
No automatic prompt is made during SWITCH. A successful preflight does not prove
that the actual event or subsequent state read will succeed. Errors include the
native status code. The implementation cannot change Logic's signature, entitlement
or sandbox, and never launches osascript, a helper, or a private media-control API.

ScreenCaptureKit remains analysis-only. External playback is heard via Spotify's
normal audio output. Generic browser/Apple Music capture remains supported for
analysis, but this SWITCH transport implementation specifically targets Spotify.
It does not control those other players. Only the DAW bus containing RefMatch is
muted. Use one controlling RefMatch instance; multiple instances can send conflicting
commands to the single Spotify player.

Closing the editor is supported; removing the plugin or quitting the host during
an already-dispatched command cannot guarantee cancellation or pause. Pause Spotify
before removing the plugin. Host parameter automation changes MIX/REF gain selection;
it does not independently issue PLAY. User-facing SWITCH drives the transport sequence.

References:
- https://developer.apple.com/documentation/coreservices/1442994-aesendmessage
- https://developer.apple.com/documentation/foundation/nsappleeventdescriptor
- https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.automation.apple-events
- https://developer.apple.com/documentation/screencapturekit/scstreamconfiguration/excludescurrentprocessaudio
