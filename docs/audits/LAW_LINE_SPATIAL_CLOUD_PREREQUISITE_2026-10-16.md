# Cloud spatial authoring commission — prerequisite evidence

**Scope:** first prerequisite only. [Full task](../Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Law_Line_Spatial_Companion/Law_Line_Spatial_Companion.md) remains incomplete. [Implementation/gap record and exact build commands](../plans/LAW_LINE_SPATIAL_COMPANION_CLOUD_2026-10-16.md).

## Observed execution

- Fresh cloud checkout of canonical `sync-from-earthcall-main` at `18456320d10f7bddfa0bc5074dd665f1092ec7fe`; branch `codex/law-line-spatial-companion-20261016`.
- Linux x86_64, GNU C++ 13.3, Debug, CMake 4.4.4/Ninja. GLFW 3.4 null backend; no physical display/input or exposed GPU device. No Mac execution or synthetic captures.
- CPU compilation includes the production TerminalChannel, LineEditor, grammar, LawManager and seeded compiler route. Initial missing X11, UUID headers and absolute scratch-path exclusion were setup failures, not failed runtime assertions.
- Direct `line_editor_test`: passed, including stale draft/cursor, invalid UTF-8/control refusal, non-submission, secret protection, Unicode ZWJ cluster boundary, active search/paste refusal.
- Direct `terminal_zones_test`: passed, including actual sense/decide/act insertion, exactly-once consumption, stale-request and wrong-Zone refusal, unchanged spoken counter and ordinary cancellation.
- Seeded compiler/template-only run: **42/42 checks passed**. The example is authored via the actual Metalaws, its sensed CSI-u shortcut inserts into the actual draft, no draft Law is committed, and a secret insertion request refuses.
- The initial broader focused run finished 286/289 Law Line checks, with exactly three new-example failures because bare `zone` denotes the structural condition token. Corrected source uses explicit `@terminal-channel.zone`; the subsequent 42/42 compiler run passes. The corrected broader run is recorded below after completion.

The test fixture's keyed Person is test-only and is inherited from the isolated seeded snapshot, not a new submitting identity hardcoded in production. Existing inhabited saves and Mac files were not edited. The actual application view and restart have not been tested.

## Not established

There is no delivered graph interface, idempotent application installer, native hit-testing, GPU manifestation, persistent view state, scale timing, graphical A–F journey, one-minute onboarding, beauty/enjoyment verdict, or Person acceptance. The source example is a transport witness, not a replacement for the commissioned instrument. No new graph/application class or rendering path was added.

## Integrity check

The caller is an ordinary authored Sequence which stages insertion and advances a request. The consumer is TerminalChannel::act, which uses the existing editor and never calls submit/speak for insertion. Successful changes announce normal draft/cursor notifications. Status and handled-request projections remain read-only. No compiler grammar, existing binding, save codec or enum was changed. Request/editor state is transient; this checkpoint promises no restart continuity for it.

Signed: Codex / model unexposed by runtime / session `92b79f696f1c` / 2026-10-16T02:51:30.317813+00:00.

## Corrected focused suite: passed

The corrected checkout ran all three selected CTest targets successfully: `law_line_zone_test` (110.99 s), `line_editor_test` (0.88 s), and `terminal_zones_test` (0.59 s); total 112.46 s. The Law Line executable's complete 289-check path passed, not just the 42-check template shortcut path. `git diff --check` also passed. This is CPU/isolated-fixture evidence only and does not change the incomplete instrument status above.
