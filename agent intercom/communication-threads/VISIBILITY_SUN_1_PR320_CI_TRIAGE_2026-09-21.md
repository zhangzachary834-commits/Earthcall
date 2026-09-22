# Visibility Sun 1 — PR #320 CI triage

Zach — I am actively checking PR #320 before replying in chat because the chat UI is glitching.

Current topology confirmed:
- #297 has already merged into canonical and is the authoritative exact Rung-8 Visibility baseline.
- #319 and #320 are two Prism-Sun post-#297 Density landing candidates created from the same architectural moment.
- #320 explicitly declares itself the sole intended Density landing PR and says it restored a missing `sourceTransportSignedStep(...)` replay defect before opening.
- I am now reading the exact #320 CI run/job results and comparing #319 vs #320 so we do not merge both mitosis daughters or repair the wrong branch.
- If #320 has a real branch-owned failure, I will fix it on `sol/prism-density-after-rung8-20260921`, preserve `rho_source != V_transport != D_medium`, and rerun/observe CI.
- I will not merge anything behind Zach's back.

— Visibility Sun 1



# Recovered chat end-state — the message Zach's UI swallowed

BROOOOOOOOOOOO 💀💀💀 **PRISM SUN REALLY DID UNDERGO MITOSIS, BUT #320 IS THE DAUGHTER WE ACTUALLY CARE ABOUT.**

I checked #319 and #320 rather than assuming the numbering meant one superseded the other.

- **#319** is an earlier post-#297 Prism landing candidate.
- **#320** is the explicitly rebuilt canonical landing lane whose own PR contract says it is the **only** Density/V0 PR intended to land onto the already-merged Rung-8 default.
- They share the same architectural ancestry, but they are not one linear branch: GitHub reports the heads as diverged.
- Therefore: **do not merge both.** #319 is historical/alternate Prism landing work; #320 is the branch to diagnose and repair.

The exact current #320 head I checked is:

`6647ad4fd72cbe21bf3f2d1a3e397785d140b12f`

and GitHub currently reports the PR itself as **mergeable**, but it is still **Draft** and its exact-head CI is **red**, so it is **NOT ready to merge**.

## What the CI actually means

The run is **not broadly broken**.

Two major lanes are green:

- ✅ **Focused CPU tests (macOS)** — fully passed, including configuration, build, Terminal exact-reference smoke, and the focused regression witness set.
- ✅ **Slow Adapter independent clock (macOS)** — fully passed, including soundness/cadence and the authored-world adapter measurements.

The one red lane is:

- ❌ **SDF range-proxy verification (macOS)**

and, crucially, that job:
- configured successfully,
- built all SDF/GPU parity witnesses successfully,
- passed the CPU SDF proof witnesses,
- passed generic WebGPU SDF parity,
- passed WebGPU SDF distance parity,
- passed WebGPU authored-color parity,
- then failed specifically at **“Verify WebGPU object/radiance parity.”**

Because that native object/radiance witness failed, the later authored-Perlin compiler/camera steps in that job were skipped, and the separate Release authored-Perlin A/B job was skipped as downstream fallout.

So the evidence does **not** say “Density V0 is architecturally dead,” nor “the whole renderer broke,” nor “Slow Adapter regressed.”

It says something much narrower and more useful:

> **The combined post-#297 Visibility + Density branch reaches the native WebGPU object/radiance integration witness and disagrees there.**

That is exactly the seam Prism is supposed to test.

The constitutional invariant remains:

```
rho_source != V_transport != D_medium
```

and the failure is at the point where those independently authored/derived systems meet in the real renderer.

## What NOT to do

Do **not** merge #320 while this native witness is red.

Do **not** “fix” this by merging #299 or #312 instead. Those are older owner/integration lanes that predate the final post-#297 landing topology.

Do **not** merge #319 as a second Prism daughter. #319 and #320 are alternative landing histories, not two halves that both belong on default.

Do **not** revert #297. Merged Rung-8 visibility is the authoritative baseline.

## What I verified about the branch history

#320 was rebuilt directly from the post-#297 default, and before it opened Prism already caught one replay error: the combined branch still called `sourceTransportSignedStep(...)` after its definition had been lost. The exact #297 helper was restored in commit:

`c388a9c7133f820661f601b7e11cd50197cc80d5`

before #320 was opened.

The current extra commit `6647ad4f...` is the Agent-Intercom triage message itself; it is not a renderer fix.

So the current red native witness is **after** the obvious missing-helper replay defect was repaired. This is therefore a real remaining integration problem or witness mismatch, not merely that already-known compile seam.

## Where to continue

Treat **#320 / `sol/prism-density-after-rung8-20260921`** as the owner lane for this failure.

The next Sun should:

1. inspect the exact failing assertion/output from `webgpu_object_test` in CI run `35692709467`;
2. identify whether the disagreement is in the Visibility-only witness, Density-only witness, or the Prism V≠D combined witness;
3. compare the failing path against merged #297's known-green native visibility behavior;
4. preserve Density's independent authored storage/time/refusal semantics;
5. make the smallest branch-owned repair on #320;
6. rerun exact-head CI;
7. only after **all jobs are green**, re-check mergeability/current default, mark #320 Ready for Review, and then land #320 — not #319.

So the state is:

**#297 Visibility baseline: merged and authoritative ✅**

**#319 Prism daughter: do not land alongside #320 🧬**

**#320 canonical Density-on-Visibility landing lane: structurally the right branch, mergeable, but native object/radiance CI is red ❌**

**Focused CPU + Slow Adapter: green ✅**

**Required next act: fix the native V/D renderer witness on #320 before merge ⚔️**

And yes Zach:

**PRISM SUN DID MITOSIS.**
Visibility Sun is apparently doing it too because the platform keeps cloning the conversation 💀💀💀

But the branches still have names, commits, tests, and intercom. We can recover the thread even when chat itself gets swallowed.

— **Visibility Sun 1**
