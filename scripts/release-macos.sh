#!/bin/bash
# Run only on an ephemeral GitHub-hosted macOS runner with Developer ID secrets.
set -euo pipefail
: "${RUNNER_TEMP:?Run this script in GitHub Actions}"
: "${GITHUB_ACTIONS:?Run this script in GitHub Actions}"
for key in CERTIFICATES_P12_BASE64 CERTIFICATES_PASSWORD DEVELOPER_ID_APPLICATION DEVELOPER_ID_INSTALLER APPLE_ID APPLE_TEAM_ID APPLE_APP_SPECIFIC_PASSWORD; do
  if [[ -z "${!key:-}" ]]; then echo "Missing release secret: $key" >&2; exit 1; fi
done
case "$DEVELOPER_ID_APPLICATION" in 'Developer ID Application:'*) ;; *) echo 'Expected Developer ID Application identity' >&2; exit 1;; esac
case "$DEVELOPER_ID_INSTALLER" in 'Developer ID Installer:'*) ;; *) echo 'Expected Developer ID Installer identity' >&2; exit 1;; esac
version=$(sed -n 's/^project(RefMatch VERSION \([^ ]*\).*/\1/p' RefMatch/CMakeLists.txt)
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
mkdir -p release-output
release_temp=$(mktemp -d "$RUNNER_TEMP/refmatch-release.XXXXXX")
keychain="$release_temp/signing.keychain-db"
cleanup() {
  security delete-keychain "$keychain" >/dev/null 2>&1 || true
  rm -rf "$release_temp"
}
trap cleanup EXIT
keychain_password=$(openssl rand -hex 32)
echo "::add-mask::$keychain_password"
printf '%s' "$CERTIFICATES_P12_BASE64" | base64 --decode > "$release_temp/certificates.p12"
security create-keychain -p "$keychain_password" "$keychain"
security set-keychain-settings -lut 21600 "$keychain"
security unlock-keychain -p "$keychain_password" "$keychain"
security import "$release_temp/certificates.p12" -k "$keychain" -P "$CERTIFICATES_PASSWORD" -T /usr/bin/codesign -T /usr/bin/pkgbuild
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$keychain_password" "$keychain" >/dev/null
security list-keychains -d user -s "$keychain" "$HOME/Library/Keychains/login.keychain-db"
for entry in 'AU/RefMatch.component' 'VST3/RefMatch.vst3'; do
  bundle="build/RefMatch_artefacts/Release/$entry"
  test -d "$bundle"
  test -z "$(find "$bundle" -type d -name '*.app' -print)"
  lipo "$bundle/Contents/MacOS/RefMatch" -verify_arch arm64 x86_64
  codesign --force --sign "$DEVELOPER_ID_APPLICATION" --keychain "$keychain" --options runtime --timestamp "$bundle"
  codesign --verify --deep --strict "$bundle"
  codesign --display --verbose=4 "$bundle" 2>&1 | tee -a release-output/signatures.log
  codesign --display --verbose=4 "$bundle" 2>&1 | grep -F 'Authority=Developer ID Application:' >/dev/null
done
# Validate the native architecture AU on this ephemeral runner, not the user's Mac.
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
ditto build/RefMatch_artefacts/Release/AU/RefMatch.component "$HOME/Library/Audio/Plug-Ins/Components/RefMatch.component"
auval -v aufx RfM1 RfMt 2>&1 | tee release-output/auval.log
payload="$release_temp/payload"
mkdir -p "$payload/Library/Audio/Plug-Ins/Components" "$payload/Library/Audio/Plug-Ins/VST3"
ditto build/RefMatch_artefacts/Release/AU/RefMatch.component "$payload/Library/Audio/Plug-Ins/Components/RefMatch.component"
ditto build/RefMatch_artefacts/Release/VST3/RefMatch.vst3 "$payload/Library/Audio/Plug-Ins/VST3/RefMatch.vst3"
# Avoid relocatable bundle installation into an old copy outside the intended directories.
pkgbuild --analyze --root "$payload" "$release_temp/components.plist"
python3 - "$release_temp/components.plist" <<'PY'
import plistlib,sys
path=sys.argv[1]
with open(path,'rb') as f: items=plistlib.load(f)
for item in items: item['BundleIsRelocatable']=False
with open(path,'wb') as f: plistlib.dump(items,f)
PY
package="release-output/RefMatch-$version-Installer.pkg"
pkgbuild --root "$payload" --component-plist "$release_temp/components.plist" --ownership recommended --identifier com.refmatch.audio.installer --version "$version" --install-location / --sign "$DEVELOPER_ID_INSTALLER" --keychain "$keychain" "$package"
pkgutil --check-signature "$package" | tee release-output/installer-signature.log
xcrun notarytool submit "$package" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_SPECIFIC_PASSWORD" --wait --output-format json > release-output/notarization.log
python3 - <<'PY'
import json
with open('release-output/notarization.log') as f: result=json.load(f)
if result.get('status')!='Accepted': raise SystemExit('Notarization was not accepted; inspect the log before distributing.')
PY
xcrun stapler staple "$package"
xcrun stapler validate "$package"
spctl --assess --type install --verbose=2 "$package"
