#!/usr/bin/env bash
# Build the release app and install it as /Applications/Earthcall.app.
#
#   scripts/macos/install_app.sh
#
# Env: EARTHCALL_INSTALL_DIR (default /Applications), EARTHCALL_JOBS (8), plus
# package_app.sh's signing variables. The previous install is kept once as
# Earthcall.previous.app. The Person's data is never touched: it lives in
# ~/Library/Application Support/Earthcall, not in the app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$ROOT/build_release"
DEST="${EARTHCALL_INSTALL_DIR:-/Applications}"
JOBS="${EARTHCALL_JOBS:-8}"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$ROOT/local_deps/openssl-3.0.22" \
  -DOPENSSL_INCLUDE_DIR="$ROOT/local_deps/openssl-3.0.22/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$ROOT/local_deps/openssl-3.0.22/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$ROOT/local_deps/openssl-3.0.22/libssl.a" >/dev/null
cmake --build "$BUILD" --target earthcall_app -j"$JOBS"

APP="$BUILD/Earthcall.app"
codesign --verify --strict "$APP"

# Stage beside the destination, then swap by rename.
mkdir -p "$DEST"
rm -rf "$DEST/Earthcall.app.new"
ditto "$APP" "$DEST/Earthcall.app.new"
if [[ -d "$DEST/Earthcall.app" ]]; then
  rm -rf "$DEST/Earthcall.previous.app"
  mv "$DEST/Earthcall.app" "$DEST/Earthcall.previous.app"
fi
mv "$DEST/Earthcall.app.new" "$DEST/Earthcall.app"

# The installed app keeps window layout in <data home>/ui/imgui.ini. Carry the
# checkout's layout over once, so the app opens the way the checkout did.
DATA="$(python3 "$ROOT/scripts/earthcall_data_home.py")"
if [[ -f "$DATA/data-home.json" && ! -f "$DATA/ui/imgui.ini" && -f "$ROOT/imgui.ini" ]]; then
  mkdir -p "$DATA/ui"
  cp "$ROOT/imgui.ini" "$DATA/ui/imgui.ini"
  echo "install_app: copied the checkout's window layout to $DATA/ui/imgui.ini"
fi

echo "install_app: installed $DEST/Earthcall.app — open it from Spotlight, Launchpad, or Finder."
echo "install_app: logs: ~/Library/Logs/Earthcall/earthcall.log"
