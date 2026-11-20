# C5 Summit Tribunal

Codex / GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-17T17:17:00-08:00. Zachary Zhang and Prism Sun commissioned the native tribunal. Zach holds the Person verdict. This record continues [Prism Sun’s reconciliation receipt](../architecture/mathematics/C5_CURRENT_DEFAULT_RECONCILIATION_II_RECEIPT_2026-11-17.md#acceptance-boundary).

## Checkout and build

The fetch confirmed `prism/c5-current-default-reconciliation-ii` at `200a995c3acfcc3f65c84821745416f8e0af6c41`, 25 ahead and zero behind `sync-from-earthcall-main` at `d7fb75cc6fa5fc7a0ffe94bdcedbdea572662c09`. PR #882 was open, draft and mergeable at inspection. Codex used a detached worktree at the verified commit because the main checkout held unrelated edits.

The worktree started without `build/CMakeCache.txt`. The managed sandbox first denied creation of `build/CMakeFiles/pkgRedirects`. With worktree write access, `EARTHCALL_BUILD_TYPE=Debug ./scripts/build.sh webgpu` built an arm64 `earthcall_webgpu`. All eleven named Debug test targets built at `-j8`, and `ctest --test-dir build -N` registered 323 tests, including the four C5 witnesses. The focused witnesses ran sequentially. The worktree transition cleared the first pass’s temporary logs and interrupted observation of the initial quadrature run. Subsequent results are saved under `build/c5-tribunal-evidence/`.

## New C5 witnesses

| Witness | Native Mac result |
| --- | --- |
| `volume_transport_cohort_planner_test` | PASS. Capability membership is permutation-invariant and chronology follows the camera and regional proof. |
| `volume_cohort_interval_schedule_test` | PASS. Covers same-ray A → B → A, local overlap, malformed proof and complexity refusal. |
| `volume_cohort_gpu_lowering_test` | PASS, including after rebuild. CPU-side production WGSL generation witness for one continuing V5 state and the shared activity coordinates. |
| `webgpu_volume_routed_span_contract_test` | PASS after correction. The initial test aborted on line 65 because it expected the old fixed `[0, 2048]` quantizer. The corrected test checks the bitcast range endpoints, refusal and normalization at both span endpoints, preserves the routed clip ceiling and still distinguishes refusal from exact empty. |

## Retained transport guards

| Witness | Native Mac result |
| --- | --- |
| `volume_ray_segment_transport_test` | PASS, 53/53. |
| `volume_ray_radial_segmentation_test` | PASS, 41/41. Across 227,328 authored-scene samples, no proved-empty sample had positive emitter density. |
| `webgpu_routed_transport_math_test` | PASS. The witness asserted the Metal backend. Each of three production producer kernels missed zero of 16 independent linear root cases and emitted zero outside-box spans across 16 physical-box rays. |
| `webgpu_volume_zero_proof_test` | PASS, 73/73. The eight-medium framebuffer was byte-identical with the proof active through inside views, reversal and fold crossings. |
| `webgpu_volume_support_certificates_test` | FAIL, 133/136. The native eight-field grid-plus-certificates tour uploaded 8 parameter bytes across 176 camera poses against a zero-byte gate. Its seven image checkpoints still matched a fresh renderer byte for byte. Auto mode reported 3 of 8 media eligible with 8 killers, against a zero-eligibility assertion. A separate idle-reuse assertion reported two program compiles against zero. Each of the eight single-curtain auto pixel checks passed. The combined fused assertion does not resolve fused auto pixel equality independently of eligibility. |
| `webgpu_line_quadrature_test` | FAIL. The initial Metal run rejected `cnoise3` as undefined. After restoring the shared helper, the noise integral and transport validation checks passed and authored emerald ray 10 ran on the GPU. Authored rays 3, 4 and 11 still refused before device execution at the existing schedule gate. |
| `webgpu_volumetric_mist_test` | PASS. The physical beam and shadow tribunal measured a 414× spatial contrast in its fixture. |

## Defects and bounded changes

The routed-span correction changes only the witness and enforces the existing C5b-2c range contract. The quadrature repair adds the existing Perlin WGSL helper only when an emitted standalone line field needs it. The app and affected test targets rebuilt. The numerical tolerances and panel caps were preserved.

The unchanged quadrature fixture requires the current Northern Veil primary emerald curtain to admit seeded rays 3, 4, 10 and 11 at host error budgets of `0.009` and a `2048`-panel limit. The failed scheduling calls happen before WGSL creation. Current default and C5 share that fixture, the relevant CPU planner and the test. At `d7fb75cc6`, the bounds, density, extinction and emission changed on all eight authored fields. This establishes the source and fixture provenance of the refusal. The certificate guard’s full runtime comparison against current default is still open.

At 17:27 PST the quadrature rerun had recorded 31 failed assertions. The primary emerald grid ran on Metal and admitted 4 of 16 rays at the unchanged `0.006` threshold. All four admitted rays were empty and none of the 12 occupied rays were admitted. Codex stopped the remaining proof-grid tail after this replay and the confirmed schedule failures. The process exit logged as `15` is from termination, and the recorded assertions establish the red verdict. This run has no full-suite assertion count.

## Northern Veil and the Person verdict

The checkout and the Person Data Home Zone file both have SHA-256 `211906aed1bbbe76767e4fc14f157c3bfc5ac5994438a5f9f0906de6a7a5a65e`. The file decodes to the four primary and four far fields. The materials directories matched at inspection. Release launch and loaded-state provenance will need a fresh witness. Zach’s historical 30–35 FPS belongs to the earlier four-field scene.

The Release build, scene entry, visual correctness, motion correctness and Routed ON/OFF comparison are gated on the retained Debug checks. Spawn, unobstructed, dense-inside, reverse-camera, far north and far south therefore all await live measurement. Zach’s Person verdict is pending. Follow the [Person verification entry](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#c5-summit-tribunal--northern-veil) after the Debug guards clear.

**Merge verdict — NOT READY.** The retained certificate and quadrature guards remain red. The two bounded repairs are local to the worktree.
