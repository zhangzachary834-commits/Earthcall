#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${EARTHCALL_SANDBOX_IMAGE:-earthcall-sandbox:latest}"

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker Desktop / docker CLI is required." >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "Docker is installed but the daemon is not reachable. Start Docker Desktop first." >&2
  exit 1
fi

echo "Building $IMAGE ..."
docker build \
  --pull \
  --tag "$IMAGE" \
  "$REPO_ROOT/scripts/gemini-sandbox"

echo
echo "Built $IMAGE"
echo "Runtime execution will use --network none and --pull=never."
