# Volumetric V5 — Medium-Set Composition Plan

Date: 2026-09-23
Owner lane: GPT-5.6 Sol
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`
Observed canonical at fresh-current reconstruction: `b5fa341329176b0257d6de58f85f99ac6a286830`

## Why V5 exists

V0-V4 made one participating medium authorable all the way from world truth to native WebGPU:

```text
D_i(p,t)
sigma_t_i(p,t)
sigma_s_i(p,t)
C_v_i(p,t)
Phi_i(p,wi,wo,t)
E_v_i(p,omega,t)
```

The renderer already accepts a collection of `VolumeDensityBinding` values. That collection is necessary infrastructure, but it is not by itself the V5 transport law.

Today each medium is integrated independently and emitted as a premultiplied volume draw:

```text
C_out = C_medium + T_medium * C_scene
```

That is exact for one medium. For overlapping independent media it is only sequential framebuffer composition. Different whole-medium draw/group order can change the answer because one already-integrated medium image is attenuated by another after the fact.

V5 therefore means **world-level participating-medium composition**, not merely "the API takes a vector."

## Constitution inherited unchanged

Preserve:

```text
rho_source != V_transport != D_medium
D_i != sigma_t_i != sigma_s_i != C_v_i != Phi_i != E_v_i
Phi_i != source alpha
E_v_i != source rho/chroma/alpha
```

Do not invent a new Medium kind. The authored Singular remains the FieldNode; `VolumeDensityBinding` is a Screen projection.

Do not smuggle in:
- multiple scattering;
- GI/path tracing;
- spectral transport;
- new light kinds;
- renderer-owned medium properties.

## V5 transport law

At a ray sample `p`, let `M(p)` be the set of admitted media whose bounded domain contains the sample and whose authored density says participating substance exists there.

Each medium keeps its own V0-V4 compatibility/default rules. V5 combines their transport **after** evaluating those independent truths.

For the current single-scattering/emission model, define:

```text
sigma_t_total(p) = Sum_i sigma_t_i(p)

q_scatter_total(p) =
    Sum_i sigma_s_i(p) * C_v_i(p) * Phi_i(p,wi,wo,t)

q_emission_total(p) =
    Sum_i E_v_i(p,omega,t)

q_total = q_scatter_total + q_emission_total
```

For one integration interval `ds`:

```text
T_new = T_old * exp(-sigma_t_total * ds)

DeltaC =
    T_old * q_total *
    (1 - exp(-sigma_t_total * ds)) / sigma_t_total
```

with the continuous `sigma_t_total -> 0` limit `DeltaC = T_old * q_total * ds`.

This algebra is chosen because, for exactly one medium, it reduces to the V4 formulas already landed:

```text
scatter:
(sigma_s / sigma_t) * C_v * Phi * (T_old - T_new)

emission:
E_v * (T_old - T_new) / sigma_t
```

The implementation must preserve the existing exact compatibility branches where V4 deliberately does so.

### Critical consequence

V5 must be permutation invariant for the same set of overlapping media.

It is NOT acceptable to make FieldNode order, AST/pipeline grouping, pointer ordering, or draw order into physical law.

## V5a — landed in this branch: canonical medium identity

The first bounded pass fixes the identity seam before changing transport.

A single helper now owns the six-channel per-medium fingerprint:

```text
(D, sigma_t, sigma_s, C_v, Phi, E_v)
```

A single helper also writes ordered world-set identity from stable FieldNode identity plus all six channel revisions.

This repairs a real V4-era gap: EngineRender's medium-set identity ended at `phaseRevision`, while WebGPU's per-medium memo already included `emissionRevision`. An E_v-only edit therefore did not participate in the world-set revision even though the renderer program correctly treated E_v as content.

V5a makes discovery identity and renderer memo identity speak the same six-channel language.

## V5b — next: exact fused-overlap baseline

Implement the smallest production path that integrates overlapping media in one shared ray transport rather than as independently integrated framebuffer layers.

Requirements:

1. Single-medium path stays exact V4 compatibility.
2. Two overlapping media are integrated sample-by-sample with total extinction/source terms.
3. Reversing the projected medium order produces identical native pixels.
4. Non-overlapping media preserve the expected local result.
5. Numeric edits refresh values without structural WGSL regeneration when topology is unchanged.
6. Membership or AST-structure changes invalidate only the medium-set program structure they actually affect.
7. Unsupported authored math refuses the affected V5 answer; stale integrated radiance must not survive.
8. Phase/emission direction variables retain their V3/V4 meanings. V5 does not invent new incident-light physics.

## V5c — indexed/incremental world discovery

Current EngineRender discovery is already bounded to the Zone's direct FieldNode ownership index rather than all Objects. Preserve that advantage.

The completed rung should move toward explicit medium-set structural revisions / relevant-change-only rebuilds so the frame path does not serialize every AST to JSON forever merely to learn that nothing changed.

This is an invalidation/indexing concern, not permission to weaken authored-content identity.

## Required V5 witnesses

Before V5 can be called landed:

- one-medium V4 pixel parity;
- overlapping two-medium permutation invariance;
- correct combined extinction versus sequential-alpha counterexample;
- independent chroma/scattering/emission contributions in overlap;
- numeric-only edit => pixel change with no structural compile;
- structural edit => bounded recompilation;
- membership add/remove => medium-set structural invalidation;
- unsupported member => named refusal and no stale contribution;
- time-only change => pixel change without structure regeneration;
- world-set revision changes for every authored channel, including E_v;
- no renderer Zone/world scan.

## Current stopping point

V5a identity groundwork is implemented and witnessed on the branch. The next implementation pass should begin at V5b with an order-reversal native witness first, so the test exposes the present sequential-composition fossil before the fused transport replaces it.

— GPT-5.6 Sol
