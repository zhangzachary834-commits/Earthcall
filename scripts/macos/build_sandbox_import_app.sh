#!/usr/bin/env bash
set -euo pipefail

checkout="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)"
node_path="$(command -v node)" || { echo "Node.js must be available on PATH." >&2; exit 1; }
build_dir="$checkout/build/sandbox-import"
mkdir -p "$build_dir"
staging="$(mktemp -d "$build_dir/.stage.XXXXXXXX")"
trap 'rm -rf "$staging"' EXIT

name="Earthcall Sandbox Import.app"
app="$staging/$name"
resources="$app/Contents/Resources"
mcp="src/Singularity/Foreign/mcp"
mkdir -p "$app/Contents/MacOS" "$resources/scripts" "$resources/$mcp"
cp "$checkout/scripts/earthcall-sandbox-import.js" "$resources/scripts/"
cp "$checkout/$mcp/earthcall-sandbox.js" "$checkout/$mcp/earthcall-sandbox-policy.js" \
    "$checkout/$mcp/earthcall-sandbox-sync.js" "$resources/$mcp/"
cat > "$app/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
    <key>CFBundleDevelopmentRegion</key><string>en</string>
    <key>CFBundleExecutable</key><string>SandboxImport</string>
    <key>CFBundleIdentifier</key><string>org.earthcall.SandboxImport</string>
    <key>CFBundleName</key><string>Earthcall Sandbox Import</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>1.0</string>
    <key>CFBundleVersion</key><string>1</string>
    <key>LSMinimumSystemVersion</key><string>14.0</string>
    <key>LSApplicationCategoryType</key><string>public.app-category.developer-tools</string>
    <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
/usr/bin/plutil -insert EarthcallCheckout -string "$checkout" "$app/Contents/Info.plist"
/usr/bin/plutil -insert EarthcallNode -string "$node_path" "$app/Contents/Info.plist"
MACOSX_DEPLOYMENT_TARGET=14.0 xcrun swiftc -parse-as-library -O \
    -module-cache-path "$build_dir/module-cache" \
    "$checkout/scripts/macos/SandboxImportApp.swift" -o "$app/Contents/MacOS/SandboxImport"
/usr/bin/codesign --force --sign - "$app"
rm -rf "$build_dir/$name"
mv "$app" "$build_dir/$name"
printf '%s\n' "$build_dir/$name"
