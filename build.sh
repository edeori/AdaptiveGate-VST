#!/usr/bin/env bash
# Builds AdaptiveGate (VST3 + AU + Standalone), Apple-signs each artefact with
# the Developer ID Application identity (no licensing - this plugin has none),
# and flattens the results into build/VST3, build/AU, build/Standalone.
#
# COPY_PLUGIN_AFTER_BUILD is OFF in CMakeLists.txt on purpose: VST3 is built
# and signed here (needed for packaging/distribution) but should not land on
# this machine's plugin folders. Only the signed AU is deployed locally, into
# ~/Library/Audio/Plug-Ins/Components, for local testing.
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

CONFIG="Release"
BUILD_DIR="build"

CODESIGN_IDENTITY="${CODESIGN_IDENTITY:-Developer ID Application: Márton Ferenczi (Y8272MS92K)}"

echo "==> Configuring ($BUILD_DIR/)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG" > /dev/null

echo "==> Building VST3 + AU + Standalone ($CONFIG)..."
cmake --build "$BUILD_DIR" --config "$CONFIG" \
    --target AdaptiveGate_VST3 AdaptiveGate_AU AdaptiveGate_Standalone \
    -j "$(sysctl -n hw.ncpu)"

ARTEFACTS="$BUILD_DIR/AdaptiveGate_artefacts/$CONFIG"

FLAT_VST3_DIR="$BUILD_DIR/VST3"
FLAT_AU_DIR="$BUILD_DIR/AU"
FLAT_STANDALONE_DIR="$BUILD_DIR/Standalone"

rm -rf "$FLAT_VST3_DIR" "$FLAT_AU_DIR" "$FLAT_STANDALONE_DIR"
mkdir -p "$FLAT_VST3_DIR" "$FLAT_AU_DIR" "$FLAT_STANDALONE_DIR"

cp -R "$ARTEFACTS/VST3/AdaptiveGate.vst3" "$FLAT_VST3_DIR/"
cp -R "$ARTEFACTS/AU/AdaptiveGate.component" "$FLAT_AU_DIR/"
cp -R "$ARTEFACTS/Standalone/AdaptiveGate.app" "$FLAT_STANDALONE_DIR/"

ENTITLEMENTS="$BUILD_DIR/entitlements.plist"
cat > "$ENTITLEMENTS" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.device.audio-input</key>
    <true/>
</dict>
</plist>
PLIST

echo "==> Codesigning with Developer ID Application..."
codesign --force --deep --options runtime --timestamp \
  --entitlements "$ENTITLEMENTS" \
  --sign "$CODESIGN_IDENTITY" \
  "$FLAT_STANDALONE_DIR/AdaptiveGate.app"

codesign --force --options runtime --timestamp \
  --sign "$CODESIGN_IDENTITY" \
  "$FLAT_VST3_DIR/AdaptiveGate.vst3"

codesign --force --options runtime --timestamp \
  --sign "$CODESIGN_IDENTITY" \
  "$FLAT_AU_DIR/AdaptiveGate.component"

echo "==> Verifying signatures..."
codesign --verify --deep --strict "$FLAT_STANDALONE_DIR/AdaptiveGate.app"
codesign --verify --strict "$FLAT_VST3_DIR/AdaptiveGate.vst3"
codesign --verify --strict "$FLAT_AU_DIR/AdaptiveGate.component"

echo "==> Deploying AU to this machine (VST3 is built/signed but not installed locally)..."
AU_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$AU_INSTALL_DIR"
rm -rf "$AU_INSTALL_DIR/AdaptiveGate.component"
cp -R "$FLAT_AU_DIR/AdaptiveGate.component" "$AU_INSTALL_DIR/"
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister \
  -f "$AU_INSTALL_DIR/AdaptiveGate.component" >/dev/null 2>&1 || true

echo
echo "==> Build complete (license-free, Apple-signed)."
echo "    VST3:       $FLAT_VST3_DIR/AdaptiveGate.vst3 (built + signed, not installed locally)"
echo "    AU:         $FLAT_AU_DIR/AdaptiveGate.component (deployed to $AU_INSTALL_DIR)"
echo "    Standalone: $FLAT_STANDALONE_DIR/AdaptiveGate.app"
