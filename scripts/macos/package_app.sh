#!/usr/bin/env bash
# Assemble and sign Earthcall.app from an already-built release tree.
#
#   scripts/macos/package_app.sh [build-dir]      (default: build_release)
#
# Normally run by the CMake target `earthcall_app` (scripts/macos/install_app.sh
# builds that). Produces <build-dir>/Earthcall.app:
#
#   Contents/Info.plist                    from Info.plist.in
#   Contents/MacOS/Earthcall               earthcall_webgpu
#   Contents/MacOS/earthcall_first_mover   signer helper (MCP grants)
#   Contents/Resources/AppIcon.icns        scripts/macos/AppIcon.png, else a placeholder
#   Contents/Resources/Web/dom_mirror.js
#   Contents/Resources/mcp/                MCP server + its node_modules closure
#   Contents/Resources/Foundation/         first-run seed (scripts/macos/Foundation/)
#
# Signing is release infrastructure, not ontology: no Earthcall identity, key,
# or ownership depends on it. EARTHCALL_SIGN_IDENTITY picks the certificate:
#   unset  -> "Earthcall Local Signing" if make_local_signing_identity.sh made
#             one (keeps macOS mic/Accessibility grants across rebuilds),
#             else ad-hoc "-" (runs on the Mac that built it; grants reset)
#   "Developer ID Application: …"  -> the future shareable path; also set
#             EARTHCALL_SIGN_TIMESTAMP=1, then notarize and staple the .app
#             this script produced (later steps; no Apple account is needed today)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/scripts/macos"
BUILD="$(cd "${1:-$ROOT/build_release}" && pwd)"
APP="$BUILD/Earthcall.app"
STAGE="$BUILD/Earthcall.app.staging"

for bin in earthcall_webgpu earthcall_first_mover; do
  [[ -x "$BUILD/$bin" ]] || { echo "package_app: missing $BUILD/$bin — build it first" >&2; exit 1; }
done

rm -rf "$STAGE"   # previous staging output only
mkdir -p "$STAGE/Contents/MacOS" "$STAGE/Contents/Resources/Web" "$STAGE/Contents/Resources/mcp"

# --- executables ---------------------------------------------------------
cp "$BUILD/earthcall_webgpu" "$STAGE/Contents/MacOS/Earthcall"
cp "$BUILD/earthcall_first_mover" "$STAGE/Contents/MacOS/earthcall_first_mover"

# --- Info.plist ----------------------------------------------------------
VERSION="$(sed -nE 's/.*project\(Earthcall VERSION ([0-9.]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
BUILD_NO="$(git -C "$ROOT" rev-list --count HEAD 2>/dev/null || echo 1)"
COMMIT="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
BUNDLE_ID="${EARTHCALL_BUNDLE_ID:-com.monkeykingzach.earthcall}"
sed -e "s/@VERSION@/${VERSION:-1.0}/" -e "s/@BUILD@/$BUILD_NO/" \
    -e "s/@COMMIT@/$COMMIT/" -e "s/@BUNDLE_ID@/$BUNDLE_ID/" \
    "$HERE/Info.plist.in" > "$STAGE/Contents/Info.plist"
plutil -lint -s "$STAGE/Contents/Info.plist"
printf 'APPL????' > "$STAGE/Contents/PkgInfo"

# --- icon ----------------------------------------------------------------
ICON_PNG="$HERE/AppIcon.png"
if [[ ! -f "$ICON_PNG" ]]; then
  ICON_PNG="$BUILD/AppIcon.placeholder.png"
  [[ -f "$ICON_PNG" ]] || python3 "$HERE/make_placeholder_icon.py" "$ICON_PNG"
fi
ICONSET="$BUILD/AppIcon.iconset"
rm -rf "$ICONSET" && mkdir -p "$ICONSET"
for size in 16 32 128 256 512; do
  sips -z $size $size "$ICON_PNG" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
  sips -z $((size * 2)) $((size * 2)) "$ICON_PNG" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$STAGE/Contents/Resources/AppIcon.icns"

# --- resources -----------------------------------------------------------
cp "$ROOT/src/Singularity/Foreign/Web/dom_mirror.js" "$STAGE/Contents/Resources/Web/"
if [[ -d "$HERE/Foundation" ]]; then
  cp -R "$HERE/Foundation" "$STAGE/Contents/Resources/Foundation"
fi

# MCP server: the script plus exactly the packages it requires, resolved from
# the checkout's node_modules (offline, no npm install). Runs on system node.
cp "$ROOT/src/Singularity/Foreign/mcp/earthcall-mcp-server.js" "$STAGE/Contents/Resources/mcp/"
if [[ -d "$ROOT/node_modules/@modelcontextprotocol/sdk" ]]; then
  node - "$ROOT/node_modules" "$STAGE/Contents/Resources/mcp/node_modules" <<'NODE'
const fs = require("fs"), path = require("path");
const [src, dst] = process.argv.slice(2);
const seen = new Set();
(function visit(name) {
  if (seen.has(name)) return;
  const dir = path.join(src, name);
  if (!fs.existsSync(path.join(dir, "package.json"))) return; // optional / platform-specific
  seen.add(name);
  const pkg = JSON.parse(fs.readFileSync(path.join(dir, "package.json"), "utf8"));
  for (const dep of Object.keys(pkg.dependencies || {})) visit(dep);
})("@modelcontextprotocol/sdk");
for (const name of seen) fs.cpSync(path.join(src, name), path.join(dst, name), { recursive: true });
console.log(`package_app: MCP closure ${seen.size} packages`);
NODE
else
  echo "package_app: ⚠️  node_modules/@modelcontextprotocol/sdk missing (run npm ci); MCP server not runnable from the app" >&2
fi

# --- signing (inside out) ------------------------------------------------
IDENTITY="${EARTHCALL_SIGN_IDENTITY:-}"
if [[ -z "$IDENTITY" ]]; then
  if security find-identity -p codesigning 2>/dev/null | grep -q '"Earthcall Local Signing"'; then
    IDENTITY="Earthcall Local Signing"
  else
    IDENTITY="-"
    echo "package_app: ⚠️  ad-hoc signature: macOS will forget microphone/Accessibility grants on every rebuild." >&2
    echo "package_app:     run scripts/macos/make_local_signing_identity.sh once to keep them." >&2
  fi
fi
TIMESTAMP="--timestamp=none"
[[ "${EARTHCALL_SIGN_TIMESTAMP:-0}" == 1 ]] && TIMESTAMP="--timestamp"
sign() { codesign --force --options runtime $TIMESTAMP --sign "$IDENTITY" "$@"; }
sign "$STAGE/Contents/MacOS/earthcall_first_mover"
sign --entitlements "$HERE/Earthcall.entitlements" "$STAGE"
codesign --verify --strict --verbose=1 "$STAGE"

rm -rf "$APP"   # the previous build output; /Applications is install_app.sh's job
mv "$STAGE" "$APP"
echo "package_app: $APP  (version ${VERSION:-1.0} build $BUILD_NO, $COMMIT, signed: $IDENTITY)"
