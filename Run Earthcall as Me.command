#!/bin/bash
# Double-click: launch Earthcall with YOU present.
#
# Asks for your passphrase (hidden), checks it actually opens your key BEFORE
# launching, then starts Earthcall exactly as "Run Earthcall.command" does --
# with you already present, so First Movers you granted (Sonnet, ...) can act.
# If your Person has no key yet, it keys you (passphrase typed twice).
#
# Your passphrase is never written to disk, history, or a log. It lives only
# in this window's memory and in the Earthcall process it launches.
#
# Built 2026-10-01 by Claude Opus 5.5 at Zach's request ("make a executable
# thingy i can double click to have it prompt me automatically ... separate
# the person verification executable from the first mover one").
set -u
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1
FM="$ROOT/build/earthcall_first_mover"
# Test seams (unset in normal use): a different save folder / Keychain service.
SAVES_DIR="${EARTHCALL_SAVES_DIR:-}"
KC_SERVICE="${EARTHCALL_KEYCHAIN_SERVICE:-earthcall-first-mover}"
fm() { if [ -n "$SAVES_DIR" ]; then "$FM" "$@" --saves "$SAVES_DIR"; else "$FM" "$@"; fi; }

finish() {
  echo
  echo "Press Return to close this window."
  read -r
  exit "${1:-0}"
}

if [ ! -x "$FM" ]; then
  echo "Building the identity tool first..."
  cmake --build build --target earthcall_first_mover -j8 >/dev/null || {
    echo "Could not build build/earthcall_first_mover. Configure the build first (see AGENTS.md)."
    finish 1
  }
fi

echo "Earthcall · who's at the machine"
echo
EARTHCALL_KEY_PASSPHRASE="" fm check-person >/dev/null 2>&1
state=$?

if [ "$state" -eq 3 ]; then
  # ---- First time: key the Person --------------------------------------
  echo "Your Person has no key yet. Choose a passphrase to create one."
  echo "There is NO recovery if you lose it. At least 8 characters."
  while true; do
    printf "New passphrase (hidden): "
    IFS= read -rs P1; echo
    printf "Type it again (hidden):  "
    IFS= read -rs P2; echo
    if [ "$P1" != "$P2" ]; then echo "They differ. Try again."; echo; continue; fi
    if [ "${#P1}" -lt 8 ]; then echo "Too short. Try again."; echo; continue; fi
    break
  done
  unset P2
  export EARTHCALL_KEY_PASSPHRASE="$P1"
  unset P1
  export EARTHCALL_MIGRATE_PERSON_IDENTITY=1
  echo
  echo "Keying you and launching Earthcall..."
else
  # ---- Keyed: unlock -------------------------------------------------
  tries=0
  while true; do
    printf "Passphrase (hidden): "
    IFS= read -rs P1; echo
    if EARTHCALL_KEY_PASSPHRASE="$P1" fm check-person >/dev/null 2>&1; then
      export EARTHCALL_KEY_PASSPHRASE="$P1"
      unset P1
      echo "Key unlocked. Launching Earthcall with you present..."
      break
    fi
    unset P1
    tries=$((tries + 1))
    echo "That passphrase did not open your key."
    if [ "$tries" -ge 3 ]; then
      echo
      printf "Launch anyway, NOT present (models you granted will be read-only)? [y/N] "
      IFS= read -r ans
      case "$ans" in
        y|Y) unset EARTHCALL_KEY_PASSPHRASE; echo "Launching without you present..."; break ;;
        *) finish 1 ;;
      esac
    fi
  done
fi

echo
if [ -n "${EARTHCALL_LAUNCH_DRYRUN:-}" ]; then   # test seam: stop before launching
  echo "DRYRUN present=${EARTHCALL_KEY_PASSPHRASE:+yes} migrate=${EARTHCALL_MIGRATE_PERSON_IDENTITY:-0}"
  exit 0
fi
if "$ROOT/scripts/build.sh" webgpu run; then
  exit 0
else
  status=$?
  echo
  echo "Earthcall could not start (exit status $status)."
  finish "$status"
fi
