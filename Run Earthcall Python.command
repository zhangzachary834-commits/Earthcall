#!/usr/bin/env bash
# Double-click this file in Finder to start Earthcall's Python backend.
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$ROOT/src/Singularity/Foreign/py"
PYTHON="$APP_DIR/venv/bin/python"

if [ ! -x "$PYTHON" ]; then
  echo "Earthcall's Python environment is not set up yet."
  echo "Run $APP_DIR/setup_env.sh once, then try again."
  echo
  echo "Press Return to close this window."
  read -r
  exit 1
fi

cd "$APP_DIR"
if "$PYTHON" app.py; then
  exit 0
else
  status=$?
  echo
  echo "Earthcall Python backend stopped (exit status $status)."
  echo "Press Return to close this window."
  read -r
  exit "$status"
fi
