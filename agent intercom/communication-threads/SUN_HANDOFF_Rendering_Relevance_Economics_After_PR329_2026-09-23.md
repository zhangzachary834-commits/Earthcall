# SUN HANDOFF — Rendering Relevance Economics after PR #329

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
Canonical branch at handoff: `sync-from-earthcall-main`
Canonical observed at handoff: `d2cdd18ed3b3060e68fcdf3bdced2ba103e22dbf`

## Read first

1. `agent intercom/communication-threads/sdf-and-rendering/CODEX_TO_SOL_SUNS_SDF_PERFORMANCE_VERDICT_2026-09-23.md`
2. `docs/audits/rendering_optimization/2026-09-23_sol_sdf_performance_followup_audit.md`
3. `agent intercom/communication-threads/SUN_UPDATE_PR329_Final_Base_Reconcile_CI_Classification_2026-09-23.md`
4. PR #329 — Scene-Spatial Synthesis DAG Rung 1

Do not restart PR #329. It is Ready for Review and its specific Sun role is complete.

## What PR #329 earned

PR #329 established the semantic/proof substrate and then deliberately stopped before granting proof state rendering authority.

Its completed evidence includes:

- Rungs 1A–1J semantic/proof witnesses;
- real OntoMath canonical subexpression sharing;
- local dependency-frontier repair;
- proof invalidation and exact fail-open fallback;
- distinct `SourceRho` / `MediumDensity` theorem authority despite shared canonical math;
- typed chroma and Timeline behavior;
- deterministic proof-work accounting;
- production `RenderedFieldSemanticObserver`;
- Renderer OFF->ON replay of already-admitted source state;
- renderer-boundary lifecycle witness;
- V4 self-emission remaining outside the density theorem family;
- consumer sweep proving no observer theorem/cache state reaches WGSL, ray marching, visibility, volumetric accumulation, source filtering, or pixels;
- `authorityBypassesApplied == 0`.

The fully reconciled focused workflow #2881 completed SUCCESS across all four jobs: Focused CPU, SDF range-proxy verification, authored-Perlin Release A/B, and Slow Adapter.

PR #329's architecture result is therefore real. It is **not** a native Perlin FPS gain yet.

## Human architectural provenance

Zach originally pushed the renderer toward Prophetic Rendering from the same principle already used in Prophetic Rete / Formation Rete: stop rebuilding/re-evaluating what can be proven ahead of time, preserve unaffected derived structure, and incrementally adjust only what authored/runtime change actually invalidates.

The successor must preserve that intent while also respecting the economic lesson learned from the SDF line: a mathematically valid proof can still be slower than exact evaluation if discovering and consulting the proof costs too much.

## Sixth Sun verdict

The Sixth Sun's sentence is the next optimization north star:

> **The proof is true; now make the question cheap.**

Inherited evidence from the SDF line shows why.

PR #298 measured roughly:

- horizon: 83,649 proof consultations for 144 exact samples saved (~581 consultations per saved sample);
- 45 degrees: 74,686 consultations for 157 saved samples (~476 consultations per saved sample).

PR #301 further showed that a smaller apparent top-level query count could conceal huge record-test cost.

Therefore the next performance equation must charge the cost of discovering relevance, not merely count exact evaluations avoided.

## Next Sun mission

Start a **new successor branch/PR from current canonical**. Do not add the next performance experiment to PR #329.

The next Sun's mission is to determine whether the semantic/proof substrate can make rendered-field relevance reach a ray at lower cost than simply evaluating the authored field.

### First bounded rung — three-arm dormant-proof comparator

Before building a new authoritative consumer, settle whether merely carrying dormant proof machinery costs the default renderer.

Create a bounded test-only/native comparison with identical current authored semantics and warmup:

1. **NO-PROOF-SHADER** — generated shader contains no range/proof branch at all;
2. **PROOF-CAPABLE-OFF** — current proof-capable shader with traversal disabled;
3. **PROOF-ON** — current traversal enabled.

Preserve exact image/parity checks, radiance semantics, AB/BA ordering, and current proof-upload invariants.

Report separately:

- GPU frame/main-pass time;
- CPU submission/gather time where measurable;
- shader/pipeline compilation cost;
- proof/query/record-test counts;
- exact SDF evaluations;
- exact evaluations saved;
- recurring upload bytes;
- resident proof bytes.

The first question is narrow:

**Does the dormant proof-capable shader have measurable cost relative to a shader with no proof machinery?**

If NO-PROOF and PROOF-CAPABLE-OFF are equal within noise, close this question. If not, measure the structural shader-variant cost before proposing a production change.

### Second bounded rung — single authored Perlin ray economics

Once the dormant-branch comparator is settled, return to the one-field authored Perlin workload.

Do not report only theorem hits or avoided evaluations. Charge the whole relevance path:

`T_on - T_off ~= N_query*C_query + N_record*C_record - N_saved*C_exact + shader/divergence + CPU_delta`

For the current authored Perlin field, record at minimum:

- all top-level relevance queries;
- all hidden branch/record/AABB tests;
- exact samples saved;
- exact samples still executed;
- per-ray hit/miss parity;
- CPU and GPU time;
- artifact compile/repair time;
- resident bytes;
- recurring uploads.

If a multi-field/shared-DAG case wins while the single field does not, call that a distinct cross-field-composition win. Do not claim the horizon-ray problem is solved.

## Authority gate

No proof result may alter rendered pixels merely because the semantic DAG exists.

Before the first pixel-changing consumer is promoted, explicitly test:

- in-place premise edits;
- source removal/re-addition;
- allocator address reuse / stale-pointer identity;
- revision changes;
- invalid-proof exact fallback;
- rho/density cross-channel refusal;
- zero density not erasing independent V4 self-emission;
- observer/proof cache growth over long authored sessions.

Identity/lifetime semantics must be stronger than diagnostic pointer+revision bookkeeping before theorem state becomes authority.

## Hard constraints

- Authored mathematics remains the truth the renderer answers to.
- Exact evaluation remains the fail-open authority whenever proof/relevance state is absent, invalid, unsupported, stale, or uneconomic.
- No global/O(world) relevance search per frame.
- Ordinary camera/runtime movement must not rebuild semantic/proof artifacts unless the proof actually depends on that runtime variable.
- Authored changes repair only affected dependency frontiers; unaffected derived state survives.
- No Big Chungus repository dumps: use targeted reads/searches.
- Do not resurrect rejected DDA/octree/sparse-tree/mip/one-AABB/route-atlas ideas without materially new evidence.
- Do not broaden theorem families merely to show activity.
- Keep volumetric V1–V4 semantics independent: source rho, medium density, extinction, scattering, chroma, phase, and self-emission are not aliases.
- Preserve the PR #329 constitutional boundary until a separate measured consumer earns authority.

## Coordination

Other Suns may move canonical while this task runs. Before every mutation:

1. re-read current canonical;
2. inspect the active successor PR/branch and CI;
3. read the latest handoff/update;
4. reconcile concurrent work instead of force-overwriting it.

Use Agent Intercom for substantial findings, CI verdicts, rejected hypotheses, and exact next continuation points.

## Definition of done for this successor task

The successor task is not “make all rendering fast.”

It is complete when the next bounded relevance-economics rung has:

- a settled three-arm dormant-proof comparator;
- a settled single-field authored-Perlin relevance-cost measurement;
- exact parity/soundness retained;
- honest accounting of query/record-test cost;
- a clear verdict on whether there is an economically promising next consumer;
- a handoff specifying that consumer or explicitly rejecting the current road.

If the evidence says the exact marcher is still cheaper, say so and stop that road. Do not grant proof rendering authority without a measured economic case.

— Successor handoff from the PR #329 Scene-Spatial Sun
