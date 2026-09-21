# OntoMath Radiance Rung 6 — omega coordinate convention

Date: 2026-09-21  
Author: GPT-5.6 Sol ("The Sun")  
Session: `sol-rung6-angular-20260921`  
Human direction: Zach asked the Suns to continue the later OntoMath Radiance rungs while preserving each earlier authored invariant without reinterpretation.

## Constitutional boundary

Rung 6 adds a third independent source invariant:

```
rho(p,t)   -> scalar
chi(p,t)   -> vec3
alpha(p,omega,t) -> scalar
```

and composes:

```
sourceEmissionRGB = rho(p,t) * chi(p,t) * alpha(p,omega,t)
```

Absence of authored alpha means exactly `alpha = 1`. Rho and chi retain their Rung-4/Rung-5 meanings and storage.

No PointLight / SpotLight / DirectionalLight / Beam kind or enum is introduced. Cones, fans, beams, lobes, and anisotropy are mathematical alpha shapes.

## Exact meaning of omega

`omega` is the **normalized world-space outgoing direction from the radiant source origin toward the receiving world-space sample**.

For the current single-source radiance path:

```
d = receiverWorldPoint - sourceWorldOrigin
omega = d / |d|
```

The current source origin is the same spatial source origin already passed to the WebGPU lighting path as the source position. The receiver point is the world-space shading sample.

"Outgoing" therefore always means **source -> receiver**, never receiver -> source and never camera direction.

This convention is world-space, not source-local. A future authored source-local frame may itself be represented mathematically/relationally, but Rung 6 does not smuggle an implicit renderer-owned orientation into omega.

## Canonical authored vocabulary

OntoMath remains scalar-variable based at its coordinate boundary. Rung 6 therefore admits three canonical scalar coordinates:

```
omega.x
omega.y
omega.z
```

They are components of one semantic vector coordinate, not three unrelated author parameters. They consume no authored parameter slots.

An authored vector omega can be constructed in the existing AST with `VectorConstruct(omega.x, omega.y, omega.z)`; no new MathNode enum member is required.

CPU and WGSL must bind these exact names to the exact same normalized world-space direction.

## Zero-length / singularity rule

If `|receiverWorldPoint - sourceWorldOrigin|` is below the shared directional epsilon, omega is **undefined**. The system must not invent `(0,0,0)`, a camera direction, an axis, or a cached prior direction.

- If no authored alpha exists, compatibility remains exact: the source uses `alpha = 1` and does not require omega.
- If authored alpha does not read omega, it may still evaluate normally.
- If authored alpha reads omega, a missing/degenerate omega context is a refusal/undefined sample, never a fabricated direction. CPU and GPU lowering must preserve this semantic boundary.

## Time

Alpha may deliberately read the same admitted source-side temporal coordinate `t` as rho and chi.

Timeline advance:
- does not mutate alpha AST;
- consumes no authored parameter slot for `t`;
- does not regenerate WGSL;
- does not upload authored parameters solely because time advanced.

## Required witnesses

Rung 6 is not complete until tests prove:

1. no-alpha sources remain pixel-identical through `alpha = 1`;
2. an authored directional lobe changes output with omega;
3. CPU and WGSL agree on the omega component convention;
4. timed/rotating alpha changes pixels when the admitted source Timeline advances without structural recompilation;
5. numeric alpha edits refresh parameter data without shader regeneration;
6. structural alpha edits regenerate only as required;
7. unsupported alpha math or an unadmitted omega context refuses without stale output;
8. rho and chi remain independently observable and unchanged.

This document is the coordinate contract that precedes implementation, as required by the Rung-5 handoff.
