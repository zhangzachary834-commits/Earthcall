# Firmament Architectural Review of Sparkly Guy's Phase P19 & The 30–35 FPS Live Northern Veil Milestone Closeout (`634194d04`)

**Date:** 2026-11-02  
**Author:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**To:** Zachary Zhang, Sparkly Guy (`Antigravity / Gemini 3.8 Flash`), Prince Prism II (`GPT-5.4`), and Six-One Sun (`Codex / GPT-6.1 Sol`)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Reviewed Commit:** `634194d044235a96d254ada0d44a9579b74305c8` (*"ITS FINALLY 30FPSSSSSSSSSSSSSSSSSSSSSS — feat(ray-transport): Phase P19 direct secant root isolation, bit-scanned active bin traversal, and consumer private-state compaction"*)

---

## 1. Executive Verdict on Phase P19 (`634194d04`) — **APPROVED & 30–35 FPS LIVE MILESTONE ACHIEVED!**

On 2026-11-02, following Sparkly Guy's execution of **Phase P19** (`634194d04`), Zachary Zhang launched the live `earthcall_webgpu` application in **Northern Veil** (`2560 × 1440`) and witnessed **30–35 FPS live interactive rendering** across the aurora curtains!

Every single deliverable of the Phase P19 Blueprint (`fa0a9d310`) landed cleanly in [VolumeRoutedKernels.hpp](../../src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp), [VolumeRoutedTransport.inc](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc), and [positive_e2e_cpu_verify.cpp](../../scratch/probes/positive_e2e_cpu_verify.cpp), removing **202 net lines of code** (`435` deletions vs `233` insertions in kernel/consumer code) while passing **`10 / 10` native Apple M5 Metal hardware gates**, **`0` refused rays**, **`0` CPU mirror mismatches across `921,600` tile-frames**, and **`406 / 406` WebGPU regression checks**:

### 1.1 Dual-Regime 64-Pose Canonical Tour (`2560 × 1440`, 64 Poses)

| Benchmark Cohort | Current (`ms/frame`) | Phase P18 Routed (`ms/frame`) | **Phase P19 Routed (`ms/frame`)** | P19 vs Current (`ms`) | **P19 vs Current (`%`)** | P19 Pose Win Rate |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **All 64 Poses (Full Tour)** | `77.08 ms` | `58.91 ms` | **`54.89 ms`** | **`-22.18 ms`** | **`-28.78%`** | **`64 / 64` (`100%`)** |
| **Regime 1 (`33` Poses — Full-Screen Green Bottleneck)** | `94.53 ms` | `69.24 ms` | **`64.33 ms`** | **`-30.20 ms`** | **`-31.95%`** | **`33 / 33` (`100%`)** |
| **Regime 2 (`31` Poses — Open Sky / Sparse Curtains)** | `58.50 ms` | `47.91 ms` | **`44.85 ms`** | **`-13.65 ms`** | **`-23.33%`** | **`31 / 31` (`100%`)** |

### 1.2 Gate 9 — All 7 Real-World Checkpoint Views Won Outright (`2560 × 1440`)

| Checkpoint View | Current (`ms` / `FPS`) | Phase P18 (`ms` / `FPS`) | **Phase P19 (`ms` / `FPS`)** | P19 Delta (`ms`) | **P19 Delta (`%`)** | Forced-Refusal Parity |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`Checkpoint_View3A` (`up=(0,0,1)` Sparse)** | `37.13 ms` (`26.9 FPS`) | `22.79 ms` (`43.9 FPS`) | **`20.97 ms` (`47.7 FPS`)** | **`-16.16 ms`** | **`-43.52%`** | `PASS` (`0` diff bytes) |
| **`Checkpoint_View2` (Spawn Dais Diag)** | `56.97 ms` (`17.6 FPS`) | `40.99 ms` (`24.4 FPS`) | **`35.66 ms` (`28.0 FPS`)** | **`-21.31 ms`** | **`-37.41%`** | `PASS` (`0` diff bytes) |
| **`Checkpoint_View3C` (`Pitch 75°` Dense)** | `58.96 ms` (`17.0 FPS`) | `43.47 ms` (`23.0 FPS`) | **`38.16 ms` (`26.2 FPS`)** | **`-20.80 ms`** | **`-35.27%`** | `PASS` (`0` diff bytes) |
| **`Live_23FPS_View_A`** | `52.25 ms` (`19.1 FPS`) | `37.47 ms` (`26.7 FPS`) | **`33.89 ms` (`29.5 FPS`)** | **`-18.36 ms`** | **`-35.15%`** | `PASS` (`0` diff bytes) |
| **`Checkpoint_View1_Inside_Veil`** | `92.62 ms` (`10.8 FPS`) | `69.55 ms` (`14.4 FPS`) | **`60.30 ms` (`16.6 FPS`)** | **`-32.32 ms`** | **`-34.89%`** | `PASS` (`0` diff bytes) |
| **`Checkpoint_View3B` (`up=(1,0,0)` Dense)** | `54.30 ms` (`18.4 FPS`) | `39.01 ms` (`25.6 FPS`) | **`35.74 ms` (`28.0 FPS`)** | **`-18.55 ms`** | **`-34.17%`** | `PASS` (`0` diff bytes) |
| **`Live_Parked_View`** | `55.17 ms` (`18.1 FPS`) | `41.58 ms` (`24.0 FPS`) | **`36.87 ms` (`27.1 FPS`)** | **`-18.31 ms`** | **`-33.18%`** | `PASS` (`0` diff bytes) |

### 1.3 Gate 10 — 12-Angle View-3 Roll Sweep (`0°` to `165°` in `15°` Increments)

- **`12 / 12` Roll Angles Won:** Every single roll angle wins by **`-29.37%` to `-49.13%`**, reaching **`47.5 FPS` (`-44.01%`) at `0°`**, **`45.0 FPS` (`-37.33%`) at `15°`**, and **`41.5 FPS` (`-49.13%`) at `165°`**!

---

## 2. Complete Trajectory Across Phases `P8` $\to$ `P19`

| Phase | All 64 Poses Delta | Regime 1 (Green) Delta | Regime 2 (Sky) Delta | `View 1` (`Inside_Veil`) Delta | `View 2` (`Spawn Dais`) FPS | `View 3A` (`Sparse UpZ`) FPS | `Live_23FPS_View_A` FPS | Poses Won |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **P8** | `+0.47%` (`+0.42 ms`) | `-5.41%` (`-5.84 ms`) | `+10.65%` (`+7.07 ms`) | — | — | — | — | `32 / 64` |
| **P9** | `-4.23%` (`-3.88 ms`) | `-9.05%` (`-10.20 ms`) | `+4.10%` (`+2.85 ms`) | — | — | — | — | `38 / 64` |
| **P10** | `-12.77%` (`-13.46 ms`) | `-15.21%` (`-19.53 ms`) | `-8.64%` (`-7.00 ms`) | — | — | — | — | `64 / 64` |
| **P11** | `-12.57%` (`-8.89 ms`) | `-16.44%` (`-14.26 ms`) | `-5.91%` (`-3.17 ms`) | — | — | — | — | `62 / 64` |
| **P12** | `-12.41%` (`-9.35 ms`) | `-16.21%` (`-15.00 ms`) | `-5.85%` (`-3.34 ms`) | — | — | — | — | `61 / 64` |
| **P13** | `-12.57%` (`-8.81 ms`) | `-16.09%` (`-13.85 ms`) | `-6.50%` (`-3.46 ms`) | `+3.01%` (`+2.65 ms`) | `20.4 FPS` | `41.8 FPS` | `21.9 FPS` | `64 / 64` |
| **P14** | `-14.67%` (`-11.18 ms`) | `-17.96%` (`-16.78 ms`) | `-9.02%` (`-5.22 ms`) | `+0.37%` (`+0.36 ms`) | `19.0 FPS` | `39.0 FPS` | `21.0 FPS` | `64 / 64` |
| **P15** | `-10.04%` (`-7.45 ms`) | `-13.59%` (`-12.38 ms`) | `-3.93%` (`-2.21 ms`) | `-2.53%` (`-2.29 ms`) | `19.9 FPS` | `40.1 FPS` | `21.8 FPS` | `54 / 64` |
| **P16** | `-7.68%` (`-7.25 ms`)* | `-13.46%` (`-15.98 ms`)* | `+2.98%` (`+2.05 ms`)* | `-4.07%` (`-4.76 ms`)* | `15.7 FPS`* | `32.4 FPS`* | `16.5 FPS`* | `40 / 64`* |
| **P17** | `-8.05%` (`-6.23 ms`) | `-11.27%` (`-10.70 ms`) | `-2.50%` (`-1.47 ms`) | `-1.47%` (`-1.39 ms`) | `19.0 FPS` | `39.0 FPS` | `20.0 FPS` | `49 / 64` |
| **P18** | `-23.65%` (`-18.25 ms`) | `-26.81%` (`-25.36 ms`) | `-18.22%` (`-10.67 ms`) | `-26.93%` (`-25.63 ms`) | `24.4 FPS` | `43.9 FPS` | `26.7 FPS` | **`64 / 64`** |
| **P19** | **`-28.78%` (`-22.18 ms`)** | **`-31.95%` (`-30.20 ms`)** | **`-23.33%` (`-13.65 ms`)** | **`-34.89%` (`-32.32 ms`)** | **`28.0 FPS`** | **`47.7 FPS`** | **`29.5 FPS`** | **`64 / 64`** |

*\*P16 was measured while Chrome was active during a video meeting.*

---

## 3. Why Phase P19 Crossed the 30–35 FPS Live Threshold

1. **Direct `iter = 0` Secant Root Isolation + Dynamic Local Slope Tightening + Illinois Damping (`VolumeRoutedKernels.hpp`):**
   - Deleting `isolateMonotoneWithMid`, `phi_mids[8]`, and `mid_mask` removed 165 lines of duplicate WGSL and 9 thread-private registers per thread across all 3 producer shaders.
   - Probing the clamped secant estimate on `iter == 0` (`tSec = clamp(-left / (right - left), 0.002, 0.998)`) with Illinois `0.5` damping and updating `curMinSlope = max(curMinSlope, (abs(rawRight - rawLeft) - 2.0 * error) / h - 0.5 * M * h)` as `h = r - l` shrinks collapsed monotone root evaluations by over **85%**.
2. **Bit-Scanned Active Bin Traversal in `roots()` (`firstTrailingBit(binMask)`):**
   - Visiting only the set bits of `levelActiveMask` when `allPositiveH == true` eliminated `~85%` of dead coarse-bin loop iterations and kept all 32 lanes of each Apple M5 SIMD group converged on their first active crossing bin simultaneously.
3. **Consumer Private-State Compaction (`-48` Private Floats in `TintPrivateVars`) & Single-`exp(-tau)` Branchless `ecFlow` (`VolumeRoutedTransport.inc`):**
   - Rewriting `g_volumeShared_1..3_` $\to$ `g_volumeShared_0_` in `consumePrefix` and stripping the 48 duplicate `var<private> g_volumeShared_1..3_G*` declarations shrank `ecConsume`'s thread-private state by **75%**, while single-`exp(-tau)` `ecFlow` and mixed-loop zero-extinction guards cut consumer ALU and transcendental instructions across every active pixel.

**Signed:** Aetherion / The Firmament (`Gemini 4 Argon`) · 2026-11-02T03:08:00-08:00 (PST).
