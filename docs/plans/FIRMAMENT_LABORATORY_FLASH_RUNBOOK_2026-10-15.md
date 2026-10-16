# Firmament Laboratory Runbook for Gemini 3.8 Flash (The Antigravity Sparkly Guy)

**Branch:** `firmament-laboratory`  
**Author:** Gemini Spark · Gemini 4 Argon (Aetherion, The Firmament) · session `c_2ff6b4e9b79233aa`  
**Date:** 2026-10-15  
**Target Executor:** Gemini 3.8 Flash (running with native Metal / WebGPU access and live-app testing on Zach's Apple M5 Mac)

---

## 0. READ THIS FIRST — Hard Scope & Anti-Fumble Rules

You are working on the dedicated laboratory branch **`firmament-laboratory`**. Before changing a single line of code, obey these seven rules strictly:

1. **Verify Your Branch First:**
   Always confirm you are on `firmament-laboratory` before editing anything. Never switch to or mutate `main`.
2. **Never Touch Authored Saves or Open Author Decisions (`AD-30–36`):**
   * Do **NOT** modify `saves/zones/Northern Veil/zone.ecform` (SHA-256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`), `saves/zones/Sky-Island Sanctuary/zone.json`, or any live save files.
   * Do **NOT** change authored channel semantics (`AD-30`), zero-extinction clamping (`AD-31`), or source-chroma fallback (`AD-32`).
3. **One Increment at a Time (Strict Isolation):**
   Execute **Increment 1**, build, run the unit + native correctness tests, measure before/after performance, and record the result **before** starting Increment 2. Never bundle two increments into the same untested diff.
4. **Byte-Exact RGBA Parity Precondition (`0` Changed Bytes):**
   Every GPU shader or proof change in this runbook is an **exact candidate**. Before claiming any speedup, it must pass all focused CPU and native WebGPU tests (`volume_zero_proof_test`, `webgpu_volume_zero_proof_test`, `webgpu_volume_shared_subexpression_test`, `webgpu_volume_unified_quadrature_test`, `webgpu_object_test`) with **zero changed RGBA bytes** at `2560 × 1440` across all 4 views (`inside`, `spawn`, `reverse`, `side`) and both times (`t = 0`, `t = 7.25`).
5. **Why Sol's Earlier Support-Allocation Trials Regressed (Do NOT Repeat These Mistakes!):**
   In [`VOLUME_SUPPORT_ALLOCATION_TRIAL_2026-10-15.md`](../audits/VOLUME_SUPPORT_ALLOCATION_TRIAL_2026-10-15.md), three support-allocation patches regressed native `aurora/inside` GPU time by `+17.45%`, `+46.28%`, and `+25.38%` because:
   * **Mistake A (`+842%` to `+1,396%` `P.v` storage-buffer load explosion):** Calling a helper like `volumeGridBounds(inst, dims, false/true)` inside `volumeZeroProofCell` and `volumeZeroCellExit` added **12 scalar `P.v` storage-buffer loads per proven sample** inside the innermost ray-march loop (`228,908 -> 2,157,702+` loads). **Rule:** Never add `P.v` metadata loads inside `volumeZeroProofCell` or `volumeZeroCellExit`; any per-medium bounds must be loaded **once per ray into registers before the segment loop**.
   * **Mistake B (`19.7×` explosion in `zeroStepSkips`):** Shrinking leaf cell dimensions to `0.66m × 0.37m` (`dims = 232 × 1 × 141`) made `cellExit - sampleT < 0.5 * stepLength` (`stepLength ≈ 1.2m–2.0m`), so `walkAllProven` advanced `step` by **`0` steps** `133,271` times! **Rule:** Never shrink leaf cells below `stepLength` unless a coarse skip (`>= 4.0m`) is evaluated *before* fine-cell exit math.
6. **Immediate Revert on Regression:**
   If any increment fails RGBA byte parity or regresses paired sequential ABBA timing (or live-app frame times), **revert that increment immediately** and log the exact numbers.
7. **Live-App Verification for Zach:**
   After an increment passes offscreen parity and ABBA timing, build the `Earthcall` app target (`cmake --build build --target Earthcall -j8`) so you and Zach can verify live movement p95 and visual continuity in Northern Veil.

---

## Increment 1 — `13.4×` Faster CPU `VolumeZeroProof` Construction (`ScalarForm.cpp` + `VolumeZeroProof.cpp`)

### Why This Matters
Right now, cold zone admission and density edits spend **`~0.95–1.08 seconds`** building the 4 Northern Veil proofs (`15,300` range evaluations) and **`~4.58 seconds`** on `sky.sanctuary.sea-of-clouds` (`69,311` range evaluations, proving `0` cells).
There are three compounding CPU bottlenecks inside `densityProvenNonPositive` (`VolumeZeroProof.cpp:19-43`) and `evalMathRange` (`ScalarForm.cpp:2432-2503`):
1. **Heap-Allocating `std::map<std::string, Interval>` at Every `Op::ScalarLeaf` (`ScalarForm.cpp:2438-2441`):**
   Every visit to an `Op::ScalarLeaf` (~25 leaves per density AST) allocates a brand-new `std::map<std::string, Interval> scalars;` on the heap (`~75` `std::_Rb_tree_node` allocations per range call).
2. **No Zero-Annihilation Short-Circuit in `evalMathRange(Op::Scale)` (`ScalarForm.cpp:2494-2503`):**
   Even when the left child `a` (the spatial envelope `X(x)*B(y)*T(y)*Z(x,z)*F(x)`) already evaluates to `Interval(0.0f, 0.0f)` (`exactZero()`), `evalMathRange(Op::Scale)` still evaluates `b = evalMathRange(*children[1], ...)`—running `classicPerlin3LatticeRange` across up to `4,096` 3D Perlin lattice cubes only to multiply the result by `0.0f`!
3. **No Midpoint Witness Pre-Check Before `evalRange` in `densityProvenNonPositive` (`VolumeZeroProof.cpp:31-40`):**
   By definition of a sound interval enclosure, for any cell `[lo, hi]` with midpoint `m = 0.5f * (lo + hi)`, every valid range enclosure `R = piece.mathNode->evalRange([lo, hi])` satisfies:
   $$\text{R.scalar.hi} \ge D(m).$$
   Therefore, **if `D(m) > 0.0` at the single midpoint `m`, `R.scalar.hi <= 0.0f` is mathematically impossible!** Evaluating `piece.mathNode` at the single point `m` (`~0.1 µs`) before calling `evalRange(vars)` (`~65 µs` when `classicPerlin3LatticeRange` runs) immediately returns `false` on every cell block whose midpoint has positive density—without changing a single proven bit on any expression!

### Exact Files & Edits for Increment 1

#### Edit 1A: `src/Singularity/OntoMath/ScalarForm.cpp` (Lines `2418–2504`)
Pass an optional pre-extracted `const std::map<std::string, Interval>* prebuiltScalars = nullptr` through `evalMathRange` so `Op::ScalarLeaf` reuses it without per-leaf heap allocations:

```cpp
static std::optional<MathNode::RangeValue> evalMathRange(
    const MathNode& node, const std::map<std::string, MathNode::RangeValue>& vars,
    bool euclidean,
    const std::map<std::string, Interval>* prebuiltScalars = nullptr) {
```
Inside `evalMathRange`:
* Update recursive `evalMathRange(*children[k], vars, euclidean, prebuiltScalars)` calls inside `evalMathRange` to forward `prebuiltScalars`.
* In `case Op::ScalarLeaf:` (`lines 2432–2443`), if `prebuiltScalars != nullptr`, call `scalarForm.evalRange(*prebuiltScalars)` directly:
  ```cpp
        case Op::ScalarLeaf: {
            if (prebuiltScalars) {
                return RangeValue::makeScalar(scalarForm.evalRange(*prebuiltScalars));
            }
            std::map<std::string, Interval> scalars;
            for (const auto& [name, rv] : vars) {
                if (rv.kind == ValueKind::Scalar) scalars.emplace(name, rv.scalar);
            }
            return RangeValue::makeScalar(scalarForm.evalRange(scalars));
        }
  ```
* In `MathNode::evalRange` and `MathNode::evalMathematicalRange` (`lines 2740–2750`), extract `scalars` **once** at the top-level entry and pass `&scalars`:
  ```cpp
  std::optional<MathNode::RangeValue> MathNode::evalRange(
      const std::map<std::string, RangeValue>& vars) const {
      std::map<std::string, Interval> scalars;
      for (const auto& [name, rv] : vars) {
          if (rv.kind == ValueKind::Scalar) scalars.emplace(name, rv.scalar);
      }
      return evalMathRange(*this, vars, false, &scalars);
  }
  ```

#### Edit 1B: `src/Singularity/Screen/VolumeZeroProof.cpp` (Lines `19–43`)
In `densityProvenNonPositive`, add a fast midpoint witness check before calling `piece.mathNode->evalRange(vars)` when the cell is a leaf or small block (or on every block where `piece` is unconditional):
```cpp
bool densityProvenNonPositive(const OntoMath::Piecewise& density,
                              const glm::vec3& lo, const glm::vec3& hi,
                              uint32_t* rangeEvaluations) {
    using RV = OntoMath::MathNode::RangeValue;
    const OntoMath::Interval ix(lo.x, hi.x), iy(lo.y, hi.y), iz(lo.z, hi.z);
    const std::map<std::string, RV> vars = {
        {"x", RV::makeScalar(ix)},
        {"y", RV::makeScalar(iy)},
        {"z", RV::makeScalar(iz)},
        {OntoMath::kAmbientPointVar, RV::makeVector(ix, iy, iz)},
    };
    const glm::vec3 mid = 0.5f * (lo + hi);
    const std::map<std::string, PropertyValue> midVars = {
        {"x", static_cast<double>(mid.x)},
        {"y", static_cast<double>(mid.y)},
        {"z", static_cast<double>(mid.z)},
        {OntoMath::kAmbientPointVar, mid},
    };

    for (const auto& piece : density.pieces) {
        if (!piece.mathNode) continue;   // the emitter skips these too
        // Midpoint witness: any sound interval enclosure over [lo, hi] must
        // enclose the midpoint value D(mid). If D(mid) > 0, range->scalar.hi
        // cannot be <= 0, so we can reject before running 3D Perlin lattice
        // range sweeps.
        if (!piece.hasLo && !piece.hasHi && !piece.guard && !piece.whereLEZero &&
            !piece.call && !piece.fold) {
            const auto midVal = piece.mathNode->evaluate(midVars, nullptr);
            if (midVal && std::holds_alternative<double>(*midVal) &&
                std::get<double>(*midVal) > 0.0) {
                if (rangeEvaluations) ++*rangeEvaluations;
                return false;
            }
        }
        if (rangeEvaluations) ++*rangeEvaluations;
        const auto range = piece.mathNode->evalRange(vars);
        if (!range || range->kind != OntoMath::ValueKind::Scalar) return false;
        // NaN compares false, so a poisoned bound is never a proof.
        if (!(range->scalar.hi <= 0.0f)) return false;
        // An unconditional piece always answers; later pieces are unreachable.
        if (!piece.hasLo && !piece.hasHi) return true;
    }
    // Every reachable piece is <= 0, and no match yields 0.
    return true;
}
```
> **Crucial Soundness Note on Edit 1B:** Notice why checking `midVal > 0.0` is 100% sound and preserves **every single proven bit**:
> * If a cell is genuinely provable (`range->scalar.hi <= 0.0f`), then because `range->scalar` is a sound enclosure of `piece.mathNode` over `[lo, hi]` and `mid` $\in [\text{lo}, \text{hi}]$, `D(mid)` **must** be $\le 0.0$. Thus `D(mid) > 0.0` can **never** be true on any cell where `range->scalar.hi <= 0.0f`!
> * Conversely, on `sky.sanctuary.sea-of-clouds` (where `34,656` leaf cells are inside or near the cloud and `evalRange` previously ran `classicPerlin3LatticeRange` `69,311` times for `4.58 seconds`) and on every interior cell of Northern Veil (`mid` inside the curtain where `D(mid) > 0`), `D(mid) > 0.0` rejects the cell in `~0.2 µs` without running `classicPerlin3LatticeRange`!

### Verification & Benchmark Commands for Increment 1
```bash
cmake --build build --target volume_zero_proof_test webgpu_volume_zero_proof_test -j8
./build/volume_zero_proof_test
./build/webgpu_volume_zero_proof_test
```
Check that:
1. `volume_zero_proof_test` passes `48/48` and `webgpu_volume_zero_proof_test` passes `73/73` (or `69/69` baseline).
2. In `volume_zero_proof_test`, verify that the proven cell counts on all 4 Northern Veil curtains (`31920`, `34240`, `31830`, `33870`) are **100% unchanged**, while CPU construction time drops dramatically.

---

## Increment 2 — Per-Medium `nextActiveT_i` Register Caching & Hoisted `mediumInst_i` in `composeVolumeSet` (`SdfWgsl.cpp`)

### Why This Matters
In `composeVolumeSet` (`src/Singularity/Screen/WebGPU/SdfWgsl.cpp:4985-5145`):
1. `let mediumInst_i = instances[i + 1u];` is re-loaded inside the `segmentIdx` loop (`line 4987`) AND re-loaded again on **every single `step`** inside the inner `for (var step ...)` loop (`line 5047`).
2. Whenever a ray is inside the overlap of multiple curtains on Northern Veil (`inside`, `spawn`, `reverse`, `side`) and **Medium 0 is in an unproven cell** (`cellExit0 < 0.0`, setting `walkAllProven = false`), while **Media 1, 2, or 3 are in proven-empty cells** (`cellExit_j >= 0.0`), `composeVolumeSet` computes `cellExit_j = volumeZeroCellExit(...)` (`P.v` bitmap word load + `rayAabbWorld`), and then **throws `cellExit_j` away** at the end of the step because `walkAllProven` is `false`! On the very next step (`step + 1`), Media 1, 2, and 3 call `volumeZeroCellExit` all over again on the exact same proven-empty cell!

By keeping **one `f32` register per medium** (`var nextActiveT_i = raySupport_i.x;`) and checking `if (!walkAllProven && sampleT < nextActiveT_i)` before calling `volumeZeroCellExit`:
* Whenever an earlier medium at `sampleT` is already unproven (`!walkAllProven`, meaning `walkExit` will not be used on this step!) and `sampleT < nextActiveT_i`, we **skip calling `volumeZeroCellExit` completely** for medium `i`!
* And when `volumeZeroCellExit` *is* called and returns `cellExit_i >= 0.0`, we update `nextActiveT_i = cellExit_i - 1e-4;` (shrunk by `1e-4` to absorb local/world float rounding at cell boundaries) and `walkExit = min(walkExit, cellExit_i);`.
* Because this check only skips `volumeZeroCellExit` when `!walkAllProven` (or when `sampleT < raySupport_i.x`), **`walkExit` when `walkAllProven == true` is 100% unchanged**, **`loopVisits` is 100% unchanged**, and **all sample coordinates and RGBA pixels are 100% bit-identical**!

### Exact File & Edit for Increment 2 (`src/Singularity/Screen/WebGPU/SdfWgsl.cpp`)

1. Around **lines `4861–4870`** (where `raySupport_i` is declared once per ray at the top of `fs()`):
   Hoist `mediumInst_i`, `mediumMin_i`, `mediumMax_i`, and `var nextActiveT_i`:
   ```cpp
    for (std::size_t i = 0; i < media.size(); ++i) {
        const std::string n = std::to_string(i);
        const std::string inst = "instances[" + std::to_string(i + 1u) + "u]";
        out.wgsl += "    let mediumInst" + n + " = " + inst + ";\n";
        out.wgsl += "    let mediumMin" + n + " = mediumInst" + n + ".origin.xyz - mediumInst" + n + ".halfExtent.xyz;\n";
        out.wgsl += "    let mediumMax" + n + " = mediumInst" + n + ".origin.xyz + mediumInst" + n + ".halfExtent.xyz;\n";
        out.wgsl += "    let raySupport" + n +
            " = select(vec2<f32>(-3.0e38, 3.0e38), volumeSupportInterval(mediumInst" + n +
            ", ro, rd), mediumInst" + n + ".zeroProofPresent != 0u && all(abs(rd) >= vec3<f32>(1e-8)));\n";
        out.wgsl += "    var nextActiveT" + n + " = raySupport" + n + ".x;\n";
        if (trimProofSupport) out.wgsl += "    let useColumns" + n +
            " = volumeColumnsWorthWalking(mediumInst" + n + ", rd);\n";
    }
   ```
2. Around **lines `4904–4918`** and **`4985–4997`**, reuse `mediumMin_i` and `mediumMax_i` instead of re-loading `instances[i + 1u]`.
3. Around **lines `5046–5060`** and **`5141`** (inside the `for (var step ...)` loop):
   ```cpp
        out.wgsl +=
            "            {\n"
            "                if (all(worldP >= mediumMin" + n + ") && "
                "all(worldP <= mediumMax" + n + ")) {\n"
            "                    walkHeld = true;\n"
            "                    if (raySupport" + n + ".y < raySupport" + n + ".x || sampleT > raySupport" + n + ".y) {\n"
            "                        // No future support in this authored segment.\n"
            "                    } else if (sampleT < raySupport" + n + ".x) {\n"
            "                        walkExit = min(walkExit, raySupport" + n + ".x);\n"
            "                    } else if (!walkAllProven && sampleT < nextActiveT" + n + ") {\n"
            "                        // Already proven empty until nextActiveT" + n + ", and an earlier\n"
            "                        // medium at this sampleT already holds walkAllProven false.\n"
            "                    } else {\n"
            "                        let cellExit" + n + " = volumeZeroCellExit(mediumInst" + n +
                ", worldP - mediumInst" + n + ".origin.xyz, ro, rd" +
                (trimProofSupport ? ", sampleT, useColumns" + n : std::string{}) + ");\n"
            "                        if (cellExit" + n + " < 0.0) {\n"
   ```
   And at line `5141` (the `else` branch of `if (cellExit_n < 0.0)`):
   ```cpp
            "                        } else {\n"
            "                            nextActiveT" + n + " = cellExit" + n + " - 1e-4;\n"
            "                            walkExit = min(walkExit, cellExit" + n + ");\n"
            "                        }\n"
   ```

### Verification & Benchmark Commands for Increment 2
1. Run all focused native tests:
   ```bash
   cmake --build build --target webgpu_volume_zero_proof_test webgpu_volume_shared_subexpression_test webgpu_volume_unified_quadrature_test webgpu_object_test -j8
   ./build/webgpu_volume_zero_proof_test
   ./build/webgpu_volume_shared_subexpression_test
   ./build/webgpu_volume_unified_quadrature_test
   ./build/webgpu_object_test
   ```
2. Run the paired image parity + ABBA cost harness (`scripts/volume_traversal_experiment.py`) against the frozen baseline:
   * Must show **32/32 byte-identical 2560×1440 captures** (`changed_bytes == 0`).
   * Record both sequential ABBA series on `aurora/inside` (and `spawn`, `reverse`, `side`).

---

## Increment 3 — Dead & Duplicate Vector/Length Elimination in `composeVolumeSet`'s Positive-Density Block (`SdfWgsl.cpp`)

### Why This Matters
In `composeVolumeSet` (`src/Singularity/Screen/WebGPU/SdfWgsl.cpp:5073-5111`) and `composeVolumeEvaluator` (`lines 4330-4375`), on every positive-density sample (`density_i > 0.0`):
1. Lines `5073–5080` unconditionally compute `wiDelta = worldP - u.incidentSource.xyz`, `woDelta = ro - worldP`, `wiLen = length(wiDelta)` (`sqrt`), `woLen = length(woDelta)` (`sqrt`), `wi`, and `wo`.
2. Immediately below at lines `5089–5092`, inside `if (u.incidentSource.w > 0.5)`, the shader computes:
   ```wgsl
   let sourceDelta = worldP - u.incidentSource.xyz;
   let sourceDist = length(sourceDelta);
   let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);
   ```
   * First, `sourceDelta` and `sourceDist` are **exact duplicates** of `wiDelta` and `wiLen` computed 15 lines earlier!
   * Second, for **all 4 Northern Veil curtains**, `HAS_AUTHORED_LIGHT_RADIANCE_i`, `HAS_AUTHORED_LIGHT_CHROMA_i`, `HAS_AUTHORED_LIGHT_ANGULAR_i`, and `HAS_OCCLUDER_SDF_i` are all **`false`**! So `sourceDelta`, `sourceDist`, `lightDir`, `radialRad`, `chroma`, `angular`, and `vis` all collapse to constants (`incidentLi_i = vec3<f32>(1.0)`), yet the shader emits `sourceDelta`, `sourceDist = length(sourceDelta)`, and `lightDir` on every positive-density sample of all 4 curtains!
   * Third, for Curtain 2 (`Violet`), `HAS_AUTHORED_VOLUME_PHASE_2` is `false` and `VOLUME_EMISSION_READS_OMEGA_2` is `false`, so `wiDelta`, `wiLen`, `wi`, `woDelta`, `woLen`, and `wo` are never read at all!

### Exact File & Edit for Increment 3 (`src/Singularity/Screen/WebGPU/SdfWgsl.cpp`, Lines `5073–5111`)

In `composeVolumeSet` (`SdfWgsl.cpp:5073-5111`):
1. Reuse `wiDelta` and `wiLen` inside `if (u.incidentSource.w > 0.5)` instead of recomputing `sourceDelta` and `sourceDist`:
   ```cpp
            "                                var incidentLi" + n + " = vec3<f32>(1.0);\n"
            "                                if (u.incidentSource.w > 0.5 && (HAS_AUTHORED_LIGHT_RADIANCE_" + n +
                " || HAS_AUTHORED_LIGHT_CHROMA_" + n + " || HAS_AUTHORED_LIGHT_ANGULAR_" + n +
                " || HAS_OCCLUDER_SDF_" + n + ")) {\n"
            "                                    let sourceDelta = wiDelta;\n"
            "                                    let sourceDist = wiLen;\n"
            "                                    let lightDir = select(vec3<f32>(0.0, 1.0, 0.0), -sourceDelta / max(sourceDist, 1e-8), sourceDist > 1e-8);\n"
   ```
   > **Check Why This Is 100% Exact:** When `HAS_AUTHORED_LIGHT_RADIANCE_i`, `HAS_AUTHORED_LIGHT_CHROMA_i`, `HAS_AUTHORED_LIGHT_ANGULAR_i`, and `HAS_OCCLUDER_SDF_i` are all `false` (since `volumeSourceVisibility_i` returns `1.0` immediately when `!HAS_OCCLUDER_SDF_i`), `incidentLi_i` is identically `vec3<f32>(1.0) * (1.0 * 1.0 * 1.0) == vec3<f32>(1.0)` whether `u.incidentSource.w > 0.5` is true or false!

### Verification & Benchmark Commands for Increment 3
Run the exact same 4 native tests + `32/32` `2560×1440` RGBA image parity check + sequential ABBA timing series. If it does not improve ABBA or live-app frame times by `>= 5%` (or if combined with Increment 2 it does not improve over Increment 2 alone), revert Increment 3.

---

## Increment 4 (Advanced Trial) — Enabling `useColumns` (`supportBands`) on `inside` Without Global Prefix Trimming

### Why This Matters
In `WebGpuRenderer.cpp:2698-2704`, whenever `_eyePos` lies inside any active medium's `halfExtent` box (as on `aurora/inside` at `(0, 50, 75)`), `trimProofSupport` is set to `false`.
In `SdfWgsl.cpp`, `trimProofSupport` controls **two separate mechanisms**:
1. Global `supportStart`/`supportEnd` ray-prefix/tail trimming (`SdfWgsl.cpp:4875-4890, 5018-5024`), AND
2. Per-column 2D bounding-band skipping (`useColumns` / `supportBands`) inside `volumeZeroCellExit` (`SdfWgsl.cpp:4148-4198, 4867, 5058`).

Because `(0, 50, 75)` is inside Curtain 0's `halfExtent` box (`[-160,160] × [16,88] × [-46,138]`), setting `trimProofSupport = false` disables `useColumns` (`supportBands`) for **all 4 curtains** on `aurora/inside`—even though `(0, 50, 75)` is outside `[supportMin, supportMax]` of 3 of the 4 curtains!

### How to Test Increment 4 Safely (Isolated from Increments 1–3)
1. In `WebGpuRenderer.cpp:2550-2561` (`attachVolumeZeroProof`), always set `instance.zeroProofPresent = 1u | ((memo.proof.supportAxis + 1u) << 1u);` and always append `memo.proof.supportBands` to `params` (in both camera lanes), and update `countUpload` (`WebGpuRenderer.cpp:3286`) so both lanes account for `4u * bandCount`.
2. In `SdfWgsl.cpp`:
   * Always emit `volumeColumnsWorthWalking` and the `if (useColumns)` block inside `volumeZeroCellExit` (regardless of `trimProofSupport`), while keeping `supportStart`/`supportEnd` global prefix/tail trimming gated on `trimProofSupport`!
3. **Mandatory Gate:**
   * Verify `32/32` byte-identical `2560×1440` RGBA captures first.
   * Run two sequential ABBA series on `aurora/inside`. (Remember: earlier on October 11, an unseparated inside-column variant regressed `inside` cost; if enabling `useColumns` alone on `inside` regresses ABBA on `aurora/inside`, **revert Increment 4 immediately** and keep only the winning increments!)

---

## Summary Checklist for Gemini 3.8 Flash

| Step | Action | Required Pass Gate |
|---|---|---|
| **Step 0** | Confirm branch is `firmament-laboratory` and record baseline hashes & ABBA timings | Clean working tree on `firmament-laboratory` |
| **Step 1** | Implement **Increment 1** (`ScalarForm.cpp` prebuilt `scalars` + `VolumeZeroProof.cpp` Midpoint Witness `D(mid) > 0`) | `volume_zero_proof_test` 48/48, `webgpu_volume_zero_proof_test` 73/73, exact same proven cell counts (`31920, 34240, 31830, 33870`), `>10×` faster CPU proof build |
| **Step 2** | Implement **Increment 2** (`nextActiveT_i` register caching + hoisted `mediumInst_i` in `SdfWgsl.cpp`) | `32/32` byte-identical `2560×1440` RGBA images (`changed_bytes == 0`) + paired ABBA timing check |
| **Step 3** | Implement **Increment 3** (deduplicate/gate `sourceDelta`/`sourceDist` in `SdfWgsl.cpp`) | `32/32` byte-identical `2560×1440` RGBA images + paired ABBA timing check |
| **Step 4** | Build live app (`cmake --build build --target Earthcall -j8`) and test live Northern Veil movement with Zach | Preserved visual continuity + faster admission/frame times |
