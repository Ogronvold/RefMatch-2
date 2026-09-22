# RefMatch commercial release preparation

The normal main.yml remains a working ad-hoc test build. release.yml is a separate,
manual-only candidate pipeline. It has not run; no signing/notarization is claimed.
It builds/tests, signs AU/VST3 with Developer ID Application, runs native auval,
creates a non-relocatable Developer ID Installer PKG, submits it to Apple, requires
Accepted, staples and validates it, then uploads the PKG as a private workflow artifact.
It does not publish a public GitHub release or deploy a webshop.

Before running it, create/configure the GitHub environment `release-signing` and add:

- CERTIFICATES_P12_BASE64: base64 P12 export containing both Developer ID Application
  and Developer ID Installer certificates AND their private keys.
- CERTIFICATES_PASSWORD: password used for that P12 export.
- DEVELOPER_ID_APPLICATION: exact Developer ID Application signing identity.
- DEVELOPER_ID_INSTALLER: exact Developer ID Installer signing identity.
- APPLE_ID: Apple account used for notarization.
- APPLE_TEAM_ID: Developer team ID.
- APPLE_APP_SPECIFIC_PASSWORD: app-specific password for notarization.

Create/export certificates in your Apple account/Keychain and enter them as GitHub
secrets, not in this repository or chat. Only run on an ephemeral GitHub-hosted runner.
Missing credentials stop the release; there is no fallback to an ad-hoc commercial PKG.
The CMake option REFMATCH_ADHOC_SIGN defaults ON for tests and is OFF in this workflow.
Developer ID signing replaces any intermediate build signature before packaging.

The PKG installs only:
/Library/Audio/Plug-Ins/Components/RefMatch.component
/Library/Audio/Plug-Ins/VST3/RefMatch.vst3

Notarization/signing do not prove host compatibility or grant screen/audio permission.
Still required before selling: real clean-Mac permission tests; Intel and Apple Silicon
host tests; tested macOS versions; AU/VST3 regression, multiple instances; licensing
and activation decisions; current JUCE licence review; branding/legal/store decisions.
MediaRemote is private and can change. Capture, reference learning and Match EQ do
not require its success: start the external player manually when transport is unavailable.

References:
https://docs.github.com/en/actions/how-tos/deploy/deploy-to-third-party-platforms/sign-xcode-applications
https://developer.apple.com/developer-id/
https://developer.apple.com/documentation/security/customizing-the-notarization-workflow
