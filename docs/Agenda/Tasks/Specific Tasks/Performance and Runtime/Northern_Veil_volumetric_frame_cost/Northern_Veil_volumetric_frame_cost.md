# Make Northern Veil inhabitable: its volumetric frame cost

**Opened by:** Zach, 2026-10-09: Northern Veil is "beautiful but inhabitable bc of lag freezing the computer." The Zone was authored by Gemini (PR #346) with Zach ("THE AURORA IS HEREEEEEEEEEE"); the 2026-09-25 resident-parameter candidate gave no responsiveness gain there (see [Sunlit Mist task](../Sunlit_Mist_saved_world_responsiveness/Sunlit_Mist_saved_world_responsiveness.md)).

**Zach's constraint (2026-10-09):** "WE MUST NEVER TRY TO 'fix' THINGS BY LOWERING RESOLUTION OR WINDOW SIZE." Only work that cannot change the image may be removed — Prism Sun's "Keep the aurora. Kill only the work that reality cannot observe."

## Measurement (2026-10-09, MacBook Air M5, native WebGPU/Metal, Debug build)

Volume-only probe over the real save, read-only: `zone.ecform` decoded to a scratch JSON copy; camera on the spawn dais looking at the curtains; GPU-synchronized. Probe source adapted from Codex's [Sunlit Mist probe](../../../../../../scratch/webgpu_sunlit_mist_perf_probe_2026_09_25.cpp); it lived only in the session scratchpad.

| Curtains (640×360) | ms/frame |
|---|---|
| any one alone | 6.6 – 9.1 |
| two (0+1, 0+2) | 21 – 22 |
| three | 46 – 50 |
| all four | 88 – 124 (drifts with machine load) |

All four at 320×180: 32 ms; at 1280×720: 418 ms. The cost is per pixel, and **each added curtain roughly doubles it**.

## Cause 1 (GPU, dominant): 96 samples per overlap segment

`src/Singularity/Screen/WebGPU/SdfWgsl.cpp` `compileVolumeSet` (~line 4092–4143) sorts every medium's entry/exit along the ray, then takes **96 samples in every occupied segment** and evaluates every medium at each sample. Four overlapping boxes cut one ray into up to 7 segments, so up to 672 samples × 4 media. Each curtain is a thin sheet inside a box up to 160×36×46, so most samples evaluate the full noise expression only to find density 0.

Candidate fixes, in order:
1. **Prove empty air empty (image-exact).** Bound each medium's authored density over regions with OntoMath `evalRange` and skip proven-zero sub-intervals, with exact fallback. This is the Sun lineage's own method (PR #259's `SdfRangeHierarchy`), which explicitly excluded FieldNode volume density. Pixel parity is the gate.
2. **⚑ AUTHOR — sample placement.** Allocating samples by distance along the ray instead of 96 per segment changes pixels (a different quadrature of the same integral). It can be made as accurate or more accurate, and it was anticipated in [V5 follow-ups](../../Rendering%20and%20OntoMath/Visual_radiance_V5_and_Rung_8_followups/Visual_radiance_V5_and_Rung_8_followups.md) ("error-controlled local quadrature"). Zach decides, because the image changes.

## Cause 2 (CPU, ~7 ms/frame): JSON serialization as change detection — fixed

Every frame `Rendering::readVolumeDensity` and `EngineRender` serialized all 24 authored channel expressions to JSON and hashed the text, only to learn whether they changed. Zach: "Y IS IT SERIALIZING INTO JSON AT ALL WE MIGRATED TO MSGPACK/FLATBUFFERS." This was never storage; the migration did not touch it.

**Fix (uncommitted in the working tree):** `geom::FieldNode` carries an authored-math revision drawn from one process-wide, never-repeating sequence. A recycled node address can never present a revision a renderer cache has seen (the SourceRho producer-rebinding hazard).
- The property bridges bump it, and skip the bump on an identical rewrite.
- `applyJson` bumps it.
- The MCP `author_volume` path, which assigns channels in place (`WebSocketServer.cpp`), now bumps it by hand.
- Fail-open: `verifiedAuthoredMathRevision()` re-hashes one channel per call, round-robin. An unrevisioned change is bumped and reported on stderr, so a missed writer heals within ten reads and never stays stale silently.

Witness: `tests/singularity/field_node_authored_revision_test.cpp` (19/19). In the full suite the verifier fired only in that test's deliberate case, so no production writer was missed.

A–B–B–A, all four curtains: CPU projection **6.75–7.50 → 0.65–0.93 ms/frame**; lit-pixel count and luminance sum identical block for block. Frame time did **not** improve, because the frame is GPU-bound and the CPU work overlapped the GPU. The win appears once Cause 1 is addressed.

Not converted: `WebGpuRenderer::drawImplicit` (~line 1453) still JSON-hashes Object-attached field channels per draw. It is the same fix, left for a follow-up.

## Found along the way

- **`.ecform` zone saves are a MsgPack envelope around one JSON string.** `SaveSystem.cpp:424` writes `{"MigrationRoot": j.dump()}` and load parses that JSON. The MsgPack/FlatBuffers migration is skin-deep for Zones; FlatBuffers appears only in `ZoneManager.cpp:2832`. This is load-time only, not this lag.
- **`RealSaveTreeGuard` does not protect `saves/worlds/`.** It restores `saves/zones` and `saves/homes` only, and an aborted test skips its destructor anyway. The 2026-10-09 full `ctest` run rewrote `saves/worlds/chess_app.ecform`. Only `matterGeneration.sha256/snapshotId` changed, and the `.ecmatter` sidecar was replaced (`7431ca37…` → `556fa746…`, 7 of 13,652 bytes differ). It was left for Zach to restore or keep.

## Suite state at this change (not an A/B)

`ctest`: 268/281 pass. Failing: chess_app/castling/click_geometry/extended_rules (Law promo-button conditions), quantifier_scaling (lag), second_nature_law_forge_zone, synthesis_studio_app, authorable_light_contract, gpu_mastery, slow_adapter_zone_perf, webgpu_perlin_exact_gradient, prism_cathedral, zone_home_ontology.
- **Known elsewhere:** authorable_light_contract and slow_adapter have fixes on unmerged branches (`d3f38014`, `dc0a8fc2`); second_nature_law_forge is named in `d3f38014`.
- **Not proven:** the rest touch code this change does not, but no baseline build was run. Another worker committed OntoMath/ProbabilityForm changes into this checkout (01:02–01:12 PDT) during the run.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01NJy6VrPVNcHAnggwFyTsmF` (local `9e6def41-f1a4-4d08-afe4-f04085c5da75`) · 2026-10-09 01:17 PDT
