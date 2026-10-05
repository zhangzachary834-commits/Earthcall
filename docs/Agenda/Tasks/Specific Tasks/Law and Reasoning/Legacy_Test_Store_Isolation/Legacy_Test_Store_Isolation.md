# Legacy test store isolation and capture baseline

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 12:08 PDT.

While following Zach's report that authored newborn cubes existed but were invisible, broader checks uncovered separate verification debt. The cube destination fix and seven focused tests are verified in [the routing audit](../../../../../audits/LAW_CREATE_ACTIVE_ZONE_ROUTING_FIX_2026-10-05.md).

Investigate legacy `chess_app_test`'s line-213 capture assertion: black e5 stays at `(4,4)` after the attempted capture. It fails in the full real-store test and in an isolated Chess/World fixture with both pre-fix and corrected routing/harness code. Keep `chess_zone_native_boot_test`, which passes, distinct from the legacy World loader. Do not rewrite sacred fixtures merely to quiet the assertion.

Move legacy real-store tests toward faithful, bounded isolated fixtures: `person_not_object_world_test` spent minutes serializing the entire hydrated store before loading its subject, reaching roughly 10 GB resident memory. `RealSaveTreeGuard` restores on normal C++ scope unwinding, but an assertion abort does not run its destructor; its claim to restore regardless of outcome needs repair. Preserve original bytes, include inactive Zones where routing needs them, and prove isolation against the real boot graph. This pass stopped the expensive checks and verified all real Zone/Home files against the pre-test backup by SHA-256; nothing differed. The broad suite was not completed.
