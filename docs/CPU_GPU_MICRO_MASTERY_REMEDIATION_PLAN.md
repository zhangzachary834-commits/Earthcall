# CPU-GPU Micro-Mastery — Remediation & Next-Rung Implementation Plan

**Date**: 2026-08-25
**Author**: Claude Opus 5, from a review of `ce5c1cbe..8bd89909`
**Status**: Plan only. Nothing in this document has been implemented.
**Reads**: [`docs/architecture/ontology/CPU_GPU_MICRO_MASTERY.md`](architecture/ontology/CPU_GPU_MICRO_MASTERY.md),
[`docs/architecture/Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md`](architecture/Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md),
`NO_BLACK_BOX.md` §3

---

## 0. What this plan is responding to

Zach built the micro-mastery substrate across `9a9e9d85`, `a291cb10`, and `8bd89909`
("Be like water" — the allocator that stops negotiating with the driver per object). The
substrate is right and the heavy test is the first real performance probe this repo has
ever had. This plan does not relitigate any of that.

What it addresses is the gap between the telos stated in `CPU_GPU_MICRO_MASTERY.md` §1 —
**10,000 discrete `Object`s at 60+ FPS**, so that Interaction-as-Law is practically viable —
and what the probe currently measures:

```
Finished 60 frames in 10520.10 ms. Average frame time: 175.33 ms
  drawCalls: 19500        (= 3000 cubes x 6 faces + 1500 fields, exactly)
  vramAllocatedBytes: 5.75 MB
  bufferSuballocations: 21000
```

4,500 objects at **5.7 FPS**, against a test budget of 300 ms (3.3 FPS). The allocator is
doing its job — 5.75 MB of VRAM for 21,000 suballocations a frame is the evidence. The cost
that remains is **draw call count**, and no allocator change reaches 60 FPS from here.

The work is sequenced so that each phase is independently landable, and so the phase that
makes the measurement trustworthy comes before any phase that will be judged by it.

---

## Phase 0 — Make the number trustworthy

**Nothing later in this plan means anything until this lands.** Every subsequent phase is
judged by a frame time, and that frame time is currently measured while three other tests
fight the probe for the machine.

### 0.1 Give `webgpu_micro_mastery_lag_test` the box to itself

`CMakeLists.txt:312` grants `RUN_SERIAL`, `TIMEOUT 300`, and `LABELS "lag"` to
`frame_lag_test` alone, with a comment that states the principle exactly: *"The lag probe
measures durations, so it must not be measured while three other tests are fighting it for
the machine… a run where nothing is enforced is a run that checked nothing."*

`webgpu_micro_mastery_lag_test` is also a duration probe, is the longest test in the suite,
and gets none of it. Evidence: standalone the timed loop runs in **10.5 s**; inside
`ctest -j4` the same test takes **22.35 s** wall. It is timing itself under contention with a
300 ms budget and thin margin.

**Change** — widen the condition at `CMakeLists.txt:312`:

```cmake
if (${TEST_NAME} STREQUAL "frame_lag_test" OR
    ${TEST_NAME} STREQUAL "webgpu_micro_mastery_lag_test")
```

Keep `TIMEOUT 300` (the micro-mastery probe needs it more — it currently runs 22 s).

**Verify**
- `ctest --test-dir build -N -L lag` lists both tests.
- `ctest --test-dir build -j4` — the micro-mastery probe's wall time drops toward its
  standalone 10.5 s. Record the before and after numbers in the commit message.

### 0.2 Adopt `frame_lag_test`'s contention guard

That same comment notes `frame_lag_test` *"detects a moving machine and stops enforcing its
timings."* Read how it does that (`tests/singularity/frame_lag_test.cpp`) and apply the same
guard to the micro-mastery probe, so a loaded CI box reports rather than fails.

### 0.3 Record a baseline

`frame_lag_test` has `tests/singularity/frame_lag_baseline.txt`. Give the micro-mastery probe
the same: commit the measured average frame time, draw calls, suballocations, and VRAM at
HEAD, so Phases 1 and 4 can be shown to have moved it rather than asserted to have.

**Exit test for Phase 0**: two consecutive `ctest -j4` runs report the micro-mastery probe's
average frame time within 10% of each other. If they don't, the number is still not a
measurement and Phase 4 cannot be evaluated.

---

## Phase 1 — The cheap win, to prove the harness works

### 1.1 Cache the bound pipeline

Four sites in `WebGpuRenderer.cpp` set a pipeline unconditionally and count it as a switch:

| Line | Call |
|---|---|
| 545–546 | `SetPipeline(_pass, _meshPipeline); pipelineSwitches++;` |
| 784–785 | `SetPipeline(_pass, sp->pipe); pipelineSwitches++;` |
| 827–828 | `SetPipeline(_pass, pipe); pipelineSwitches++;` |
| 1012–1013 | `SetPipeline(_pass, _imagePipe); pipelineSwitches++;` |

In the heavy test the mesh pipeline is re-bound ~19,500 times per frame while the number of
actual distinct pipeline changes is about two. This is both ~19,500 redundant driver calls
and a telemetry lie: `@screen-channel.pipelineSwitches` reports the count of *bind attempts*,
not switches.

**Change** — add to `WebGpuRenderer`:

```cpp
// Last pipeline bound on the CURRENT pass. Kernel state: a driver-object
// handle, not governable — reset whenever a pass begins.
WGPURenderPipeline _boundPipeline = nullptr;

void bindPipeline(WGPURenderPipeline p) {
    if (p == _boundPipeline) return;
    wgpuRenderPassEncoderSetPipeline(_pass, p);
    _boundPipeline = p;
    mutableFrameStats().pipelineSwitches++;
}
```

Replace all four call sites with `bindPipeline(...)`.

**Reset it in `beginFrameOffscreen`** (`WebGpuRenderer.cpp:496`), next to
`mutableFrameStats() = FrameStats{};` and immediately before `_pass` is created — the comment
already sitting at line ~519 (*"Each verb sets its own pipeline… nothing is pre-bound here"*)
is exactly the invariant this cache depends on, so update it to say the cache starts empty.

**Trap**: a new pass invalidates the binding. If any other code path creates a render pass,
it must clear `_boundPipeline` too. Grep `wgpuCommandEncoderBeginRenderPass` and cover every
site; a missed one silently skips a required bind and renders with the wrong shader.

**Verify**
- `webgpu_micro_mastery_lag_test` — `pipelineSwitches` in the printed stats falls from
  ~19,500 to single digits. **Add `pipelineSwitches` to the test's stat printout; it is not
  currently printed.**
- `webgpu_sdf_parity_test` and `webgpu_object_test` still pass (they are the pixel-level
  witnesses that nothing renders wrong).
- Record the frame-time delta against the Phase 0.3 baseline.

**Expected**: a real but modest gain. The point of doing it first is that it is small, safe,
and *measurable* — it proves the Phase 0 harness can detect an improvement before Phase 4
stakes a large refactor on that same harness.

---

## Phase 2 — Correctness bugs

### 2.1 `GpuMeshCache` serves stale geometry on in-place vertex edits

`GpuMeshCache.cpp:29` gates the cache hit on:

```cpp
if (it->second.vertexCount == mesh.tris.size() && it->second.meshId == mesh.id)
```

Deform a mesh — move vertices, keep the triangle count — and neither value changes, so the
stale VRAM buffer keeps drawing. `GpuMeshCache.hpp:17` claims *"Re-uploading occurs only when
geometry is marked dirty"*; **there is no dirty flag in the class.** This is the same shape as
the FaceTexture staleness already hit once with the Pottery tool.

**Change** — add a mutation counter to `geom::TessMesh`
(`ConstructedBeing/Singular/Object/Geometry/SmoothSurface.hpp:30`), beside the existing `id`:

```cpp
struct TessMesh {
    std::vector<TessVertex> tris;
    uint64_t id = nextTessMeshId();
    uint64_t revision = 0;      // bumped by every writer that mutates `tris`
};
```

Cache `revision` in `CachedMesh` and add it to the hit condition. Then find every writer that
mutates an existing `TessMesh::tris` in place and bump `revision`.

**⚑ DECISION for the implementer**: a `revision` field is only as good as the discipline of
its writers, and this repo has been bitten by exactly that before. The alternative is a cheap
content hash of `tris` computed in `getOrUpload` — always correct, never forgotten, costs an
O(n) walk per mesh per frame. Given that the whole point of this subsystem is to avoid
per-frame per-object CPU work, `revision` is the right default **provided** every writer is
found. Enumerate them explicitly in the commit message. If the enumeration is not confidently
complete, use the hash and take the cost; a correct slow frame beats a fast wrong one.

**Verify** — new case in `gpu_mastery_test` (device-backed, see 3.1): upload a mesh, mutate a
vertex position without changing the triangle count, re-upload, assert the returned buffer's
contents changed. Confirm the test **fails before** the fix.

### 2.2 `GpuBufferPool::init()` leaks chunks from a previous device

`GpuBufferPool.cpp:7` zeroes `_totalVramBytes` and the chunk indices but never clears
`_uniformChunks` / `_vertexChunks` / `_storageChunks`. A second `init()` — device loss,
resize, backend switch — leaves live `WGPUBuffer`s belonging to a dead device, under-reports
VRAM, and then writes into them.

**Change** — call `shutdown()` at the top of `init()`, or assert the vectors are empty. The
former is safer; the latter is louder. Prefer `shutdown()` with a one-line comment saying why.

**Verify** — `gpu_mastery_test`: `init(); suballocate; init();` then assert
`totalVramBytes() == 0` and the chunk vectors are empty. Currently this would report a stale
non-zero and hold dead buffers.

---

## Phase 3 — Guard what the audit already claimed

`CPU_GPU_MICRO_MASTERY.md` §2 and §3 each describe a fixed bug. **Neither has a regression
test.** `gpu_mastery_test` §3 exercises only the null-device and empty-mesh paths, because
the real paths need a GPU device — and that test does not create one.

### 3.1 Give `gpu_mastery_test` a device

`webgpu_micro_mastery_lag_test.cpp:25` shows the pattern (`wgpu::Device gpu; gpu.init()`), and
`CMakeLists.txt:257` already links `gpu_mastery_test` against the WebGPU sources for exactly
this reason. Add a device-backed section, skipping gracefully (`return 0` with a printed
notice) when no device is available, so headless CI does not go red.

### 3.2 Cover the two claimed fixes

- **Chunk saturation (§2)**: init a pool with a small `uniformChunkSize`, suballocate past one
  chunk for several frames with `resetFrame()` between, assert `totalVramBytes()` **stops
  growing** after the working set is reached. This is the actual leak that was fixed —
  unbounded chunk growth across frames.
- **Reincarnation + GC (§3)**: upload mesh A, destroy it, construct mesh B (verify it lands at
  the same address or force the case), assert `getOrUpload(B)` returns a different buffer.
  Then advance `beginFrame` past `lastUsedFrame + 10` without drawing and assert
  `cachedMeshCount()` drops and `totalCachedBytes()` returns to zero.

### 3.3 Correct §2 of the doc

The doc says the linear search ensures *"existing memory is saturated before requesting new
driver allocations."* It does not: `suballocateFrom` searches from `currentIdx + 1`
(`GpuBufferPool.cpp:83`) and never revisits earlier chunks, so intra-frame leftovers are
skipped exactly as before. What it actually fixes is **chunk growth across frames**, which was
the real leak and is the more valuable claim. Rewrite the sentence to say that. Do not "fix"
the code to match the doc — full saturation would mean a free-list, which is not warranted at
5.75 MB.

---

## Phase 4 — The actual next rung: collapse the draw calls

This is the phase that addresses §1's telos. Do not start it before Phase 0 lands.

### 4.1 The measurement

`19500 = 3000 x 6 + 1500`, exactly. `ObjectRender.cpp:170`:

```cpp
void Object::drawCube() const {
    for (int f = 0; f < 6; ++f)
        currentRenderer().drawMesh(cubeFace(f), resolveRenderMaterial(_materialId, faceAlbedo(f)));
}
```

**Six draw calls per cube**, one per face, so each face can bind its own texture. That is the
FaceTexture design showing up as a 6x multiplier on the hottest loop in the engine. At 60 FPS
the budget is ~16 ms; 19,500 calls in 16 ms is ~0.8 µs/call, at or past what any driver does
on the CPU timeline.

### 4.2 Rung one — merge faces that share a material

An unpainted cube resolves the same material for all six faces. When
`resolveRenderMaterial` returns the same material and texture for every face, issue **one**
draw of the merged cube mesh instead of six.

This is a strict 6x reduction on the common case and requires no new GPU concepts. It is
almost certainly the largest single win available and should be measured before 4.3 is
designed.

**Trap**: the divergence rule in `AGENTS.md` — paint is on the Material and materials are
shared; `Object::setFaceColor` / `ownMaterial` diverge an object onto its own
`material.<identifier>` on the first stroke. The merge must therefore be decided **per draw,
from the resolved materials**, never cached on the Object — a single brush stroke must
silently drop that object back to the six-draw path. Guard this with a test that paints one
face and asserts the object's draw count goes 1 → 6.

### 4.3 Rung two — instance across objects

One draw for N objects sharing a pipeline, with per-object transforms and material params
indexed out of a storage buffer. `_storageChunks` and `suballocateStorage`
(`GpuBufferPool.cpp:127`) are already exactly the right substrate — the micro-mastery work
built it and stopped before the payoff.

Shape: group the frame's draws by (pipeline, texture), write one `MeshUniforms`-equivalent
array per group into a storage suballocation, and issue one instanced draw per group with the
instance index selecting the row.

**⚑ AUTHOR — Zach's call, not the implementer's.** Batching means the renderer decides how to
group beings that Law authored individually. That is a subsystem making a decision about
beings, which is the shape the six refusals exist to catch. My reading is that it stays clean:
grouping is *how the machine acts*, not *what a thing is*, and nothing about a being changes
based on which batch it landed in — the same argument that lets the collision dispatcher sort
objects. But it should be said out loud in `CPU_GPU_MICRO_MASTERY.md` before the code lands,
not discovered in review afterward.

**Verify** — the heavy probe at 10,000 objects, not 4,500. Raise the population to the number
§1 actually names, and set the budget where the telos is (16 ms), marking it a known-failing
target until it is met rather than trimming the budget to fit the code.

---

## Phase 5 — Honesty of the channel

### 5.1 Derived telemetry must be read-only

`ScreenChannel.cpp:55-62` registers all seven metrics via `PropertyRef`, so a Law can write
`@screen-channel.drawCalls = 9999` and have it silently clobbered by the next
`updateMetrics`. `NO_BLACK_BOX.md` §3 is *readable by law, writable **unless genuinely
derived***; these are the definition of derived. `wireframe` is the one that should stay
writable, and it correctly is — verified end to end (`EngineRender.cpp:81` →
`setWireframe` → `_wireframe` → `WebGpuRenderer.cpp:529` → `LineList`).

**Change** — register the seven metrics as `ComputedProperty` with a getter and a **null
setter**, which `ComputedProperty.hpp:18` already defines as read-only. The unused
`#include "ComputedProperty.hpp"` already at the top of `ScreenChannel.cpp` suggests this was
considered; finish it.

**No test changes needed**: `no_black_box_test.cpp:257` already treats a refused write as a
valid answer (*"read-only, or type-refused"*), and `:153` states the principle —
*"read-only is a real answer, not a hidden field."*

### 5.2 Byte counts must be `double`, not `float`

`ScreenChannel.hpp:42-43` types `vramAllocatedBytes` and `uniformBytesWritten` as `float`.
Past 16.7 MB a `float` cannot represent consecutive integers; at 1 GB the granularity is 64
bytes. These are law-addressable paths that law text will compare against thresholds.

`Renderer.hpp:43-44` already types them correctly as `size_t` — the precision is lost purely
at the `static_cast<float>` on `EngineRender.cpp:139-140`. `double` is a `PropertyValue`
alternative (`PropertyValue.hpp:38`), so nothing downstream needs to change.

**Change** — `double` in `ScreenChannel`, `updateMetrics`, and the two casts at
`EngineRender.cpp:139-140`. Update `gpu_mastery_test` `[2]`, which currently asserts through a
`static_cast<float>` round trip.

### 5.3 Say what the allocator is

`CPU_GPU_MICRO_MASTERY.md` §2 calls `GpuBufferPool` a "ring-buffer memory sub-allocator." It
is a per-frame bump allocator reset by `resetFrame()`, with no frame rotation and no fence.
Under WebGPU this is **safe** — `wgpuQueueWriteBuffer` is ordered on the queue timeline
against previously submitted command buffers, so frame N+1's write cannot land before frame
N's draws read. That safety is a property of the API, not of the design, and the same code
ported to Vulkan or Metal directly would corrupt in-flight data.

**Change** — say so in §2, in one sentence. Someone will port this.

---

## Ordering and exit criteria

| Phase | Depends on | Done when |
|---|---|---|
| 0 Trustworthy measurement | — | two `ctest -j4` runs agree within 10%; baseline committed |
| 1 Pipeline cache | 0 | `pipelineSwitches` ~19,500 → single digits; parity tests green; delta recorded |
| 2 Correctness bugs | 3.1 for its tests | both new tests confirmed failing before the fix |
| 3 Guard the claims | — | §2 and §3 of the doc each have a device-backed test |
| 4 Draw-call collapse | 0, 1 | 10,000 objects measured against a 16 ms budget |
| 5 Channel honesty | — | metrics read-only and `double`; three doc sentences corrected |

Phases 2, 3, and 5 are independent of 4 and can go in parallel with it. **Phase 0 blocks 1
and 4 absolutely** — landing a performance change measured by a contended probe is how a
regression gets committed as an improvement.

---

## Two things not in scope, deliberately

- **A free-list or full-saturation allocator.** At 5.75 MB of VRAM for 21,000 suballocations
  a frame, the allocator is not the problem. Phase 3.3 corrects the doc's claim rather than
  chasing the code to match it.
- **Retiring the per-face draw path.** Phase 4.2 merges faces *when their materials resolve
  identically*; it does not remove the ability to paint a single face. The FaceTexture
  ontology is not the bug — its interaction with an unbatched renderer is.

---

## The human thread

The substrate, the heavy probe, and the `@screen-channel` telemetry are Zach's
(`9a9e9d85`, `a291cb10`, `8bd89909`). The telos this plan measures against — 10,000 beings at
60+ FPS so that Interaction-as-Law is viable — is §1 of his own doc, not an external standard
imposed here. The Phase 0 principle is quoted from his `CMakeLists.txt:307` comment on
`frame_lag_test`, applied to the probe that did not get it. The diagnosis that draw call count
rather than allocation is now the binding constraint, the `19500 = 3000 x 6 + 1500`
decomposition, the phase ordering, and the ⚑ decision points are mine.

Zach's open question from `93a36719` — whether the 2D interaction scaffolding should be
strictly Singularity or first-movers replaceable by Law — bears directly on Phase 4 and is
tracked separately in the to-do list. If that scaffolding stays C++-only, Phase 4 is
optimizing a path the authored UI will not travel.
