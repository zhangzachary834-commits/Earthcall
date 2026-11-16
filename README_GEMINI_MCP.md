# Earthcall Gemini MCP Gate — Vision + Disposable Laboratory

This bridge restores Gemini Tasks' ability to inspect the **actual local Earthcall checkout** and adds a tightly bounded execution laboratory for experiments and tests.

The remote Gemini gate observes the real checkout and edits or runs commands inside a disposable sandbox copy. Use the local import command to bring the resulting changes into the checkout.

## Architecture

```text
Gemini Task / Firmament / Hydrogen
              |
       public HTTPS MCP
              |
      127.0.0.1:8765
              |
  Earthcall Gemini MCP broker
       /               \
      /                 \
 real checkout        disposable lab
 READ ONLY            WRITE + EXECUTE
      |                    |
 root jail           Docker container
                    network = none
                    rootfs = read-only
                    cap-drop = ALL
                    no-new-privileges
                    CPU/RAM/PID/time bounds
                    no Docker socket
                    no host HOME/secrets
```

The sandbox source copy includes current uncommitted local files, but excludes `.git`, `node_modules`, `build`, caches, and common secret-bearing paths/files (`.env`, `.ssh`, `.aws`, private-key formats, credentials files, etc.). `.env.example` / `.env.sample` remain available as non-secret examples.

## Real-checkout observation tools

- `earthcall_list_directory`
- `earthcall_read_file`
- `earthcall_search_text`
- `earthcall_git_status`
- `earthcall_git_diff`

These remain read-only and root-jailed to Earthcall.

## Disposable laboratory tools

- `earthcall_get_sandbox_policy` — inspect current approval policy.
- `earthcall_sandbox_create` — copy the current working tree into a disposable lab session.
- `earthcall_sandbox_read_file` — inspect the lab copy.
- `earthcall_sandbox_write_file` — edit only the lab copy.
- `earthcall_sandbox_run` — execute a structured `argv` in the Docker sandbox.
- `earthcall_sandbox_diff` — compare the lab copy against the source snapshot.
- `earthcall_get_approval_status` — inspect a pending/approved/denied request.
- `earthcall_sandbox_discard` — destroy the lab session.

There is deliberately **no shell-string tool**. Commands are represented as an argv array, e.g.:

```json
{
  "argv": ["ctest", "--test-dir", "build", "-R", "mcp"]
}
```

A shell interpreter such as `bash` can still be invoked as a program if policy permits, but the default review list makes shell interpreters require manual approval.

## Bring Gemini's changes into your checkout

### Mac app

Open **Earthcall Sandbox Import**, select a session, then review the diff and click
**Import & Open GitHub Desktop**. Review the imported files in GitHub Desktop's
**Changes** tab. **Refresh** (⌘R) picks up new sessions and sandbox edits. If the
patch changes after preview, the app asks you to refresh before importing.

To build the app for this checkout, run `bash scripts/macos/build_sandbox_import_app.sh`.
It prints the path to the signed `.app` in `build/sandbox-import/`. Open it in Finder
or copy it into `~/Applications`. The app bundles the importer and uses the
checkout and Node executable recorded at build time. It calls GitHub Desktop's
[command-line launcher](https://docs.github.com/en/desktop/overview/launching-github-desktop-from-the-command-line)
with the checkout path.

### Terminal

From the checkout that created the session, use its label or the ID from `list`:

```bash
node scripts/earthcall-sandbox-import.js list
node scripts/earthcall-sandbox-import.js preview horizon-resurrection
node scripts/earthcall-sandbox-import.js apply horizon-resurrection
```

`preview` shows the Git diff and checks whether it applies. `apply` brings additions,
edits, and deletions of regular files into your working tree as unstaged changes.
It uses the session's creation manifest so uncommitted edits already present when
Gemini started count as baseline. Existing sessions record file size and modification
time, so conflict detection depends on those values. The importer retains existing
executable bits, skips Git-ignored output and submodules, and leaves the sandbox
available for another preview or apply. It runs locally without Docker or an MCP
restart.

You can import the same session again when Gemini appends to an existing file.
If the sandbox file starts with every byte currently in the checkout, the preview
shows the additional content. Divergent edits stop the batch and print the
conflicting paths. From the terminal, use `--` followed by checkout-relative
paths to transfer an unaffected subset:

```bash
node scripts/earthcall-sandbox-import.js apply horizon-resurrection -- scripts/horizon_resurrection.py
```

*Codex · GPT-6 · session `01a1e178-dec7-7060-b866-efbe0035660d` ·
2026-11-15 15:16 PST. Zach requested an end to the “Zach's Fingers Transmission Network.”*

## Approval policy

Policy lives at:

```text
config/earthcall-gemini-sandbox-policy.json
```

The server reloads it on each action, so changes apply without restarting the MCP gate.

### File edit modes

- `manual` — every sandbox file edit creates a manual approval request.
- `auto_sandbox` — every sandbox file edit is automatically accepted. **Default.**
- `auto_safe_paths` — edits auto-apply except configured path patterns (for example scripts/config/build files).

### Command modes

- `manual` — every command requires local approval.
- `auto` — all sandbox commands auto-run at the approval layer.
- `auto_except_review` — commands auto-run except those whose program or argv text matches configured review entries. **Default.**

These modes control *approval friction*, not the sandbox's hard capabilities. Even `commands.mode = auto` does not grant network, host-home access, Docker socket access, or writes to the real Earthcall checkout.

### Convenient local presets

```bash
node scripts/earthcall-mcp-policy.js show
node scripts/earthcall-mcp-policy.js lockdown   # manual edits + manual commands
node scripts/earthcall-mcp-policy.js lab        # auto edits + auto commands except review list
node scripts/earthcall-mcp-policy.js auto       # auto edits + auto sandbox commands
```

Add/remove custom review conditions:

```bash
node scripts/earthcall-mcp-policy.js add-review-token "--hard"
node scripts/earthcall-mcp-policy.js remove-review-token "--hard"
node scripts/earthcall-mcp-policy.js add-review-program curl
node scripts/earthcall-mcp-policy.js remove-review-program curl
```

The exact-token list is a **policy convenience**, not the security wall. The real boundary is the container capability cage.

## Manual approvals are local-only

If a tool returns:

```text
status: approval_required
approval_id: <id>
```

Gemini cannot approve it. On the Mac, inspect and approve the exact action:

```bash
node scripts/earthcall-mcp-approve.js <approval-id>
```

or explicitly:

```bash
node scripts/earthcall-mcp-approve.js <approval-id> --yes
node scripts/earthcall-mcp-approve.js <approval-id> --deny
```

Approval records live outside the repo in Earthcall's local application-state directory. Each approval is cryptographically fingerprinted to the exact action and is one-shot. Changing argv, cwd, path, or file content invalidates the approval.

## Build the laboratory image

Install/start Docker Desktop, then:

```bash
./scripts/build-gemini-sandbox.sh
```

This builds `earthcall-sandbox:latest`, based on Node 22 / Debian with common CPU-development tools (`clang`, `cmake`, `ninja`, `git`, `python3`, `ripgrep`, etc.). Building the image may use the network; **runtime sandbox execution uses `--network none` and `--pull=never`.**

The Linux lab is authoritative for generic Node/CPU/build logic, not for native macOS/Metal/WebGPU performance or behavior. Native renderer/performance verification belongs in a later macOS-native/VM execution profile.

## Start the gate

```bash
npm install
brew install cloudflared
./scripts/build-gemini-sandbox.sh
./scripts/start-gemini-mcp.sh
```

The launcher obtains/creates a 256-bit access token in macOS Keychain, starts the localhost MCP server, opens a Cloudflare Quick Tunnel, and prints the Gemini MCP URL.

Treat the full `.../mcp?token=...` URL as a secret. Quick Tunnel + query-token authentication is a development/bootstrap configuration, not the desired permanent production design.

## Suggested Firmament test

```text
@Earthcall Local
Create a disposable sandbox named "first-lab".
Read package.json from the sandbox.
Run ["node", "--version"].
Then run the Gemini MCP policy/unit tests.
Show me the resulting sandbox diff.
Do not modify the real Earthcall checkout.
```

For a manual-approval demonstration:

```bash
node scripts/earthcall-mcp-policy.js lockdown
```

Then ask Gemini to edit a sandbox file or run a command. It should return an approval id rather than executing.

## Verification

```bash
npm run mcp:gemini:test
```

The tests cover:

- root/path confinement
- secret/binary refusal for real-checkout reads
- sandbox secret exclusion
- sandbox edits never touching the source repo
- manual file approvals and one-shot exact-action fingerprints
- auto/manual/auto-except-review policy behavior
- token/program review matching
- sandbox diff generation
- traversal refusal
- Docker command construction enforcing `--network none`, read-only root, capability drop and no-new-privileges

A live Docker execution smoke test must be run on a machine with Docker Desktop and the sandbox image built.
