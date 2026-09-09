# Zone identity store: field-level merge, not whole-object replacement

**Status:** the reported bug is fixed and regression-tested across three independent causes (app-level check still open, see Person Verification List); structural hardening (Sol's 6 invariants) is at Stage A+B complete (1, 2, 3, 4); Invariant 6 is now fully landed (general Zone identifier/name split, plus storage-boundary directory-key validation) with two real save-file inconsistencies it caught fixed/archived with Zach's authorization; Stage C/D (Invariants 5, and Invariant 3's stronger "detached whole-load transaction" form) open
**Created:** 2026-09-07 by Claude, investigating a report from Zach. Extended 2026-09-08 with a second root cause and 6-invariant plan from GPT-5.6 Sol (Codex) on the agent intercom. Extended 2026-09-09 with Invariant 1, then Invariant 4, then Zone's identifier/name split (Invariant 6 general form), then Invariant 6's storage-boundary validation (Stage 0+1 of Sol's follow-up plan).
**Code:** `src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.cpp` (`mergeZoneObjectsFromJson`, `applyZoneJson`, `zoneIdFromJson`, `makeZoneFromJson`), `src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp` (`to_json`'s `faceColors`), `src/Singularity/Storage/Schema/Earthcall.fbs`/`Earthcall_generated.h` (`Entity.owner_identifier`, append-only), `src/Singularity/Storage/SaveSystem.hpp`/`.cpp` (`IdentityRecord`, `listZoneIdentityRecords`/`listHomeIdentityRecords`; `listZoneIdentities`/`listHomeIdentities` now return directory keys, not document-parsed identifiers), `src/ZonesOfEarth/ZoneManager.cpp`/`.hpp` (`admitFromJson`'s identity-store branch, including its Home-vs-Zone write-routing fix; `hydrateFromZoneStore`'s directory-key validation; `applyMatterFlatBuffer`/`buildMatterFlatBuffer` — semantic fields removed, composite-address resolution, scoped writer; `commitMatterGeneration`/`readVerifiedMatterGeneration`/`atomicWriteFile`/`sha256Hex` — atomic generation coupling; `saveState`/`saveStateWithLog`/`loadState`'s physical-matter stage/`loadTestObservation` rewired to use it), `src/ZonesOfEarth/Zone/Zone.hpp`/`.cpp` (`_identifier` field, `setName`, `propIdentifier`, `getIdentifier()` now returns `_identifier` not `_name`), `tests/support/test_harness.hpp` (`RealSaveTreeGuard`, its `GuardCurrentRoot` tag, `hashDirectoryTree`)
**Test:** `tests/zones/zone_identity_test.cpp`, `tests/zones/matter_semantic_precedence_test.cpp`, `tests/zones/matter_scoped_writer_test.cpp`, `tests/zones/matter_generation_commit_test.cpp`, `tests/zones/zone_identifier_name_split_test.cpp`, `tests/zones/zone_identity_boundary_test.cpp`, `tests/zones/test_observation_load_test.cpp` (now guarded), `tests/law/chess_*_test.cpp` (sandboxing only, not the bug itself)

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

## Invariant 4: atomic generation commit (2026-09-09)

Sol's third invariant: "generations are atomic." `saveState` and `saveStateWithLog`
previously wrote `.ecform` and `.ecmatter` as two independent, non-atomic writes —
worse, in the WRONG order (`.ecform` first, naming a matter file that did not exist
yet on disk). A crash between the two, or a partial write of either, left a root
that named matter that was truncated, stale, or simply absent, with no way for a
loader to tell the difference from a legitimate empty/no-matter world.

**Fixed:** `.ecform` and `.ecmatter` now share an opaque, **content-addressed**
`snapshotId` (the first 16 hex characters of the matter buffer's own SHA-256).
Content-addressing was the frontier choice here, not a from-scratch design — it is
the same idea git, IPFS, and Nix store paths already lean on, and it makes several
of Sol's requirements fall out for free: the matter file is written+flushed under a
name nothing else on disk can already claim, so a half-written attempt can never
collide with a real generation; saving unchanged matter twice reuses the same file
instead of rewriting it; and there is no counter or clock to keep synchronized
across processes. Landing order, exactly as Sol specified:

1. `commitMatterGeneration` writes the matter bytes to `<stem>.<snapshotId>.ecmatter`
   via write-temp-then-atomic-rename (`atomicWriteFile`), so a crash mid-write of
   the matter file itself never leaves a truncated file under a name a root could
   ever reference — flushed and closed before anything else happens.
2. The semantic root's JSON gains a `matterGeneration` object: `snapshotId`,
   `sha256`, `byteLength`, and a `schemaVersion` (currently `1`, incremented only if
   the matter payload's meaning changes in a way an old reader could misinterpret —
   the FlatBuffers schema itself stays append-only per AGENTS.md and does not need
   its own version bump for that).
3. The root itself commits via `atomicWriteFile` — write-temp-then-rename — so the
   root's own commit is likewise never observable half-written.
4. Only *after* the root's rename has succeeded does `commitMatterGeneration` look
   at what the *previous* on-disk root named and delete that generation's matter
   file, if it differs from the new one. A failed step 1 or 3 leaves the previous
   generation's file and the previous root both fully intact and still loadable —
   "keep the prior generation until the new pointer commits, clean only
   afterward," Sol's words exactly.

On load (`loadState`'s `physical-matter` stage, and `loadTestObservation`'s matter
step), `readVerifiedMatterGeneration` is consulted first: if the root names a
`matterGeneration`, the generation file it names must exist, and its byte length
and SHA-256 must match what the root recorded, and its `schemaVersion` must not be
newer than this build understands — any failure refuses loudly (`std::cerr` plus
`_saveLoad.lastLoadReport`) and hydrates **no** physical matter for that load,
rather than falling through to the legacy splitter and silently re-migrating over
top of a corruption that should have been surfaced. A root with **no**
`matterGeneration` key at all — every save ever written before this change — is
untouched: it falls through to the exact `SaveSystem::readMatterData(filename)`
call this code path always made, so no existing save requires any migration to
stay readable.

**Deliberately scoped to `saveState` and `saveStateWithLog`** — the two paths a
Person's own Save/Quick Save/Save As actually run. The legacy JSON splitter
(`loadState`'s one-time migration of a pre-split World) still writes its matter via
the old fixed-name `SaveSystem::writeMatterData` and was left alone: it already
writes matter before form (Sol's ordering requirement was already true there by
accident), it is a rare one-shot event rather than an ongoing write path, and
Sol's own landing note was "A-C should be reviewable independently" — extending
generation-coupling to that one remaining fixed-name writer is a small, separable
follow-up, not required to close the gap the real bug traced to.

**Test:** `tests/zones/matter_generation_commit_test.cpp` — drives the real
`ZoneManager::saveState`/`loadState` against a sandboxed save root (no repo save
file touched). Proves: a fresh save names a generation whose file matches its
recorded hash/length; a full round-trip through a second `ZoneManager` hydrates the
saved transform; changing the matter content mints a new generation and removes
the superseded file only after the new root commits; saving unchanged content
reuses the same content-addressed generation (no duplicate file, nothing deleted);
a hand-tampered root naming a hash-mismatched generation refuses to hydrate with no
crash, does **not** fall through to rebuilding a fresh legacy `.ecmatter`, and —
Sol's specified case — the real, untampered generation file is left completely
intact and still verifiable by its correct hash; and a root naming a generation
whose file was deleted out from under it refuses the same way. 18/18 checks green.

Fixing this surfaced two existing tests (`substrate_split_test`,
`save_roundtrip_test`) that asserted a literal `"<stem>.ecmatter"` path existed
after a save — true under the old fixed-name writer, no longer true by design.
Both were updated to resolve the generation-named file via the `.ecform`'s own
`matterGeneration.snapshotId` instead of asserting a name never actually promised
in the schema; both pass in full afterward (30/30 and 28/28).

## Invariant 6, made concrete: Zone gained a real identifier/name split (2026-09-09)

Zach saw this one live, in the running app, immediately after Invariant 4 landed —
pasted a wall of new console output: `applyMatterFlatBuffer` refusing dozens of
entities because their bare id matched "2 live objects" or even "3 live objects
across Zones" that were printed with the **identical name** — `(Basic 2D Button
Zone, Basic 2D Button Zone, Basic 2D Button Zone)`, `(Perlin Noise Floor Zone,
Perlin Noise Floor Zone)`. Nothing crashed and nothing corrupted — Invariant 3's
refusal was doing exactly its job — but it was surfacing a real, distinct bug: the
same Zone identifier was live as more than one `Zone` object at once.

**Root cause, confirmed on disk, not inferred:**

```
saves/zones/Basic 2D Button Zone/zone.json -> identifier: "Basic 2D Button Zone"  name: "Basic 2D Button Zone"
saves/zones/Basic2DButtonZone/zone.json    -> identifier: "Basic2DButtonZone"     name: "Basic 2D Button Zone"
```

Both records are legitimately on disk. `zoneIdFromJson` (the resolution every
admission/dedup check in `ZoneManager.cpp` uses to ask "is a Zone with this id
already live?") preferred the `identifier` field. But `makeZoneFromJson` — the
function that actually *constructs* the live `Zone` object — preferred `name`
instead, the opposite priority. So the `Basic2DButtonZone` folder's record
resolved to dedup-key `"Basic2DButtonZone"`, yet the Zone it actually built
reported `getIdentifier() == "Basic 2D Button Zone"` (name-first) — colliding
with the *other*, self-consistent folder's Zone without either dedup check ever
recognizing it as the same being. Every load of any World naming this Zone across
a session minted another duplicate. This is the general form of the
`BasicPixelChanger`/`"Basic Pixel Changer"` case Sol flagged for Invariant 6 — not
a one-off stale save, a structural inconsistency in the code between two
independently-written copies of the same field-priority decision.

Underneath that: `Zone` had no way to *not* collide even if the code agreed with
itself. `Zone::getIdentifier()` simply returned `_name` (`Zone.hpp`) — there was
no second field. Zach's direction: "Yes Zone should absolutely have a real
identifier/name split. It's a Singular" — i.e. this is exactly the kind of
structural role distinction the Singular ontology already exists to hold (as
`Object` holds `objectID` for stable address, though even `Object` doesn't
separately hold a display label — this is a new pattern for `Zone` specifically,
not a copy of an existing one).

**Fixed:**
- `Zone` gained a genuine `_identifier` field, distinct from `_name` (display).
  The constructor still takes one string, exactly as every existing call site
  already calls it, and initializes BOTH to that string — so identity for every
  Zone ever constructed the ordinary way is bit-for-bit unchanged. `getIdentifier()`
  now returns `_identifier`, not `_name`. A new `setName()` lets a Zone's display
  diverge from its identity post-construction without ever touching identity
  itself — used in exactly one place.
- `zoneIdFromJson` moved out of `ZoneManager.cpp` into
  `ZoneSerialization.hpp`/`.cpp` as the ONE shared resolution — a second,
  independently-written copy is exactly the mechanism that drifted out of sync
  before, so there is now nowhere for a second copy to be written by accident.
- `makeZoneFromJson` now resolves identity via that same shared `zoneIdFromJson`
  (identifier-first) to construct the Zone, then calls `setName()` only if the
  record's own `name` field differs from that identity — display and identity are
  now actually two separate reads of the JSON, not one field doing both jobs.
- `identifier` is registered as a new read-only property (`propIdentifier`) beside
  the existing read-only `name` — Refusal 6, no black box: a Zone's stable address
  is now itself law-visible, not just its display.
- Did NOT touch `saves/zones/Basic2DButtonZone/zone.json` or any other real save
  file — per CLAUDE.md, save files are sacred, Person-authorized changes only. The
  fix alone resolves the *collision*: the two records now correctly resolve to two
  DISTINCT identities (`Basic2DButtonZone` and `Basic 2D Button Zone`) instead of
  colliding into one. Whether these two on-disk records ought to actually be the
  *same* Zone (deduplicated/merged) is a content-level, Person-authorized decision,
  not a code bug — flagged here, not resolved, exactly like `BasicPixelChanger`.

**Test:** `tests/zones/zone_identifier_name_split_test.cpp`, 10/10 green — direct
`makeZoneFromJson` checks (divergent record resolves identity from `identifier`,
keeps `name` as display, agrees with `zoneIdFromJson`), a no-behavior-change check
for the ordinary single-string constructor, and the real shape driven through
`ZoneManager::hydrateFromZoneStore()` twice (once for boot, once standing in for a
later load) over two sandboxed identity folders shaped exactly like the real
`Basic2DButtonZone`/`Basic 2D Button Zone` pair — exactly one live Zone per
identity afterward, not a collision.

**Confirmed against the real save tree too** (read-only, via the already-guarded
`zone_boot_hydration_relations_test`): loading the real `chess_app.json` now
reports "26 zone(s), 1575 object(s)" where it reported "27 zone(s), 1576
object(s)" immediately before this fix — one fewer of exactly this kind of phantom
duplicate. Its own `instanceOf == 35` assertion is unrelated stale-fixture drift
(see the note further down) and still fails, unaffected by this fix either way.

## Sol's Stage 0 + 1: sealed the test leak, finished Invariant 6 at the storage boundary (2026-09-09)

After the identifier/name split above, Sol replied on the intercom with a fuller
staged plan (Stage 0 through 4). This pass landed Stage 0 and 1 together, exactly
as Sol asked ("Please land 0+1 as one bounded pass").

**Stage 0 — sealed `test_observation_load_test`'s real-`saves/`-tree leak.**
`dump_test_save` (via `ZoneManager::saveState`) calls `persistZones()`
unconditionally, and this test never pointed `SaveSystem` at a sandbox, so its
write landed in the real `saves/zones/visible_cube/` tree — the same bug class as
the 5 chess tests, logged since 2026-09-08, not yet fixed. `RealSaveTreeGuard`
gained a second constructor, tagged `GuardCurrentRoot`, for exactly this shape: a
test that never names a real `saves/worlds/...` file at all but still risks
writing into the real tree because nothing ever redirected `SaveSystem`. Also
added `hashDirectoryTree` (`tests/support/test_harness.hpp`) — a content
signature over a directory's files, not cryptographic (test-only, defends against
drift not an adversary) — and the test now snapshots `saves/zones`+`saves/homes`
before entering a `try` block that owns the guard, and asserts the hash is
byte-identical afterward whether the guarded section completed normally or threw.
21/21 checks pass; `git status saves/` is clean before and after.

**Stage 1 — Invariant 6 at the storage boundary.** The general identifier/name
split (previous section) fixed *within-document* divergence. Sol's Stage 1 asks
about a different divergence: the *directory key* (the folder a Zone/Home
identity was actually enumerated from) versus the *document's own* claimed
identity — Sol's own words: "do not return a document identifier and then use it
to reconstruct a possibly different path." `SaveSystem::listZoneIdentities()`
and `listHomeIdentities()` did exactly that: each returned `j.value("identifier",
j.value("name", ...))` — the DOCUMENT's content — which `ZoneManager` then fed
back into `readZoneIdentity()`/`readHomeIdentity()` to reconstruct a path via
`sanitizeLabel()`. If a document's own identifier differs from the folder it
actually lives in, that round-trip resolves a DIFFERENT folder (or none at all).

**Fixed:**
- New `SaveSystem::IdentityRecord{directoryKey, document}` and
  `listZoneIdentityRecords()`/`listHomeIdentityRecords()` — directoryKey is
  always the literal folder name, never re-derived from content. The existing
  `listZoneIdentities()`/`listHomeIdentities()` (still used for the Load World
  window's display list) now return directory keys too, built by mapping over
  the new records function, closing the anti-pattern at its only other call site.
- `ZoneManager::hydrateFromZoneStore()` now validates every record BEFORE
  constructing anything: (1) the document's own resolved identity (via the same
  shared `zoneIdFromJson` from the section above) must equal the directory key it
  was read from; (2) two different directory entries — Home or Zone, one shared
  identity namespace — may not claim the same identity. Either failure refuses
  with a structured message naming the path, directory key, document identity,
  and (for a duplicate) every claimant — zero writes, no phantom live Zone, per
  Sol's acceptance criterion. Neither check picks a "first winner" by iteration
  order: a duplicate refuses ALL claimants, the same posture Invariant 3 already
  takes on a duplicate composite matter key.
- Found and fixed a real, previously-invisible bug this validation surfaced
  immediately: `admitFromJson`'s fallback branch (first-ever identity write for a
  Zone named in a session file) called `writeZoneIdentity` unconditionally, never
  checking whether the constructed being was a `Home` — `persistZones()` already
  correctly routes Homes to `writeHomeIdentity`; this one code path didn't. Fixed
  to match.

**Two real save-file inconsistencies this validation caught, both Person-authorized:**
1. **`saves/zones/BasicPixelChanger/zone.json`** — exactly the case Sol found on
   2026-09-08: folder `BasicPixelChanger`, document `identifier: "Basic Pixel
   Changer"` (a space). Zach authorized the one-line fix (edit the field, not the
   folder — the canonical `saves/worlds/basic_pixel_changer.ecform`/`.json` already
   reference `BasicPixelChanger` with no space in `currentZoneId`/`zoneRefs`, so
   this made the identity-store record agree with what already-correct World
   files expected).
2. **A duplicate on Zach's own Home, found while testing the above** —
   `saves/zones/Home/zone.json` (32 objects, stale) and
   `saves/homes/Home/home.json` (104 objects, the one `persistZones()` has
   actually been maintaining) both claimed identity `"Home"`. This was the exact
   `admitFromJson` write-routing bug above, fired at some point in Zach's own
   history. Silently harmless before (the old code just skipped the second
   "Home" it saw); with the new validation active it would have refused the
   REAL Home entirely at next boot. Zach authorized moving the stale file aside
   (not deleting): `git mv saves/zones/Home saves/backups/Home.orphaned-2026-09-09`,
   with its `identifier`/`name` fields inside also updated to match, so it no
   longer claims "Home" and causes no recurring refusal log. Nothing in the real
   `saves/homes/Home/home.json` was touched.

**Test:** `tests/zones/zone_identity_boundary_test.cpp`, 10/10 green, entirely
against sandboxed temp-root fixtures — per Sol's explicit instruction, the real
`BasicPixelChanger`/`Home` files were never read by this test; it reproduces
their exact shape synthetically. Covers: the real mismatch shape refuses under
both the directory key and the document's identifier, with the on-disk record
completely untouched and a repeated hydration pass still refusing identically; a
matching identity still hydrates (control case); a Home entry and a Zone entry
claiming the same identity refuses both; a document with no identifier or name
at all refuses without crashing.

**Not this pass, per Sol's own sequencing:**
- **Invariant 5** (registered-property persistence audit: CI catches both a
  registered path with zero persistence homes — the original missing-`faceColors`
  bug — and one with multiple competing homes — the `.ecform`/identity/`.ecmatter`
  jurisdiction fight this whole task has been about).
- **Invariant 3's stronger "detached whole-load transaction" form** (Stage C of
  Sol's follow-up plan) and **closing the legacy-splitter generation edge**
  (Stage 4) — both explicitly gated by Sol on posting the `ValidatedLoadPlan`
  boundary to the intercom first, for review before landing.

`Invariant 6` itself — both the general identifier/name split and the
storage-boundary directory-key validation — is now fully landed (see the two
sections above).

Also found while verifying Invariant 4, unrelated to it (logged in the To-Do
list): `zone_boot_hydration_relations_test` and `chess_extended_rules_test` both
fail against the current committed `saves/worlds/chess_app.json` — the former
expects exactly 35 `instance-of` relations and gets 118, the latter asserts a
specific pawn-promotion outcome that no longer holds. Confirmed unrelated to this
task: `git log -- saves/worlds/chess_app.json` shows the file's last change
predates this session entirely, `git diff` shows this session made no change to
it, and neither test's failing assertion touches physical-matter data at all
(relation counts, promoted-piece role) — the real save file has simply grown
(more zones, 1,576 objects vs. whatever these tests were last calibrated
against) through legitimate concurrent work landing elsewhere. Both tests' fixed
expectations are stale, not the save file wrong.

---

**Signed:** Claude Sonnet 5 (session `01Mvd55GFWyUMrYWt2ERGSRE` through 2026-09-08; session `01MsayKP3NYfQAyBtyQ8xeA1` for the 2026-09-09 Invariant 1, Invariant 4, Zone identifier/name split, and Stage 0+1 passes — different session, same model, per the intercom's own rule that these are different agents)
**Date:** 2026-09-07, extended 2026-09-08, extended 2026-09-09 (four times)

Both real-save-file authorizations in this pass — the `BasicPixelChanger`
identifier fix and moving the stale `Home` duplicate aside — were given
explicitly by Zach in-session before either file was touched, per CLAUDE.md's
"Save files are sacred... only ever modified with authorization from their
owner/stakeholder Persons."

**Diagnosis credited to:** Codex (GPT-5.6 Sol), session `01a0707e-f743-71b1-8fb9-63975012e66d`, for the `.ecmatter` matter/semantic precedence finding and the six-invariant structural-hardening plan — see "Second red-canvas cause" and "Structural hardening" above. Implementation (Invariants 1, 2, and 3) is mine; the findings, invariant framing, and staged landing order are Sol's, posted on `agent intercom/communication-threads/Basic Pixel Changer Zone Identity Bug 9-7-26.md`. Sol's own framing: "Zach originated the demand that this never become a bureaucracy again and that serialization follow the Singular ontology; I am extending that human direction into the invariants and landing sequence." Also thanks to Claude Opus 5 (session `01F9nK3F`), concurrently restructuring the Law/Rete engine in the same checkout, for proactively confirming on the intercom that their work touches none of `Singularity/Storage`, ruling that out as the source of several test failures that turned out to be resource contention between two agent sessions building on the same machine.

Zone's identifier/name split is credited to Zach directly: seeing the live console
output and being asked whether to pursue it, he answered without hesitation —
"Yes Zone should absolutely have a real identifier/name split. Its a Singular" —
naming both the fix and its ontological grounding in one sentence.
