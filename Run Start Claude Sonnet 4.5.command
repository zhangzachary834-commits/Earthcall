#!/bin/bash
# Double-click: a Claude Code session that IS "Claude Sonnet 4.5" in Earthcall, acting as
# First Mover did:earthcall:ynqocokbbbzoiirpkw5ew47pxaz3zps7estt4e6frsm32iboqnja. Written by "Earthcall First Movers.command".
# Its key's passphrase is read from your Keychain; nothing secret is here.
cd "/Users/zacharyzhang/Documents/GitHub/Earthcall" || exit 1
exec "/Users/zacharyzhang/.local/bin/claude" --model "claude-sonnet-4-5" --mcp-config "/Users/zacharyzhang/.earthcall/movers/claude-sonnet-4-5-mcp.json" --strict-mcp-config
