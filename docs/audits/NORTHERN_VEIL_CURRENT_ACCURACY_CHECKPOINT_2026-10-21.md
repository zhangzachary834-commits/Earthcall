# Northern Veil: a current-accuracy shipping checkpoint and three rejected execution screens

**Scope correction after Firmament review:** every historical native timing/image screen below used a **volume-only compatibility-source fixture**, whose source binding omitted the root's authored radiance expression. These are valid paired results for that input, not the app's authored-source workload or full inhabited frame. The fixture is now corrected; see the appended source-projection record. No rejected candidate is revived by this correction.


**Commission and human decision.** Zach explicitly accepts accuracy about the same as (or better than) the current renderer if a simpler ray-transport path gives a substantial native performance improvement. He does not require the entire ray-transport campaign in one PR. His purpose is inhabiting Northern Veil. This changes the shipping prerequisite: completion of the general certified-integral campaign is not required before a bounded, measured native optimization may ship.

**Result of this pass:** the checkpoint contract is narrowed, but **no new performance optimization is retained**. Three execution screens fail their speed gates. Production source is restored byte-for-byte and the restored binary reproduces the frozen full-resolution capture. These negative results are a bounded preflight, not completion of Zach's requested performance outcome.

This continues [Zach’s scope decision in the existing ray-transport Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#zach-authorizes-a-bounded-native-performance-checkpoint--current-accuracy-is-sufficient).

## The smaller shipping contract

The current production renderer is the performance and accuracy baseline. Preserve authored equations, scene content, viewport, source/medium channels and clocks, gates/clamps/extinction floor, scattering, shared attenuation through overlaps, clipping, output-budget/cutoff behavior, and the meanings of existing quality controls. Do not manufacture a gain by lowering resolution, turning off contributions, changing lighting, lowering a quality setting, or loosening an authored error budget.

A different integration algorithm may use a different set of nodes while maintaining the current quality contract. This is distinct from simply lowering `volumeSamplesPerChord`. The author has permitted current-or-better approximation quality; it is inappropriate to require completion of every general-purpose proof feature as a prerequisite. It is equally inappropriate to call a visibly degraded image equivalent merely because a byte threshold was convenient.

For an arithmetic/scheduling-only change, prefer exact native comparisons. For a numerical-method change, compare **both methods** with an independently converged authored-equation reference, including peak and aggregate RGB/transmittance errors, thin features, overlapping curtains, camera/time changes and temporal stability. Establish convergence of that reference; the conditional grid's refused rays are not an oracle. Keep the old path for unsupported expressions/states. Finite witnesses establish the tested envelope, not universal hardware equivalence.

Use a substantial-gain engineering target of **at least 20% lower native inside-view frame cost**, repeated in controlled paired series, without repeated regressions elsewhere. This is a proposed implementation target, not an already-measured gain or a number supplied by Zach. The lower 5% screen used below was only to decide whether a local experiment warranted broader validation. The existing live 30 FPS movement milestone remains separate: a 20% reduction from a true 20 FPS workload would yield about 25 FPS if everything else stayed equal, not 30.

A PR for this checkpoint may leave the full generic integral/error campaign open. Mathematical meaning, any reusable reduced representation and integration derivation stay with OntoMath; Screen lowers that mathematics and manages device execution. Do not create a renderer-private mathematical gift shop. No hardware-model allowance is silently tightened to pass a speed or accuracy gate.

## Reproducible native screen

Branch `claude/ray-domain-transport-rung0`, worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`, baseline `6bcbb1798`. Fresh `earthcall_webgpu` build, native desktop WebGPU/Metal, configured Debug host objects. This is GPU-dominated offscreen evidence; CPU preparation/submit remain recorded separately. No Release FPS or live movement claim is inferred.

Read-only `scratch/probes/volume_demand_probe.cpp` loads the existing frozen `saves/zones/Northern Veil/zone.ecform` once. Four original curtains, 2560×1440, fixed time 7.25, inside eye `(0,50,75)` toward `(40,62,110)`. Each candidate has exactly two sequential A–B–B–A series, 24 timed frames/run after four warm frames and a completion fence. Cost runs perform no image readback or instrumentation; each batch ends with a GPU completion fence. All **24 runs / 576 timed frames** are retained. Ordinary timestamp queries remain disabled.

Every warm run records zero program compilation, lowering, parameter upload and proof upload. No GPU workload from these probes runs concurrently. Baseline run means vary substantially across the sequence; these are paired measurements, not attribution of thermal/driver causes. Absolute times are not causal comparisons with historical 136–141 ms series or Zach's live ~20 FPS.

| Candidate | Baseline / candidate pooled ms | Separate series cost changes | Full-resolution inside image | Decision |
|---|---:|---:|---|---|
| Lazy shared directions within one sample | 100.706 / 110.013 | +10.710%, +7.983% | all 14,745,600 bytes identical | reject; +9.242% pooled |
| Complete one Perlin Z-plane before the other, no cache | 99.121 / 99.118 | +0.247%, −0.210% | all bytes identical | reject; flat, −0.002% pooled |
| Original fused trace in 8×8 compute + full-resolution composite | 101.803 / 102.569 | +0.826%, +0.690% | 15,942 bytes differ, max 1 level; mean absolute byte difference 0.001081 | reject; +0.752% pooled and not exact parity |

The direction trial retains original expressions for deltas, lengths and normalization and reuses their values only among positive media at the same `worldP`. It adds sample-local state. Register pressure is a plausible explanation, **not observed hardware-counter evidence**.

The Perlin trial retains corner hashing, gradient arithmetic, Taylor normalization, lane mapping, dots, fade and mixing, but finishes one Z plane before constructing the next. No gradient/hash cache, numerical approximation, table or new authored state is introduced. Fewer visibly simultaneous temporaries do not establish different driver register allocation; this screen is flat.

The compute screen copies the production renderer into scratch, translates the fused fragment calculation into a compute entrypoint, replaces discard with a zero contribution, and composites its premultiplied result through RGBA32Float. All original source/medium upload, GPU time preparation, zero proofs, sample coordinates, gates, accumulation and cutoff remain in the path. Extra image traffic is roughly 59 MB written and 59 MB read at this size, plus a dispatch and fullscreen draw. It keeps the same field/integration work and is not faster. The scratch resource registry is deliberately process-owned and fixed-size: it is **not** production lifecycle/resize code and must never be copied into canon. The one-level differences were not investigated further because this candidate already fails speed; no accuracy-reference or visual acceptance claim follows from them.

## Restoration and next implementation boundary

Both source edits are reverted to the frozen baseline. The production renderer was never edited for the compute screen. `earthcall_webgpu` rebuilds; a fresh restored fixture binary matches the frozen full-resolution baseline byte-for-byte. The fixture SHA-256 is unchanged. No full suite is claimed or needed to approve a production change: there is no retained production change. No Person-visible change was made, so no new Person verification task is created; existing movement/appearance checks remain open.

The next performance preflight must show an actual reduction in field or integration work before investing in another backend/lifetime rearrangement. The existing ray-synthesis components and empirical quadrature oracle are useful starting points, but the oracle's ~8.5 field calls cannot be quoted as the cost of complete RGB transport: projected gates/floors/scattering and different-colour overlaps still need the native consumer. A reduced or estimated-error transport candidate can now be evaluated against the current accuracy contract without first finishing the proof-grid campaign. It must carry preparation, fallback, overlap, compositing and lifecycle cost into the comparison. Do not assume that relaxing the certification prerequisite itself supplies a faster algorithm.

**Artifacts:** `/private/tmp/earthcall-native-checkpoint-20261021/`: frozen source/baseline/provenance, exact baseline and candidate images, restored binary/capture, all cost logs and JSON, rejected plane source, scratch compute renderer and its generating script. `make_compute_trial.py` and `compute-WebGpuRenderer.cpp` are isolated experiment artifacts, not production helpers. The original probe, installed app, live data home, saves, defaults, budgets, sample control, lighting and timestamps are untouched. No PR or remote push is made.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T01:41:32.155806-07:00`.


## Firmament review: source-projection repair and corrected baseline scope

Zach alerted me to new counsel on `sync-from-earthcall-main`. I read the main-thread reply and standalone `FIRMAMENT_COUNSEL_ZERO_BOOKKEEPING_1D_RAY_SYNTHESIS_2026-10-21.md` and relayed the original onto the ray branch, preserving attribution; the signed response is in both existing Intercom thread copies. The default-branch source counsel was uncommitted when read; local delivery does not establish remote publication.

The proposed scalar optical-depth reduction is worth preflighting under Zach's smaller checkpoint. Its raw identity is real; projected density gating/extinction floor, source conditions, different-colour overlaps, clocks and invalidation still determine native eligibility. The 2.5–3.5× forecast is not measured. The Simpson oracle discovers support with a 20,000-interval scan and picks work against the reference on an 8×8 ray sample/time 0; it is not free support discovery, complete RGB integration or an executable error/admission policy. The current proof grid is already geometry-shaped with a hierarchical block prover, not a flat 32×32×32 grid. Logical cells occupy 24 bits per f32 word, so proposed logical-bit counts are not physical payload bytes. The detailed reply corrects these premises before implementation.

**A real benchmark omission:** EngineRender projects a light's authored radiance/chroma/angular expressions and its source clock. The old `volume_demand_probe.cpp` supplied only light coefficients, leaving radianceExpr null. That selects compatibility radiance instead of Northern Veil's root constant-zero radiance recorded in §6z. The old 136–141 ms historical runs and this audit's three original screens also draw no surface SDF objects: frame contains only beginFrameOffscreen/composeVolumes/endFrame. They cannot be decomposed as a 24-object surface scene plus volume pass. Their native source workload differs from the authored app projection. I should have made that distinction in the original report.

**Repair, fixture only:** default or `EC_VOLUME_SOURCE_MODE=authored` now projects root radiance/chroma/angular, verified authored revisions, and fixed source time/delta as EngineRender does. Explicit `EC_VOLUME_SOURCE_MODE=compatibility` retains the old workload and prints `compatibility-counterfactual`. This frozen fixture still admits the one root light and four curtains; it does not become a complete inhabited Engine scene or surface/occluder timing harness. No production renderer code changes.

Native verification at 2560×1440 inside/time 7.25: explicit compatibility exactly reproduces the frozen old capture (all 14,745,600 bytes); explicit authored and default authored captures exactly match each other and differ from compatibility. Authored logs report radiance expression present, chroma/angular absent. The fixture's save hash is unchanged. One authored-source baseline observation with 24 timed frames after the existing warmup is **79.706 ms**, prep 0.815 ms / submit 0.422 ms; final warm compile/lowering/parameter/proof counters are zero. This single observation changes input semantics; it is **not a paired optimization gain**, isolated GPU duration, Release claim or live FPS. Artifacts are `source-projection-verification.json`, `authored-source-baseline-observation.json`, `authored-source-probe` and `source-*.rgba` under the same scratch root above.

The next numerical preflight must use this authored-source projection (and label any counterfactual) and measure how much work actually admits the proportional reduction before assuming four exponentials per pixel. Current-or-better native accuracy, fallback and total cost still gate promotion. **No new shipped gain, save/data-home/app/default/quality/lighting/timestamp change or Person acceptance.**

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T01:55:38.339904-07:00`.
