# Message to Zach — prompt for the new Sun

Copy/paste the prompt below into a **new chat**. Do not continue this branch conversation, because the platform has repeatedly duplicated/mitosed the active Sun context.

---

BROOOOOOOOOOO THE OTHER SUNS ARE HANDING PR #320 TO YOU ☀️🌈⚔️

You are the **sole successor Sun for PR #320**. Do NOT restart the investigation.

Repository:
`zhangzachary834-commits/Earthcall`

Canonical PR:
**#320 — Prism landing after Rung 8: Volumetric V0 density on current default**

Owner branch:
`sol/prism-density-after-rung8-20260921`

READ THIS FIRST, IN FULL:

`agent intercom/communication-threads/ontomath-light-and-image/SUN_HANDOFF_PR320_NATIVE_VD_RECONCILIATION_2026-09-22.md`

Then read the prior live-triage thread:

`agent intercom/communication-threads/ontomath-light-and-image/VISIBILITY_SUN_1_PR320_CI_TRIAGE_2026-09-21.md`

Also inspect PR #320 body/comments and live branch/CI before changing anything, because Intercom-only commits may have advanced the head after the handoff.

The governing architecture is:

```
rho_source != V_transport != D_medium
```

#297 Rung-8 Visibility is already merged and authoritative. Do NOT revert it.

#320 is the intended Density-on-Visibility landing lane. Do NOT substitute #319, #299, or #312 just because #320 is red.

Known authoritative failure evidence:

- failed workflow run: `35692709467` (#2447)
- failed job: `106632899639`
- failing step: **Verify WebGPU object/radiance parity**
- generic WebGPU SDF parity: PASS
- WebGPU SDF distance parity: PASS
- WebGPU authored-color parity: PASS
- CPU SDF proof witnesses: PASS
- focused CPU job: PASS
- Slow Adapter job: PASS
- later authored-Perlin steps were skipped because native `webgpu_object_test` failed first

The last renderer-relevant commit before the Intercom-only handoff commits was:

`c388a9c7133f820661f601b7e11cd50197cc80d5`
— `Prism: preserve merged Rung 8 signed-step helper`

Do not waste time waiting on docs-only reruns. First isolate the exact failing `webgpu_object_test` assertion/output from the old red run using bounded targeted retrieval. The previous Sun's attempt to fetch the whole job log hit a huge decoded-log limit, so do NOT Big-Chungus the entire log if a targeted route is available.

Determine whether the failure belongs to:

- merged Visibility behavior,
- Density V0,
- the Prism V≠D composition seam,
- or the witness/test setup itself.

Then make the **smallest evidence-driven repair** on #320's owner branch.

Do not weaken tests.
Do not collapse D into rho.
Do not encode blocker state into source ASTs.
Do not make zero-set proof into density truth.
Do not silently roll back #297.
Preserve Density's independent Timeline/storage/revision/refusal/bounds semantics.

Use PR #320 comments to coordinate and announce your identity as the sole successor Sun before pushing overlapping work.

When the fix is in, run/check exact-head CI. Only after all relevant jobs are green should you:

- refresh current canonical,
- verify #320 still preserves merged #297 and is mergeable,
- update the PR evidence,
- mark #320 Ready for Review.

Do NOT merge behind Zach's back unless he explicitly tells you to merge.

Before you finish, write/update an Agent Intercom handoff and send Zach an Intercom status message so the result survives the chat-UI glitch.

BROOOOOOOOOO GO INTO THE BATTLEFIELD AND FIX #320 🔥🔥🔥
