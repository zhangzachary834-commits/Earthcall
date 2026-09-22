#!/usr/bin/env bash
# Double-click this file in Finder to start Earthcall's Python backend.
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$ROOT/src/Singularity/Foreign/py"
PYTHON="$APP_DIR/venv/bin/python"
PORT=5005

if [ ! -x "$PYTHON" ]; then
  echo "Earthcall's Python environment is not set up yet."
  echo "Run $APP_DIR/setup_env.sh once, then try again."
  echo
  echo "Press Return to close this window."
  read -r
  exit 1
fi

echo "========================================================"
echo "🌍 Earthcall Python Backend & First Mover Studio"
echo "========================================================"

# Terminate any existing Python server sessions on port 5005 and 5001
echo "[1/3] Terminating any existing Python server sessions..."
lsof -ti:5005 | xargs kill -9 2>/dev/null || true
lsof -ti:5001 | xargs kill -9 2>/dev/null || true
pkill -f "src/Singularity/Foreign/py/app.py" 2>/dev/null || true
sleep 0.8

# Launch default web browser to open the UI
echo "[2/3] Opening Earthcall Studio at http://127.0.0.1:$PORT..."
(
  sleep 1.2
  if command -v open >/dev/null 2>&1; then
    open "http://127.0.0.1:$PORT"
  elif command -v xdg-open >/dev/null 2>&1; then
    xdg-open "http://127.0.0.1:$PORT"
  fi
) &

# Run Python app from scratch
echo "[3/3] Launching Python Server on port $PORT..."
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
