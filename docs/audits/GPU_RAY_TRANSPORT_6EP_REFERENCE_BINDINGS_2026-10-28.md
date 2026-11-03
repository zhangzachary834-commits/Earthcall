# §6ep — worker-local reference bindings: exact parity, measured modest savings

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28, America/Los_Angeles; exact timestamp in the machine record.

Zach requested the next pass after [§6eo](GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.md). This implements and tests **Aetherion / The Firmament's worker-local binding-map proposal** from his [post-§6en counsel](FIRMAMENT_COUNSEL_POST_6EN_AND_SONNET_MERGE_WHY_MODE2_WON_AND_6EO_BRIDGE_PLAN_2026-10-28.md), continuing the existing [Opus handoff thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). His proposed5–10× gain and linear thread scaling were hypotheses, not acceptance criteria or measured facts.

**Result:** all six bounded CPU runs reproduce the independent canonical controls exactly. Binding reuse reduces the fresh256-ray process from59.958s to46.316s (**22.75% less wall time** in this ordered pair). On1,024 identical rays the derived reference takes198.548s with four threads and128.071s with eight: **1.55× measured thread scaling**, rather than2×. The eight-thread rate projects~480.27min/**8.00hours** for230,400 rays. That full job remains unlaunched. No new GPU execution or gameplay performance is established.

## What changed, and whose idea it was

Firmament identified repeated construction of the outer eight-entry `std::map<std::string,PropertyValue>` at each sample/member visit. Each worker now constructs that map once. At the original visit site it refreshes `p`, scalar `x/y/z`, and the three directional arguments with the **same types, casts and expressions**; the shared job clock is initialized once. References to map slots remain worker-private and stable. The separate diagnostic map remains unchanged. Worker storage ends with the job; there is no cross-frame cache.

I implemented a closed, reversible two-site source transformation, pinned to the reviewed original SHA256. The canonical `veil_projected_reference.cpp` remains unchanged (`bdcdd79e…a443b1f`). The generated source has its own SHA256 (`4aa1fd5f…f23930`). Reversing the two edits reconstructs the canonical source exactly. AST calls, sample positions, support predicates, casts, floors, channel arithmetic, quadrature and output formatting remain in the original code.

The derived main is freshly compiled with the same optimization/default floating-point contract and linked to the **same optimized-first evaluator objects** used by the canonical control. No fast-math, engine edits, shared-build object grafts, rendering-specific OntoMath API or new numerical evaluator was introduced. This optimizes a scratch reference's binding adapter; OntoMath retains mathematical meaning.

## Source identity without impersonation

The Oracle wrapper formerly recognized only the canonical generator. It now recognizes exactly two cases: the current canonical source, or the source computed by the pinned worker-binding transformation with an exact derivation receipt. Callers cannot authorize another generator by providing its hash. The latter receipt records the actual derived generator hash, transformation kind, canonical parent and derived hash; it never claims that modified code is the original generator.

The pure transformation module has no build/runtime imports. This avoids adding an import-path dependency when the verifier is loaded from outside the probes directory. Unknown source hashes, missing/stale derivations, canonical impersonation, changed canonical source and a source with one refresh removed all refuse. Existing ray/fixture/clock/output coverage and convergence checks remain; the shared full-workload numerical checker and all six accuracy gates are unchanged. Supporting this mechanically checked derivation does not certify an arbitrary optimization or a different physical transport model.

## Frozen controls and actual executions

Before execution, the selection file froze18 retained continuity rays,74 uniform/stress rays,256 uniform ranks of the previous1,024-ray cohort, and that full1,024-ray cohort. **Expected values come from the separately hash-bound canonical §6eo archive**, not the derived program. A fresh canonical256-ray process additionally checks the current control binary against those retained values. All eight parsed binary64 values per ray are compared exactly, including signed zeros and both nested resolutions.

Each process is capped at300seconds. It runs the same `2^16` finest / `2^15` half reference at the original camera/origin, recorded directions/clips and shared clock7.25. Different medium clocks still refuse. Before/after source, binary, fixture, input and build identities are retained. No timed-out or changed job can produce a passing receipt.

| Actual process | Rays | Threads | Wall seconds | Exact canonical match |
|---|---:|---:|---:|---|
| Derived continuity |18 |4 |4.130 |yes |
| Derived uniform/stress |74 |4 |12.742 |yes |
| Derived fixed pilot |256 |4 |46.316 |yes |
| Fresh canonical fixed pilot |256 |4 |59.958 |yes |
| Derived cost cohort |1,024 |4 |198.548 |yes |
| Derived cost cohort |1,024 |8 |128.071 |yes |

All21,216 compared values match the canonical controls. Maximum finest/half differences across these cohorts remain6.81554679e−8 T /4.01704683e−8 RGB, below the unchanged1e−6 diagnostic refusal. This is numerical self-consistency of the existing projected reference, not a formal continuum error enclosure or an authored output budget.

The original §6eo1,024-ray canonical run took256.798s on four threads; the new four-thread198.548s observation is22.68% lower, consistent with the fresh256-ray pair. The combined historical four-thread canonical versus eight-thread derived comparison is roughly2×, but it includes both binding reuse and more threads. It must not be narrated as a2× gain from map reuse alone. These are individual ordered CPU observations with process startup/fixture load included and compilation excluded, not a repeated benchmark or confidence interval.

Firmament's5–10× proposal was not observed. Hoisting the outer map does not remove allocation inside AST evaluation. The previous local partially Debug-backed build profile also remains relevant. The measured eight-thread result disproves assuming linear4→8 scaling for job budgeting.

The pilots reuse the same**1,094 distinct new-capture rays** already referenced by §6eo, plus18 old continuity rays. **This pass adds zero new population coverage.** The other229,306 distinct captured rays still lack their new independent reference; contemporary candidate/Current outputs and whole-population accuracy/economics remain unqualified. The eight-hour extrapolation is not a measured full-run duration or a launched background job.

## Evidence, controls and reproduction

[Machine record](GPU_RAY_TRANSPORT_6EP_REFERENCE_BINDINGS_2026-10-28.json), [compact evidence](GPU_RAY_TRANSPORT_6EP_REFERENCE_BINDINGS_2026-10-28.zip), [runner/build adapter](../../scratch/probes/ray_transport_6ep_reference_bindings.py), [pure source derivation](../../scratch/probes/ray_transport_6ep_reference_derivation.py), [replay](../../scratch/probes/ray_transport_6ep_replay.py). The archive retains fixed inputs and canonical controls, actual outputs/logs/receipts, compiler/link metadata, exact generated C++, the source tools and identity controls. Canonical controls are checked back to the prior archive's original input/reference members, not trusted merely because a local file labels itself canonical.

Replay passes14 identity/exactness controls, including a re-signed one-ULP result mutation, a forged eight-thread receipt for actual four-thread arguments, and a timeout labelled as completed. The11 closed-identity/comparator/source-scope controls also run independently. All17 existing Sonnet integration controls and prior §6eo replay pass after the verifier extension. Source syntax and whitespace checks pass. No engine suite or GPU execution is claimed for these scratch Python/reference changes.

```sh
python3 -B scratch/probes/ray_transport_6ep_replay.py \
 --record docs/audits/GPU_RAY_TRANSPORT_6EP_REFERENCE_BINDINGS_2026-10-28.json \
 --archive docs/audits/GPU_RAY_TRANSPORT_6EP_REFERENCE_BINDINGS_2026-10-28.zip \
 --output /tmp/6ep-replay.json
```

For fresh execution, `ray_transport_6ep_reference_bindings.py prepare --out <fresh-work>` reconstructs the fixed controls from the prior archive. `build --base-build <verified-6eo-optimized-first-build> --out <fresh-external-build>` preserves its source-matching engine/evaluator linkage and compiles the derived main. Run `continuity`, `stress`, then `pilot256` with four threads and exact control comparison before `cost1024`; each `run` requires a fresh tag, explicit binary/selection and `--threads 4` or`8`. Full replay does not rerun those CPU jobs.

## Reply and remaining shipping account

Firmament: the storage-lifetime proposal is retained with exact source-bound witnesses. Its observed benefit is modest and useful; its5–10×/linear-scaling expectations are replaced by the measurements above. The zero-inner-allocation claim remains unsupported. The consumer/producer parameter distinction and register/occupancy hypotheses from §6eo remain as previously qualified; this reference pass supplies no new GPU causal profile.

**All three shipping gates remain OPEN:** complete distinct-population numerical/economic qualification; production Screen/depth/composition/validity connection; inhabited frame performance and Person acceptance. The next independent runtime step is the isolated **all-refused Screen publication witness**, preserving Current exactly before accepted routed results enter composition. Full-reference work now has a supported derived-generator identity and measured budget, but needs explicit bounded/resumable job scheduling rather than a silent eight-hour launch. Further oracle speed work should be driven by a measured call-path profile, not another unmeasured multiplier.

No original reference source, engine source, save, app/default, lighting or resolution changed. The Oracle provenance wrapper and scratch tools changed as described. There is no new Person-visible feature/check; existing Person checks stay open. No GPU lane was claimed and no push was performed by this pass.
