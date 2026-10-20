# SourceRho Measurement Sun — bounded economic handoff (2026-10-20)

Scope: isolated measurement successor of draft PR #482, **not** a production promotion or canonical reconciliation. Read the committed analysis at `docs/analysis/source_rho_measurement_successor_native_paired_checkpoint_2026-10-17.md` and predecessor verdict at `docs/analysis/source_rho_zero_visibility_elision_native_economics_verdict_2026-10-10.md`.

## Native evidence

Three independent macOS ARM64 native SourceRho benchmark runs executed identical benchmark source blob `9b08e6c44717d00f96ccf0f8967547a43a918705`:

| GitHub Actions run | Wall reduction by arm medians | Median paired delta (exact minus authority) | Paired MAD | Authority-favorable pairs |
|---|---:|---:|---:|---:|
| [39010536979](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39010536979) | 28.57% | +2.530042 ms | 2.699041 ms | 10/12 |
| [39021012172](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39021012172) | 0.32% | -0.002375 ms | 0.911979 ms | 6/12 |
| [39032315751](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39032315751) | 0.0734% | +0.078292 ms | 0.780896 ms | 6/12 |

Each dedicated native SourceRho witness passed and counted **5,624,857** eliminated GPU-executed shadow-loop SDF steps for the proven-zero source, with **6,778,135** unchanged live-source steps. Existing byte-exact framebuffer, diagnostic/ordinary, visibility ON/OFF occlusion, hostile lifetime, repair/fail-open and recovery witnesses passed in this bounded fixture. Recurring shader compiles and parameter uploads were zero. The subsequent shared CPU-SDF step failed and remains unclassified; do not attribute it to SourceRho without evidence.

No valid GPU timestamps were recorded in either arm: unavailable is **not** zero GPU cost. Poll time is not a GPU duration measurement. Adapter identity, scheduling, thermals, and residency are not controlled enough to assign the observed variance to a single mechanism.

## Decision and custody

**Economic rejection / do not ship or merge**: actual work elimination is proven in this fixture, but the large wall-frame gain did not reproduce in two independent same-code runs. Do not select the favorable run as representative or claim that the optimization intrinsically has zero GPU value.

Preserve `rho_source != V_transport != D_medium`, independent ambient/receiver/V1–V4, stable identity/revision/generation/channel gates, direct-key lookup, and all existing correctness witnesses. Do not modify production renderer semantics, predecessor branch, or canonical. No PR or merge is authorized.

The measurement branch `sol/sourcerho-measurement-20261016` contains benchmark-only instrumentation and analysis. Future work, if separately authorized, should target genuine GPU timestamps and controlled adapter/environment replication, not more uncontrolled reruns seeking a favorable number.

This handoff records the bounded outcome and open attribution/CPU-SDF questions. If a write gate blocks future housekeeping, report the block without bypassing it; keep the user-requested monitoring task enabled unless explicitly instructed otherwise.
