# Northern Veil — Data Home Persistence Desync Root Cause & Resolution Audit

**Date:** 2026-11-02  
**Sign-off:** Antigravity · Sparkly Guy · session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
**Commissioned by:** Zach & Prism Sun  
**Context:** Investigating why Firmament's extensive Northern Veil aurora redesign (commit `fbfcf69ec`) appeared unchanged in the running application.

---

## 1. Executive Summary

When Firmament authored the high-contrast auroral upgrade for Northern Veil (24× ray-pillar contrast, 2.2m ionization hem, 3D billow, multi-tiered depth-altitude geometry, and 4 procedural glacial sanctuary materials) into `saves/zones/Northern Veil/zone.ecform` at commit `fbfcf69ec`, live runs of Earthcall showed no visual changes.

Investigation proved this was **not** an OntoMath compilation failure, shader regression, or ray transport bug. It was a **silent persistence-boundary desync**:

1. Earthcall runtime selects the Person Data Home (`~/Library/Application Support/Earthcall`) via `SaveSystem::chooseDataRoot()`.
2. When migrating checkout saves to Data Home, `SaveSystem::seedMissingCheckoutIdentities()` uses a `skip_existing` policy for all zones **except** a hardcoded whitelist (`isSyncedShowcase`), which historically only included `Sky-Island Sanctuary` and `SynthesisStudio`.
3. Because `Northern Veil` was migrated to Data Home on 2026-10-12 and already existed there, `seedMissingCheckoutIdentities()` silently skipped it on boot.
4. Consequently, the live application continued loading the stale 2026-10-12 snapshot (97,635 bytes) rather than the newly authored 2026-11-02 commit (110,906 bytes).

---

## 2. Step 0 Evidence — Byte-for-Byte Proof

Before making any mutations, we verified the file states, cryptographic hashes, and directory listings across both boundaries:

### Checkout vs Data Home Inspection

| Boundary | Path | File Size | SHA-256 Hash | Last Modified |
| :--- | :--- | :--- | :--- | :--- |
| **Checkout (Repo)** | `saves/zones/Northern Veil/zone.ecform` | 110,906 bytes | `76930b80491bf51754ce627ea4c6f6eb8a7c3d3fd3af6ee767b547af1a21c58a` | 2026-11-02 20:01 (Commit `fbfcf69ec`) |
| **Data Home** | `~/Library/Application Support/Earthcall/zones/Northern Veil/zone.ecform` | 97,635 bytes | `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496` | 2026-10-12 12:33 (Commit `71a8503d`) |
| **Pre-Commit Checkout (`fbfcf69ec^`)** | Git tree `fbfcf69ec^:saves/zones/Northern Veil/zone.ecform` | 97,635 bytes | `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496` | Pre-redesign snapshot |

**Critical Invariant Verified:**  
`git show fbfcf69ec^:saves/zones/Northern Veil/zone.ecform == Data Home zone.ecform` returned **`True`**.  
The Data Home file was byte-identical to the pre-redesign repository commit. There were zero live gameplay mutations, placed objects, or custom Person edits in Data Home's Northern Veil folder.

---

## 3. Structural Decoding & Verification of Firmament's Redesign

Decoding both files with MessagePack (`msgpack`) confirmed the full authored scope in checkout:

- **Zone Identity:** `Northern Veil` (128 Lexemes preserved).
- **Spatial Fields (4 upgraded volumetric fields):**
  1. `northern_veil.aurora.primary-emerald-curtain`: Origin `[0, 46, 75]`, Scale `[164, 36, 46]`. Upgraded with 3-harmonic ray-pillar fold equation ($0.04..0.96$, 24× contrast), 2.2m lower ionization rim ($a_{\text{rise}} = 0.45$), and 12.5× 3D cellular Perlin billow.
  2. `northern_veil.aurora.secondary-cyan-ribbon`: Origin `[12, 52, 128]`, Scale `[156, 38, 44]`. Repositioned to upper depth shell with electric teal-aquamarine emission.
  3. `northern_veil.aurora.accent-violet-crest`: Origin `[-5, 78, 84]`, Scale `[164, 38, 46]`. Royal violet-magenta corona ribbons with vertical altitude concentration $H_{\text{alt}}(y)$.
  4. `northern_veil.aurora.deep-crimson-fringe`: Origin `[16, 72, 66]`, Scale `[104, 60, 36]`. Neon rose-magenta/crimson upper curtain fringe.
- **Sanctuary Procedural Materials (4 OntoMath AST materials):**
  1. `northern_veil.obsidian_lake`
  2. `northern_veil.glacial_dais`
  3. `northern_veil.arctic_peak`
  4. `northern_veil.stele_monolith`

---

## 4. Corrective Action & Persistence Synchronization

1. **Immutable Backup:**  
   Created timestamped backup at `~/Library/Application Support/Earthcall/backups/Northern_Veil_backup_20261102/` before any writes.
2. **Data Home Synchronization:**  
   Updated `~/Library/Application Support/Earthcall/zones/Northern Veil/zone.ecform` to match the checkout file (SHA-256 `76930b804...`, 110,906 bytes). Cleaned up stale 14-byte `test.json`.
3. **Data Home Git Commit:**  
   Committed the updated Northern Veil into the Data Home git repository (commit `75d6f2f`) with full authorial attribution:
   ```
   commit 75d6f2f: Sync Firmament Northern Veil aurora redesign from checkout commit fbfcf69ec
   ```
4. **Codebase Repair in `src/Singularity/Storage/SaveSystem.cpp`:**  
   Expanded `isSyncedShowcase` to include Northern Veil and its identifiers:
   ```cpp
   const bool isSyncedShowcase =
       (fname == "Sky-Island Sanctuary" ||
        fname.rfind("sky.sanctuary.", 0) == 0 ||
        fname.rfind("law-sky-sanctuary-", 0) == 0 ||
        fname == "sky_island_sanctuary.json" ||
        fname == "SynthesisStudio" ||
        fname == "SynthesisStudio.LivingInstrument" ||
        fname.rfind("studio.", 0) == 0 ||
        fname.rfind("law-studio-", 0) == 0 ||
        fname == "synthesis_studio.json" ||
        fname == "synthesis_studio_living.json" ||
        fname == "Northern Veil" ||
        fname.rfind("northern_veil.", 0) == 0 ||
        fname.rfind("northern.veil.", 0) == 0 ||
        fname.rfind("law-northern-veil-", 0) == 0 ||
        fname == "northern_veil.json");
   ```
   This ensures that future git pulls or repo commits to Northern Veil automatically synchronize into Data Home on boot without manual copying.
5. **Runtime Hydration Consistency in `src/ZonesOfEarth/ZoneManager.cpp`:**  
   Updated line 1327 to admit Northern Veil for live json reapplication alongside Sky-Island Sanctuary:
   ```cpp
   if (live->getOwnedObjects().empty() || id == "Sky-Island Sanctuary" || id == "Northern Veil")
       applyZoneJson(*live, zj, true);
   ```

---

## 5. Verification Status

- `earthcall_webgpu` compiles cleanly and links (`build/earthcall_webgpu`).
- `SaveSystem::chooseDataRoot` and `SaveSystem::readZoneIdentity("Northern Veil")` verified: resolves to Data Home and returns the 4 upgraded spatialFields and 4 sanctuary materials.
- `save_system_error_test`, `zone_identity_test`, and `zone_spatial_field_roundtrip_test` pass 100%.
- Both `./build/earthcall_webgpu` and `/Applications/Earthcall.app` updated.
