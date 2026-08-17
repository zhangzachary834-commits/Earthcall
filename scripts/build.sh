#!/usr/bin/env bash
# Earthcall build helper
# Usage:
#   ./scripts/build.sh              # configure + build
#   ./scripts/build.sh run          # configure + build + run
#   ./scripts/build.sh webgpu run   # configure + build the WebGPU app + run
#   ./scripts/build.sh test         # configure + build + test
#   ./scripts/build.sh quick        # build only (skip configure)
#   ./scripts/build.sh quick run    # build only + run
#   ./scripts/build.sh clean        # wipe build dir and reconfigure
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
JOBS="${EARTHCALL_JOBS:-8}"

configure() {
  cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE="${EARTHCALL_BUILD_TYPE:-Debug}" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DOPENSSL_ROOT_DIR="$ROOT/local_deps/openssl-3.0.13" \
    -DOPENSSL_INCLUDE_DIR="$ROOT/local_deps/openssl-3.0.13/include" \
    -DOPENSSL_CRYPTO_LIBRARY="$ROOT/local_deps/openssl-3.0.13/libcrypto.a" \
    -DOPENSSL_SSL_LIBRARY="$ROOT/local_deps/openssl-3.0.13/libssl.a"
}

build() {
  cmake --build "$BUILD" --target earthcall -j"$JOBS"
}

build_webgpu() {
  cmake --build "$BUILD" --target earthcall_webgpu -j"$JOBS"
}

build_tests() {
  cmake --build "$BUILD" -j"$JOBS"
}

run() {
  exec "$BUILD/earthcall" "$@"
}

run_webgpu() {
  exec "$BUILD/earthcall_webgpu" "$@"
}

test_all() {
  build_tests
  ctest --test-dir "$BUILD" --output-on-failure -j4
}

# --- main ---
ACTION="${1:-}"
shift 2>/dev/null || true

case "$ACTION" in
  quick)
    build
    if [[ "${1:-}" == "run" ]]; then shift; run "$@"; fi
    ;;
  test)
    configure
    test_all
    ;;
  run)
    configure
    build
    run "$@"
    ;;
  webgpu)
    configure
    build_webgpu
    if [[ "${1:-}" == "run" ]]; then shift; run_webgpu "$@"; fi
    ;;
  clean)
    rm -rf "$BUILD"
    configure
    build
    ;;
  "")
    configure
    build
    ;;
  *)
    echo "Unknown action: $ACTION"
    echo "Usage: $0 [run|webgpu [run]|test|quick|clean]"
    exit 1
    ;;
esac
