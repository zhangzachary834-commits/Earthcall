#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${EARTHCALL_MCP_PORT:-8765}"
HOST="${EARTHCALL_MCP_HOST:-127.0.0.1}"
TOKEN_SERVICE="earthcall-gemini-mcp"
TOKEN_ACCOUNT="${USER:-earthcall}"
SANDBOX_IMAGE="${EARTHCALL_SANDBOX_IMAGE:-earthcall-sandbox:latest}"

cd "$REPO_ROOT"

if ! command -v node >/dev/null 2>&1; then
  echo "Node.js is required." >&2
  exit 1
fi

if [ ! -d node_modules/@modelcontextprotocol/sdk ]; then
  echo "Earthcall's npm dependencies are not installed. Run: npm install" >&2
  exit 1
fi

if ! command -v cloudflared >/dev/null 2>&1; then
  echo "cloudflared is required for the temporary HTTPS tunnel." >&2
  echo "Install it with Homebrew: brew install cloudflared" >&2
  exit 1
fi

LAB_STATUS="unavailable"
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
  if docker image inspect "$SANDBOX_IMAGE" >/dev/null 2>&1; then
    LAB_STATUS="ready"
  else
    LAB_STATUS="image-missing"
  fi
fi

if [ -z "${EARTHCALL_MCP_TOKEN:-}" ]; then
  if command -v security >/dev/null 2>&1; then
    EARTHCALL_MCP_TOKEN="$(security find-generic-password -s "$TOKEN_SERVICE" -a "$TOKEN_ACCOUNT" -w 2>/dev/null || true)"
  else
    EARTHCALL_MCP_TOKEN=""
  fi
fi

if [ -z "${EARTHCALL_MCP_TOKEN:-}" ]; then
  EARTHCALL_MCP_TOKEN="$(openssl rand -hex 32)"
  if command -v security >/dev/null 2>&1; then
    security add-generic-password -U -s "$TOKEN_SERVICE" -a "$TOKEN_ACCOUNT" -w "$EARTHCALL_MCP_TOKEN" >/dev/null
    echo "Stored the Gemini MCP token in macOS Keychain (service: $TOKEN_SERVICE)."
  fi
fi

export EARTHCALL_MCP_TOKEN
export EARTHCALL_MCP_ROOT="$REPO_ROOT"
export EARTHCALL_MCP_HOST="$HOST"
export EARTHCALL_MCP_PORT="$PORT"
export EARTHCALL_SANDBOX_IMAGE="$SANDBOX_IMAGE"

LOG_DIR="${TMPDIR:-/tmp}/earthcall-gemini-mcp"
mkdir -p "$LOG_DIR"
CF_LOG="$LOG_DIR/cloudflared.log"
: > "$CF_LOG"

cleanup() {
  if [ -n "${CF_PID:-}" ]; then kill "$CF_PID" 2>/dev/null || true; fi
  if [ -n "${MCP_PID:-}" ]; then kill "$MCP_PID" 2>/dev/null || true; fi
}
trap cleanup EXIT INT TERM

node scripts/mcp-gemini-server.js &
MCP_PID=$!

for _ in $(seq 1 40); do
  if curl -fsS "http://$HOST:$PORT/healthz" >/dev/null 2>&1; then
    break
  fi
  if ! kill -0 "$MCP_PID" 2>/dev/null; then
    echo "Earthcall Gemini MCP exited during startup." >&2
    wait "$MCP_PID" || true
    exit 1
  fi
  sleep 0.25
done

if ! curl -fsS "http://$HOST:$PORT/healthz" >/dev/null 2>&1; then
  echo "Earthcall Gemini MCP did not become healthy on port $PORT." >&2
  exit 1
fi

echo
echo "Earthcall Gemini MCP is alive locally. Opening a temporary HTTPS tunnel..."
cloudflared tunnel --no-autoupdate --url "http://$HOST:$PORT" 2>&1 | tee "$CF_LOG" &
CF_PID=$!

PUBLIC_URL=""
for _ in $(seq 1 120); do
  PUBLIC_URL="$(grep -Eo 'https://[-a-zA-Z0-9]+\.trycloudflare\.com' "$CF_LOG" | head -n 1 || true)"
  if [ -n "$PUBLIC_URL" ]; then break; fi
  if ! kill -0 "$CF_PID" 2>/dev/null; then
    echo "cloudflared exited before creating a tunnel." >&2
    wait "$CF_PID" || true
    exit 1
  fi
  sleep 0.25
done

if [ -z "$PUBLIC_URL" ]; then
  echo "Could not detect the Cloudflare URL. Check: $CF_LOG" >&2
  exit 1
fi

cat <<MSG

======================================================================
  EARTHCALL <-> GEMINI GATE IS OPEN  (VISION + LAB)
======================================================================

Gemini MCP URL:

  ${PUBLIC_URL}/mcp?token=${EARTHCALL_MCP_TOKEN}

Sandbox laboratory: ${LAB_STATUS}
Sandbox image:      ${SANDBOX_IMAGE}

In Gemini web:
  Settings -> Gemini Apps Developer Mode -> Create new
  MCP Server URL -> paste the URL above
  Authorization -> No (the temporary URL already carries a bearer token)

Real-workspace observation tools:
  earthcall_list_directory
  earthcall_read_file
  earthcall_search_text
  earthcall_git_status
  earthcall_git_diff

Disposable laboratory tools:
  earthcall_get_sandbox_policy
  earthcall_sandbox_create
  earthcall_sandbox_read_file
  earthcall_sandbox_write_file
  earthcall_sandbox_run
  earthcall_sandbox_diff
  earthcall_get_approval_status
  earthcall_sandbox_discard

Manual approval command:
  node scripts/earthcall-mcp-approve.js <approval-id>

If sandbox laboratory is 'image-missing', run in another terminal:
  ./scripts/build-gemini-sandbox.sh

Test prompt:
  @Earthcall Local create a sandbox, run node --version in it, then show the sandbox diff.

Keep this terminal open. Closing it closes the temporary gate.
Treat the full URL as a secret because it contains the access token.
======================================================================

MSG

wait "$CF_PID"
