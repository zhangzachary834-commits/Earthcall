# Zone identity store: field-level merge, not whole-object replacement

**Status:** fixed and regression-tested
**Created:** 2026-09-07 by Claude, investigating a report from Zach
**Code:** `src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.cpp` (`mergeZoneObjectsFromJson`, `applyZoneJson`), `src/ZonesOfEarth/ZoneManager.cpp` (`admitFromJson`'s identity-store branch)
**Test:** `tests/zones/zone_identity_test.cpp` (new block at the end)

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

## A related but distinct gap found while verifying: `faceColors` never round-trips at all

Watching a fresh `saves/zones/BasicPixelChanger/` identity file get written (Zach
reloaded the World live, past the fix above) showed it *still* has no `faceColors`
key — not staleness, but because `Object::to_json`
(`ObjectSerialization.cpp`) has no `faceColors` write at all; only `from_json`
reads it. This means every identity-store snapshot, past and future, omits
`faceColors` unconditionally, which is why the field-level merge above is a
permanent necessity for this field, not a one-time fix for an old file. It also
means a Person who changes an object's `faceColors` live via a Law (not by
re-authoring the World) will lose that change on the next app restart — the
in-memory value is correct until then, but never reaches disk. Not fixed here
(out of scope for what was asked), but worth a future pass adding `faceColors` to
`to_json` if a Person ever reports a live color change not surviving a restart.

## Addendum: the test-isolation bug found while investigating this

Verifying the fix required running `chess_extended_rules_test`, which pointed
`SaveSystem::setSaveRoot` directly at the real repo `saves/` tree — so
`BootedEngineHarness`'s `hydrateFromZoneStore()` loaded every real Zone identity
(Chess, FarLands, SynthesisStudio, both Homes), and a subsequent load's "preserve
unsaved work" write-back re-serialized all of them with fresh relation-event
timestamps, silently drifting real save files on every test run. Caught via
`git status`/`git diff` before anything was committed; the polluted files were
reverted with `git checkout`. Fixed by copying `saves/zones/`, `saves/homes/`,
and only the one world file the test needs into a disposable temp sandbox before
calling `setSaveRoot`, matching `unsaved_preserve_test`/`world_switch_test`'s
existing pattern. `synthesis_studio_app_test` did not need this — it parses its
save file directly into local objects and never touches `SaveSystem`/
`ZoneManager`. See the To-Do list's Housekeeping entry.

---

**Signed:** Claude Sonnet 5
**Session:** `01Mvd55GFWyUMrYWt2ERGSRE`
**Date:** 2026-09-07
