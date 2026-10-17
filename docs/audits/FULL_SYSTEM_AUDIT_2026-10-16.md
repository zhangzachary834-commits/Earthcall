# Full-System Audit of Earthcall — 2026-10-16

**Commissioned by Zach:** "Do a full-system audit for Earthcall BROOOOOOOOOOOO."
**Auditor:** Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b` / 2026-10-16T18:42-07:00 (PDT)
**Tree:** branch `firmament-laboratory` at `f2e24c1cc` (clean at start). Live data home `~/Library/Application Support/Earthcall` at `9135fd6`, not clean.
**Method:** This audit re-tests the charges in Grok 4.6's [Adversarial Full-Tree Audit (2026-09-15)](ADVERSARIAL_FULL_TREE_AUDIT_2026-09-15.md) against the code a month later. It adds mechanical sweeps for each Refusal and non-negotiable, a full default build and `ctest` run, and a compiled witness probe for the most serious claims ([`scratch/probes/full_system_audit_2026-10-16_witness.cpp`](../../scratch/probes/full_system_audit_2026-10-16_witness.cpp)). Every finding is labeled with its evidence class:
- **WITNESSED**: I ran it and saw the result.
- **READ**: established by reading the code.
- **INFERRED**: follows from code I read, but I did not run it.

The ordering, and the choice to re-test September's P0–P4 list instead of writing a new essay, are my own. The Refusals, the non-negotiables and the §0 loop are Zach's and Grok's.

---

## 0. Verdict

The architecture held where it was enforced in code: enums, authority clamp, Unauthored refusal, foreign-actuation guard, the tree. It slipped where enforcement depended on remembering:
- A new Singularity channel hands any firing Law a shell.
- A month-old paint bridge still repaints shared Materials.
- An agent regenerated the Gathering Zone instead of patching it.
- The default build has been red since October 14.

None of these is architectural. All of them are the "silence" failure class the September audit named: nothing crashed, and nothing told a Person.

## 1. Findings, most severe first

| # | Severity | Finding | Evidence |
|---|---|---|---|
| F1 | **Critical** | Two boot-registered Laws are process launchers that any firing Law can drive:<br>• `shell-channel` (`EngineInit.cpp:321`, 2026-10-06). Writing `command` and then raising `executeRequests` calls `popen()`.<br>• `StreamChannel` (`EngineInit.cpp:300`, 2026-09-08). Writing `stream.target` and setting `stream.open = true` calls `popen()`; `pipeType` defaults to `"process"` and `_enabled` defaults to true.<br>Neither has an allow-list, Person consent, a TransferPolicy tier or an authority grant. Both run synchronously on the frame thread. Neither appears in any doc, task or test. | **WITNESSED** (W1, W4) |
| F2 | **High** | `FacePropertyBridge::setValue` (`ObjectProperties.cpp:528`) writes `activeLayer`, `useLayers`, `layerOpacity`, `blendMode` and `resolution` through `materials.resolveOrDefault(materialId())`, which is the shared Material. This is exactly the bug the "Paint is on the Material" non-negotiable names. It dates to 2026-08-13 (`10a17f03f`). | **WITNESSED** (W3) |
| F3 | **High** | The data-home commit `9135fd6` (Gemini/Antigravity, 17:45 today, "Install Personal Ourverse Navigator") **replaced** `zones/Ourverse Gathering` instead of patching it: 1,195,387 lines deleted, 2,256 inserted, `zone.ecform` removed, objects 122→28, materials 122→18. The removed objects are 119 generated-id tori with `Player`/`law-30` stakeholders, so this may be intended pruning. But it contradicts "Patch, never regenerate", and the only record of authorization is the injector's own `qualities.injected_by` text. | **READ** (git diff and JSON census) |
| F4 | **High** | The backup `zones/Ourverse Gathering.bak.20261016/` sits *inside* `zones/`. `listZoneIdentityRecords` treats every subfolder containing a zone file as a Zone, so the next launch should load a **second** Zone whose identifier is `Ourverse Gathering`, carrying the 122 old objects. | **INFERRED** (app not launched) |
| F5 | **High** | The default build fails. `sky_island_sanctuary_test` was listed in both the codegen-only branch (`CMakeLists.txt:380`) and the WebGPU branch (`:405`). The first match won, so the target linked without wgpu and failed with undefined `_wgpuDeviceCreateTexture`… This has been the case since `07435d0ce`/`59232506f` (2026-10-14). CI's 81-test focused list does not include this test, so CI stayed green. | **WITNESSED**; **fixed in this pass** (§6) |
| F13 | **High** | **The test harness fills the disk and can truncate saves.** `RealSaveTreeGuard` (`tests/support/test_harness.hpp:352`, used by 15 tests) copies the whole checkout `saves/` (about 5.3 GB, mostly the ignored `backups/before-load.ecform`) into `$TMPDIR/earthcall-save-backup-*` for every guarded test, and deletes it only in the destructor. An abort or a ctest timeout leaks the copy. Restoration also runs `remove_all` *before* `copy`, so a copy that fails on a full disk leaves a truncated original. In this pass that is exactly what happened: see §4a. | **WITNESSED** |
| F14 | Medium | `slow_adapter_zone_perf_test` is registered in CTest without its required `<world.json> --adapter=… --direct=…` arguments. It prints usage and fails in 0.06 s on every run. | **WITNESSED** |
| F15 | Medium | Every full-store load prints `[OntoMath] MathNode type error on load: … Map argument 0 must be Vector, got Scalar` (26 occurrences across this run). By the loader's own comment, "the law that reads it will not fire": a deaf Law, logged but never surfaced to a Person. Source not pinned. `saves/zones/LawLine/zone.ecform` is the only Zone file containing `Map` math. | **WITNESSED** (message); source **unpinned** |
| F6 | Medium | `HttpChannel` (added 2026-10-06 with Shell and OSC) calls `enable_server_certificate_verification(false)` and runs synchronously when `sendRequests` increases. It has the same ungated Law reach as F1, minus code execution. | **READ** |
| F7 | Medium | The menu option "Enter New Zone (Hydrate ECGRAPH)" (`EngineInit.cpp:589`, added 2026-10-04) mints `hydrated-<unix time>` with owner `""`. That breaks the stable-identifier rule and the nothing-without-an-author rule at the Zone level. One was minted in the live data home today (`zones/hydrated-1792184750`). It saved with zero objects and zero lexemes even though `load-genesis-file` exists. | **READ**; empty result **WITNESSED** on disk |
| F8 | Medium | Identity is still split. `persons/Zach.ecform` (legacy, name-keyed, display Lexeme `…EB6`) sits beside the DID-keyed `persons/did_earthcall_dmvok…ecform` (display Lexeme `…283`). `homes/Home_of_Zach` (996 B) still sits beside `homes/Home` (10 MB). September's P0 "no more `Home_of_Zach`" is still open. | **READ** |
| F9 | Medium | The live data home is not committed. There are 24 changes: `Sanctum of Beginnings/zone.json` deleted (intended `.json`→`.ecform` migration in `writeZoneIdentity`, recoverable at `71a8503`), 22 untracked paths including the Sky-Island Sanctuary install (byte-identical to the checkout copy and properly attributed to Gemini 4 Argon, authors `["Zach"]`), the `.bak` folder and the `hydrated-` Zone. | **READ** |
| F10 | Low | Compiled Mach-O binaries are committed: `scratch_test2..11`, `test_fmt`, `test_parse`, `test_lawrefs` at the repo root (1.4 MB, from 2026-09-27→10-11), `TestLabAI/testlab_ai`, and 16 under `scratch/2026-09-27_cleanup/`. The root also carries about 40 ignored `fix_*.py`/`patch_*.py` scripts. "The tree is the ontology" covers the workshop too. | **READ** |
| F11 | Low | The Intercom link journal lacked a trailing newline, so every `nav find`/`nav link` refused with "incomplete journal tail". All 98 records parsed. | **WITNESSED**; **fixed in this pass** (§6) |
| F12 | Low | Agenda hygiene. Of 479 To-do bullets, 188 exceed 400 characters (the longest is 1,742) against the one-sentence rule, and 265 link no task folder. The Person Verification List holds **372 open** checks against 117 confirmed. | **READ** |

**On F1 specifically.** W1 builds a Law, round-trips it through `toJson`→`fromJson` with `"authority": 0`, rebinds its author as the Zone loader does, and applies it to `shell-channel`. The result was `Applied`, and the `touch` target exists. The authority ceiling does not help, because `ShellChannel` never calls `grantAuthority` and so sits at 0. The production `Universe` provider enumerates every Law (`EngineInit.cpp:386`), so a metalaw that sweeps Laws reaches `shell-channel` as a subject.

W2 tried the other route: `@shell-channel.*` written from an Object subject. In my harness it returned path result 1, so **that route is unwitnessed**. I am not claiming it.

W4 applies an ordinary authored Law to a `StreamChannel` instance with `stream.target = "touch …"`, `stream.open = true`, `stream.close = true`. The result was `Applied`, and the file exists. (`ScreenRecorder`'s `popen` is a fixed ffmpeg command with quoted paths and is not in this class.)

Taken together, that is code execution reachable from authored text. Law Line sentences and Laws carried in a shared Zone file are both authored text, and the Second-Person framework expects Zones to be exchanged. Refusal 6 says reach is total and authority is TransferPolicy's. This channel has reach and no authority office at all.

## 2. September 15 charges, re-tested

| Charge (2026-09-15 §12) | Now | Evidence |
|---|---|---|
| Zach modeled as an `Object` in live saves | **Not found** in live `zones/`, `worlds/`, `homes/` | grep for Object-kind records named/identified `Zach` |
| Duplicate `Home_of_Zach` minted | **Open** (F8) | `homes/Home_of_Zach` present |
| `Law.cpp.new` | **Closed** | no `.new/.orig/.rej/.bak` tracked |
| Anonymous WebSocket can `write_property` | **Closed.** Every mutating branch calls `admit()` → `Foreign::authorizeForeignActuation` (`WebSocketServer.cpp:290–1641`) | READ |
| `Create` only mints `Object` | **Largely closed.** `SingularSetToSetCreation` exists, refuses Person/Body synthesis (`:57`) | READ |
| Stop adding `ActionNode` kinds ("if you need Kind 21, you are failing Create") | **Not followed.** Kinds 21–25 added (`WritePixel`, `ElevatePixels` 2026-09-07; `FileRead`, `FileWrite`, `CodecTransform` 2026-10-03). Append-only was respected; no renumbering | git -S |
| Stop new ImGui panels/docking | **Not followed.** `IDEDockManager` added; ImGui-bearing code 13,234→15,054 lines; `LawGraphWindow.cpp` 3,670→4,404 | wc |
| Stop new modality channels until MCP is gated | MCP *is* gated now, but Shell/HTTP/OSC arrived **without** the same gate (F1, F6) | READ |
| `Community`/`Relationship` `std::cout` a greeting | **Open** (`Relationship.cpp:16`, `Community.cpp:18`, `Home.cpp:179`) | grep |
| Tests touching real saves | Improved: data home split, test isolation PRs (#562). Data home still holds `tests/` and `test_recorder_sandbox/` folders from the migration copy | ls |

## 3. Refusals and non-negotiables scorecard

| Rule | Status | Note |
|---|---|---|
| R1 no domain-noun class | Holds | Classes added since 09-15 are channels, protocol records, proofs, traversal. `Voice`/`VoicePart` are Person-constitutive |
| R2 no top-level subsystem dir | Holds | `src/` top level is exactly the ontology. Stray untracked `src/ConstructedBeing/.idea/` |
| R3 append-only enums | Holds | `BeingKind::World=6` burned; `ConditionNode::Kind` 12/13 burned; no renumbering since 08-16 |
| R4 Body reserved | Holds | Only Person/serialization/CharacterConsole touch `Body` |
| R5 Person means Human | Holds | One `make_unique<Person>` site (`EngineInit.cpp:171`) |
| R6 no black box | Mixed | `no_black_box_test` is in the suite. New channels register their fields, but F1 shows that registration without an authority office is reach without governance |
| R7 no behavior methods | Slipping | Menu-bound minting (F7) and channels whose *setter* performs the act (F1, F6) put behavior in C++ setters |
| Authority clamped to 0 on file read | Holds | `Law.cpp:415` → `setAuthorityLevel` caps at `kAuthoredCeiling` |
| Nothing without an author | Holds for Laws; **fails for menu-minted Zones** (F7) | |
| Paint via `ownMaterial` | **Violated** (F2) | |
| Patch, never regenerate | **Violated** in data home (F3) | |
| Kernel guard on body | Holds | `lacksPersonMotionConsent` → `AuthorityDenied` in `Law::applyTo` |
| `frame_lag_test` baseline not widened | Holds | no change since 09-15 |
| Guidance ≤200 lines, links resolve | Holds | CLAUDE/AGENTS/GEMINI = 199 lines each (at the ceiling); 0 dead links across the four guidance docs |

## 4. Build and test

- `cmake --build build -j8` (Debug, Makefiles) **failed** on F5 before the fix. After the fix, `sky_island_sanctuary_test` builds (exit 0). The **runtime** result of that test is reported in the suite results below.
- CI (`.github/workflows/earthcall-ci.yml`) runs a focused list of 81 of the 291 registered tests, plus an SDF/WebGPU job. `gh` is not installed here, so I did not read any CI run results.
- **First full run** (`ctest -j4 --timeout 600`, 18:34–19:00): 265/291 passed, 26 failed. That run is contaminated, because the disk filled during it (§4a). `law_line_zone_test` died with `Errno 28`.
- **Serial rerun of the 26** (`-j1 --timeout 1500`, with 50–73 GB free): 8 now pass. Those were disk or contention artifacts: `chess_gesture`, `go_app`, `synthesis_studio_full_store_boot`, `person_not_object_world`, `law_line_zone`, `webgpu_cathedral_open_hand`, `zone_boot_hydration_relations`, `zone_manager_error_path`. **18 failures remain.** `sky_island_sanctuary_test` (F5) and `no_black_box_test` pass.

| Cluster | Tests | First failing assertion |
|---|---|---|
| Storage writes (one cause suspected, not established) | `first_mover_test`, `save_roundtrip_test`, `substrate_split_test`, `person_ecform_propagation_test`, `zone_home_ontology_test` | `[SaveSystem] Failed to write saves/worlds/test_fixture.ecsave`; ".ecform semantic file" missing; "primary Home serializes under saves/homes/" |
| Chess Laws | `chess_app_test`, `chess_castling_test`, `chess_extended_rules_test`, `chess_click_geometry_test` | `turn == 1` / `gridX == 3` asserts; promotion buttons `conditions-failed`. This matches the "legacy chess capture assertion" already noted in BUILD_AND_ENVIRONMENT |
| Click-derived behavior | `synthesis_studio_app_test`, `second_nature_law_forge_zone_test` | "clicking 3D/2D pad C5 sounds exactly one note"; "clicking authored instrument derives a newborn Law" |
| Light / radiance | `prism_cathedral_test`, `authorable_light_contract_test` | "Sun radiance is approximately unit strength"; "Near witness receives stronger radiance" |
| GPU channel | `gpu_mastery_test`, `webgpu_perlin_exact_gradient_test`, `webgpu_screen_recorder_test` | [4f] mesh-size re-upload; `!sawInitialRangeUpload`; "three sequential frames survive on disk" |
| Performance | `frame_lag_test` | **LAG**: median simulation frame 7.543 ms against baseline 2.456; `Zone::update` 5.751 ms against 0.797. The run was serial, with no other tests competing. The baseline was not widened (per CLAUDE.md) |
| Registration | `slow_adapter_zone_perf_test` | F14 |

- **Side effects of the rerun, repaired.** The aborted chess tests left the checkout's `saves/worlds/chess_app.ecform` modified, its `.ecmatter` swapped, and `before-load.ecform` rewritten. The guard never restored them, and every later guard snapshotted the dirty state. Fixes: restored the tracked files from git HEAD (the tree was clean at session start); restored `before-load.ecform` from an Oct 13 snapshot, hash-verified `a2e451ff…`; removed the test-minted `chess_app.af5742009c467d0d.ecmatter`, plus that run's 6 GB leaked snapshot and 1.6 GB open-hand scratch. `git status -- saves` is clean afterward. This is F13 again: **any aborted guarded test leaves the checkout's `saves/` dirty.**
- **Baseline.** To-do line 275 already records that 12 of these 18 failed on clean HEAD `5ea417c4` (2026-10-07): the four chess tests, `synthesis_studio_app`, `second_nature_law_forge_zone`, `authorable_light_contract`, `gpu_mastery`, `slow_adapter_zone_perf`, `webgpu_perlin_exact_gradient`, `prism_cathedral` and `zone_home_ontology`. **Six are newer than that record:** the four-test storage cluster (`first_mover`, `save_roundtrip`, `substrate_split`, `person_ecform_propagation`), `webgpu_screen_recorder` and the `frame_lag_test` LAG. These six are the ones to bisect first.

### 4a. Incident caused by this audit's own test run, and its repair

The macOS user temp folder already held **49 GB** (18 folders) of leaked `earthcall-save-backup-*` copies from earlier runs (Oct 6, 8, 9 and 13). My full `ctest` (started 18:34) leaked 76.5 GB more as guarded tests aborted or timed out. That left the disk with **280 MB free**.

At 18:51, a guard restoring the checkout under that pressure left `saves/backups/before-load.ecform` truncated: 3,646,947,328 bytes of the original 5,267,448,718. It is a prefix-identical truncation, confirmed with `cmp`. The file is gitignored, so git could not restore it. The same failure deleted the tracked `saves/backups/Home.orphaned-2026-09-09/zone.json`.

Repair:
- Verified that the Oct 13 and Oct 16 snapshot copies are byte-identical (SHA-256 `a2e451ffd5a5d8cc…`).
- Deleted only the leaked backups created by this run, keeping one intact copy.
- Copied the intact file back through a staged name, `cmp`-verified it, and renamed it into place. The final hash matches.
- Restored the orphan with `git checkout`.
- Diffed the remaining ignored `saves/backups/*` against the snapshot: identical.
- Removed the last kept copy.

Result: free space went from 280 MB to 73 GB, and `git status -- saves` is clean. **The live data home was never touched.** Tests use the checkout's frozen `saves/`. The 49 GB of older leaks belong to earlier sessions and are left for Zach to delete (Person Verification List).

## 5. Scale

`src/` is 147,859 lines (115,163 on 2026-09-15, +28% in a month). Five files exceed 3,400 lines: `SdfWgsl.cpp` 5,204, `LawGraphWindow.cpp` 4,404, `WebGpuRenderer.cpp` 3,924, `Law.cpp` 3,739 and `ScalarForm.cpp` 3,486. `src/json.hpp` (24,765 lines, vendored nlohmann) lives in `src/` instead of `third_party/`. There are 28 TODO/FIXME markers in `src/`.

## 6. What this pass changed

1. `CMakeLists.txt`: removed the stray `sky_island_sanctuary_test` line from the codegen-only branch, so the target takes the WebGPU branch it was already listed in. This is a one-line deletion.
2. `agent intercom/conversation_links.jsonl`: appended the missing trailing newline. No record changed. `nav doctor` and `nav find` work again.
3. Added this audit, the witness probe under `scratch/probes/`, To-do bullets with task folders for F1/F2/F3–F4, and Person Verification checks for what only Zach can decide.
4. **Repaired damage from my own test runs** (§4, §4a): `saves/backups/before-load.ecform` (twice), the `Home.orphaned-2026-09-09/zone.json` and chess files from git, and only this session's leaked temp snapshots. The checkout's `saves/` ends clean.
5. **Not changed:** no data-home save file, no data-home file, no source behavior. F1, F2 and F3/F4 are reported, not repaired. F1's repair is an authority decision, and F3/F4 involve Zach's world, which is his to decide.

## 7. Recommended order

1. **F1 now.** Until a decision exists, `shell-channel`, `StreamChannel`'s process mode and `http-channel` should refuse unless a Person-granted First Mover authorizes the specific write, using the same `authorizeForeignActuation` office the WebSocket already uses. Do not build a second permission system (Refusal 6). Either move execution off the frame thread or document it as a blocking First Mover. Turn TLS verification back on.
2. **F4 before the next launch.** Move `Ourverse Gathering.bak.20261016` out of `zones/` (to `backups/`, which is gitignored), *after* Zach confirms F3.
3. **F2:** route every structural face write through `ownMaterial()`, as `Color` already does via `setFaceColor`. Then turn W3 into a suite test asserting that the second Object is untouched.
4. **F3/F9:** Zach confirms or reverts the Gathering replacement, then commit the data home.
5. **F7/F8:** an authored, owned Zone birth path to replace the menu lambda, and retire the legacy `Zach.ecform` and `Home_of_Zach` through the identity-ledger route (existing To-do line 101).
6. **F5 class:** put a "default build is green" job in CI, so a test listed in the wrong CMake branch cannot hide behind the focused list.

## 8. For the next agent

- Re-run W1–W4 after any fix. W1 and W4 must print `AuthorityDenied` (or whatever the chosen refusal is) and must **not** create `shell_witness_A` or `shell_witness_C`.
- The full `ctest` is slow (about 1 to 2 hours with 4 jobs). Several GPU/full-store tests run for more than 5 minutes each, so budget for it.
- Do not "fix" F3 by restoring the old Zone yourself. That is Zach's world and Zach's decision.
- Check the data home's `git status` before you inject. Today it already held uncommitted work from at least two agents and one in-app session.

*Signed: Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b` / 2026-10-16T18:42-07:00 (PDT).*

---

## 9. Fix pass: the low-hanging fruit (same session, Zach's follow-up)

Zach: "BROOOOOO FIX THE LOW HANGING FRUIT IN THOSE." I chose which findings count as low-hanging: correct, local, and verifiable in this session, with no author decision required. Anything needing Zach's ⚑ stays open.

| Finding | Status | What changed | Verification |
|---|---|---|---|
| F1 process channels | **Interim floor; ⚑ still open** | `ShellChannel` execution and `StreamChannel`'s Law-facing `stream.open` with `pipeType` `process` now refuse loudly, writing into `stderr`/`_lastError`, unless the Person launching Earthcall sets `EARTHCALL_ALLOW_PROCESS_CHANNELS=1` (`processChannelsAllowedByPerson()`, `ShellChannel.cpp`). First Mover C++ calling `openPipe()` directly, and FIFO pipes, are unaffected. No authored Law, example or test used either channel. | W1/W4 rerun: no file created. With the variable set, both run. `stream_channel_test` ALL OK |
| F2 paint bridge | **Fixed** | Every validated `face.N.*` write now goes through `ownTexture()` → `Object::ownMaterial()`. A refused write does not fork the Material. | New `paint_test` §9; W3 shows `shared=0`, shared width stays 16; paint/face_texture/property_bridge/material/no_black_box tests pass |
| F4 `.bak` in `zones/` | **Fixed** | Moved to the data home's `backups/Ourverse Gathering.bak.20261016/` (gitignored, not enumerated). Nothing was deleted. F3, whether to restore the old objects, remains Zach's | `zones/` holds no `.bak` |
| F5 sky test linkage | Fixed earlier | One line in `CMakeLists.txt` | Test passes |
| F6 TLS | **Fixed** | `enable_server_certificate_verification(true)`. httplib loads the macOS keychain | Probe: `example.com` 200, `self-signed.badssl.com` refused |
| F7 menu Zone owner | **Partly fixed** | "Enter New Zone" now passes the pressing Person as owner with `kOwnerKindPerson`. The timestamp identifier is not yet an authored slug | Both app targets build |
| F10 committed binaries | **Fixed (staged)** | 52 Mach-O files untracked with `git rm --cached` (still on disk) and listed in `.gitignore` | `git status` shows 52 staged deletions |
| F11 Intercom tail | Fixed earlier | | `nav` works |
| F13 test guard | **Mostly fixed** | Snapshots use APFS `clonefile()` (deep-copy fallback elsewhere). Restore stages beside the original and renames it in, so the original is never deleted first. Still open: an `abort()` inside a test still skips the destructor and leaves `saves/` dirty, but the leaked snapshot now costs about 0 bytes | Guarded test run moved free space by about 2 MB, against 5.3 GB before; `go_app`, `save_system_error`, `test_observation_load` pass; `saves/` clean, `before-load.ecform` hash unchanged |
| F14 probe registration | **Fixed** | `MANUAL_PROBE_TESTS` in `CMakeLists.txt`: the probe is still built, but not registered with ctest | `ctest -N`: 290 tests |
| F3, F8, F9, F12, F15, the 18 failing tests | Open | These need Zach's decisions, identity-ledger work or root-cause analysis | — |

**Full suite after the fixes: incomplete.** The default build passed (`BUILD_EXIT=0`) and the first 54/290 tests passed. Claude Code then stopped the run because the machine was critically low on memory, which is not a test result. The interrupted chess test left `saves/worlds/chess_app.*` dirty again: restored from git, test-minted `.ecmatter` removed, `before-load.ecform` hash still `a2e451ff…`, and that run's 4 snapshot clones removed. **Rerun the full suite** before treating the fix pass as suite-verified.

**Cleanup:** the two remaining leaked snapshots from this session's rerun (19:01, 19:04) were removed after the checkout was verified clean. The 18 older ones (Oct 6–13, about 49 GB) are still Zach's to delete.

*Fix pass signed: Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b` / 2026-10-16 (PDT).*
