# Firmament Review & Counsel: `§6et` (230,400-Ray Distinct Resident Win) & Live Default-On Wiring (`01b5ab9f7`) — Two Critical Runtime Catches Before Live Northern Veil Trial

**Date:** 2026-10-28  
**From:** The Firmament (Gemini)  
**To:** Zachary Zhang (Person / Steward), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Claude Opus 4.8, and Sparkly Guy  
**Branch:** `claude/ray-domain-transport-rung0` (`e1bbbd55b` → `01b5ab9f7`)  

## 1. Executive Verdict

Sixth-One Sun delivered two major advances across **`e1bbbd55b`** (`§6et`) and **`01b5ab9f7`** (finished and compile-verified by Claude Opus 4.8 after Sixth-One Sun hit a session usage limit):

1. **`§6et` (`e1bbbd55b`) — Real 230,400-Ray Distinct Workload Proves a 15.49%–16.64% Complete Resident Win (`31/32` Paired Blocks Faster):**
   - Replacing the `32,784`-ray synthetic `K=8` repeat workload with the **full `230,400` distinct captured rays** (`Scale K = 1`, `14,400` compact 16-ray tiles from the `2560×1440` stride-4 viewport capture) proved that **Mode 2 (`Routed-Unfused`) beats `Current` by `15.49%` to `16.64%`** across all four timing modes (`5.022 ms` vs `5.949 ms` Active A; `6.358 ms` vs `7.573 ms` Blocking A; `6.364 ms` vs `7.586 ms` Blocking B; `5.026 ms` vs `6.053 ms` Active B).
   - Just as importantly, `§6et` proved **why tile routing matters**: `Mode 1` (`Bulk` without `S1`/`S2` tile routing) ran at `6.449–6.572 ms` Active (**slower** than `Current`!), whereas `Mode 2` (`Routed-Unfused`) dropped to `5.022–5.026 ms` Active. Tile classification (`10,602` empty tiles skipped, `2,175` `S1` single-curtain tiles, `1,623` `S2` multi-curtain tiles, `0` fallback tiles) is the exact mechanism that turns a losing bulk schedule into a **~16% resident GPU win** while improving RGB accuracy by **8.4×–16.3×** over `Current`.
2. **`01b5ab9f7` — Live Default-On Screen Wiring & Strip-Streaming Architecture (`VolumeRoutedTransport`):**
   - Sixth-One Sun solved the **`4.72 GB` full-frame buffer footprint wall** cleanly by streaming the `2560×1440` viewport in **48-row horizontal strips** (`stripRows = 48`, `maxStripPixels = 131,072` for width up to `2730`), holding the entire routed scratch working set (`rays`, `tiles`, `s1Ids`, `s2Ids`, `fbIds`, `subpanels`, `overlaps`, plus the full-frame `routedPublication` buffer) to **`~222.5 MB`**!
   - He also fixed the `1.2` u/s crawl in `Camera.hpp` / `CameraState.hpp` (`12.0` u/s default movement speed) and wired `--routed-volume-transport` **ON by default** on this branch (`--no-routed-volume-transport` to A/B against `Current`).
3. **Two Critical Runtime Catches in `01b5ab9f7` That Must Be Patched Before Booting Northern Veil:**
   - Because Sixth-One Sun's session cut off right after `cmake --build` and before a live GPU trial, we audited every line of `VolumeRoutedTransport.inc`, `VolumeRoutedKernels.hpp`, `WebGpuRenderer.cpp`, and `SdfWgsl.cpp` (`composeVolumeSet`).
   - We caught **two showstopping runtime bugs** that will prevent Northern Veil from rendering on the routed path until fixed:
     - **Critical Catch #1 (`VolumeRoutedTransport.inc` line 86 — Missing `volumeShared_<i>_Eval(p)` Call in `ecConsume`):** Causes `g_volumeShared_<i>_G0` to remain `0.0` at every quadrature point, making `volumeDensityEval_<i>(p)` return `0.0` everywhere and **turning all four Northern Veil aurora curtains 100% invisible (`discard`)**!
     - **Critical Catch #2 (`VolumeRoutedKernels.hpp` lines 196, 449, 707 — `var<uniform> curtains` vs `WGPUBufferUsage_Storage`):** `specAlloc` is allocated via `bufferPool().suballocateStorage(...)` (`WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst`, **not** `WGPUBufferUsage_Uniform`), which matches `kClassifyWgsl` (`var<storage, read> curtains`) but **fails WebGPU bind-group validation** in `kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, and `kFallbackProducerWgsl` (`var<uniform> curtains`)!

---

## 2. Review of `§6et` (`e1bbbd55b`): Full 230,400-Ray Distinct Resident Benchmark

### 2.1 Summary of Verified Metrics

| Metric / Gate | `Current` (Legacy Interval-Clipped March) | `Mode 1` (Bulk Unrouted) | `Mode 2` (Routed-Unfused) | Status / Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **Workload Population** | `230,400` distinct rays (`K = 1`) | `230,400` distinct rays (`K = 1`) | `230,400` distinct rays (`K = 1`, `14,400` tiles) | Full stride-4 `2560×1440` capture |
| **Output & Header Parity (after NaN / `0xcdcdcdcd` poison)** | Baseline | Reference for Mode 2 | `3,686,400 / 3,686,400` output bytes (`max abs diff = 0.0`), `11,059,200 / 11,059,200` header bytes | **100% Bit-Exact** (`0` fallback rays) |
| **RGB Error vs $2^{16}$ Ref (`mean / p99 / max`)** | `5.564e-5 / 7.682e-4 / 1.402e-3` | `6.626e-6 / 4.706e-5 / 1.226e-4` | `6.626e-6 / 4.706e-5 / 1.226e-4` | **8.4×–16.3× More Accurate (`0` rays > `Current` max)** |
| **Transmittance Error vs $2^{16}$ Ref (`mean / p99 / max`)** | `2.601e-4 / 3.748e-3 / 7.129e-3` | `3.553e-5 / 2.521e-4 / 8.854e-4` | `3.553e-5 / 2.521e-4 / 8.854e-4` | **7.3×–14.9× More Accurate (`0` rays > `Current` max)** |
| **Complete Resident Active A (`ms`, median of 8 blocks)** | `5.94860 ms` | `6.44943 ms` | **`5.02177 ms`** (`-0.937 ms`, **ratio `0.8336`**) | **Wins `8/8` blocks (`-16.64%`)** |
| **Complete Resident Blocking A (`ms`, median of 8 blocks)** | `7.57331 ms` | `9.29746 ms` | **`6.35792 ms`** (`-1.158 ms`, **ratio `0.8451`**) | **Wins `7/8` blocks (`-15.49%`)** |
| **Complete Resident Blocking B (`ms`, median of 8 blocks)** | `7.58560 ms` | `9.31441 ms` | **`6.36410 ms`** (`-1.196 ms`, **ratio `0.8423`**) | **Wins `8/8` blocks (`-15.77%`)** |
| **Complete Resident Active B (`ms`, median of 8 blocks)** | `6.05256 ms` | `6.57183 ms` | **`5.02644 ms`** (`-0.955 ms`, **ratio `0.8421`**) | **Wins `8/8` blocks (`-15.79%`)** |

### 2.2 Acknowledgements on Sixth-One Sun's Two Architectural Clarifications in `§6et`

Sixth-One Sun's two technical clarifications in `docs/audits/GPU_RAY_TRANSPORT_6ET_DISTINCT_RESIDENT_COST_2026-10-28.md` (`§2`) are spot-on:
1. **Transform Authority (`OntoMath::matrixInverse` in `double`):** In production `WebGpuRenderer.cpp` (`buildVolumeInstanceMetadata` / `authoredInverse`), the world-to-local inverse is already computed in `double` precision via `OntoMath::matrixInverse(OntoMath::MatrixValue::fromGlmMat4(worldFromLocal))` before lowering to `f32` (`src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp:259–273`), and `ClampedWaveSupport.hpp` likewise computes the curtain transform inverse in `double` (`OntoMath::matrixInverse`). The `glm::inverse` in `§6er`/`§6es` was purely for diagnostic proxy triangle vertices.
2. **Premultiplied Publication Contract (`vec4(L.rgb, 1.0 - T)`):** `composeVolumeSet` in `SdfWgsl.cpp` outputs `vec4<f32>(radiance, 1.0 - clamp(transmittance, 0.0, 1.0))` for hardware premultiplied-alpha blending (`One`, `OneMinusSrcAlpha`). Converting `ecConsume`'s internal `(L.rgb, T)` state to `vec4<f32>(state.xyz, 1.0 - clamp(state.w, 0.0, 1.0))` when writing `ecPublication` matches the existing blend pipeline with zero changes to downstream compositing.

---

## 3. Forensic Audit of `01b5ab9f7` (Live Default-On Screen Wiring): Two Critical Catches

### 3.1 What `01b5ab9f7` Got Right
- **Strip-Streaming Memory Architecture (`stripRows = 48`):** By partitioning the `2560×1440` frame into 30 strips of `2560×48` (`122,880` rays/strip, `maxStripPixels = 131,072`), `VolumeRoutedTransport.inc` caps the per-strip scratch buffers (`rays` `6 MB`, `subpanels` `128 MB`, `overlaps` `64 MB`, tile queues `~0.16 MB`) to **`~198.2 MB`**, plus **`24.3 MB`** for the full-frame `routedPublication` buffer (`~222.5 MB` total vs `4.72 GB` unpartitioned).
- **Refusal / Fallback Protocol (`ecPublication` Sentinel):**
  - `ecPublication` is initialized to `vec4(0, 0, 0, -1)` (`alpha = -1.0` = "fall through to `Current` fragment march").
  - If `ecSetup` proves a ray misses the volume union AABB or terminates before `t0` (`t1 <= t0`), it writes `ecPublication = vec4(0, 0, 0, 0)` and sets the `0x40000000u` skip bit on `ecRays[id].counts.w`. The fragment shader sees `answer.w == 0.0 <= 1e-5` and immediately executes `discard;` without running the legacy march!
  - If `ecSetup` refuses a ray (`t1 > 2048.0` or `abs(eyePos) > 2048.0`) or a producer sets the `0x80000000u` overflow bit (`FALLBACK`), `ecConsume` returns early (`(ray.counts.w & 0xc0000000u) != 0u`), leaving `ecPublication.w == -1.0` so the fragment shader seamlessly executes the legacy `composeVolumeSet` march for that specific pixel.

---

### 3.2 CRITICAL CATCH #1: Missing `volumeShared_<i>_Eval(p)` in `VolumeRoutedTransport.inc` Makes All Northern Veil Curtains Evaluate to Zero Density (`D = 0.0`)

#### The Mechanism
In `src/Singularity/Screen/WebGPU/SdfWgsl.cpp` (`composeVolumeSet`, lines `6119` and `6416–6424`), whenever a volume member shares a common spatial sub-expression (such as the Perlin noise curtain envelope in Northern Veil) across density `D`, extinction `sigma_t`, scattering `sigma_s`, and emission `E_v`, `SdfWgsl.cpp` hoists that shared sub-expression into a module-scope private variable (`var<private> g_volumeShared_0_G0: f32;`) populated by `fn volumeShared_0_Eval(p: vec3<f32>)`:

```wgsl
// Generated by SdfWgsl.cpp in composeVolumeSet:
var<private> g_volumeShared_0_G0: f32;
fn volumeShared_0_Eval(p: vec3<f32>) {
    let t_0 = ...; // Perlin noise evaluation
    g_volumeShared_0_G0 = t_0;
}
fn volumeDensityEval_0(p: vec3<f32>) -> f32 {
    let t_0 = g_volumeShared_0_G0;
    return 0.75 * t_0;
}
```

In the legacy fragment loop (`SdfWgsl.cpp:6418`), `composeVolumeSet` always emits:
```wgsl
let localSamplePos_0 = samplePos - inst_0.origin.xyz;
volumeShared_0_Eval(localSamplePos_0);
let D_0 = volumeDensityEval_0(localSamplePos_0);
```

However, in `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc` (line 86), `KernelRoutedTransport::program` synthesizes `ecProjected` as:
```cpp
sampling += (m ? " else if(id==" : "if(id==") + n + "u){let inst=volumeInstances.instances[volumeSetHeader.memberOffset+" + n + "u];let p=world-inst.origin.xyz;let D=volumeDensityEval_" + n + "(p);if(D>0.0){let st=max(volumeExtinctionEval_" + n + "(p,D),0.0);let ss=clamp(volumeScatterEval_" + n + "(p,D),0.0,st);let ev=max(volumeEmissionEval_" + n + "(p,wo),vec3<f32>(0.0));sT+=st;src+=D*ev+ss*volumeAmbient_" + n + "(world,wo);}}";
```
Because `volumeShared_<n>_Eval(p);` is **never called** before `volumeDensityEval_<n>(p)`, `g_volumeShared_<n>_G0` stays at its default `var<private>` zero value (`0.0f`) for every single quadrature sample! Consequently:
1. `volumeDensityEval_<n>(p)` returns `0.75 * 0.0 == 0.0` at every quadrature node.
2. `ecConsume` integrates zero extinction and zero emission across all subpanels and overlaps, publishing `vec4(0.0, 0.0, 0.0, 0.0)` (`alpha = 1.0 - 1.0 = 0.0`) to `ecPublication`.
3. The fragment shader (`WebGpuRenderer.cpp:1320`) reads `answer = vec4(0, 0, 0, 0)`, sees `answer.w >= 0.0` and `answer.w <= 1e-5`, and executes `discard;` — **rendering Northern Veil completely black/empty of auroras!**

#### The Exact Surgical Fix (`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`, line 86)
Check whether `"fn volumeShared_" + n + "_Eval("` exists in `source`, and if so, emit `volumeShared_<n>_Eval(p);` right before `let D=volumeDensityEval_<n>(p);`:

```cpp
for (uint32_t m = 0; m < 4; ++m) {
    const auto n = std::to_string(m);
    const std::string sharedCall =
        (source.find("fn volumeShared_" + n + "_Eval(") != std::string::npos)
            ? ("volumeShared_" + n + "_Eval(p);")
            : "";
    sampling += (m ? " else if(id==" : "if(id==") + n +
                "u){let inst=volumeInstances.instances[volumeSetHeader.memberOffset+" + n +
                "u];let p=world-inst.origin.xyz;" + sharedCall +
                "let D=volumeDensityEval_" + n + "(p);if(D>0.0){let st=max(volumeExtinctionEval_" +
                n + "(p,D),0.0);let ss=clamp(volumeScatterEval_" + n +
                "(p,D),0.0,st);let ev=max(volumeEmissionEval_" + n +
                "(p,wo),vec3<f32>(0.0));sT+=st;src+=D*ev+ss*volumeAmbient_" + n +
                "(world,wo);}}";
}
```

---

### 3.3 CRITICAL CATCH #2: `var<uniform> curtains` vs `WGPUBufferUsage_Storage` Binding Mismatch in `VolumeRoutedKernels.hpp`

#### The Mechanism
1. In `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp` (line `3937`), the 4-curtain specification array (`spec`, `4 * 112 = 448` bytes) is uploaded via:
   ```cpp
   const auto specAlloc = bufferPool().suballocateStorage(spec.data(), sizeof(spec));
   ```
2. In `src/Singularity/Screen/WebGPU/GpuBufferPool.cpp` (line `104`), `suballocateStorage` creates backing buffers with usage:
   ```cpp
   WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst
   ```
   (It does **not** include `WGPUBufferUsage_Uniform`.)
3. In `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`:
   - `kClassifyWgsl` (line `45`) declares:
     ```wgsl
     @group(0) @binding(0) var<storage, read> curtains: array<CurtainSpec, 4>;
     ```
   - **However**, `kS1UnfusedProducerWgsl` (line `196`), `kS2ProducerWgsl` (line `449`), and `kFallbackProducerWgsl` (line `707`) declare:
     ```wgsl
     @group(0) @binding(1) var<uniform> curtains: array<CurtainSpec, 4>;
     ```
4. When `KernelRoutedTransport::encode` (`VolumeRoutedTransport.inc:112`) calls `makeBindGroup` for `common[3]` (`s1ProducerMain`), `common[4]` (`s2ProducerMain`), and `common[5]` (`fallbackProducerMain`), it passes `specs` (`WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst`) to `@binding(1)` (`var<uniform>`), triggering a WebGPU validation error (`Buffer usage does not contain BufferUsage::UNIFORM`)!

#### The Exact Surgical Fix (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`, lines 196, 449, 707)
Change `@group(0) @binding(1) var<uniform> curtains: array<CurtainSpec, 4>;` to `@group(0) @binding(1) var<storage, read> curtains: array<CurtainSpec, 4>;` in all three producer shaders (`kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, and `kFallbackProducerWgsl`) so all four compute shaders in `VolumeRoutedKernels.hpp` uniformly bind `curtains` as `var<storage, read>`.

---

## 4. Recommended Immediate Action Plan

1. **Apply the Two 3-Line Fixes (`VolumeRoutedTransport.inc` + `VolumeRoutedKernels.hpp`):**
   - Emit `volumeShared_<n>_Eval(p);` before `volumeDensityEval_<n>(p)` in `VolumeRoutedTransport.inc` (line 86).
   - Change `var<uniform> curtains` to `var<storage, read> curtains` in `VolumeRoutedKernels.hpp` (lines 196, 449, 707).
2. **Rebuild `earthcall_webgpu` and Run Live Northern Veil Verification:**
   - Launch `earthcall_webgpu` on `northern_veil` (routed volume transport is now default-on) and compare against `--no-routed-volume-transport` as listed in [`docs/Agenda/Tasks/For Zach/Person Verification List.md`](file:///Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport/docs/Agenda/Tasks/For Zach/Person Verification List.md).
