# The Merry Dancers ≥30 FPS — Ray-Domain Transport Milestone

**Status:** commissioned / open  
**Date:** 2026-11-03  
**Human commission:** Zachary Zhang — “NEXT MILESTONE FOR OUR RAY DOMAIN TRANSPORT BRANCH MAKE THIS ZONE ALSO 30FPS OR MORE”  
**Codification:** GPT-5.6 Sol — Original Prism Sun  
**Branch:** \`prism/general-volume-cohort-architecture\`  
**Primary Zone:** \`The Merry Dancers\`  
**Read beside:** [Ray-Domain Transport Compilation](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md), [The Merry Dancers](../Agenda/Tasks/Specific%20Tasks/Zones%20and%20Ourverse/The_Merry_Dancers/The_Merry_Dancers.md), [General Volume Transport Cohort Architecture](../architecture/mathematics/GENERAL_VOLUME_TRANSPORT_COHORT_ARCHITECTURE.md), and [Performance as Rightly Ordered Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md).

---

## 0. Milestone

The next substantial ray-domain transport milestone is:

\[
\boxed{\text{The Merry Dancers} \ge 30\ \text{FPS in the live native app}}
\]

without buying that number by diminishing the authored world.

This is not a benchmark-only target. It is a Person-facing inhabitation target.

The Northern Veil campaign established that authored volumetrics can be made fast enough for a Person to move beneath them. The Merry Dancers raises the difficulty: a far wider auroral composition, a radiance-field sky, mountains/ridges, spruces, a reflective lake-like mirror world, and volumetric sheets all participate in the same view.

The engineering question is therefore no longer:

> Can one special four-curtain field family reach 30 FPS?

It is:

> Can Earthcall's ray-domain architecture carry a richer whole Zone at 30 FPS or better while preserving the exact authored scene?

---

## 1. Current baseline and evidence boundary

The current dated offscreen probe at 2560×1440 reported:

| Canonical view | Existing measured cost | Approximate rate |
|---|---:|---:|
| North — lake + sky | 162 ms | 6.2 FPS |
| Zenith | 115 ms | 8.7 FPS |
| Northeast | 101 ms | 9.9 FPS |
| Northwest | 100 ms | 10.0 FPS |
| South | 60 ms | 16.7 FPS |

These are **not** current live-window FPS claims. They were Debug-host, offscreen, readback measurements, and the recorded build may have included concurrent renderer edits.

The current source record also reports:

- media: roughly 56–87 ms;
- sky: roughly 26 ms;
- shore: roughly 14–19 ms;
- spruces: under 7 ms;
- an earlier four-medium Merry Dancers form measured routed transport slower than Current:
  - north media: ~125 ms routed vs ~67.5 ms Current;
  - zenith media: ~108 ms routed vs ~63 ms Current;
- the current Zone has three media and remains on Current;
- one suspected cause is a sheet envelope 4–6× wider than Northern Veil's;
- repeated Let expansion has already been identified as a cost multiplier;
- \`MathNode::typeOf\`'s exponential bug has been fixed separately.

Therefore the first act of this milestone is **fresh attribution**, not assuming Northern Veil's winning path will transfer unchanged.

---

## 2. Acceptance contract

The milestone is accepted only when all of the following hold.

### A. Live Person performance

In a Release build of the actual Earthcall native app, at the Person's normal/default native window settings:

- **North / lake / aurora hero view:** ≥30 FPS sustained.
- **Zenith / corona view:** ≥30 FPS sustained.
- **At least one oblique horizon view:** ≥30 FPS sustained.
- **Continuous movement / turning through the Zone:** remains at or above the intended 30 FPS class without repeated long stalls or pathological frame spikes.

The north view is mandatory because it currently combines the hardest visible obligations: reflected world, aurora, sky, and landscape.

This milestone is stronger than Northern Veil's earlier one-view checkpoint. It aims to make the **Zone**, not one selected camera, inhabitable.

### B. No resolution retreat

Do not lower native framebuffer resolution, render scale, ray density, or screen-space sample density merely to meet the number.

A later authorable quality policy may exist, but it is not the proof for this milestone.

### C. No authored-world impoverishment

Do not win by deleting or materially simplifying:

- the auroral sheets;
- the lake/mirror-world effect;
- the radiance-field sky;
- the distant mountains/ridges;
- the stars;
- the foreground spruce silhouettes;
- the intended color hierarchy;
- the current spatial scale and breadth of the composition.

The engine must become a better vessel for the world, not make the world poorer for the engine.

### D. Fidelity

Every retained optimization must satisfy the established numerical/rendered contract against Current or a stricter independent reference.

“Looks similar” is not the numerical gate.

### E. Honest fallback

Unsupported, stale, unproved, overflowing, or dynamically invalidated cases retain an authoritative general path.

No partial ray or cohort may publish as complete.

### F. Live visual acceptance

Zach's direct live verdict remains distinct from numerical equality:

- aurora still appears breathtaking;
- mountains and sky still render at intended distance;
- reflection/lake remains coherent;
- no clipping, missing fields, stale frames, seam artifacts, or cohort-order errors appear while moving.

---

## 3. Performance budget

30 FPS corresponds to approximately:

\[
33.3\text{ ms/frame}.
\]

The current north probe's 162 ms implies roughly an 80% total-cost reduction relative to that historical measurement.

That number is not a promised required reduction against the current live Release build; the baseline must be remeasured first.

The budget should be treated as a whole-frame allocation problem:

\[
T_{\text{frame}}
=
T_{\text{media}}
+
T_{\text{sky}}
+
T_{\text{surfaces}}
+
T_{\text{composition}}
+
T_{\text{host/backend}}
+
T_{\text{other}}
\le 33.3\text{ ms}.
\]

No subsystem may claim victory by moving cost into another unmeasured stage.

---

## 4. First investigation order

### P0 — Freeze a real contemporary baseline

Capture the actual current branch, Release configuration, native surface dimensions, camera transforms, clock, and Zone save bytes.

Record:

- full-frame median;
- p95 frame cost;
- north;
- zenith;
- NE/NW;
- south;
- one turning path;
- one movement path;
- Current vs any routed/cohort candidate;
- complete CPU + GPU wall time;
- no readback/instrumentation in the clean timing lane.

Keep a separate correctness capture path.

### P1 — Attribute the whole frame

Do not begin by optimizing the historical suspect.

Measure the current complete frame and isolate at least:

- volumetric media transport;
- sky/radiance-field evaluation;
- reflection/mirror obligations;
- distant SDF/surface rendering;
- foreground surfaces/spruces;
- composition/present/host work.

The new 100-distance SDF bugfix changed the visible world, so old cost attribution cannot simply be carried forward.

### P2 — Explain why routed loses on Merry Dancers

The first major ray-domain question is now an adversarial generality witness:

> Why did the routed transport that won Northern Veil become ~1.7–1.85× slower on the earlier wide-sheet Merry Dancers form?

Do not assume the answer.

Test hypotheses independently:

- wider support envelopes causing excessive route/proof work;
- too many routed intervals/panels;
- poor empty-space economics;
- route producer cost dominating consumer savings;
- divergent long-tail rays;
- general-expression lowering overhead;
- parameter / Let duplication;
- composition or memory traffic;
- camera-inside / grazing-ray behavior;
- cohort partitioning;
- reflection doubling the effective media burden.

A successful architecture must be able to **refuse routed execution when Current is cheaper**, even if routed is mathematically admissible.

Mathematical admission and economic admission are different facts.

### P3 — General cohort planner, not Zone-specific routing

Implement the branch's [General Volume Transport Cohort Architecture](../architecture/mathematics/GENERAL_VOLUME_TRANSPORT_COHORT_ARCHITECTURE.md):

\[
\text{Authored Fields}
\to
\text{Capability Facts}
\to
\text{Candidate Cohorts}
\to
\text{Separability + Order Proof}
\to
\text{Schedule}
\to
\text{Specialized Execution}.
\]

The Merry Dancers is the first major stress witness because its media are not the same family as Northern Veil.

Required negative:

> permuting \`spatialFields\` must not change physics, cohort assignment, or final pixels.

### P4 — Query-relative support as the dominant work filter

The Zone's wide sheets make this the likely next frontier.

A wide authored bounding box must not imply expensive evaluation everywhere inside it.

Push the proof boundary toward the actual contributing support along the current query:

- conservative support intervals;
- zero-density regions;
- per-ray / per-tile relevance;
- horizon/zenith-specific occupancy;
- interval chronology;
- support-lifetime reuse.

The aim is not “smaller boxes.” It is:

> carry only the part of the authored field this ray can actually encounter.

### P5 — Residual-family expansion beyond Northern Veil

Northern Veil's EMPTY / S1 / Ordered-S2 / Fallback structure may not match Merry Dancers.

Derive residual families from Merry Dancers' actual ASTs after line restriction.

Do not force the new Zone into the old family taxonomy.

The architecture should discover new profitable species if the mathematics warrants them.

### P6 — Stop repeating shared expression work

The Zone already exposed one important structural defect:

> Let is textually inlined at every use.

This milestone should measure and remove repeated channel-independent work where valid.

Candidate targets:

- one shared altitude spectrum evaluation;
- shared density/emission gates;
- common spatial phase/fold terms;
- common color weights;
- source-independent subexpressions;
- ray-invariant preparation;
- time-only preparation;
- medium-shared or cohort-shared terms.

Sharing must preserve binding identity and revision validity.

### P7 — Reflection / mirror-world transport

The north view is currently the hardest because the reflected world and aurora share the frame.

Investigate whether the mirror symmetry permits a more lawful representation than evaluating two unrelated worlds.

Potential direction:

\[
\text{mirror transform}
+
\text{shared authored field}
\to
\text{transformed query}
\]

rather than duplicated evaluation.

This must be proved from the Zone's authored symmetry. Do not hardcode “water reflection” into the renderer.

### P8 — Radiance-field sky path

The sky already costs on the order of tens of milliseconds in the historical probe and is evaluated once per pixel.

Ray-domain transport cannot declare the milestone won while a separate per-pixel radiance field consumes the frame budget.

Inspect:

- expression sharing;
- directional parameterization;
- static-vs-time-varying terms;
- horizon-band conditional demand;
- stars/ridges/fan separation;
- compiled expression form;
- possible Direct-Screen / radiance-field specialization.

This remains the same principle:

> compile perception according to the authored mathematics actually demanded by this pixel.

### P9 — Full-frame scheduling and physical placement

After eliminating unnecessary arithmetic, measure whether remaining cost is memory, occupancy, dispatch, or synchronization bound.

Then apply:

- retained programs;
- stable parameter residency;
- bounded GPU-resident continuation where measured;
- compaction/re-cohorting only where profitable;
- fewer intermediate buffers/passes where lifetime permits;
- execution ordering by actual producer/consumer demand.

Do not invent generic scheduler machinery before a measured bottleneck asks for it.

### P10 — Inhabited qualification

After native numerical qualification:

1. launch the actual Release app;
2. enter The Merry Dancers;
3. stand at the shore facing north;
4. look zenith;
5. turn through oblique and south views;
6. walk/fly through the Zone;
7. record live FPS and frame pacing;
8. let Zach judge the image.

The final gate belongs to the Person inhabiting the place.

---

## 5. New architectural distinction discovered by this milestone

Northern Veil established:

\[
\text{mathematical admission}
\neq
\text{fallback}.
\]

The Merry Dancers adds:

\[
\boxed{
\text{mathematical admission}
\neq
\text{economic admission}
}
\]

A route may be correct and still be the wrong execution choice.

Therefore future execution-family selection should eventually use two independent gates:

1. **Correctness/capability gate:** may this field/query execute through this family?
2. **Economic gate:** is this family actually cheaper for this workload on this substrate?

The economic gate may use retained measured characteristics of structural/query classes, but it must not silently redefine mathematical truth.

Correctness decides what is allowed.

Economics decides which allowed path to choose.

---

## 6. Anti-cheat list

The milestone does **not** count if 30 FPS comes from:

- lowering resolution;
- shrinking the Zone;
- deleting distant mountains;
- removing reflection;
- reducing aurora coverage;
- turning dynamic authored math into a baked screenshot;
- hardcoding Merry Dancers identifiers into WebGPU;
- assuming serialization order carries transport semantics;
- changing the numerical method without an accuracy gate;
- reporting volume-only cost as whole-frame FPS;
- reporting an offscreen probe as Person live FPS;
- reporting one transient frame over 30 as sustained performance.

---

## 7. Victory condition

The campaign is complete when Zach can enter The Merry Dancers in the live native Release app, look across the lake into the auroral horizon, look straight up into the corona, turn through the surrounding sky, and move through the Zone while the world remains visually whole and the frame rate remains **30 FPS or better in the accepted representative views**.

The point is not that the renderer has become impressive.

The point is that the renderer becomes quiet enough for the Person to attend to the sky.

\[
\boxed{
\text{The world must remain itself, and it must arrive in time.}
}
\]
