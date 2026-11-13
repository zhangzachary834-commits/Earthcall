# Legacy test store isolation and capture baseline

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 12:08 PDT.

While following Zach's report that authored newborn cubes existed but were invisible, broader checks uncovered separate verification debt. The cube destination fix and seven focused tests are verified in [the routing audit](../../../../../audits/LAW_CREATE_ACTIVE_ZONE_ROUTING_FIX_2026-10-05.md).

Investigate legacy `chess_app_test`'s line-213 capture assertion: black e5 stays at `(4,4)` after the attempted capture. It fails in the full real-store test and in an isolated Chess/World fixture with both pre-fix and corrected routing/harness code. Keep `chess_zone_native_boot_test`, which passes, distinct from the legacy World loader. Do not rewrite sacred fixtures merely to quiet the assertion.

Move legacy real-store tests toward faithful, bounded isolated fixtures: `person_not_object_world_test` spent minutes serializing the entire hydrated store before loading its subject, reaching roughly 10 GB resident memory. `RealSaveTreeGuard` restores on normal C++ scope unwinding, but an assertion abort does not run its destructor; its claim to restore regardless of outcome needs repair. Preserve original bytes, include inactive Zones where routing needs them, and prove isolation against the real boot graph. This pass stopped the expensive checks and verified all real Zone/Home files against the pre-test backup by SHA-256; nothing differed. The broad suite was not completed.

## 2026-10-20 — still failing at `6bd248d94`; the turn never passes after the second white move; Zach suspects the promotion laws

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169`. Found while running every `tests/law` target one at a time.*

**Results:**
- `chess_app_test`, `chess_castling_test` and `chess_extended_rules_test` abort, and `chess_click_geometry_test` exits 1, on `claude/ray-domain-transport-rung0`.
- A throwaway worktree built at `6bd248d94`, before any of that branch's work, fails `chess_app_test` at the same line (213). This is the baseline failure this task already records, not a new regression.

**What the log shows:**
- e2–e4 and e7–e5 each run the full chain: `square-clicked` → `pawn-*-double` → check scan → `check-evaluated` → `commit`, and the turn changes.
- On d2–d4, the board-click tick records only `law-chess-click` applied, the four `law-chess-promo-btn-*` refusals and `law-chess-select`. No pawn law records anything, not even `conditions-failed`, and no check/commit chain runs. Yet `gridY == 3` holds for the d-pawn afterwards.
- So the turn stays white, black's select refuses, and the capture assertion fails. `chess_castling_test` fails on the same thing (`turn == 1`).

**Zach's hypothesis (2026-10-20): "The tests i think fail on promotion laws."** Not yet tested. The promotion laws are `law-chess-promo-btn-*` (publish on a HUD click), `law-chess-promo-apply-*` and `law-chess-promote-white/black` (on `turn-changed`/`move-committed`, a pawn on rank 7/0, setting `@state.chess.promoActive`). Next step: read `@state.chess.promoActive` and `selectionActive` before the d4 click, and the event-agenda rounds of that tick.

**Store side effect, witnessed:** each run of these tests rewrites the tracked `saves/worlds/chess_app.ecform` and rotates an `.ecmatter` sidecar in the checkout running it. I restored the tracked files from `HEAD` afterwards. This is the isolation half of this task.
