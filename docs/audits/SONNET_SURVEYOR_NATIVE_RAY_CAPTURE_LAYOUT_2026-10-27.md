# Surveyor — distinct native rays and general tile layout (ready to execute, not executed on GPU)

Claude / Sonnet 5.5 (`claude-sonnet-5-5`) / Claude Code desktop / session `06f17252-810f-48ee-ad18-95ed12c6ff27` / 2026-10-27T23:30 PDT. Branch `claude/native-ray-capture-layout-9b40ea`, worktree `.claude/worktrees/native-ray-capture-layout-9b40ea`, started at `46ebde497`; code and first report landed as `acf252b0a`; a follow-up commit corrects one count. Nothing pushed or merged.

**Origin.** Sixth-One Sun (Codex / GPT-6.1 Sol) commissioned this, operating Claude through computer use on Zachary Zhang's instruction; Zach's goal as relayed is faithful authored rendering with a useful Northern Veil performance checkpoint. This is the Sun's wording, not Zach's. It answers the "next owner" paragraph of [§6em](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md) ("a bounded denser set of distinct recorded native rays with correct tile topology, finite coverage and actual reference/parity checks"). The fixed 256×144 route it generalises is Codex's `ray_transport_wide_capture.py` and the pose capture; the M5 adapter limits come from Antigravity's [§6ek](GPU_RAY_TRANSPORT_6EK_INDIRECT_CONSUMER_AND_MULTI_SCALE_2026-10-27.md). My extension: the grid/tile/budget generalisation, the corruption witnesses, and the harness patch.

**Gate status.** Per 6em all three shipping gates stay OPEN. No GPU capture, timing or reference ran; no renderer `src`, save, app, default, lighting or resolution changed. Files owned: `scratch/probes/sonnet_surveyor_*` and this document.

## What the old route pinned (found by reading, then confirmed by execution of the replacement)

| Pin | Where | Replaced by |
|---|---|---|
| capture grid ∈ {64×36, 128×72, 256×144} | `assert` in `ray_transport_cf4_trial.build` | any grid whose stride divides the viewport (validated); in-memory patched copy, file untouched |
| `build/` read from the source root | same | `--build-root` (engine `src` tree hash must match; refused otherwise) |
| 10 px cell, 4×4 tile, `groups4x4.size()==2304` | generated 6em harness | `layout.json` built and validated in Python, re-validated in C++ |
| `len==32784`, `capture_grid==[256,144]` | `ray_transport_gpu_wide_stream.verify_case` | variable-N `sonnet_surveyor_capture.verify_case` |
| `rank-select 32784` | `wide_capture.py` | keep every completed ray by default; optional uniform-rank subset (chooses real rows, alters none) |

## Deliverables

- [`sonnet_surveyor_layout.py`](../../scratch/probes/sonnet_surveyor_layout.py): census contract (256 B slot, 16 live words, rest must be zero), decoder, tile layout (`compact` = non-empty tiles, identical to the legacy `std::map` order; `dense` = every tile incl. empties), `validate_layout`, buffer-by-buffer `budget`, feasibility table.
- [`sonnet_surveyor_capture.py`](../../scratch/probes/sonnet_surveyor_capture.py): `plan | prepare | capture | convert | verify | patch-harness`. `capture` refuses without `--execute-gpu`.
- [`sonnet_surveyor_harness_patch.py`](../../scratch/probes/sonnet_surveyor_harness_patch.py): text patch of the generated 6em clean harness; refuses if any anchor drifts or is applied twice.
- [`sonnet_surveyor_test.py`](../../scratch/probes/sonnet_surveyor_test.py): 54 CPU witnesses.

**Provenance preserved.** One row = one real pixel centre: pixel, native origin (bitwise-equal across rays, else refused), *unnormalised* direction, depth clips `[t0,t1]`, native `T/L`. No jitter, normalisation, replication or upsampling. The 23-column row schema is unchanged. The per-medium clock is **not** in the census; it is fixed by (zone sha256, clock argument), both bound in the manifest and re-checked by `verify` (zone hash recomputed). That is binding, not per-ray evidence.

## CPU results (this session)

`python3 -B scratch/probes/sonnet_surveyor_test.py` → **54/54 witnesses**. Highlights:

- Decode: pixel/origin/direction/clips/outputs bitwise-equal to what was written; directions deliberately non-unit stay non-unit.
- 16 census corruptions refused each for their own reason: short/long file, bad valid or completion word, nonzero word beyond the 16-word record, data in an untouched slot, pixel not the cell centre, wrong cell, NaN direction, inf origin, reversed clip, zero direction, T>1, negative radiance, mixed origin, duplicate direction/clip.
- Layout: compact layout equals an independent re-implementation of the pinned 6dq grouping on a non-2304-tile population, **and on the real stored 32,784-ray/2,304-tile Codex case** (`/private/tmp/earthcall-wide-case/rays.json`; that check silently passes if the file is absent). Six grid/tile shapes × two modes (edge-partial tiles, 8×2, 16×1, 3×5, 1×1), tiny populations (2 rays; 1 ray in 8 dense tiles).
- 11 layout corruptions: duplicate/dropped/out-of-range index, non-monotone offsets, bad first/last offset, tile over 16, two rays in the wrong tile with order preserved, unsorted tile, stale tile id, empty tile in a compact layout.
- Budget boundaries are exact: 131,072 rays admitted and 131,073 refused under default limits; working set, u32 `id*maxSubPerRay`, dispatch-per-dimension and stride-1 census overflow each refuse.
- Case round trip (all rays / uniform-rank subset / dense) and **8 tamper-and-re-sign cases**: the manifest hashes are recomputed so they agree and only independent re-derivation can catch the lie (swapped rows, NaN direction, duplicated row, swapped layout index, lying count, shifted holdout, pixel off centre, relabelled grid). An unsigned tamper is caught by hash.
- Harness patch: pin removed; drifted-anchor and double-patch refused. `-fsyntax-only` of the patched harness with the engine's own `entry.cpp` flags and `-std=c++20` succeeds (an earlier misplaced guard failed this same check, so it is not vacuous).

**Capture probe.** Source trees match (`3603add4018961529a91bc9592d76f9a553500b1` in this worktree and the build checkout). The stride-10 and stride-4 generated renderers differ **only** by the grid constants (`diff` shows the census size and four shader lines), so the shader text at the new grid is the already-exercised 256×144 text with different numbers; stride 10 is the same text the old route generated. The stride-4 probe **was fully built, not run**: `/private/tmp/earthcall-sonnet-surveyor-s4/cf4`, sha256 `5a6a56a8b566aef0d20690487231d037450e2f1c3d9102a7f9fcd996e791c259`, renderer sha256 `1135b264…e027a`, 58,982,400 B census. The WGSL has not been compiled by a GPU pipeline, so a pipeline-creation failure is the one thing only the first capture can reveal. Dependency-file flags are stripped so the shared build tree is not written.

## Feasible distinct-ray sizes (2560×1440, wide-inside coverage assumed 88.9% as observed at stride 10)

| stride | grid | slots | est. completed rays | census | consumer GPU total* | capture device (default limits) | consumer, default limits | consumer, M5 limits |
|---:|---|---:|---:|---:|---:|---|---|---|
| 10 | 256×144 | 36,864 | 32,784 (observed) | 9 MiB | 0.18 GiB | ok | ok | ok |
| 8 | 320×180 | 57,600 | ~51,000 | 14 MiB | 0.23 GiB | ok | ok | ok |
| 5 | 512×288 | 147,456 | ~131,000 | 36 MiB | 0.48 GiB | ok | **refused** (subpanels 128.06 MiB) | ok |
| **4** | **640×360** | **230,400** | **~205,000** | **56 MiB** | **0.70 GiB** | **ok (largest)** | refused | ok |
| 2 | 1280×720 | 921,600 | ~820,000 | 225 MiB | 2.6 GiB | **refused** | refused | ok |
| 1 | 2560×1440 | 3,686,400 | ~3.3 M | 900 MiB | — | refused | refused | refused (working-set assumption) |

*K=1, subpanel cap 64, overlap cap 16, includes the 80 MiB streaming buffers and the diagnostic readbacks.

Two different devices: the capture probe calls `gpu.init()` with no `requiredLimits` (WebGPU defaults: 128 MiB binding, so ≤524,288 slots of 256 B), while the 6em harness requests the adapter's limits (§6ek recorded `maxStorageBufferBindingSize` 4,294,967,292 and `maxBufferSize` 9,534,832,640 on the M5). So **the capturable ceiling today is stride 4 (≤230,400 slots)**; stride 2 needs a banded capture or limits raised in the probe, which I did not do. The consumer ceiling on the M5 is memory, not binding size: 8 GiB working-set assumption (half of `hw.memsize` 16 GiB, not a measured Metal limit) gives ≈3.1 M rays by this model. The patched harness re-checks the real device limits at run start and aborts on violation, so the table is a plan, not a guarantee.

Dominant cost per ray is the fixed 64-subpanel allocation (1,024 B/ray); it is why default limits stop at 131,072 rays. Reducing `EC_GPU_SUB_CAP` would be a new capacity policy and was not touched.

## Exact proposed invocations (Sun schedules the GPU step)

```sh
# 0. CPU witnesses (done; rerunnable, ~seconds)
python3 -B scratch/probes/sonnet_surveyor_test.py

# 1. capture probe: already built at /private/tmp/earthcall-sonnet-surveyor-s4/cf4 (rebuild with --full-build to a fresh dir)
python3 -B scratch/probes/sonnet_surveyor_capture.py prepare --root . \
  --build-root /Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport \
  --output /private/tmp/earthcall-sonnet-surveyor-s4 --stride 4 --full-build

# 2. ONE GPU capture (Sun only): writes census.bin and case/{rays,holdout-rays,layout,manifest}.json
python3 -B scratch/probes/sonnet_surveyor_capture.py capture --root . \
  --capture-bin /private/tmp/earthcall-sonnet-surveyor-s4/cf4 \
  --work /private/tmp/earthcall-sonnet-surveyor-case-s4-inside --stride 4 --case inside --execute-gpu

# 3. independent re-verification from files alone, and the patched consumer harness
python3 -B scratch/probes/sonnet_surveyor_capture.py verify --root . --manifest /private/tmp/earthcall-sonnet-surveyor-case-s4-inside/case/manifest.json
python3 -B scratch/probes/sonnet_surveyor_capture.py patch-harness --root . \
  --build-root /Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport --output <fresh>/surveyor_harness.cpp --syntax-check
```

`--case inside` is the same pose/clock as the 32,784-ray wide-inside run (`0,50,75,40,62,110`, 7.25 s, 2560×1440), so counts compare directly. Other poses: `reverse side spawn inside-zero-clock`. The manifest's `harness_env` lists the environment for the patched harness (`EC_POSE_RAYS`, `EC_SURVEYOR_LAYOUT`, `EC_POSE_CLOCK`, `EC_GPU_SCALE=1`).

## Limitations and open items (so nothing is over-read)

- **Not run on GPU**: the census format, new-grid WGSL, patched harness and consumer behaviour at ~205 k rays are unexecuted. Compile/syntax checks and synthetic censuses are not native evidence.
- **Reference not produced.** The CPU reference is the heavy job I was told not to run; `manifest.reference_command` records it (192 rank-selected holdout rays, `holdout-rays.json`, selected before any evaluation). Until it exists the manifest points `EC_POSE_REFERENCE` at `reference-STUB-NOT-A-REFERENCE.json` (empty rows) so parity/timing can start; **it carries no accuracy information**. 6em's accuracy gate (`ray_transport_gpu_pose_qualification`/`wide_stream`) still pins 32,784 rays and 256×144; wiring `verify_case` in is one assignment (`wide.verify_case = surveyor.verify_case`) but I did not edit those files. Only the holdout is densely referenced, as before.
- **Empty tiles**: shaders tolerate `rayCount==0`, but how classification routes an empty tile (and the `s1+s2+fb==numTiles` accounting) is unverified on GPU. Default is `compact`; `dense` is CPU-validated only.
- **Tile capacity is hard-wired to 16** in the shaders (`i < 16u`, `lid & 15u`); `validate_layout` and the C++ re-check refuse anything larger, since an over-full tile would silently drop rays.
- **Coverage is sparse by construction**: a census row exists only where the volume fragment ran to completion; rays outside the medium are absent, not zero. Single-origin harness: mixed origins are refused.
- **A dense population is distinct, not a frame**: it is still instrumented pixel centres at a stride (every 4th pixel in x and y), not a full-viewport workload or an FPS claim. Memory locality and traversal mix of a true 3.7 M-pixel frame are not represented, and the stride-4 grid is fixed-pitch, so tiles hold regular footprints. Distinct rays and pixel identity are preserved; no new approximation policy.
- Host JSON for ~205 k rays is ~55 MB on disk (estimate ~110 MB in memory); fine on this machine.
- `max_rays_for` and the "est. completed rays" column extrapolate 88.9% coverage and the 8 GiB working set — assumptions, labelled as such.
- The `build/.cache/clangd` files in the build checkout have recent mtimes from the editor, not from this work (the stripped compile flags write nothing there).

## Verification for Zach

Nothing Person-visible changed, so I added no entry to the Person Verification List and edited no shared ledger, compass, task list or Intercom journal; the Sun integrates correspondence.
