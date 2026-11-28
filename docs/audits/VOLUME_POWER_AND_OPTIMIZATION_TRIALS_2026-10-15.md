# Volume power repair and three rejected optimization trials

**Retained:** route-aware power lowering, numerical witnesses and an external experiment harness. **Rejected and removed from production:** per-group demand placement, a coarse AND occupancy index, and product-local arithmetic reuse. No new FPS improvement or movement milestone is established.

Zach commissioned implementation of the [demand/support plan](../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md), following his direction to combine repeated mathematics, compile once, repair incrementally and preserve the authored beauty at native resolution. This responds to [Firmament's supplied probes and power review](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#firmament-to-sixth-one-sun--astra--complete-probe-provenance-in-privatetmp-accepted-epistemic-qualifications-and-ontopow-three-route-parity-contract). The implementation and trials below are Codex's extensions of that counsel. They select no new channel semantics and resolve no author decision.

## Retained power correction

`src/Singularity/Screen/WebGPU/SdfWgsl.cpp` now provides one shared helper library for surface/direct and volume prefixes. Actual lowering keeps distinct existing CPU contracts:

- Scalar/vector MathNode Pow uses the existing exact negative-base/noninteger zero rule, with componentwise vector evaluation.
- ScalarForm Term and explicitly constructed legacy RPN use their existing `std::pow` domain behavior through a separate raw helper. The contemporary string authoring route prefers MathNode, so the witness explicitly supplies RPN tokens rather than accidentally testing MathNode twice.
- Exact binary32 bit classification replaces the previous epsilon-defined zero domain and approximate exponent integrality. Tiny normal bases are evaluated. Zero to a negative power produces signed IEEE infinity where appropriate, not a finite `1e38` substitute. Odd/even classification includes 8388609, which is representable and odd. Missing operands refuse admission.

No CPU equation implementation, parameter-slot order, authored expression, sampling lattice, authority mechanism or save codec is changed. `tests/support/webgpu_power_witness.hpp` appends compute entrypoints to complete production-emitted modules and uses actual parameter buffers/evaluators. Cases cover tiny normals, neighboring nonintegers, signs/zero, parity around 2^23 and 2^24, vectors, underflow/overflow and nonfinite inputs. Ordinary native compiler options and timestamps are unchanged.

**Numerical limit:** this establishes the tested route/domain behavior for uploaded binary32 values, not universal CPU-double/native bit equality. ScalarForm `FLT_MAX^-1` has a CPU subnormal result about `2.93874e-39` and returns zero on the retained native profile. The first expanded witness failed (74/75); that failure log remains preserved. A separate direct native-intrinsic witness also returns zero. The final test reports this as a profile boundary and checks intrinsic consistency separately; it does not relabel the subnormal case as CPU parity or introduce a new clamp. A complete backend arithmetic enclosure remains unfinished.

## Reproduced probe provenance

Firmament's replacement 501-line radial/anisotropic probe and separate Northern Veil subbox probe were copied, rebuilt using actual configured app compiler/link commands, and executed against the frozen fixtures. Full commands, source hashes and table outputs are retained. This supersedes the planning pass's missing-input limitation; its dated report remains historical.

The radial cloud count reproduces 0/34,656 -> 10,968/34,656 proved cells; the halo count reproduces 6,400/35,200 -> 19,648/35,200. The Northern Veil four-view tables also reproduce. For example, inside baseline visits/queries/unknowns/fold/fold-zero/noise are 173409/228908/147710/132725/65526/67199; the proposed two-level screen reports 140890/168354/79024/76401/9202/67199.

These are **CPU full-lattice models on 2,304 rays per view**, omitting noise AST execution and legacy opacity termination. They establish modeled work, not native instructions, certificate eligibility or measured GPU savings. Tighter subboxes, finer XZ proofs, new column eligibility and sampled GPU certificates were not adopted. Zero sampled holes does not prove a general convexity theorem.

## Implemented candidates and retention results

Each candidate used an independently frozen compatible **power-corrected** control. Correctness used RGBA8 offscreen images at **2560×1440**, both frozen Zones, four views, times 0/7.25 and compatibility/full-lattice modes: 32 comparisons per candidate. No resolution reduction, sampling change or image instrumentation was used.

| Candidate | Exact image gate | Predeclared primary economics, two ABBA series | Disposition |
|---|---|---|---|
| Per-group demand placement | All 32 byte-identical | Cloud/inside **+5.4086%, +1.5237%** time | Rejected; no repeated 5% improvement |
| Coarse AND occupancy index | All 32 byte-identical for the final guarded variant | Northern Veil/inside **+11.4208%, +9.8992%** time | Rejected; repeatably slower |
| Product-local repeated-value reuse | Northern Veil 16 exact; cloud 16 differ, maximum one level | Not timed after exactness failure | Rejected; no conservative bounded-output proof |

Positive time change means slower. These are completion-fenced **offscreen batch throughput** results, including CPU preparation/queue/GPU completion; no isolated volume GPU time, surface-acquire measurement or live-frame p95 is inferred. Each run has four warm draws and 24 measured frames. Both camera variants warm before timing. Completed primary ABBA series are preserved; remaining views were stopped after primary rejection, rather than presented as successful retention coverage. Thermal/load drift is visible and limits causal attribution. No FMA cause is asserted.

### Demand placement

The compiler captured actual substitution reads, including cached-fragment replay, and retained transitive group dependencies. It placed cloud G3 before density and G0–G2 after the existing contribution predicate, preserving parameter layout and the one-group Northern Veil modules. Both single/fused call sites consumed the split schedule. Exact images passed, but cloud/inside controls versus candidates averaged **471.056 -> 496.533 ms** and **518.959 -> 526.867 ms**. The recorded final-frame lowering/compile/parameter/proof-upload counters were zero; the batch tool does not aggregate those counters over every frame. Only one series exceeds 3%; do not describe that particular threshold as repeatably exceeded. No scheduling metadata or dispatch changes remain in production.

### Coarse occupancy trial

This was a separate generic index over **existing** fine proof bits, not Firmament's tighter/finer-grid proposal. Each aligned 4×4×4 parent was proven empty only if every covered child bit was empty. Packed macro words added approximately 2% storage; this was not a same-byte-budget claim. Presence and per-instance eligibility were separate flags; uncertain coordinates/directions retained the old fine walk. The final guarded implementation passed CPU/native lifecycle consumers and all 32 images. Its primary cost nevertheless worsened in both series. All macro fields, flags, CPU writer, WGSL traversal and experiment-specific tests were removed; the existing proof format and renderer were restored.

### Product-local reuse

Local value numbering within the existing product emitter retained numeric slot collection and sharing split/join invalidation. CPU codegen showed the cloud density's `distance` calls fall **2 -> 1**, with the same 66 parameters. All four Northern Veil standalone modules and parameter arrays were identical; the halo remained unchanged. This was a real source reduction but no aurora saving claim.

All 16 cloud captures changed: inside 17 bytes, spawn 8, reverse 6 and side 2 at each tested time/mode, totaling 132 byte differences with maximum one level. Finite one-level differences do not prove the bounded contract. The candidate was removed before performance measurement; no alias/signature changes remain.

## Final restored validation

The final restored app and eight focused targets build successfully. Fresh execution after all three rollbacks:

| Target | Result |
|---|---|
| `volume_shared_subexpression_test` | 60/60 |
| `sdf_wgsl_parameter_refresh_test` | PASS |
| `volume_zero_proof_test` | 32/32 |
| `webgpu_volume_shared_subexpression_test` | 76/76, including route-aware power and explicitly separate subnormal/intrinsic checks |
| `webgpu_volume_zero_proof_test` | 69/69 |
| `webgpu_volume_unified_quadrature_test` | 15/15 |
| `webgpu_object_test` | ALL OK |
| `webgpu_ontomath_native_resolution_parity_test` | PASS; 1280×720, zero mismatched pixels |

All five final native targets executed with a GPU and **no skips**. Their complete logs remain external; result tails and exits are in the durable record. Existing witnesses cover clocks, numeric edits, unchanged/crossing compilation, parameter/proof residency, removal/reordering, density/extent changes and device recreation. They are focused evidence, not a complete-suite result.

**Original defective helper versus final power-only repair:** all **32 RGBA comparisons at 2560×1440 are byte-identical** across the two frozen Zones, four views, two times and both integration modes. This is finite-scene preservation evidence; synthetic numerical cases intentionally correct the old defective behavior. Final `SdfWgsl.cpp` equals the retained power-only reference. Compiler header, renderer and proof sources are restored; original CPU ScalarForm source, frozen saves and both supplied probe sources match their starting hashes.

The checkout WebGPU app is rebuilt; no installed app or live save is changed. Native tests require a desktop GPU; an unavailable-device skip is not counted as a pass. Full-suite testing, a deterministic live 60-second route at Zach's actual usual physical framebuffer, certified output tails and Person appearance/movement acceptance remain outstanding. The 30/60 FPS targets remain unverified.

## Durable evidence and reproduction

The [machine-readable record](VOLUME_POWER_AND_OPTIMIZATION_TRIALS_2026-10-15.json) carries final source/input fingerprints, image comparisons, all primary timing samples, candidate dispositions and final focused-test outputs. Large source/binary/raw RGBA/log artifacts remain at `/private/tmp/earthcall-volume-demand-20261015/`; temporary storage is not promised durable. Rejected implementations are archived there for review, not left behind feature flags in the app.

Relevant artifact directories: `rejected-demand-source`, `binaries`, `images`, `cost`; `rejected-macro-source`, `macro-final-binaries`, `macro-final-images`, `macro-final-cost`; `rejected-alias-source`, `alias-binaries`, `alias-codegen`, `alias-images`; and `final-power-binaries`, `final-power-images`. The `subnormal-first-*` logs preserve the initial native numerical failure.

`python3 -B scripts/volume_traversal_experiment.py --help` documents the retained separate harness. Build the checkout app first. Build mode takes an explicit compatible directory containing frozen `SdfWgsl.cpp` and `WebGpuRenderer.cpp`; the caller must also ensure header/core ABI compatibility. It copies sources, records real compile/link commands and hashes binaries. Image/cost invocations reuse those binaries and verify current source/input fingerprints. Output must be a new external directory; existing outputs refuse. Cost additionally requires the separate passing image report for the same dimensions/binaries. No `bash -c`, cleanup command, live application hook or save write is involved. A new structural candidate still needs its own focused lifecycle tests and admission argument.

## Next boundaries

Retain the power correction and this negative economic/fidelity evidence. Reconsidering a rejected transformation needs a new structural reason and fresh gates. The next mathematical work is narrow Euclidean range coverage with explicit consumer scope; changing shared `evalRange` can immediately strengthen existing native consumers, so unsupported backend/scope contracts must remain Unknown. Dependency-directed changed-tree repair also remains unfinished. Neither omission is disguised as a completed plan, and unresolved earlier AD-30–36 have no new verdict here.

**Signed:** Codex / GPT-6.1 Sol (Sixth-one Sun) / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T05:55:11.285893-07:00 (PDT)
