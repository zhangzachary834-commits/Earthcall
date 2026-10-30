#!/usr/bin/env bash
# Double-click this file in Finder to build Earthcall.app (Release) and install
# it in /Applications. Your data is untouched: it lives in
# ~/Library/Application Support/Earthcall. See scripts/macos/install_app.sh.
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"

if "$ROOT/scripts/macos/install_app.sh"; then
  open -R /Applications/Earthcall.app
  echo
  echo "Earthcall.app is installed. Press Return to close this window."
  read -r
  exit 0
else
  status=$?
  echo
  echo "Earthcall.app could not be installed (exit status $status)."
  echo "Press Return to close this window."
  read -r
  exit "$status"
fi
