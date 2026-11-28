# Northern Veil checkpoint: pre-merge correctness audit and bounded repairs

**Codex / GPT-6 / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-11-02 (America/Los_Angeles).** Human commission: Zach reported consistent **30 FPS near spawn, 35 FPS in some views, and approximately 30 FPS while flying up with an unobstructed Veil view**, then requested a final correctness audit and structural repairs without brute overhead. This continues the [Opus handoff thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sixth-one-sun-premerge-correctness-2026-11-02), [Firmament's P19 review](FIRMAMENT_REVIEW_OF_SPARKLY_P19_30_35_FPS_LIVE_MILESTONE_2026-11-02.md), and [the earlier P1–P11 review](P1_P11_STRUCTURAL_REVIEW_2026-11-01.md).

## Outcome

**The bounded Northern Veil checkpoint passes this corrective audit.** Two reproduced GPU defects are repaired, unsafe coordinate/grouping admission is tightened, focused tests pass, and the measured advantage over Current remains. The full arbitrary-expression/error-certification campaign is still open. No merge or push is performed by this pass.

Source reviewed was P19 at `634194d04`, followed by docs-only successors `2e2bdf5e0` and `dc4c15961`; the inspected kernel/transport/CPU/native-probe files were byte-identical across that handover. Work is in the actual `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport` checkout on `claude/ray-domain-transport-rung0`. Default renderer source was not edited.

The three P1–P11 defects were already corrected in P12. This pass examined P12–P19 classification, ordered/multi-member routing, root enclosure, band absorption, physical support clipping, shared evaluation/private storage, producer/consumer addressing, publication/refusal ownership, viewport/resource lifetime, and the witnesses' actual claims. Normal producer and consumer step-major addresses agree. The three copies of the root/proof helpers normalize to identical functions; the regression executes each actual WGSL copy independently.

## Repairs

### 1. Unresolved zero is an enclosure, not a directional sign

P18/P19 only stopped on `abs(v) <= error` when the resulting band was at most `0.0095` wide. Otherwise an exact `v == 0` advanced a bracket endpoint, erased the strict sign at that endpoint, and could march to the opposite end while publishing success.

Independent example: `phi(s) = -0.5 + s/16`, bracket `[0,16]`, exact root **8**, error `0.001`. The old producer returned approximately **[15.9999924,16]**, reason zero. This is not a tolerance dispute: its reported enclosure excludes the exact root.

Repair: always retain `[max(l,mid-w), min(r,mid+w)]`, with the existing `w = 2*error/curMinSlope`, and stop when the sign is unresolved. Secant interpolation, damping, local slope tightening, bit scanning, streaming, capacities, and refusal machinery remain. **No field evaluations or subdivision passes were added.** The CPU example now returns approximately `[7.968,8.032]` after one field evaluation. Native exact linear roots include both signs and shallow slopes.

### 2. Proof slack cannot enlarge authored physical boxes

The clipped slab used `min(physicalHalf,supportHalf)` and then added `curv.y` to the result. After the consumer's per-sample physical-box check had been removed, that expanded the authored medium itself.

Independent native fixture: a physical half-extent `0.01` crossed from unit distance should be confined to **[0.99,1.01]**. Every old producer emitted spans beginning near **0.989**, outside the physical box. The final regression exercises all six directed box faces.

Repair: compute **`min(physicalHalf, supportHalf + slack)`**, then use that extent directly in the existing slab test. Derived support remains conservative; the authored box stays hard. There is **no added per-sample AABB test or GPU dispatch**. CPU mirrors use the same corrected domain.

### 3. Unknown grouping or coordinates retain Current

`clampedWaveGroupingSlackFits` lives in OntoMath and accepts a supplied coordinate bound; it contains no renderer/Zone identity. It uses `gamma_8` and coefficient magnitudes to reject an additive-tree discrepancy too large for the existing support slack. The IEEE witness with `A=2^25` produces different f32 results for `(z+A)-A` and `z+(A-A)`; this unsafe case refuses, while the ordinary Veil range fits. This is a necessary regrouping guard, **not** a complete transcendental or transport-error certificate.

Renderer coordinate admission now requires the bounded comparison to be true, so NaNs cannot slip through a negated greater-than check. These checks run during the existing per-medium host admission, not inside each ray/sample. The final native correctness run still admits the Veil and reproduces the measured repaired run's complete pose-0/32 parity/publication records exactly.

## Executed evidence

Machine-readable evidence and hashes: [summary](data/ray_transport_premerge_2026-11-02.json). The evidence files distinguish original kernels, repaired kernels, and final host-admission source.

- **Permanent independent regression:** `webgpu_routed_transport_math_test` compiles actual WGSL functions/entry points from all three production producers. Final result: **0 missed roots / 48 root cases; 0 out-of-box rays / 48 producer-ray instances**, plus grouping admission checks. It does not approve itself by comparing two mirrors.
- **Negative control:** the same final regression compiled against the saved original kernels fails with **30 missed roots and 48 out-of-box rays**, exit **42**. No gate or tolerance was widened. An initial shell wrapper used zsh's read-only `status` name; the complete reports were inspected and the expected process exit was independently reconfirmed.
- **CPU trajectory:** all 64 poses; **921,600 tile frames; 878,400 S1 rays; 576,112 S2 rays**; zero classifier/producer mismatches and zero refused rays. This is route/schedule evidence, not an independent integral reference.
- **Native production renderer:** full baseline and repaired runs each pass the existing **10/10 gates**, including poisoned storage, forced-Current parity, empty-strip transitions, overlap, tall viewport/resize, checkpoint views and orientation sweep. A final current-source run repeats **8/8 correctness gates**, explicitly omitting repeated timings; its parity records equal the measured repaired run. It is not labeled a second 10-gate timing run.
- **CTest, final build, sequential:** `webgpu_routed_transport_math_test`, `webgpu_volume_zero_proof_test`, `webgpu_volume_support_certificates_test`, and `webgpu_line_quadrature_test`: **4/4 pass**. CMake was reconfigured after adding the permanent test. Release app cache is `Release`, `-O3 -DNDEBUG`; no installation or live-save mutation was performed.

### Independent authored-equation check

The repairs change native image bytes, so old hashes were not treated as an accuracy oracle. Fresh **24 native captures** cover the existing four scenes (inside, turned, outside, opaque-front). The original **320 frozen pixel pairs** and tolerances were retained. Original converged `2^16` / `2^15` CPU references were reused only after checking f32-bit equality of every ray input, plus Zone/time/camera, reference generator and ScalarForm source hashes.

**All 162 scoped mean/p99/max checks pass**, including own-input RGB/T, common ideal-camera comparisons, and actual unmodified Screen comparisons. Reference convergence is at most `1.052e-7`. No candidate sample exceeds Current's corresponding cohort maximum error. Example own-input max RGB errors at `2^16`:

| Scene | Current | Repaired routed |
|---|---:|---:|
| Inside | 0.00239756 | 0.00004832 |
| Turned | 0.00134207 | 0.00007203 |
| Outside | 0.00159139 | 0.00001816 |
| Opaque-front | 0.00239756 | 0.00004832 |

The first temporary observer attempt refused because it copied buffers inside an open coalesced compute pass. The temporary observer was corrected to split the pass around its copies. Production shaders were unchanged by observation. All full-frame Screen equality and full-publication hash controls passed afterward. This instrumentation is not shipped.

The retained [selected native rows and 24 capture receipts](data/ray_transport_premerge_selected_2026-11-02.json) support numerical replay against the existing §6ew reference archive; full temporary captures remain under `/private/tmp/ec-premerge-20261102/reference-capture-v2`. **This is the existing bounded pilot, not a full-population maximum or pointwise-dominance claim.** The arbitrary-expression certificate campaign is not closed by it.

### Complete-cost screen

Zach closed Earthcall before measurement. Frozen Release binaries used the same existing completion-fenced ABBA harness and scene inputs. No GPU tests ran in parallel with these timing screens.

| Group | Pre-repair routed ms | Repaired routed ms | Contemporaneous repaired-run Current ms |
|---|---:|---:|---:|
| All 64 poses | 59.13 | **57.73** | 83.80 |
| Dense/full-screen group | 69.94 | **68.12** | 102.61 |
| Open-sky group | 47.62 | **46.66** | 63.79 |

The repaired run wins **64/64 poses**, with **31.12% lower mean frame time than Current** overall. All seven checkpoint and twelve orientation views also retain a Current advantage. Captured live-view A is **28.4 FPS** in both runs; sparse View 3A is 45.3 FPS after repair. These are native offscreen measurements, not fresh Person acceptance of the rebuilt application. The small pre/post timing differences are not promoted to a new speedup claim; neither group shows an aggregate repair regression in this screen.

## Handoff

The Release executable in this checkout is rebuilt. Zach's reported live 30–35 FPS milestone is recorded as Person-observed evidence before these repairs. A brief look at the corrected build can confirm the already-accepted inhabited view remains visually sound; it need not become a new campaign of every camera/roll angle. Current remains the fallback and feature-branch routed default remains on.

No other concrete rendering blocker was found within this bounded checkpoint review. Remaining universal error certification, full-population reference closure, broader devices/scenes, and the separate multi-app investigation retain their prior scope. Incidental Intercom navigation failure from preexisting unversioned status rows in `conversation_links.jsonl` is reported to the council; that journal and historical thread contents are not rewritten by this renderer repair.
