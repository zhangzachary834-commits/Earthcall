# Named Screen regions through Law Line Metalaws

Codex / GPT-6.1 Sol · session `01a10992-828e-7e80-890c-c64b09141e18` · 2026-10-07 19:16 PDT.

## Human direction and this implementation

Zach selected the earlier explanation's unfinished ability to name a displayed region, read it, govern/edit it, and derive other things from it as an aspect of its bearer, then explicitly requested the Law Line Metalaw architecture implement it. His prior direction was “PRECONFIGURED NOT BESPOKE” and a direct Screen medium that does not require Object/Material/FaceTexture carriers. The implementation follows that direction through authored value constructors and existing property actions. Codex supplied the explicit source-binding and dated-observation contract. An optional question about source editing versus creating an overlay received no answer; this pass proceeds with editing an explicitly bound source. It does not infer consent, source ownership, an inverse compositor, or a universal live pixel l-value.

## Implemented contract

`ScreenRegion` compiles to an ordinary dictionary with a typed `color` VectorField and `selector` ScalarField. Any existing Singular can bear that dictionary; the public examples put the source on Screen and carry observations onto the Person. The native fixture additionally renders a Person-owned region. `ScreenSample` compiles to an ordinary request record with qualified region path, explicit integer rectangle, sample limit and request token; `ScreenSampleAtTime` additionally supplies a finite time. Each word has one disabled constructor-meaning Law and one enabled compiler Metalaw. No visual class, enum value, action opcode or Screen-specific parser verb was added.

The maintained recipes are [create/read gold region](../../examples/law_line_screen_region.txt) and [recolour/resample cyan region](../../examples/law_line_screen_region_edit.txt). Two sentences in each line use the existing saved sentence compiler and register in source order. Initialization uses Sequence for intra-Law action order; source registration order does not promise global Rete firing order. The reader Law waits for the requested successful token.

The colour and selector remain separate authored mathematics: a selector does not implicitly clip its colour field. Output binds explicitly to the colour source with `output.colorPath`. Quoted binding strings need a qualified stable identifier; quoted `my.halo` is ordinary text, whereas unquoted `my.halo` uses the authored root alias. Alias expansion now applies consistently to read expressions, write destinations and condition paths, with longest matching root and refusal of ambiguity.

Generic granular field paths expose the existing canonical serialized field definition, including piece nodes and coefficients. Traversal uses operation-local detached structural views, preserving lists rather than interpreting three-element arrays as vectors. Writes validate finite values, field type/arity and canonical round-trip retention before committing through registered setters or authored root storage. Tiny numeric edits survive; unsupported or lossy edits refuse without changing the field. Root change notifications wake dependent Laws. Computed/derived roots retain read-only semantics. A typed field leaf is replaced at its bearer slot, rather than mutating another slot sharing its old pointer.

`ScreenChannel::senseOutput` is the irreducible sensing channel: after the production Engine finishes scene/direct/compatibility viewport draws and before Dear ImGui overlay/presentation, it resolves the region afresh, evaluates bounded local selector mathematics at physical top-left pixel centres, and reads the renderer's actual RGBA8 framebuffer bytes. Requests admit 1,048,576 scanned locations, 65,536 selected samples and 256 MiB framebuffer readback at most. The author's smaller selected-sample limit is enforced without partial success. Calls, folds, implicit world guards, unbound coordinates, malformed/unused arguments and nonfinite values refuse. Time is bound only when explicitly supplied.

A changed nonempty token identifies one capture attempt; both success and refusal consume it. Results identify token, region, frame, dimensions, completed-viewport stage and, on success, selector/time. Source edits do not reinterpret an earlier observation. `sample.result` is registered read-only; its getter returns a detached snapshot so copying it into editable Person memory cannot mutate canonical sensor truth through pointer aliasing. Pixel entries expose integer x/y, normalized RGB and alpha. A Law can inspect those paths and derive ordinary predicates from them; `my.haloReading` demonstrates the route. Membership has no retained cache.

Governance uses existing Law/property access and TransferPolicy boundaries; this pass creates no second permission system and proves no new multi-Person authority policy. Sample bounds/results are registered observations; GPU transfer buffers are explicitly Kernel machinery. The path picker now recognizes the existing `dict` and `list` type labels.

## Verification executed

The WebGPU app builds. Eight focused suites passed: `property_memory_access_test`, `law_line_zone_test`, `law_sentence_test`, `terminal_zones_test`, `line_editor_test`, `channel_paths_test`, `no_black_box_test`, and `property_path_precalc_test`. After a final repeated-removal repair, `law_line_zone_test` was rebuilt and rerun: **231/231 checks**, CTest PASS. These are focused checks, not a full-suite claim. The tests exercise canonical registered/dynamic field edits, invalid/readonly paths, serialization, small coefficient updates, dependency firing, authored compiler availability, alias destinations and CLI recipes.

The production-linked native probe `python3 scratch/probes/law_line_screen_probe.py` passed at **2026-10-07 19:16:08 PDT**. It uses the real Terminal→Metalaws→Law→Engine→WebGPU route in an isolated first-seed store, a public author DID and sanctioned C++ presence seam. It does not unlock human keys or demonstrate real Person acceptance. Physical keyboard/mouse polling is disabled only in this fixture to prevent external typing from changing its active Zone; each tick verifies it remains in LawLine. Frames are display-paced.

Retained evidence: [result.json](../../scratch/verification/law-line-screen-regions-2026-10-07/result.json), [gold](../../scratch/verification/law-line-screen-regions-2026-10-07/region-gold.png), [cyan](../../scratch/verification/law-line-screen-regions-2026-10-07/region-cyan.png), [Person-owned source](../../scratch/verification/law-line-screen-regions-2026-10-07/person-region.png). The report is evidence accompanying independently decoded captures, not a substitute for pixels.

| Native witness | Decoded samples/pixels | Maximum byte error |
|---|---:|---:|
| Gold region membership/source | 481 | 0 |
| Cyan edited region membership/source | 481 | 0 |
| Each region observation versus independently decoded same-frame RGBA capture | 481 | 0 |
| Full gradient | 3,686,400 | 0 |
| Luminous Lens, first time | 3,686,400 | 0 |
| Luminous Lens, later time | 3,686,400 | 0 |
| Person-owned cyan field | 3,686,400 | 0 |
| Selected gold physical pixel | 1 | 0 |

All captures are native 2560×1440. The probe also verifies immutable-token frame retention, readonly nested observation writes, editable carried-copy isolation, authored observation derivation, explicit sample-limit refusal with no partial/old list, unchanged owned-Object count, and repeated authored output withdrawal with no refusal or recreated slots.

The original radius-12 disc revealed an honest precision boundary: CPU membership included 441 centres, but GPU float32 distance arithmetic excluded four exactly-zero edge locations (437 coloured pixels). Sensing correctly returned the actual background bytes there. The public recipe now uses radius **12.25**, giving 481 unambiguous centres. This does not prove universal CPU/GPU exact-boundary parity; the guide states that limitation.

## Failures found and repaired

- A new dynamic source must be granted with AddProperty before ordinary Map/Set use; maintained examples follow the existing action semantics.
- Authored root aliases expanded in expression reads but not destinations; the shared resolver now covers both, including the Person-carried observation.
- Approximate change comparison could discard tiny field edits; canonical field commits compare exactly and reject nonfinite values before equality checks.
- A copied PropertyDict could alias the canonical sensor snapshot; the derived getter now detaches it. Native verification edits the carried copy and re-reads protected sensor truth.
- Path-picker metadata omitted `dict`/`list` labels; the caller now labels them correctly, with channel-path regression coverage.
- Repeated RemoveProperty on an already deleted authored slot cleared its surviving bridge and recreated it as monostate. ActionModel now refuses the absent authored slot before the first-mover clearing fallback. Native output cleanup and the focused regression verify absence remains absence.
- Desktop GPU acquisition and fixture keyboard/Zone changes were environmental witness failures, not proof of a rendering defect. Final evidence uses desktop access and an isolated, input-stable fixture.

## Save integrity and concurrent migration

Only the authorized LawLine native seed and six new shared Law roots were authored. The existing Zone was patched by insertion, staged, checked and atomically replaced; backup: `scratch/backups/law-line/LawLine-zone-before-law-line-patch-20261007-183800.ecform`. A subsequent integrity check reproduced the exact current JSON bytes from insertion into the backup: all **188 old Lexemes, 145 old formation Relations, 166 old Law refs**, other members and original text survived. Additions are three Lexemes, three denotes Relations and six Law refs. Each new Law records Zach's keyed public DID `did:earthcall:dmvokvvtp4jwmhhkyzv23xanzyukspdv5aykm7okru3mdidyncga` as author; `injected_by` records Codex / GPT-6.1 Sol / this session / 2026-10-07. No demo regions or Persons were injected into an inhabited world. A final seed rerun left the native Zone and all direct Screen Law roots byte-identical.

Zach announced a concurrent Sonnet 5.5 world-conglomerate→Zone-specific saving migration and authorized Intercom coordination. Scope and typed-field preservation requirements were appended to [its live thread](../../agent%20intercom/communication-threads/saves-and-zones/Zone_Native_Closure_Rungs_2026-10-07.md). This pass does not edit ZoneManager, ZoneNativePersistence, SaveSystem, MaterialManager or Sonnet's Material/Zone migration files, and does not claim his work. Derived channel observations must not become authored save truth.

## Remaining acceptance and frontier

[Person checks](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md) cover actual unlock/typing, gold appearance near (120,120), cyan edit, readable Person predicate, viewport resize, stopping and inhabited save/restart. Source inspection, focused tests and native fixture pixels do not discharge those checks.

Open: automatic live projected addressing, pointer-selected regions, inverse painting, multi-source/multi-Person composition and authorial arbitration, other render backends, exact-boundary numerical parity, and existing Object texture-projection invalidation debt. This implementation supplies explicit authored source editing plus bounded dated actual-pixel sensing; it does not silently choose those unresolved semantics.
