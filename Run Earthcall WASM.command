#!/usr/bin/env bash
# Double-click this file in Finder to rebuild and serve Earthcall's WASM app.
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build-wasm"

pause_on_error() {
  status=$1
  echo
  echo "Earthcall WASM could not start (exit status $status)."
  echo "Press Return to close this window."
  read -r
  exit "$status"
}

if ! command -v emcmake >/dev/null 2>&1 && [ -f "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" ]; then
  # Finder's Terminal session may not have inherited the shell that activated Emscripten.
  source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh"
fi

if ! command -v emcmake >/dev/null 2>&1; then
  echo "Emscripten is not available in this Terminal session."
  echo "Activate the Emscripten SDK (for example: source /path/to/emsdk/emsdk_env.sh) and try again."
  pause_on_error 1
fi

if emcmake cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE="${EARTHCALL_BUILD_TYPE:-Debug}"; then
  :
else
  pause_on_error $?
fi

if cmake --build "$BUILD" --target earthcall_wasm -j"${EARTHCALL_JOBS:-8}"; then
  :
else
  pause_on_error $?
fi

echo
echo "Earthcall WASM is available at http://localhost:8000/web_ui/wasm.html"
echo "Keep this Terminal window open while using the app."
exec python3 "$ROOT/scripts/serve_wasm.py" 8000 ..
