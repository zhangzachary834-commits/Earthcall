#!/usr/bin/env bash
# Double-click this file in Finder to configure, build, and start Earthcall's WebGPU app.
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"

if "$ROOT/scripts/build.sh" webgpu run; then
  exit 0
else
  status=$?
  echo
  echo "Earthcall could not start (exit status $status)."
  echo "Press Return to close this window."
  read -r
  exit "$status"
fi
