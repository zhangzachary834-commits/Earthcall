# SUN HANDOFF — PR #320 Native Visibility × Density Reconciliation

**Date:** 2026-09-22  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Canonical landing PR:** #320 — *Prism landing after Rung 8: Volumetric V0 density on current default*  
**Owner branch:** `sol/prism-density-after-rung8-20260921`  
**Handoff from:** Visibility Sun 1  
**Successor role:** take sole ownership of #320 until its native V/D integration witness is understood and green.

---

## Do not restart this investigation

PR #320 is already the intentionally rebuilt post-#297 landing lane.

The governing constitution is:

```
rho_source != V_transport != D_medium
```

Keep those three meanings independent.

- `rho/chi/alpha` are authored radiance/source invariants.
- `V` is derived geometric source→receiver transport visibility.
- `D` is participating-medium density.

Do not collapse Density into source radiance.
Do not encode blocker state into source ASTs.
Do not treat zero-set proof as density truth.
Do not revert merged Rung-8 visibility to make V0 pass.

---

## Branch topology / Prism mitosis

### #297 — merged, authoritative Visibility baseline

#297 landed on `sync-from-earthcall-main` at:

`4ab5ebe25aaac99915d25d63770960a2e085a34b`

That merge is correct and must remain authoritative for:

- `sourceTransportSignedStep(...)`
- `sourceVisibility(...)`
- per-source `pathVisibility`
- `directRadiance`
- `lightControl.y`
- signed receiver-penetration escape
- the native receiver/self-shadow and selective blocker witnesses

Do **not** revert #297.

### #299 — Density owner/history lane

Do not merge #299 directly to current default. It predates the final #297 landing on shared renderer files.

### #312 — Prism integration laboratory

Useful as historical V×D reconciliation evidence, but it is based on #299 and is not the canonical default landing lane.

### #316 — Density witness cleanup

Its corrected witness is already represented in #320. It does not need a separate default merge.

### #318 — obsolete

Closed pre-#297 canonical landing candidate.

### #319 — Prism mitosis daughter

#319 is an earlier post-#297 landing candidate:
- head observed at handoff: `9d6625045971677b629a3e1cae09722c6c6dad7a`
- draft/open
- same broad architectural moment as #320

Do **not** merge #319 in addition to #320. The heads diverged; they are alternative landing histories, not two halves of one change.

### #320 — canonical Density-on-Visibility landing lane

This is the branch to fix.

At the start of this handoff, PR #320 was:
- open
- draft
- GitHub-mergeable
- based on `sync-from-earthcall-main`
- 14 V0/V×D implementation/test files after the Intercom thread was added

The last renderer-relevant commit before handoff Intercom-only commits is:

`c388a9c7133f820661f601b7e11cd50197cc80d5`
— **Prism: preserve merged Rung 8 signed-step helper**

After that, the observed commits were Agent-Intercom-only:
- `6647ad4fd72cbe21bf3f2d1a3e397785d140b12f` — begin PR-320 CI triage
- `154e854d0c3d2e74be3acdc3e9dc47f7df7df505` — recover swallowed PR-320 CI verdict

This handoff and Zach-prompt thread will advance the PR head again with documentation only. Therefore **always distinguish the live PR head from the last renderer/test-code head** before interpreting a new CI run.

---

## Replay defect already found and fixed

Before #320 opened, Prism found an old replay defect:

`sourceVisibility()` still called `sourceTransportSignedStep(...)`, but the helper definition had disappeared during integration replay.

The exact merged-#297 helper was restored in:

`c388a9c7133f820661f601b7e11cd50197cc80d5`

So do not rediscover that as if it were the current failure. The red native witness below occurs **after** this helper restoration.

---

## Authoritative red CI evidence

The important failed run is:

**GitHub Actions run:** `35692709467`  
**Workflow run number:** #2447  
**Tested head:** `6647ad4fd72cbe21bf3f2d1a3e397785d140b12f`

Because `6647ad4...` differs from renderer code only by an Intercom file, it is valid evidence about renderer/test state `c388a9c...`.

### Green jobs

**Focused CPU tests (macOS): PASS**
- setup
- checkout
- runner/library architecture
- shell-helper validation
- configure
- focused regression build
- Terminal exact-reference smoke
- focused regression witnesses

**Slow Adapter independent clock (macOS): PASS**
- configure/build
- soundness and independent cadence
- dramatic Law-Direct A/B
- authored-world adapter impact

These results matter: #320 is not suffering a broad C++ compile collapse, general Terminal failure, or Slow Adapter regression.

### Red job

**SDF range-proxy verification (macOS): FAIL**

Within that job:

- ✅ Configure Earthcall
- ✅ Build SDF proof and GPU parity witnesses
- ✅ Run CPU SDF proof witnesses
- ✅ Verify generic WebGPU SDF parity
- ✅ Verify WebGPU SDF distance parity
- ✅ Verify WebGPU authored-color parity
- ❌ **Verify WebGPU object/radiance parity**
- ⏭ authored-Perlin compiler gate skipped after failure
- ⏭ authored-Perlin range-traversal camera gate skipped after failure

The separate Release authored-Perlin A/B job was skipped as downstream fallout.

Therefore the current evidence says:

> the post-#297 V×D branch builds and clears generic SDF/color/CPU gates, but its native `webgpu_object_test` object/radiance integration witness disagrees.

Do not generalize this into “volumetrics are broken everywhere.”

---

## Current CI after Intercom-only commits

At handoff start, the then-live docs-only head `154e854d...` had queued:

**run `35696873537` / workflow run #2475**
- Focused CPU tests — queued
- SDF range-proxy verification — queued
- Slow Adapter — queued

Those jobs were caused by Intercom-only commits. This handoff/prompt will create more docs-only head movement.

Do **not** wait for repeated docs-only CI as a substitute for diagnosis. The renderer/test tree is unchanged. Use the old red run to identify the assertion, repair code/tests, then care about the first CI run whose head contains a real repair.

---

## Missing piece: exact failing assertion

This handoff session identified the exact failed job and step, but did not successfully recover the individual `webgpu_object_test` assertion text.

An attempt to fetch the complete job log through the connector hit a massive decoded-log size limit before filtering could occur.

So the successor's **first technical action** is targeted failure extraction:

- inspect job `106632899639`
- run `35692709467`
- step **Verify WebGPU object/radiance parity**
- isolate `webgpu_object_test`'s final printed lines/assertion rather than downloading/reading the entire job blindly

Use bounded retrieval. No Big Chungus.

---

## What the failure could mean — do not assume which until evidence

The failed native object/radiance witness contains several distinct regimes. Determine which one actually failed:

1. **Merged #297 Visibility-only regression**
   - receiver-only visibility
   - selective red blocker
   - blue-source independence
   - blocker value motion / stale-shadow witness

2. **Density-only V0 regression**
   - density mode/storage semantics
   - volume Timeline/value refresh
   - density memo invalidation
   - medium bounds/origin±scale behavior

3. **Actual Prism V≠D seam**
   - visibility changes transport while density independently changes medium attenuation/scattering
   - V and D accidentally alias one control/storage/time channel
   - depth-aware composition/order issue
   - volume state accidentally affects surface radiance witness
   - test setup expects old baseline after density became active

4. **Witness bug rather than renderer bug**
   - stale state between native test blocks
   - test enables Density/Visibility and fails to restore a prior control
   - assertion compares an invalid baseline
   - parameter revision/value refresh expectation is wrong

Do not patch until the exact assertion identifies which category owns the disagreement.

---

## Required debugging sequence

1. Read:
   - this handoff,
   - `agent intercom/communication-threads/VISIBILITY_SUN_1_PR320_CI_TRIAGE_2026-09-21.md`,
   - PR #320 body/comments,
   - the V0 density plan:
     `docs/plans/VOLUMETRIC_V0_DENSITY_SOVEREIGNTY_PLAN_2026-09-21.md`.

2. Re-fetch live #320 head/base/comments/CI. Another agent may have advanced it.

3. Verify whether any commit after this handoff changes renderer/test code or is only Intercom documentation.

4. Extract the exact failing `webgpu_object_test` assertion/output from run `35692709467`, job `106632899639`.

5. Map the failure to the smallest semantic owner:
   - V,
   - D,
   - Prism composition,
   - or witness setup.

6. Compare the failing code path to merged #297's green version before changing Visibility.

7. Preserve V0's independent density semantics:
   - explicit Legacy / None / Authored density meaning,
   - independent volume Timeline,
   - density structure/value revision separation,
   - dedicated volume composition,
   - correct origin±scale medium bounds,
   - refusal behavior.

8. Make the smallest repair on:
   `sol/prism-density-after-rung8-20260921`

9. Do not weaken/delete the witness just to get green.

10. Run/check exact-head CI.

11. Only when all relevant jobs are green:
    - refresh canonical base,
    - check #320 is still conflict-free/mergeable and semantically preserves merged #297,
    - update PR evidence,
    - mark #320 Ready for Review.
    - Do **not** merge behind Zach's back unless he explicitly asks.

---

## Merge order after repair

The intended default landing remains:

**#297 already merged → #320 next**

Do not merge #319, #299, or #312 as an alternative shortcut around a red #320.

After #320 is genuinely green and landed, old Density/Prism owner lanes can be archived/closed as appropriate.

---

## Communication protocol

To stop assistant mitosis from destroying continuity:

- identify yourself as the sole successor owner of PR #320,
- use PR #320 comments for coordination,
- update this handoff / write a successor handoff before leaving,
- tell Zach in Agent Intercom before relying on a chat-only final,
- use targeted reads and exact SHA/run IDs,
- if another Sun has advanced #320, reconcile first rather than overwriting.

---

## Bottom line

#320 is **the right branch** but **not merge-ready yet**.

The known red evidence is narrow and valuable:

**native WebGPU object/radiance integration fails after generic SDF/color/CPU gates pass.**

Find the exact native assertion, preserve

```
rho_source != V_transport != D_medium
```

repair only the owning seam, and make #320 green without rolling back either Rung-8 Visibility or Density sovereignty.

— **Visibility Sun 1**
