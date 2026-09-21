# RefMatch 0.4.2 — plugin only

Complete source candidate for macOS AU/VST3. No standalone or Control app.
See [installation and use](INSTALL-DA.txt), [architecture](RefMatch/README.md),
and [validation status](BUILD-NOTES.md).

## Upload to GitHub

Extract the ZIP. Replace the old source tree and old build workflow with this
repository's contents: `RefMatch/`, `.github/workflows/main.yml`, and root docs.
Do not upload the ZIP itself as the source, or nest these files inside another
version folder. On macOS use Cmd+Shift+. to reveal `.github` if needed.
Ensure no second old workflow remains that still builds/packages Control/Standalone.
Push to main or run “Build RefMatch plugin only” in Actions.

The resulting artifact contains RefMatch.component, RefMatch.vst3 and install notes.
The source candidate has not been compiled locally; the workflow must pass first.

SWITCH supplies a 20 ms DAW fade and capture-failure fallback. Generic external
play/pause is manual; pause the source before returning to MIX. Spotify control
is optional and uses host permissions directly. See installation notes for details.
