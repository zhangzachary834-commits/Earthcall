# SUN UPDATE — V5 membership invalidation + refused-member native witness landed

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live-state inspection

Canonical was re-inspected before implementation and had advanced to:

`67bb9d0cd446c557bedd01c7d9544d7b6f89ec2a`

PR #343 entered this pass at:

`a237b93a64457c0bcbfb1540c09802c51dccae6c`

It remained open, Draft, and mergeable.

The original V3/V4 handoff remains closed and is not being restarted.

A targeted compare from the historical V5 merge base to current canonical shows 100 canonical commits of drift. Among the renderer/test surfaces relevant to this pass, canonical touches `.github/workflows/earthcall-ci.yml` and `src/Singularity/Screen/Renderer.hpp`, but not the V5-owned `WebGpuRenderer.*`, `SdfWgsl.*`, or `webgpu_object_test.cpp` seams. Reconciliation is still required before landing, followed by exact-head CI.

## Implementation landed

Native membership/refusal witness commit:

`3ff006d966d27d66488fba9fc3bf97d1d41e6a68`

Message:

`test: prove V5 membership invalidation and fail-closed refusal`

Only the targeted native WebGPU witness was changed. No production renderer code was modified because the existing cache/refusal seam already expresses the intended law; the witness gets first right of accusation.

## Membership-add witness

The already-established two-member fused set `{A,B}` is the baseline.

A third admitted medium C is added with independent expression identities and visible blue self-emission.

Required proof in the native framebuffer path:

- the C contribution visibly changes the integrated answer;
- `volumeProgramCompiles == 1` on the first `{A,B,C}` frame.

This proves membership addition selects/builds one new fused set structure rather than mutating or flushing unrelated cached structures.

## Membership-remove / bounded reuse witness

C is removed, returning to the already-established `{A,B}` structural key.

Important telemetry clarification found during this pass:

A changed projected set revision deliberately enters the content-refresh branch, so the first `{A,B,C} -> {A,B}` restoration is NOT required to increment the historical `volumeProgramCacheHits` counter. That counter is reserved for unchanged-content reuse.

The truthful first restoration proof is therefore:

- native pixels return to the previous `{A,B}` answer;
- `volumeProgramCompiles == 0`.

A second identical `{A,B}` frame at the same revision then requires:

- `volumeProgramCompiles == 0`;
- `volumeProgramCacheHits >= 2`.

This demonstrates both bounded structural reuse and the strict unchanged-content memo-hit path without falsifying telemetry semantics.

## Refused-member / no-stale-output witness

After valid bright `{A,B}` is established, only B's `E_v` structure is replaced with unsupported `Raycast`.

B deliberately remains an admitted member even though its density value is zero: invalid authored truth is still invalid, and the current fused answer must fail closed.

The witness requires:

- `volumeProgramRefusals >= 1`;
- the refusal names `volume set member 1`;
- the refusal names emission and `Raycast`;
- the center framebuffer is black/clear within the existing refusal tolerance.

This proves that a previously valid cached fused program may remain as an artifact but is not replayed as the current world answer after one admitted member refuses.

The witness then restores lawful B and requires:

- the prior lawful `{A,B}` pixels return;
- `volumeProgramCompiles == 0`.

Therefore a refused sibling-set memo must not poison future lawful reuse.

## Targeted stale-batch audit

The frame lifecycle was checked around the volume composite.

After every volume composite pass, the renderer clears per-pipeline volume batches, parameter batches, draw-instance overrides, and `_activeVolumePipelines`.

Therefore the intended fail-closed path does not depend on stale frame state magically disappearing later: a refused current set does not append a pipeline/batch, while prior frame batches are explicitly cleared after their pass.

## CI state

The implementation commit started Earthcall focused CI run:

`35938304514`

At the time of this update, the Slow Adapter job was in progress and the Focused CPU + SDF range-proxy/WebGPU jobs were queued.

This Intercom documentation commit may supersede that run through workflow concurrency. The authoritative gate is the newest exact-head descendant run containing `3ff006d...`.

Do not call membership/refusal green until the native SDF/WebGPU job succeeds on that exact test code.

## Remaining V5 proof audit

A targeted audit of the V5 compiler test shows existing structural evidence for:

- shared `totalExtinction`;
- shared `totalSource`;
- one `intervalGain = (oldT - transmittance) / totalExtinction`;
- reversed membership staying on fused sample-level transport;
- compiler-level refused-member naming.

However the V5 plan separately asks for direct proof of:

1. correct combined extinction versus a sequential-alpha counterexample;
2. independent chroma/scattering/emission contributions in overlap.

Those requirements are not yet clearly discharged by a dedicated native numerical witness. Do not silently mark them complete merely because the WGSL contains the right accumulator symbols.

## Exact continuation point

1. Inspect the newest exact-head focused CI descendant containing `3ff006d...`.
2. If the new native witness fails, fetch the SDF/WebGPU job log and repair only the narrow fused set-key/content-refresh/refusal/batch seam exposed by the failure.
3. If green, close membership add/remove + refused-member/no-stale-output in the V5 plan.
4. Then add a bounded native overlap proof that distinguishes fused combined extinction from sequential whole-medium alpha and independently witnesses member chroma/scattering/emission contribution.
5. Reconcile with current canonical and rerun final exact-head focused CI.

## Guardrails

- No global volume-cache flush on membership change.
- No numeric/time values in structural identity.
- No stale framebuffer replay after refusal.
- No sequential whole-medium transport regression.
- No V6, multiple scattering, GI, spectral transport, or new Medium nouns.
- Targeted reads only; no big chungus.

— GPT-5.6 Sol
