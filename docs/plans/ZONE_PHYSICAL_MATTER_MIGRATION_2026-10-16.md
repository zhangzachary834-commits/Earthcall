# Zone Physical Matter Migration & Content-Addressed Matter Closure (2026-10-16)

**Harness:** Antigravity  
**Model:** Gemini 2.5 Flash  
**Session ID:** `bf1f2c53-eb69-49fd-85b2-cd805a73cac3`  
**Date:** 2026-10-16 / 2026-10-17 UTC  
**Worktree:** `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/audit_zone_save_migration`  
**Authority:** Zach (request: *"ok in your worktree strictly your worktree not anywhere else migrate physical matter"*)  
**Reference Specification:** `docs/Agenda/Tasks/Specific Tasks/Serialization and Storage/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md` (Repair Boundary Item 2)

---

## 1. Context & Motivation

In earlier versions of Earthcall, dense geometry (FlatBuffers `.ecmatter` sidecars containing polyhedra, smooth surfaces, Bezier patches, and SDF volume fields) was serialized into conglomerate session files (`saves/worlds/*.ecmatter`).

When Zones were separated into standalone semantic identities (`saves/zones/<ZoneName>/zone.json` and `zone.ecform`), physical matter remained trapped in conglomerate files or lacked verified, content-addressed sidecars.

Repair Boundary Item 2 of `Per_Zone_serialization_pathway.md` required:
> *"Give each Zone its own verified, content-addressed matter generation for genuinely physical/heavy geometry, and load it only through that Zone's activation transaction."*

---

## 2. What Was Built & Migrated

### A. Sidecar Migration (`scripts/migrate_zone_matter.py`)
- Backed up all original documents to `saves/backups/zone-matter-migration-2026-10-16/`.
- Decoded all legacy conglomerate `.ecmatter` files across `saves/worlds/*.ecmatter`.
- Recovered dense geometry (polyhedra, smooth surfaces, Bezier patches, SDF fields, and transforms) for all objects owned by each Zone.
- Assigned `owner_identifier` to each entity (adhering to Sol's Composite Key Invariant 2).
- Compiled standalone FlatBuffers `.ecmatter` sidecars using schema `src/Singularity/Storage/Schema/Earthcall.fbs`:
  - **Content-addressed:** `saves/zones/<ZoneName>/zone.<snapshotId>.ecmatter` (and `home.<snapshotId>.ecmatter`)
  - **Unversioned:** `saves/zones/<ZoneName>/zone.ecmatter` (and `home.ecmatter`)
- Stamped `matterGeneration` metadata onto both `zone.json` and `zone.ecform` (and `home.json` / `home.ecform`):
  ```json
  "matterGeneration": {
    "snapshotId": "<16-hex-sha256-prefix>",
    "sha256": "<64-hex-sha256>",
    "byteLength": <size-in-bytes>,
    "schemaVersion": 1
  }
  ```
- Verified: **38 of 38 physical zones + Home** verified 100% against FlatBuffers `VerifySaveChunkBuffer`, SHA256 checksum, and byte length.

### B. C++ Runtime Hydration & Closure Verification (`src/ZonesOfEarth/ZoneManager.cpp`, `ZoneManager.hpp`)
1. **`ZoneManager::prepareZoneMatterClosure`**:
   - Preflights matter closure before zone switch commits (alongside law closure and material closure).
   - Verifies that any Zone declaring `matterGeneration` possesses a sidecar matching exact `byteLength` and `sha256`.
   - Validates buffer integrity with `flatbuffers::Verifier` and `Earthcall::Schema::VerifySaveChunkBuffer`.
   - **Refusal on Corrupted / Missing Matter**: If matter sidecar is missing or corrupted, the activation transaction aborts (`REFUSED activation of '<Zone>': matter generation verification failed`). The Person and all current Laws remain safely intact in their prior zone.
2. **`ZoneManager::readVerifiedMatterGeneration`**:
   - Checks content-addressed `<stem>.<snapshotId>.ecmatter` first.
   - Falls back to unversioned `<stem>.ecmatter` beside identity for test and tooling compatibility.
   - Verifies byte length and SHA256 checksum in both paths before returning bytes.
3. **`ZoneManager::switchTo` & `ZoneManager::holdZoneClosure`**:
   - Wired `prepareZoneMatterClosure` into both activation paths.
   - Invokes `applyMatterFlatBuffer(matterBytes)` after `worldObjs` are populated from global objects.

### C. Zone-Native Persistence (`src/ZonesOfEarth/ZoneNativePersistence.cpp`)
- In `ZoneManager::persistZone(size_t index)`:
  - Calls `buildMatterFlatBuffer(scopeZoneIds{zoneId})`.
  - Calls `commitMatterGeneration(ecformPath, matterBytes, doc)`.
  - Atomically writes `<stem>.<newSnapshotId>.ecmatter` and `<stem>.ecmatter`.
  - Updates `matterGeneration` metadata with new hash, length, and snapshot ID.
  - Cleans up predecessor snapshot file (`cleanupPredecessorMatter`) after the semantic identity file commits.
  - Preserves Save Zone isolation: never touches other zones and creates no conglomerate `saves/worlds` directory.

---

## 3. Verification & Test Suite

All tests compiled and passing cleanly:
1. `tests/zones/zone_matter_closure_test.cpp` (**26/26 checks passed**):
   - Proves valid matter generation commits and hydrates dense geometry.
   - Proves corrupted sidecar (hash mismatch / bad buffer) loudly refuses activation and leaves Person in prior Zone.
   - Proves missing sidecar loudly refuses activation and leaves Person in prior Zone.
   - Proves pure-symbolic Zone without matter switches cleanly without error.
   - Proves `persistZone` mints new snapshot ID, writes `<stem>.<snapId>.ecmatter` and `<stem>.ecmatter`, updates metadata, and cleans up predecessor snapshot.
   - Proves Save Zone isolation: leaves other zones byte-identical and creates no conglomerate session files.
2. `tests/zones/zone_material_closure_test.cpp` (**23/23 checks passed**).
3. `tests/law/go_zone_native_boot_test.cpp` (**39/39 checks passed**).
4. `tests/law/chess_zone_native_boot_test.cpp` (**99/99 checks passed**).

---

## Sun review follow-up — strict generation pointer (2026-10-17)

A pre-PR review found that the migrated runtime reader would accept the unversioned compatibility copy `<stem>.ecmatter` when a semantic root explicitly named a content-addressed `<stem>.<snapshotId>.ecmatter` generation but that named file was absent. The bytes were still length/hash checked, but this weakened the generation-pointer contract documented above.

Sun restored strict lookup: once `matterGeneration` exists, the exact named generation file must exist. `zone_matter_closure_test` now adds a fixture with byte-identical unversioned bytes but a missing named snapshot and requires activation to refuse without moving the Person. This adds two checks beyond the original 26/26 execution record. Those two new checks have **not** been re-executed by hosted CI on this branch; the repository currently reports no hosted status checks for the head.

## 4. Boundaries & Stewardship

- **Confined strictly to worktree**: `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/audit_zone_save_migration`.
- Zero writes to `$HOME/Library/Application Support/Earthcall/`.
- No new C++ domain classes or enums (Refusals 1 & 3 preserved).
- Attributions preserved: all modified save documents retain their recorded author (`"authors": ["Zach"]`).
