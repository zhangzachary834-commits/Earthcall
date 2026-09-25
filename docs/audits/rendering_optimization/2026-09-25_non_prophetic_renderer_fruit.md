# Cheap work the renderer can stop doing now

- **Harness and model:** Codex / GPT-6
- **Session:** `01a0cfbf-c751-7af0-b160-df07da055bc0`
- **Date and time:** 2026-09-25 11:48 PDT
- **Inspected base:** `c645315d` in isolated worktree `volume-param-residency`
- **Commission:** Zach asked for low and middle hanging rendering improvements from industry practice that obey his **minimum-maximum principle** while the Sol Suns pursue Prophetic Rendering. The Person's authored OntoMath remains untouched; all changes here are Screen-channel execution and measurement.

## The bounded win implemented

Apple's [Metal resource guidance](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/PersistentObjects.html) recommends retaining GPU buffers whose storage can be reused, while its [dynamic-data guidance](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/TripleBuffering.html) explains why frequently changing frame values need a different lifetime. Earthcall already applies this distinction to SDF parameters and range-proof words. I applied the same existing Screen pattern to authored **volume parameter batches**: the compiled medium's float values stay in a pipeline-local GPU buffer, grow geometrically, and upload only when their exact bytes change. Camera, Timeline, instance position, and the global uniform still use the frame ring. No authored field, transport equation, shader structure, or new kind of being was introduced.

The renderer now counts `volumeParameterBytesUploaded` in its Kernel `FrameStats`. An unchanged fused two-medium frame went from the old unconditional parameter ring upload to **0 authored-parameter bytes uploaded**. In the native witness, the first draw uploaded **20 B**, the next identical draw uploaded **0 B**, and the stable frame used **2** ring suballocations (global uniform and instance data; the old code also suballocated parameters). Timeline-only movement likewise uploaded **0 B** while visibly changing `E_v(p,t)` through the per-instance time coordinate. A numeric `E_v` edit uploaded new parameter bytes, changed native pixels, and reused WGSL. Allocation failure retains the previous frame-ring path; shader reload and renderer shutdown release the resident buffers before pipeline-address keys are retired. Their VRAM is included in frame accounting.

The second small change removes a needless per-frame allocation in plural-source radiance: `flushSdfDraws` previously constructed a fresh byte vector from every `RadianceSourceGpuData` batch only to compare it with its existing mirror. It now compares directly against the batch bytes and copies into the mirror only when values change. The native two-source witness in `webgpu_object_test` passed after the change. This removes one CPU allocation on an unchanged plural-source frame; it is not a reported FPS gain.

## Why this is within the minimum-maximum principle

These are two lifetimes for data the Screen kernel already owns. Authored expressions remain the maximal vocabulary; a GPU buffer handle is an irreducible channel artifact, never an ontological property of a light or medium. The implementation adds no general caching framework, no user-facing mode, no enum of visual things, and no sampling shortcut. Exact float-byte comparison is the conservative invalidation rule: a changed authored value uploads, regardless of whether a higher-level revision happens to be trustworthy. The old complete frame-ring route remains available if device allocation fails.

This also has a limit. Each distinct compiled volume pipeline can retain a buffer until shader reload or shutdown, as the pre-existing SDF pipeline caches do. Long authoring sessions may accumulate resident buffers; measure cache entries and VRAM before adding eviction. The CPU still assembles and compares the parameter vector every frame. This pass removes redundant GPU transfer and one per-pipeline ring allocation; it does **not** remove medium discovery, Piecewise serialization, fragment sampling, or generated WGSL size.

## Ranked next candidates, with their proof burden

| Candidate | Why it is promising | Required gate before implementation |
|---|---|---|
| **Revision-led medium discovery** | `readVolumeDensity` currently serializes/hash-checks all six authored Piecewise channels per Northern Veil medium each frame: 24 serializations before GPU work. It may exceed the tiny parameter-transfer bill removed here. | Measure CPU time in the saved Zone. Declare every in-place edit, creation/removal, and Timeline dependency that invalidates a cached projection. Keep a conservative full-content fallback until the mutation ledger is complete. |
| **Bind-group reuse or dynamic offsets** | WebGPU creates fresh mesh, SDF, and volume bind groups during rendering. Immutable bindings could be reused where buffer and texture identities truly stay stable. | Count group creations and CPU time in a representative many-pipeline scene. Current uniform/instance rings change offsets every frame, so caching the current descriptors by appearance alone would be stale. Only adopt a measured lifetime-aware design. |
| **Pipeline and resident-buffer lifetime** | Programs, pipelines, and now volume value buffers live until reload/shutdown. A prolonged authoring session can create many structures. | Record count, bytes, and edit/revisit behavior over a long session; then choose an eviction policy that leaves authored output exact and cannot reuse a retired pipeline pointer. |
| **GPU-bound Perlin field evaluation** | The separate [Sol SDF audit](2026-09-23_sol_sdf_performance_followup_audit.md) and native runs locate the horizon cost in exact field evaluation at native resolution. | Continue the Suns' paired image/CPU/GPU/query-cost witnesses. These CPU transfer wins should never be sold as a cure for that GPU workload. |

Some attractive sounding “standard” work is already done: meshes use a persistent vertex cache, SDF values/proof words avoid unchanged uploads, and the heavy 4,500-object native probe now batches **3,000 meshes plus 1,500 fields into two draw calls and five buffer suballocations**. I ran that existing probe on this machine at base `c645315d`: 60 frames averaged **41.13 ms** (38.91 ms calibration-normalized) and passed its standing baseline. That result is a baseline and prioritization clue, not a before/after measurement of this volume change; the heavy probe does not exercise volumes. Another general batching campaign would be poorly aimed at its remaining cost.

## Verification and visible effect

- Fresh isolated Debug build of `webgpu_object_test` and `webgpu_v5_overlap_physics_test` succeeded using the repository's required CMake/OpenSSL configuration and local dependency sources.
- Both native WebGPU tests passed on the Mac GPU after the change. The object witness checks unchanged-frame upload avoidance and pixel identity, Timeline-only zero upload with visible time response, numeric emission upload with visible change, two-source radiance, and the existing refusal/order cases. The V5 closed-form overlap test passed separately.
- The existing heavy 4,500-object probe passed on the unchanged main binary, serving only as a current baseline.
- A native GPU device was unavailable inside the sandbox; the successful native witnesses ran outside it. No Northern Veil screenshot or new FPS delta was measured.
- A Person should see **the same authored pixels** under the same camera/time. The expected effect is less recurring CPU/GPU transfer work in stable volumetric scenes; only an instrumented saved-world run can say whether frame time materially improves. No save file or authored being was changed.

**Next-agent direction:** Keep the resident buffer's reload/shutdown/VRAM accounting together if editing pipeline lifetime. Use the existing native object and V5 tests as gates. Profile Northern Veil's CPU medium discovery and compile/main-pass cost separately before optimizing anything else in that path. Leave Prophetic proof authority and SDF sampling with their active Suns.

**Signed:** Codex / GPT-6 · session `01a0cfbf-c751-7af0-b160-df07da055bc0` · 2026-09-25 11:48 PDT
