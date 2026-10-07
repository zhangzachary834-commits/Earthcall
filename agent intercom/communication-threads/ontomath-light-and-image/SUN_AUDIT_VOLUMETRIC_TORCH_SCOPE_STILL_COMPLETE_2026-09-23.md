# SUN AUDIT — volumetric torch scope remains complete on current canonical

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
Canonical: `sync-from-earthcall-main`

## Re-entry and current state

Re-entered from `SUN_HANDOFF_V3_V4_LANDED_VOLUMETRIC_ARCHITECTURE_2026-09-23.md` and re-inspected current state rather than trusting historical SHAs.

Canonical head at this pass: `d0ea104ea85078038b85bcf0a37750326feb8fbb` (`Screen Channel and Mathematical Zone Bounds`), which is newer than both the V3/V4 landing commits and the prior completion audit.

The handoff is explicit: V3 and V4 are landed, must not be restarted, and V5 is not authorized by that handoff. A prior targeted audit (`SUN_AUDIT_V3_V4_HANDOFF_SCOPE_COMPLETE_2026-09-23.md`) already reached the same conclusion on an earlier canonical head.

## Targeted review

- Re-read the V3/V4 landed constitution and completion boundary.
- Searched current canonical for the successor vocabulary. The roadmap still describes `V5+` only broadly as multiple participating media / richer transport composition; it does not supply a bounded accepted V5 invariant.
- Inspected current open PR state. The visible newest open work is unrelated (for example #359 is a Formation/Prophetic Rete collision-absorption specification), not a volumetric continuation PR.
- Queried current canonical commit status/workflow association. No commit statuses or PR-triggered workflow runs are attached to exact head `d0ea104...`; therefore I do **not** claim exact-head CI green for this newer canonical commit.

## Implementation decision

No production renderer/medium code was changed. This is intentional, not a blocked implementation pass: the requested handoff's planned work is fully complete, and inventing a V5 implementation here would violate its explicit scope boundary.

The landed constitution remains:

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi != E_v
Phi != source alpha
E_v != source rho/chroma/alpha
```

and authored medium truth remains:

```text
volume.density.ast    -> D(p,t)
volume.extinction.ast -> sigma_t(p,t)
volume.scattering.ast -> sigma_s(p,t)
volume.chroma.ast     -> C_v(p,t)
volume.phase.ast      -> Phi(p,wi,wo,t)
volume.emission.ast   -> E_v(p,omega,t) -> vec3
```

## Remaining risk

The only actionable risk identified by this handoff is successor scope ambiguity. Older roadmap prose names multiple media/richer transport after V4, but the V3/V4 closure explicitly requires a new bounded successor design rather than treating that broad roadmap label as implementation authorization.

Also, because canonical advanced after the earlier green exact-head audit and the current exact head has no attached status/workflow result through the queried GitHub surfaces, current-head CI health is presently unproven by this pass. That is not evidence of a failure.

## Exact continuation point

**This handoff is complete. Stop implementation here.**

The next volumetric campaign should begin only from an explicitly accepted bounded V5 constitution. Before code, define exactly one new semantic invariant and its compatibility default, persistence/PropertyPath reachability, CPU/WGSL meaning, structural-vs-numeric invalidation behavior, refusal semantics, renderer integration boundary, and native witness. Then branch from the then-current canonical head and implement that accepted rung without reopening V3/V4.

— GPT-5.6 Sol, 2026-09-23
