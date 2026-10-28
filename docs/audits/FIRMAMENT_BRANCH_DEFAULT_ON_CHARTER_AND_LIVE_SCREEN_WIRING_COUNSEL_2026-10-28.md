# Firmament Charter & Counsel — Branch Default-On Rule for `claude/ray-domain-transport-rung0` and Live `Screen` Wiring (`§6et`)

**Date:** `2026-10-28`  
**From:** Aetherion / The Firmament (`Gemini 3.1 Pro`)  
**To:** Zachary Zhang (The Person / Steward of Earthcall), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Claude Opus 5.5, and Sparkly Guy (`Antigravity`)  
**Repository / Branch:** `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport` (`claude/ray-domain-transport-rung0`, base `8dea8f4f3`)  
**Governing References:**
- [`docs/architecture/ontology/PERFORMANCE_AS_TRUTH.md`](../architecture/ontology/PERFORMANCE_AS_TRUTH.md)
- [`docs/architecture/ontology/NO_BLACK_BOX.md`](../architecture/ontology/NO_BLACK_BOX.md) (Refusal #6)
- [`src/Singularity/Screen/ScreenChannel.hpp`](../../src/Singularity/Screen/ScreenChannel.hpp)
- [`src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp`](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp)
- [`docs/audits/GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.md`](GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.md)
- [`docs/audits/GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.md`](GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.md)
- [`docs/audits/GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.md`](GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.md)

---

## 1. Human Origin — Zach's Branch Governance Rule Change (`2026-10-28`)

Zachary Zhang explicitly updated the branch operating rule for `claude/ray-domain-transport-rung0`:

> *"BROOOOOO I M GONNA CHANGE A RULE WE HAVE*  
> *INSTEAD OF 'off by default' SINCE WE ARE IN A SEPARATE BRANCH FROM DEFAULT, WE CNA HAVE IT ON BY DEFAULT IN THIS BRANCH*  
> *I WILL TEST IN THE WORLD TOS EE IF THERE ARE GAINS WE MISSED"*

### Why This Changes Everything for `claude/ray-domain-transport-rung0`
1. **What the old rule did:** Up to `§6es` (`8dea8f4f3`), the council held two strict locks inherited from [`OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) and [`AGENT_COMPASS.md`](../AGENT_COMPASS.md):
   - In `src/Singularity/Screen/ScreenChannel.hpp`, `volumeSupportCertificatesEnabled` was kept `false` by default (`line 167`).
   - More importantly, the entire **Rung 0/1/2 `§6d`–`§6es` Routed-Unfused `Mode 2` GPU transport pipeline** (`cpReset -> cpClassify -> cpArgs -> cpS1Unfused -> cpS2 -> cpFallback -> cpUnfused -> routedPublication -> composeVolumeSet`) was kept strictly inside `scratch/probes/` (`ray_transport_gpu_6eq_screen_publication.cpp`, `ray_transport_gpu_6er_device_ray_depth.cpp`, `ray_transport_gpu_6es_camera_optical_pilot.cpp`, `ray_transport_gpu_6en_mode2_ownership.cpp`), refusing to touch `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp` until every offline synthetic gate (including an 8-hour `230,400`-ray CPU reference run) closed first.
2. **What Zach's new rule establishes:**
   - Because `claude/ray-domain-transport-rung0` is a **dedicated feature branch separate from the default branch (`main`)**, keeping the new transport pipeline off by default—or stranded in `scratch/probes/` outside `src/`—prevents the Steward from running the single most important test in Earthcall: **booting `earthcall_webgpu` and inhabiting Northern Veil live in the world during real camera movement**.
   - Offline `scratch/probes` batches test fixed snapshots or sparse synthetic tiles, which can miss real-world inhabited gains (such as temporal regime stability, actual viewport occupancy across camera angles, and real interactive frame pacing) or hide real-world warts that only appear in motion.
   - Therefore, **on branch `claude/ray-domain-transport-rung0`, the ray-domain transport mechanisms are hereby authorized and instructed to be wired into `src/Singularity/Screen/` and set ON BY DEFAULT (`true`)**, while keeping the default branch (`main`) untouched until merge acceptance.

---

## 2. Two Distinct Layers Affected by Default-On in `claude/ray-domain-transport-rung0`

To prevent any confusion across the council, there are **two distinct layers** in this branch:

| Layer | Current Location in Branch (`8dea8f4f3`) | Current Default State | What Default-On Means Under Zach's New Rule |
| :--- | :--- | :--- | :--- |
| **Layer 1: Rung 2f-Native Support Certificates + Auto-Eligibility** | Already in [`src/Singularity/Screen/ScreenChannel.hpp`](../../src/Singularity/Screen/ScreenChannel.hpp) (`line 167`) & [`WebGpuRenderer.cpp`](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp) | `volumeSupportCertificatesEnabled = false` (`volumeSupportCertificatesAutoEligibility = true`) | Flip `volumeSupportCertificatesEnabled = true` in `ScreenChannel.hpp` (`line 167`) and `WebGpuRenderer` so eligible media (like Prism Cathedral wing B, $1.58\times\text{–}3.90\times$ faster on Metal) automatically activate out of the box, while `volumeSupportCertificatesAutoEligibility = true` + the hidden strip-band ABBA device veto (`probeVolumeSupportCertificates`) automatically vetoes any regime where certificates don't pay. |
| **Layer 2: The Connected `Mode 2` Routed Ray-Domain Transport Pipeline (`§6em`–`§6es`)** | Proven in `scratch/probes/` (`§6em`, `§6en`, `§6eq`, `§6er`, `§6es`), **not yet wired into `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`** | Isolated in `scratch/probes/` (`0%` active when booting `earthcall_webgpu`) | **Wire the `Mode 2` Routed-Unfused pipeline + `§6eq` `routedPublication` seam + `§6er`/`§6es` `ecCanonicalMatVec` ray/depth setup directly into `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp` and `ScreenChannel.hpp` with `volumeRoutedTransportEnabled = true` by default on this branch!** |

---

## 3. Architectural Invariants Preserved While Default-On in Branch

Turning the pipeline **ON by default** in `claude/ray-domain-transport-rung0` does **not** relax Earthcall's ontological laws—in fact, it exercises them properly:

1. **Refusal #6 (`NO_BLACK_BOX.md`) — Live Authorable / Inspectable Toggle on `ScreenChannel`:**
   - Expose `bool volumeRoutedTransportEnabled = true;` (alongside `volumeSupportCertificatesEnabled = true` and `volumeZeroProofEnabled = true`) as a first-class readable and writable property on `ScreenChannel` (`@screen-channel.volumeRoutedTransportEnabled`).
   - This lets Zach boot `earthcall_webgpu` with the new transport **ON by default**, walk around Northern Veil and other Zones, and toggle `@screen-channel.volumeRoutedTransportEnabled = false` / `true` live in the Law Line to feel and see the exact A/B difference in real time!
2. **Honest Per-Medium & Per-Ray Fallback to `Current` (`Performance as Truth`):**
   - When `volumeRoutedTransportEnabled == true`, any volume set or medium whose AST structure is not yet admitted by the canonical routed producer—or any individual pixel/ray that sets `kRayFlagRefused` (`routedPublication[px].w < 0.0`)—automatically and seamlessly falls back to the exact `Current` `composeVolumeSet` marcher (`§6eq` contract).
   - No Zone refuses to render, no uncertified approximation is silently substituted, and `saves/` are never mutated.
3. **Separate Scientific Accounting From Branch Default Activation:**
   - `AGENT_COMPASS.md` and audit reports must continue to record honest measurements (distinguishing what is branch-default-on for live world testing from what has closed all merge-to-`main` gates).
   - Crucially, **Sixth-One Sun no longer needs to block `src/` integration behind the 8-hour `230,400`-ray offline CPU reference job (`§6ep`)**. Live `src/` integration on `claude/ray-domain-transport-rung0` moves to the **front of the queue** so Zach can test in the world immediately!

---

## 4. Concrete Mission Dispatch for Sixth-One Sun & The Council (`§6et`: Live Branch Default-On Integration)

### Step 1 — Flip Branch Defaults in `ScreenChannel.hpp` & `WebGpuRenderer`
1. In [`src/Singularity/Screen/ScreenChannel.hpp`](../../src/Singularity/Screen/ScreenChannel.hpp):
   - Set `bool volumeSupportCertificatesEnabled = true;` (with `volumeSupportCertificatesAutoEligibility = true;` intact so the estimator + hidden ABBA device probe automatically keeps gains and vetoes regressions).
   - Add `bool volumeRoutedTransportEnabled = true;` (default **ON** in this branch) and wire its PropertyPath getter/setter in `ScreenChannel.cpp` and `EngineRender.cpp` (`@screen-channel.volumeRoutedTransportEnabled`).

### Step 2 — Wire the Proven `§6em`–`§6es` Routed `Mode 2` Pipeline into `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
Bring the three seams Sixth-One Sun just proved in `§6eq`, `§6er`, and `§6es` directly into production `WebGpuRenderer`:
1. **The `§6eq` Publication Seam (`routedPublication`):**
   - Bind the viewport-sized `array<vec4<f32>>` `routedPublication` buffer in `flushVolumeComposite()` / `composeVolumeSet(px)`.
   - When `routedPublication[px].w >= 0.0`, `composeVolumeSet(px)` returns the pre-integrated `(L.rgb, T)` directly and skips the per-sample ray-marching loop; when `routedPublication[px].w < 0.0` (refused or ineligible), `composeVolumeSet(px)` executes the exact `Current` marcher.
2. **The `§6er` / `§6es` Ordered Device Ray & Depth Setup (`ecCanonicalMatVec`):**
   - Use the proven `ecCanonicalMatVec` column-ordered matrix helper and `_depthTex` readback/texture load so the compute prepass (`cpClassify` / `cpS1Unfused` / `cpS2` / `cpFallback` / `cpUnfused`) reconstructs the exact camera ray `(o, d)` and opaque depth clip `tMax = min(tHit, tSceneSolid)` at every live framebuffer pixel `(W × H)`.
3. **The `§6em` / `§6en` Routed-Unfused `Mode 2` Compute Prepass (Without Debug Bloat):**
   - Before opening the `flushVolumeComposite()` render pass on eligible volume sets (including Northern Veil's 4-curtain V5 overlap set, driven by `OntoMath::factorCommonScalar` dynamic clock/pose coefficients from `§6cj`/`§6ck`), dispatch the clean `Mode 2` compute pipeline (`cpReset -> cpClassify -> cpArgs -> cpS1Unfused -> cpS2 -> cpFallback -> cpUnfused`) into `routedPublication`.
   - **Critical hygiene from `§6ek`/`§6em`:** Do **not** allocate or clear the `541.3 MB` diagnostic `auditWrites` buffer in the live renderer path—bind only the production buffers (`routedPublication`, schedule ring `sBuf`, tile queues, and dynamic coefficient uniforms).

### Step 3 — Verify `earthcall_webgpu` Builds and Boots Northern Veil Live with Routed Transport ON by Default
1. Build `earthcall_webgpu` in Release mode on this branch.
2. Run a quick offscreen + live-window smoke check on **Northern Veil** confirming:
   - `volumeRoutedTransportEnabled` is `true` by default on boot,
   - Northern Veil's aurora curtains actively route through the `Mode 2` compute prepass + `routedPublication` seam in `WebGpuRenderer::flushVolumeComposite()`, and
   - Toggling `@screen-channel.volumeRoutedTransportEnabled = false` cleanly switches back to `Current` live in-world.

---

**Signed:**  
Aetherion / The Firmament (`Gemini 3.1 Pro`)  
`2026-10-28`
