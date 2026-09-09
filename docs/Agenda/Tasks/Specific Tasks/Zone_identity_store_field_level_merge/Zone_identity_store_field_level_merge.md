# Zone identity store: field-level merge, not whole-object replacement

**Status:** the reported bug is fixed and regression-tested across three independent causes (app-level check still open, see Person Verification List); structural hardening (Sol's 6 invariants) is at Stage A complete (1, 2, 3), Stage B/C/D and Invariant 6 open
**Created:** 2026-09-07 by Claude, investigating a report from Zach. Extended 2026-09-08 with a second root cause and 6-invariant plan from GPT-5.6 Sol (Codex) on the agent intercom. Extended 2026-09-09 with Invariant 1.
**Code:** `src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.cpp` (`mergeZoneObjectsFromJson`, `applyZoneJson`), `src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp` (`to_json`'s `faceColors`), `src/Singularity/Storage/Schema/Earthcall.fbs`/`Earthcall_generated.h` (`Entity.owner_identifier`, append-only), `src/ZonesOfEarth/ZoneManager.cpp`/`.hpp` (`admitFromJson`'s identity-store branch; `applyMatterFlatBuffer`/`buildMatterFlatBuffer` — semantic fields removed, composite-address resolution, scoped writer), `tests/support/test_harness.hpp` (`RealSaveTreeGuard`)
**Test:** `tests/zones/zone_identity_test.cpp`, `tests/zones/matter_semantic_precedence_test.cpp`, `tests/zones/matter_scoped_writer_test.cpp`, `tests/law/chess_*_test.cpp` (sandboxing only, not the bug itself)

---

## Human intent

Zach reported the Basic Pixel Changer canvas rendering red with white label text
instead of white with no visible click effect, and separately named a pattern he'd
already noticed himself: "certain zones would load with 2D objects loading red
instead of their authored color." That second sentence is what turned this from a
one-off stale-save-file fix into an architecture fix — Zach's own prior observation
confirmed the bug was systemic before any code was changed.

## What was actually wrong

Earthcall keeps a per-Zone identity store (`saves/zones/<id>/zone.json`) so a
Person's in-place edits to a Zone — a moved position, a painted texture — survive
re-loading an older World snapshot. `ZoneManager::admitFromJson` (the per-zone entry
point inside `ZoneManager::loadState`) honors that by building a Zone from its
identity-store snapshot when one exists, in preference to the World's own copy.

The bug: `applyZoneJson` decided whether to touch a Zone's object list with one
check — `if (zone.getOwnedObjects().empty())`. Once the identity-store branch had
already populated the zone, that condition was false, so the World's own `objects`/
`world.objects` array was **never read again at all** — not merged, not diffed,
completely ignored. Any field the World authored *after* the identity snapshot was
taken — `faceColors`, in the case that surfaced this — was not merely stale in the
snapshot; it did not exist there at all, so `Object`'s deserializer left it at its
raw C++ default. `faceColors[0]` defaults to `{1,0,0}` (a legacy cube-face
red/red/green/green/blue/blue default, `Object.hpp:256-260`), which is exactly the
red Zach saw. A whole new object authored into the World after the snapshot would
have been silently dropped for that Zone the same way — not just a stale field, but
a stale *object list*.

## How this was traced

The investigation (Claude, same session) went through several wrong turns worth
naming so a future pass doesn't repeat them:
- First suspected the WebGPU render path (a real, separate bug Codex had already
  fixed: `WebGpuRenderer.cpp`'s `drawImage2D` pipeline never attached its
  `WGPUFragmentState` to the descriptor, causing a first-click validation panic —
  see `Authorable_Pixel_Writer.md`).
- Then suspected the pixel write itself was failing silently. Reading
  `logs/law_audit.log` directly settled this: the Law's `WritePixel` action
  reported `SUCCESS` (computed from `trace.anyWrote()`, not just condition-match)
  at every one of Zach's test clicks — the write was real.
- Then decoded the actual persisted `pixelsB64` texture data out of a saved
  `.ecform` file by hand (base64 → raw RGBA, diffed against pure white) and found
  it genuinely unmodified — proving the render, not the write, was the dead end.
- The decisive clue was Zach's own report that the canvas rendered "red with white
  letters" — `faceColors[0]`'s legacy default, plus the black/white contrast pick
  `draw2DObject` already does for label text. That pointed straight at "this
  object's faceColors were never loaded from any save at all."

## Scope check: how many Zones this actually touched

Scanning every `saves/zones/*/zone.json` for a 2D object (`x2D`/`y2D` present)
missing `faceColors` found roughly 14 of ~25 identity-store files affected,
including `SynthesisStudio`, `Chess`, `FarLands`, `Perlin Noise Floor Zone`, and
`Basic 2D Button Zone`. Not all of these necessarily *rendered* visibly red — an
object repainted by a Law-driven `Set` shortly after load would have its default
masked before a Person ever saw it. Objects whose color is authored once and never
reasserted (the pixel-changer canvas being the clean case) were the ones actually
showing it. No further action needed per-Zone: the fix below is load-time and
self-healing on next load, not a per-file patch.

## The fix

`mergeZoneObjectsFromJson` (new, `ZoneSerialization.cpp`) merges the World's
authored object list into an already-populated Zone **per object, per field**,
using RFC 7386 JSON merge-patch (`nlohmann::json::merge_patch` — already a
dependency, no new library):

- World's authored JSON for an object is the base.
- The Zone's live object (built moments earlier from the identity store) is
  serialized back to JSON and applied as the merge-patch overlay.
- A field the overlay actually specifies wins (a Person's in-session paint, a
  moved position — preserving the entire reason the identity store exists).
- A field the overlay does not specify falls through to the World's authored
  value, instead of the object's raw C++ default.
- An object id present in the World but absent from the identity store (added to
  the World after the snapshot) is admitted fresh rather than dropped.

This is called from exactly one place: `ZoneManager.cpp`'s identity-store branch
inside `admitFromJson`, immediately after the existing
`applyZoneJson(*z, zj, /*replaceObjects=*/false)` call. It is **not** folded into
`applyZoneJson` itself — that function has a second caller (the `findLive` branch,
same file) where "the zone already has objects" means "this Zone is live from the
running session," and there another file's snapshot of the same Zone must touch
nothing at all (`tests/zones/unsaved_preserve_test.cpp` enforces this explicitly).
Folding the merge into `applyZoneJson` broke that test on the first pass — both
callers pass `replaceObjects=false` but mean different things by "already has
objects," and `applyZoneJson` has no way to tell them apart. The merge lives at the
one call site that actually means "just hydrated from the store."

## Verification

- `tests/zones/zone_identity_test.cpp`: new block hand-constructs a stale identity
  snapshot (an object with an authored `displayName` but no `faceColors`) and a
  World file authored later (same object, now with `faceColors`, plus a brand-new
  sibling object). After `loadState`, asserts: the World's `faceColors` comes
  through (the fix), the store's `displayName` still wins (the invariant the fix
  must not break), and the new sibling object is admitted, not dropped.
- `tests/zones/unsaved_preserve_test.cpp` and `tests/zones/world_switch_test.cpp`
  both regressed on the first implementation (merge folded into `applyZoneJson`)
  and pass after moving the merge to the single correct call site.
- Full suite (`ctest`, 111 tests) re-run clean against the pre-existing baseline:
  the only failures are the ones already documented in
  `docs/BUILD_AND_ENVIRONMENT.md` (`smooth_tessellation_cache_test`,
  `chess_extended_rules_test`, `synthesis_studio_app_test`,
  `zone_boot_hydration_relations_test`, `frame_lag_test` — confirmed pre-existing
  by stashing this change and re-running the same subset).
- Deleted the two stale identity files that started this
  (`saves/zones/BasicPixelChanger/`, `saves/zones/Basic Pixel Changer/`, both
  duplicates of the pixel-changer canvas predating its `faceColors`/Law
  authoring), with Zach's authorization. Not strictly required after the fix —
  the merge would have healed them on next load — but no reason to leave a known
  stale snapshot sitting there.

## Remaining open item

Whether an object the World adds after a Zone's identity snapshot exists should
be admitted silently on every subsequent load (what this fix does) or require an
explicit "re-sync from World" action is Zach's call, not assumed here. Flagging
it rather than deciding it: nothing today exercises the alternative, and no Person
has asked for it.

## `faceColors` never round-tripped at all — fixed

Watching a fresh `saves/zones/BasicPixelChanger/` identity file get written (Zach
reloaded the World live, past the field-level-merge fix above) showed it *still*
had no `faceColors` key — not staleness, but because `Object::to_json`
(`ObjectSerialization.cpp`) had no `faceColors` write at all; only `from_json`
read it, as a deliberate-but-incomplete step of an in-progress "migrate paint to
Material" substrate split. `Shape2D`'s flat/untextured fallback
(`Object::draw2DObject`) still reads `faceColors[0]` directly, so the omission
was actively destroying data for anything not yet migrated off it — not just the
pixel-changer canvas; the same gap was found in ~14 of ~25 `saves/zones/*/zone.json`
files, including `SynthesisStudio` and `Chess`.

**Fixed**: `to_json` now serializes `faceColors` unconditionally, matching every
other field in that function. Flagged in its own comment as provisional pending
an eventual migration of 2D flat-plate rendering to read `Material.baseColor`
instead — that migration is NOT attempted here; it would touch every 2D object's
authored colour across every affected zone and needs its own sign-off. Until then,
this fix is what makes the identity store carry the correct value at all.

## Addendum: the test-isolation bug found while investigating this

Verifying the fix required running the 5 `chess_*_test` binaries, which pointed
`SaveSystem::setSaveRoot` directly at the real repo `saves/` tree — so
`BootedEngineHarness`'s `hydrateFromZoneStore()` loaded every real Zone identity
(Chess, FarLands, SynthesisStudio, both Homes), and a subsequent load's "preserve
unsaved work" write-back re-serialized all of them with fresh relation-event
timestamps, silently drifting real save files on every test run — compounded
across repeated runs to over 14,000 duplicate lines in `saves/zones/Chess/zone.json`
at one point. Caught via `git status`/`git diff` before anything was committed;
the polluted files were reverted with `git checkout` each time it recurred.

A first fix — copy `saves/zones/`, `saves/homes/`, and the one needed world file
into a disposable sandbox, point `SaveSystem` there — traded that bug for a
different, never-fully-explained one: against an otherwise byte-identical copy,
`chess_click_geometry_test` saw every piece at `(0,0,0)` and every Law answer
`conditions-failed`, regardless of which real subdirectories were included in the
copy (even after adding the one that looked most likely, `saves/persons/`, First
Mover identity). Rather than keep guessing which piece of state secretly depends
on the tree's real absolute path, landed instead: `TestSupport::RealSaveTreeGuard`
(`tests/support/test_harness.hpp`) runs each test against the REAL tree exactly
as it always ran (so there is no surface for an unexplained difference to appear
on), backing up `saves/zones/` and `saves/homes/` first and restoring them
unconditionally when the guard goes out of scope. All 5 chess tests pass now, or
fail identically to the documented baseline (`chess_extended_rules_test`'s
pawn-promotion assertion is the one pre-existing, unrelated failure).
`synthesis_studio_app_test` needed no fix — it parses its save file directly into
local objects and never touches `SaveSystem`/`ZoneManager`. See the To-Do list's
Housekeeping entry.

## Second red-canvas cause, found by Sol on the agent intercom 2026-09-08

Even with both fixes above, Zach reported the canvas still red after a full
quit/relaunch. GPT-5.6 Sol (session `01a0707e-f743-71b1-8fb9-63975012e66d`) found
the actual remaining mechanism by decoding the real `.ecmatter` FlatBuffer sidecar
directly: `ZoneManager::applyMatterFlatBuffer` runs in `loadState`'s
`"physical-matter"` stage, which is explicitly ordered AFTER the semantic
JSON/Zone-identity load ("Ourverse semantic root hydrated after Zones and laws").
It resolves objects with a single `std::unordered_map<std::string, Object*>` keyed
by bare object identifier across EVERY live Zone at once — no owning-Zone or Home
disambiguation — and then unconditionally applies `material_id`/`face_textures`/
`face_colors` from whatever FlatBuffer `Entity` matches, last one in iteration
order winning. Sol found the real `basic_pixel_changer.ecmatter` sidecar
carries 1,441 entities for what should be a handful of objects, with 382 bare ids
appearing more than once — including two records for `basic-pixel-canvas` itself,
one correctly white and one the legacy cube-face red default. Whichever the loop
visited last silently overwrote whatever the semantic JSON path had already
loaded correctly, every single time, regardless of how correct that JSON path was.

**Fixed** (Sol's "immediate compatibility fix", implemented here): `material_id`,
`face_textures`, and `face_colors` are semantic/Material state —
`Material::toJson` already round-trips all three correctly through the JSON path
— so `applyMatterFlatBuffer` no longer applies any of them, and
`buildMatterFlatBuffer` no longer writes real values into those FlatBuffer slots
(left as empty/0 offsets rather than removed from the schema, so an old buffer
that still carries real data in those fields stays *readable*, just inert —
Sol's instruction: "leave schema slots readable/append-only"). New regression
test: `tests/zones/matter_semantic_precedence_test.cpp` — a hand-built legacy
matter buffer with real red `face_colors` for an object whose semantic state says
white, fed through the real `applyMatterFlatBuffer`, must finish white; a freshly
built matter buffer must carry no `face_colors` data at all.

**Not fixed, and flagged by Sol as Person-authorized-only**: the 382 duplicate
bare object ids are a general problem, not unique to this one canvas — any object
whose bare id repeats across Zones is exposed to the same last-writer-wins
collision for whatever fields still route through the matter buffer (geometry,
transform). Sol's recommended structural fix (not attempted): add an owning
Zone/Home stable id to each sidecar `Entity` (an append-only FlatBuffer field) and
resolve by `(owner-id, object-id)`; add save-time and load-time duplicate-key
validation; regression-test the actual precedence against the real `.ecform` +
`.ecmatter` pair, not only a synthetic buffer. Separately, Sol also found that
`saves/zones/BasicPixelChanger/zone.json`'s own `identifier` field reads
`"Basic Pixel Changer"` (the display name) while the folder and every
`zoneRef`/`currentZoneId` reference use `"BasicPixelChanger"` — an identity
invariant break that can make Zone enumeration/read incoherent independent of
this bug. Sol declined to touch it without Person authorization, and neither have
I; recorded here so it isn't lost.

## Structural hardening: composite (owner, object) identity, Invariants 2 and 3

Zach visually confirmed the canvas is white with black text (2026-09-08) and asked
that this become the frontier-safe structural fix, not just the point repair. Sol
posted six invariants on the intercom thread with a staged landing order
(A: scoped writer + composite identity + preflight/legacy resolution + collision
tests; B: generation coupling/atomic commit; C: detached whole-load transaction;
D: registered-property persistence audit). This session implemented the core of
stage A — Invariants 2 and 3 — and explicitly deferred the rest; see below.

**Invariant 2 (composite owner identity), implemented:**
`Earthcall.fbs`'s `Entity` table gained an append-only `owner_identifier: string`
field (regenerated `Earthcall_generated.h` with `flatc` 25.12.19, matching the
version this project's vendored FlatBuffers headers already assert — the diff is
exactly the one new field, nothing else touched). `buildMatterFlatBuffer` now
writes each object's owning Zone/Home identifier into it. `applyMatterFlatBuffer`
resolves the canonical address `(owner_identifier, id)` via a `byComposite` map
built from every currently-live Zone.

**Invariant 3 (preflight, refuse on ambiguity — "no iteration order, no
unordered_map replacement, no last-record-wins anywhere"), implemented as a
two-pass resolve-then-apply:** pass 1 resolves every entity in the buffer to an
Object and a composite key without mutating anything; pass 2 applies fields only
for entities whose resolved key is unique in that buffer, and refuses (skips,
logs once per key) any key that resolves more than once. This is what makes the
real `basic_pixel_changer.ecmatter`'s two "basic-pixel-canvas" records — both
legacy/ownerless, since `owner_identifier` did not exist when that file was
written — refuse together instead of the second silently winning.

**A resolution-order subtlety found while testing, not anticipated by the
invariants as written:** `owner_identifier` cannot always be trusted as
absolute — `ZoneManager::loadTestObservation` deliberately re-parents a dump's
objects into a freshly-named `test.<stem>` Zone, different from whatever Zone
owned them when the `.ecmatter` was written, so a legitimately-moved object's
`owner_identifier` will never match post-move. The first implementation treated a
non-matching `owner_identifier` as "skip, stale record" and broke
`test_observation_load_test` (`FAILED: loaded cube kept its position` — the
object silently kept its default-constructed transform because resolution never
found it). Fixed: an `owner_identifier` that names no live Zone holding that bare
id falls through to the same unambiguous-bare-id resolution an ownerless legacy
record already gets, rather than refusing outright — `owner_identifier`
disambiguates a real collision; it does not veto a resolution that is otherwise
perfectly safe. `matter_semantic_precedence_test.cpp` now has a named regression
block for exactly this case, plus the composite-round-trip, duplicate-key-refusal,
and ambiguous-ownerless-refusal tests Sol specified. 9/9 green; full suite matches
the documented baseline with no new failures.

## Invariant 1 (scoped writer) — the actual reason the sidecar reached 1,441 entities

Zach asked me to keep going on Sol's staged plan. This is the piece that was
missing to CLOSE the hole rather than just make its consequences safe:
Invariants 2/3 (above) make a bare-id collision refuse instead of silently
picking a winner, but they don't stop `buildMatterFlatBuffer` from dumping
every live Zone's objects into a matter buffer meant for one World in the
first place.

`buildMatterFlatBuffer` is called from four places. Two of them —
`saveState`/`saveStateWithLog`, an ordinary Save/Quick Save — are actually
FINE unscoped: `buildSaveJson` embeds every live `_zones` into the `.ecform`
side too, at the exact same call, so both artifacts already agree on
membership there. The one call site that is NOT symmetric — and the one that
actually caused the bug — is `loadState`'s "Legacy JSON splitter": the first
time a plain-JSON World (no `.ecmatter` yet) is loaded, it mints one via
`buildMatterFlatBuffer()`, but by then `_zones` holds whatever boot-time
`hydrateFromZoneStore()` already pulled in (every Zone under `saves/zones/`)
PLUS whatever this specific load just admitted — a strict superset of the
handful of Zones the loaded World's own `zones`/`zoneRefs` actually name.

**Fixed:** `buildMatterFlatBuffer` gained an optional
`std::optional<std::unordered_set<std::string>> scopeZoneIds` parameter
(default `std::nullopt` = every live Zone, preserving the two symmetric call
sites unchanged). The legacy-splitter call site now computes the exact set of
Zone ids the just-loaded World's own `zones`/`zoneRefs` name and passes that
as the scope; an empty scope (a degenerate World naming no Zone at all) skips
minting a matter file entirely rather than falling back to "no filter," which
would silently reintroduce the exact bug this closes. Also added a lighter
form of Sol's requested membership assertion — `buildMatterFlatBuffer` itself
logs loudly (not a hard `assert()`; a Storage-mechanism sanity check must not
abort a Person's save) if a caller's scope names a Zone id that isn't
currently live, since that would otherwise silently mean the matter buffer is
just missing that Zone's physical state with no signal at all.

**Test:** new `tests/zones/matter_scoped_writer_test.cpp` reproduces the real
shape directly — a "Bystander" Zone stands in for boot-hydration bloat, a
legacy World naming only "OnlyZone" is loaded through the real
`ZoneManager::loadState`, and the resulting `.ecmatter` is decoded from disk
and asserted to contain exactly OnlyZone's object, not Bystander's. 4/4 green.

**Stage A (Invariants 1, 2, 3) is now complete.** Full suite re-verified
clean at 126 tests (the suite has grown from 111 to 126 since this task
started, from concurrent work — Formation Rete performance fixes from Opus 5,
a Harmonic Save Studio pass, File Watcher/VFS work — none of which touch
`Singularity/Storage`, `.ecform`/`.ecmatter`, or `ZoneManager.cpp`; confirmed
directly with Opus 5 on the intercom before concluding several failures seen
in one `-j4` run were resource contention from two concurrent agent sessions
building/testing on the same machine, not real regressions — see the
Housekeeping entries in the To-Do list for the two false leads that cost real
time chasing: a `git stash`/pop leaving a stale object file, and an unrelated
pre-existing `synthesis_studio_living_test` failure).

**Explicitly deferred, per Sol's own staged sequencing — not attempted this
pass:**
- **Invariant 4** (atomic generation commit: paired `snapshot_id`, matter
  hash/length/schema version in the semantic root, write-then-atomic-rename, keep
  the prior generation until commit).
- **Invariant 5** (registered-property persistence audit: CI catches both a
  registered path with zero persistence homes — the original missing-`faceColors`
  bug — and one with multiple competing homes — the `.ecform`/identity/`.ecmatter`
  jurisdiction fight this whole task has been about).
- **Invariant 6** (validate `saves/zones/<id>/`'s directory key against the
  document's own `identifier` and every referencing `zoneRef`/`currentZoneId` at
  every boundary; refuse/log a mismatch rather than silently deriving identity
  from display text). The concrete case — `BasicPixelChanger` vs
  `"Basic Pixel Changer"` — is unchanged from the previous section: Person-
  authorized repair only.

Also found and reverted while verifying, not fixed (logged in the To-Do list):
`test_observation_load_test` has the same real-`saves/`-tree-pollution bug the 5
chess tests had — `saves/zones/visible_cube/zone.json` picked up 34 lines of
drift running the full suite. Needs the same `RealSaveTreeGuard` treatment.

---

**Signed:** Claude Sonnet 5 (session `01Mvd55GFWyUMrYWt2ERGSRE` through 2026-09-08; session `01MsayKP3NYfQAyBtyQ8xeA1` for the 2026-09-09 Invariant 1 pass — different session, same model, per the intercom's own rule that these are different agents)
**Date:** 2026-09-07, extended 2026-09-08, extended 2026-09-09

**Diagnosis credited to:** Codex (GPT-5.6 Sol), session `01a0707e-f743-71b1-8fb9-63975012e66d`, for the `.ecmatter` matter/semantic precedence finding and the six-invariant structural-hardening plan — see "Second red-canvas cause" and "Structural hardening" above. Implementation (Invariants 1, 2, and 3) is mine; the findings, invariant framing, and staged landing order are Sol's, posted on `agent intercom/communication-threads/Basic Pixel Changer Zone Identity Bug 9-7-26.md`. Sol's own framing: "Zach originated the demand that this never become a bureaucracy again and that serialization follow the Singular ontology; I am extending that human direction into the invariants and landing sequence." Also thanks to Claude Opus 5 (session `01F9nK3F`), concurrently restructuring the Law/Rete engine in the same checkout, for proactively confirming on the intercom that their work touches none of `Singularity/Storage`, ruling that out as the source of several test failures that turned out to be resource contention between two agent sessions building on the same machine.
