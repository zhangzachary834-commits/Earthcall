#!/bin/bash
# Double-click: give a model (Claude Sonnet 4.5, ...) standing to act in
# Earthcall as a First Mover, change what it may touch, or take it back.
#
#   1  Add a model      mints its key, keeps the key's passphrase in your
#                       macOS Keychain (generated; you never need to know it),
#                       grants it scopes with YOUR key, and writes
#                       "Start <name>.command" -- double-click that to open a
#                       Claude Code session that IS that model in Earthcall.
#   2  Change scopes    re-grant with new scopes
#   3  Remove a model   revoke its standing (its past authorship stays true)
#   4  List
#
# Granting needs your passphrase (asked hidden, checked before use). The
# project's shared MCP config is never touched: if it named one model, every
# Claude session here would act as that model, and authorship would lie.
#
# Built 2026-10-01 by Claude Opus 5.5 at Zach's request.
set -u
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT" || exit 1
FM="$ROOT/build/earthcall_first_mover"
# Test seams (unset in normal use): a different save folder / Keychain service.
SAVES_DIR="${EARTHCALL_SAVES_DIR:-}"
KC_SERVICE="${EARTHCALL_KEYCHAIN_SERVICE:-earthcall-first-mover}"
fm() { if [ -n "$SAVES_DIR" ]; then "$FM" "$@" --saves "$SAVES_DIR"; else "$FM" "$@"; fi; }
MOVERS_DIR="${EARTHCALL_HOME:-$HOME/.earthcall}/movers"
LAUNCHER_DIR="${EARTHCALL_LAUNCHER_DIR:-$ROOT}"   # test seam
mkdir -p "$MOVERS_DIR"

finish() { echo; echo "Press Return to close this window."; read -r; exit "${1:-0}"; }

if [ ! -x "$FM" ]; then
  echo "Building the identity tool first..."
  cmake --build build --target earthcall_first_mover -j8 >/dev/null || {
    echo "Could not build build/earthcall_first_mover. Configure the build first (see AGENTS.md)."
    finish 1
  }
fi

# Your passphrase, checked against your key before anything is granted.
ask_person() {
  EARTHCALL_KEY_PASSPHRASE="" fm check-person >/dev/null 2>&1
  if [ $? -eq 3 ]; then
    echo "Your Person has no key yet. Double-click \"Run Earthcall as Me.command\" first; it keys you."
    finish 1
  fi
  local tries=0
  while [ $tries -lt 3 ]; do
    printf "YOUR passphrase (hidden): "
    IFS= read -rs PP; echo
    if EARTHCALL_KEY_PASSPHRASE="$PP" fm check-person >/dev/null 2>&1; then
      export EARTHCALL_KEY_PASSPHRASE="$PP"
      unset PP
      return 0
    fi
    unset PP
    tries=$((tries + 1))
    echo "That did not open your key."
  done
  echo "Giving up after 3 tries; nothing was changed."
  finish 1
}

slug() { echo "$1" | tr '[:upper:]' '[:lower:]' | sed -E 's/[^a-z0-9]+/-/g; s/^-+|-+$//g'; }

ask_scopes() {   # sets SCOPE_ARGS from a Zone, a Law prefix, and snapshots
  local name="$1" word zone prefix snap
  word=$(echo "$name" | sed -E 's/^[Cc]laude[[:space:]]+//' | awk '{print $1}')
  [ -z "$word" ] && word="Model"
  printf "Zone it may build in [%sGarden]: " "$word"; IFS= read -r zone
  [ -z "$zone" ] && zone="${word}Garden"
  prefix=$(slug "$word")
  printf "Its Laws' identifiers start with [%s-]: " "$prefix"; IFS= read -r p2
  [ -n "$p2" ] && prefix="${p2%-}"
  printf "Allow it to take screen snapshots (so it can SEE)? [Y/n] "; IFS= read -r snap
  SCOPE_ARGS=(--scope "zones/$zone/**" --scope "laws/$prefix-*/**")
  case "$snap" in n|N) ;; *) SCOPE_ARGS+=(--scope "laws/screen-recorder/**") ;; esac
  echo "Scopes: ${SCOPE_ARGS[*]}" | sed 's/--scope //g'
}

pick_mover() {   # sets MOVER_ID / MOVER_NAME from the register
  local ids names i=1 choice
  ids=(); names=()
  while IFS=$'\t' read -r id name; do ids+=("$id"); names+=("$name"); done < <(
    fm list 2>/dev/null | awk '/^[^ ]/ && /did:earthcall:/ {id=$NF; $NF=""; sub(/[ \t]+$/,""); print id "\t" $0}')
  if [ ${#ids[@]} -eq 0 ]; then echo "No models have standing yet."; finish 0; fi
  for n in "${names[@]}"; do echo "  $i  $n"; i=$((i + 1)); done
  printf "Which one? "; IFS= read -r choice
  if ! [[ "$choice" =~ ^[0-9]+$ ]] || [ "$choice" -lt 1 ] || [ "$choice" -gt ${#ids[@]} ]; then
    echo "No such choice."; finish 1
  fi
  MOVER_ID="${ids[$((choice - 1))]}"; MOVER_NAME="${names[$((choice - 1))]}"
}

write_launcher() {   # $1 name, $2 mover id
  local name="$1" id="$2" model cfg launcher
  case "$name" in
    *[Ss]onnet*4.5*|*[Ss]onnet*4-5*) model="claude-sonnet-4-5" ;;
    *) model="" ;;
  esac
  printf "Claude Code model for %s [%s]: " "$name" "${model:-type the model id}"; IFS= read -r m2
  [ -n "$m2" ] && model="$m2"
  [ -z "$model" ] && { echo "No model id; skipping the launcher."; return; }
  cfg="$MOVERS_DIR/$(slug "$name")-mcp.json"
  # Absolute paths: a Finder-launched window may not have Homebrew or
  # ~/.local/bin on PATH.
  local node_bin claude_bin
  node_bin="$(command -v node || echo node)"
  claude_bin="$(command -v claude || echo claude)"
  cat > "$cfg" <<JSON
{
  "mcpServers": {
    "earthcall": {
      "type": "stdio",
      "command": "$node_bin",
      "args": ["$ROOT/scripts/mcp-server.js"],
      "env": {
        "EARTHCALL_FIRST_MOVER_ID": "$id",
        "EARTHCALL_FIRST_MOVER_SIGNER": "$FM"
      }
    }
  }
}
JSON
  launcher="$LAUNCHER_DIR/Start $name.command"
  cat > "$launcher" <<SH
#!/bin/bash
# Double-click: a Claude Code session that IS "$name" in Earthcall, acting as
# First Mover $id. Written by "Earthcall First Movers.command".
# Its key's passphrase is read from your Keychain; nothing secret is here.
cd "$ROOT" || exit 1
exec "$claude_bin" --model "$model" --mcp-config "$cfg" --strict-mcp-config
SH
  chmod +x "$launcher"
  echo "Wrote \"Start $name.command\": double-click it to open $name in Earthcall."
}

echo "Earthcall · First Movers"
echo
echo "  1  Add a model"
echo "  2  Change what a model may touch"
echo "  3  Remove a model"
echo "  4  List"
printf "Choose 1-4: "; IFS= read -r CHOICE
echo

case "$CHOICE" in
1)
  printf "Model's name (e.g. Claude Sonnet 4.5): "; IFS= read -r NAME
  [ -z "$NAME" ] && { echo "A name is required."; finish 1; }
  ask_person
  ask_scopes "$NAME"
  # The mover's own passphrase: random, never shown, kept only in Keychain.
  MP=$(openssl rand -base64 32) || { echo "Could not generate a passphrase."; finish 1; }
  ID=$(EARTHCALL_MOVER_PASSPHRASE="$MP" fm mint --name "$NAME" 2>/dev/null | tail -1)
  case "$ID" in did:earthcall:*) ;; *) unset MP; echo "Minting the key failed."; finish 1 ;; esac
  security add-generic-password -U -s "$KC_SERVICE" -a "$ID" -w "$MP" >/dev/null 2>&1 || {
    unset MP; echo "Could not save the key's passphrase to your Keychain."; finish 1; }
  unset MP
  KP=$(security find-generic-password -s "$KC_SERVICE" -a "$ID" -w 2>/dev/null)
  if ! EARTHCALL_MOVER_PASSPHRASE="$KP" fm check-mover --mover "$ID" >/dev/null 2>&1; then
    unset KP; echo "The Keychain entry does not open the new key; nothing granted."; finish 1
  fi
  unset KP
  fm grant --mover "$ID" --name "$NAME" "${SCOPE_ARGS[@]}" || finish 1
  echo
  write_launcher "$NAME" "$ID"
  echo
  echo "$NAME is $ID"
  echo "It can act only while YOU are present: launch with \"Run Earthcall as Me.command\"."
  ;;
2)
  pick_mover
  ask_person
  ask_scopes "$MOVER_NAME"
  fm grant --mover "$MOVER_ID" --name "$MOVER_NAME" "${SCOPE_ARGS[@]}" || finish 1
  ;;
3)
  pick_mover
  printf "Remove %s's standing? [y/N] " "$MOVER_NAME"; IFS= read -r ok
  case "$ok" in y|Y) ;; *) echo "Nothing changed."; finish 0 ;; esac
  ask_person
  fm revoke --mover "$MOVER_ID" || finish 1
  echo "Its key stays in your Keychain and ~/.earthcall/identity; it simply no longer stands."
  ;;
4)
  fm list
  ;;
*)
  echo "Nothing chosen."
  ;;
esac
finish 0
