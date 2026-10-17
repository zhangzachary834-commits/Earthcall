# Data home outside the checkout

**Origin (Zach, 2026-10-12):** "right now Earthcall stores most of its data inside the repository folder … we should make it store it like an actual app and especially sacred vessels of the Person and their Home and their Joys and Relations and other things like Laws/Zones/Objects in a higher up place on disk." Zach chose, when asked: Application Support as the location, the data home as **its own git repository** so history continues there, and **copy everything except `backups/`**. He also noted that `~/Earthcall` is a very stale toy-physics-engine copy of the repository, untouched here, which he might repurpose later.

## What landed (2026-10-12)

| Piece | Where |
|---|---|
| Platform data home: `$EARTHCALL_DATA`, else macOS `~/Library/Application Support/Earthcall`, Linux `$XDG_DATA_HOME/earthcall` (`~/.local/share/earthcall`), Windows `%APPDATA%\Earthcall` | `SaveSystem::platformDataHome` |
| Boot choice: a home carrying `data-home.json` wins. Otherwise the checkout's `saves/` is used and the engine **says so loudly**. With neither, the home is created and marked (an installed app's first run). | `SaveSystem::chooseDataRoot`, called from `Engine::init` and `earthcall_first_mover` |
| One fallback rule (empty root means `./saves`, which tests rely on) | `SaveSystem::rootPath`, replacing seven copies of `empty() ? "saves" : root` |
| Authored `"saves/..."` strings inside Laws and materials (e.g. `load-genesis-file` → `saves/seed.ecgraph`, `export-world-json` → `saves/worlds/export.json`) resolve under the root **without editing any save file** | `VirtualFileSystem::resolve` rule 8 (relative paths whose first component is exactly `saves`) |
| Terminal history/log, recorder output, MCP server `SAVES_DIR`, Python API `/saves` follow the root | `TerminalChannel.cpp`, `AudioRecorder.cpp`, `ScreenRecorder.cpp`, `earthcall-mcp-server.js`, `py/api/routes.py` |
| One-time migration: read-only on the checkout; copy into staging; compare every file by size and SHA-256; write `.gitignore` and the marker; `git init` and commit; rename onto the home atomically | `scripts/migrate_saves_to_data_home.py` |
| Python rule for authoring/injection scripts | `scripts/earthcall_data_home.py` (`data_root()`) |
| Witness | `tests/singularity/data_home_test.cpp` (15 checks) |

**Run on Zach's machine, 2026-10-12 12:33 PDT:** 686 files (1.11 GB) were verified byte-identical into `~/Library/Application Support/Earthcall`, from checkout commit `1b469427c`. The data-home git repo was initialized and committed as `71a8503`. The checkout's `saves/` still has its 9.7 GB, unmodified (`git status saves` was clean afterward).

**Not moved:** signing keys stay in `~/.earthcall/identity` (`Identity::KeyStore`, overridable with `$EARTHCALL_HOME`). Private keys don't belong in a git repo. The public `identity/first-movers.json` register did move with the saves.

## Pitfalls for the next agent

- **The checkout's `saves/` is now a frozen snapshot plus test fixtures.** Anything written there no longer reaches the running app. Write save injections against `data_root()` (Python) or `SaveSystem::rootPath()` (C++), then commit them in the **data home's** git repo, signing as usual.
- About 27 `scripts/*.py` authoring scripts and many `scratch/*.py` scripts still hardcode `saves/`. They are one-off seeds; port a script to `earthcall_data_home.data_root()` before re-running it.
- Tests that use `TestSupport::RealSaveTreeGuard` against the "real tree" now exercise the checkout snapshot. That is safer, but it drifts from the live data.
- `DeveloperToolsWindow` still reads test observations from the checkout's `saves/tests` on purpose: tests write them there.
- `first_mover_test`, `save_roundtrip_test` and `zone_home_ontology_test` fail on clean HEAD `1b469427c` with the same assertions both before and after this change (A/B in a worktree). They predate this work.

## Open

- [ ] ⚑ ZACH DECIDES: prune the checkout's tracked `saves/` (682 files) down to the fixtures and seeds that tests and docs name, now that history continues in the data home.
- [ ] Port the `scripts/` authoring scripts to `earthcall_data_home.data_root()`.
- [ ] Decide whether the app commits to the data-home repo itself (for example, on Save Zone) or commits stay manual.
- [x] Separate the asset root from the cwd: done 2026-10-12 by `AppPaths` and Earthcall.app ([macOS app task](../../Housekeeping%20and%20Build/macOS_App_Bundle/macOS_App_Bundle.md)).
- [ ] Person verification: see the [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md#data-home-outside-the-checkout).

*Claude Code / Claude Opus 5.5 / session `16d04786-8407-4a70-a57c-fa5c11094c91` / 2026-10-12T12:34-07:00. The location, the git-history choice and the migration scope are Zach's. The marker-gated fallback, the VFS legacy-spelling rule and the verified staging-rename migration are my design.*
