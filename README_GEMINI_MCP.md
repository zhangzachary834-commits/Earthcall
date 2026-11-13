# Earthcall Gemini Local MCP Bridge

A read-only MCP gateway that restores Gemini's ability to inspect the **actual local Earthcall checkout** even when Gemini Tasks' native Mac folder binding is missing.

## Why this exists

Earthcall already has a powerful MCP bridge for interacting with the live world and First Mover system. This gateway is intentionally separate: it exposes only source-workspace observation tools over a remote HTTPS MCP endpoint so a Gemini Task can read local files and local git state without exposing Earthcall's mutation-capable world tools to the public internet.

The server uses the repository's existing `@modelcontextprotocol/sdk` v1 line, which implements MCP through `2025-11-25` — the same maximum protocol revision currently documented by Google's Connected Apps Platform. Gemini requires Streamable HTTP at a public HTTPS endpoint.

## Tools

- `earthcall_list_directory` — enumerate a directory inside the Earthcall checkout.
- `earthcall_read_file` — read a UTF-8 text file with line numbers and paging.
- `earthcall_search_text` — literal text search across the local checkout, with glob filtering.
- `earthcall_git_status` — inspect local/uncommitted working-tree state.
- `earthcall_git_diff` — inspect local or staged diffs, optionally scoped to a path.

Every tool is read-only and carries MCP `readOnlyHint` / non-destructive annotations.

## Safety boundary

The root jail is enforced in code, not by prompt. Canonical paths must remain under `EARTHCALL_MCP_ROOT`; symlink escapes and `..` traversal are refused. The bridge also refuses secret-like paths such as `.env`, private-key formats, `.git`, `.ssh`, and `secrets/` by default. Binary files are not returned. Search skips bulky generated trees including `.git`, `node_modules`, and `build`.

The HTTP server binds to `127.0.0.1` by default. Remote access is expected to arrive through an HTTPS tunnel. A minimum-24-character bearer token is mandatory. For the Gemini custom-app bootstrap flow, the launcher carries this token in the endpoint query string because Gemini Developer Mode does not provide an arbitrary-header field. Treat the complete endpoint URL as a secret.

For long-lived use, replace the temporary bearer-URL mechanism with OAuth 2.1 / PKCE using Gemini Connected Apps' supported authentication flow.

## Fastest macOS setup

From the Earthcall repository:

```bash
npm install
brew install cloudflared
./scripts/start-gemini-mcp.sh
```

The launcher:

1. Finds or creates a 256-bit token. On macOS it stores the token in Keychain under service `earthcall-gemini-mcp`.
2. Starts the read-only MCP server on `127.0.0.1:8765`.
3. Starts a temporary Cloudflare Quick Tunnel.
4. Prints the complete Gemini MCP URL.

Then open Gemini on the web:

1. `Settings` -> `Gemini Apps Developer Mode`.
2. `Create new` custom MCP app.
3. Paste the printed `https://...trycloudflare.com/mcp?token=...` URL.
4. Choose **No authorization** for this temporary bootstrap endpoint.
5. Confirm that all five Earthcall tools appear.
6. Save it as something like **Earthcall Local**.

Example test:

```text
@Earthcall Local list the repository root, inspect git status, then find every file mentioning "Northern Veil".
```

## Manual server start

If you do not want the launcher:

```bash
export EARTHCALL_MCP_TOKEN="$(openssl rand -hex 32)"
export EARTHCALL_MCP_ROOT="/Users/zacharyzhang/Documents/GitHub/Earthcall"
node scripts/mcp-gemini-server.js
```

The local endpoint is:

```text
http://127.0.0.1:8765/mcp?token=<token>
```

Health check:

```bash
curl http://127.0.0.1:8765/healthz
```

Expose only the localhost port through your chosen HTTPS tunnel, then give Gemini the resulting HTTPS `/mcp?token=...` endpoint.

## Configuration

- `EARTHCALL_MCP_ROOT` — root jail; defaults to the repository containing `scripts/mcp-gemini-server.js`.
- `EARTHCALL_MCP_HOST` — bind host; defaults to `127.0.0.1`.
- `EARTHCALL_MCP_PORT` — bind port; defaults to `8765`.
- `EARTHCALL_MCP_TOKEN` — required bearer token, minimum 24 characters.
- `EARTHCALL_MCP_ALLOW_SENSITIVE=1` — disables secret-like-file refusal. Avoid this for a remotely reachable server.

## Verification

The repository-tools module has Node built-in tests covering:

- root traversal confinement
- directory listing
- paged/numbered reads
- secret and binary refusal
- literal glob-filtered search
- symlink escape prevention
- read-only git status/diff behavior

Run them with:

```bash
node --test tests/singularity/mcp_gemini_repo_tools_test.js
```

## Architecture

```text
Gemini Task / Firmament / Hydrogen
              |
        Streamable HTTP MCP
              |
       public HTTPS tunnel
              |
        127.0.0.1:8765
              |
   Earthcall Gemini MCP gateway
              |
     hard read-only root jail
              |
 /Users/zacharyzhang/Documents/GitHub/Earthcall
```

The existing Earthcall First Mover MCP remains separate and unchanged. This is deliberate: observation can be remotely exposed with a narrow capability surface, while world mutation stays behind Earthcall's Person-authored First Mover authorization boundary.
