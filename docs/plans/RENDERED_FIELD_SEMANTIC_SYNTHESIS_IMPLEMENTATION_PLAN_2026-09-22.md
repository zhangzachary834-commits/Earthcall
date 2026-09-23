# Rendered Field Semantic Synthesis — Full Implementation Plan

**Date:** 2026-09-22  
**Architectural direction:** Zach  
**Recorded by:** GPT-5.6 Sol  
**Status:** implementation roadmap; test-only semantic compiler work is active on PR #329

## 0. Purpose

Earthcall's renderer is becoming a consumer of authored mathematical world-truth rather than a collection of shader-owned effects.

The rendered world already contains or is explicitly planned to contain several different kinds of truth:

```text
geometry / SDF               F(p,...)
surface/material fields      M(p,n,wi,wo,t,...)

source radiance magnitude    rho_s(p,t)
source chroma                chi_s(p,t)
source angular emission      alpha_s(p,omega,t)
derived visibility           V_s(source,p,...)

medium density               D_m(p,t)
medium extinction            sigma_t,m(p,t)
medium scattering            sigma_s,m(p,t)
medium chroma                C_v,m(p,t)
medium phase                 Phi_m(p,wi,wo,t)
medium emission              E_v,m(p,omega,t)

indirect transport / GI      derived transport over the above
camera/presentation          screen-channel transformation, not world ontology
```

These meanings must remain independent even when they are represented by identical mathematics.

The implementation objective is a shared **semantic execution substrate** beneath them:

```text
authored OntoMath / world semantics
            ↓
canonical mathematical execution DAG
            ↓
dependency provenance + incremental repair
            ↓
channel-scoped conservative proofs
            ↓
small direct execution artifacts / crystallized roads
            ↓
exact renderer truth with fall-open fallback
```

The shared substrate may deduplicate mathematical calculation. It must never deduplicate semantic authority merely because two channels happen to contain the same equation.

The constitutional law is:

> **A proof may remove work. Missing, stale, or inapplicable proof may never remove truth.**

---

## 1. Current status at this roadmap update

### 1.1 Geometry / scene-spatial synthesis

PR #329 has established a test-only sequence:

- canonical DAG sharing for real `OntoMath::MathNode` trees;
- incremental source-parent repair;
- runtime-value changes without semantic rebuild;
- conservative proof attached directly to a compiled execution road;
- invalid proof falling open to exact evaluation;
- explicit re-proof after authored premise mutation;
- proof invalidation distinct from canonical semantic-node reuse.

Rung 1F on PR #329 additionally tests the cross-domain boundary by compiling independently authored geometry, source-radiance, and medium-density expressions that contain identical mathematics.

No production renderer/WGSL path consumes the PR #329 compiler yet.

### 1.2 Source radiance

The canonical radiance architecture preserves independent source invariants:

```text
rho_s(p,t)                 scalar source strength
chi_s(p,t)                 vec3 source chroma
alpha_s(p,omega,t)         scalar angular emission
```

Multiple sources compose above an individual source expression.

The direct source invariant is:

```text
E_s(p,omega,t) = rho_s * chi_s * alpha_s
```

Rungs through multi-source composition are already established in the radiance work. Visibility/shadows remain a separate derived-transport meaning and must not be written into `rho`, `chi`, or `alpha`.

### 1.3 Participating media

Canonical Earthcall now has independently authored medium channels through Volumetric V2:

```text
D(p,t)         density
sigma_t(p,t)   extinction
sigma_s(p,t)   scattering magnitude
C_v(p,t)       medium chroma
```

The planned next independent medium channels remain:

```text
Phi(p,wi,wo,t)       phase / angular scattering
E_v(p,omega,t)       volumetric emission
```

These are not aliases of source radiance. In particular:

```text
rho_source != D_medium
D != sigma_t != sigma_s != C_v
source chi != medium C_v
source alpha != medium Phi
```

### 1.4 Surface/material response

Surface color/material mathematics belong to their own authored material domain. Existing and planned OntoMath-driven material/color fields must remain distinct from source emission, geometry, and transport.

Material response may later include diffuse/specular lobes, roughness/metalness, transmission/refraction, opacity, or other authored response terms where Earthcall's material architecture admits them. Exact property vocabulary belongs to the material-field task rather than being invented by the renderer.

### 1.5 Visibility and indirect transport

Visibility is derived transport:

```text
direct_s = E_s * V_s
```

A blocker moving between source and receiver changes `V_s`; it does not mutate the source AST.

Indirect transport / GI is later derived transport over geometry, source emission, material response, visibility, and media. It must not be hidden inside `rho` or another first-order authored field.

---

## 2. Full rendered composition model

This section is architectural decomposition, not a claim that every term is implemented today.

For a source `s`:

```text
E_s(p,omega,t)
    = rho_s(p,t)
    * chi_s(p,t)
    * alpha_s(p,omega,t)
```

For direct surface transport:

```text
L_direct
    = sum_s [
        E_s
        * V_s
        * MaterialResponse(...)
      ]
```

For a participating medium along a ray parameter `u`:

```text
opticalDepth(u)
    = integral sigma_t(p(u),t) du

T(u)
    = exp(-opticalDepth(u))

mediumScatter(u)
    = sigma_s
    * C_v
    * Phi
    * incidentRadiance

mediumEmission(u)
    = E_v

L_volume
    = integral T(u)
        * (mediumScatter(u) + mediumEmission(u))
        du
```

Later indirect transport may contribute:

```text
L_indirect = GI(geometry, materials, sources, visibility, media, ...)
```

Conceptually:

```text
world radiance
    = direct surface transport
    + participating-medium transport
    + indirect transport
```

Camera/exposure/tone mapping/presentation operate after world-light transport at the Screen boundary. They must not mutate world truth to achieve a display result.

---

## 3. Common compiler substrate vs semantic channels

### 3.1 What may be shared

The common substrate may own:

- canonical mathematical node identity;
- common-subexpression elimination;
- executable operation topology;
- runtime input/value caching;
- source-to-compiled provenance;
- parent/dependent indexes;
- incremental semantic repair;
- revision tracking;
- proof dependency indexing;
- backend lowering artifacts where semantics permit;
- instrumentation for visits, bypasses, fallbacks, repair cost, and resident bytes.

Example:

```text
rho(p) = (x + 2) * 3
D(p)   = (x + 2) * 3
```

The mathematical execution node for `(x+2)*3` may be shared if canonical identity says they are exactly the same calculation.

### 3.2 What must remain channel-scoped

Proof meaning and optimization authority must include semantic role.

A single compiled mathematical node may simultaneously carry or be referenced by distinct theorem records such as:

```text
Geometry:
    distance/support theorem

Radiance:
    contribution/support theorem

Medium density:
    density-support / zero-medium theorem
```

A density theorem must never become radiance authority merely because `D` and `rho` share a compiled math node.

The preferred architectural shape after the PR #329 cross-domain witness is therefore:

```text
canonical math DAG
      |
      +---- geometry proof overlay
      |
      +---- radiance proof overlay
      |
      +---- visibility proof overlay
      |
      +---- medium-density proof overlay
      |
      +---- extinction/scattering/phase proof overlays
      |
      +---- material / transport proof overlays
```

Semantic role should enter **proof identity**. It should enter mathematical compiled identity only when the operation's meaning or evaluation contract actually differs by channel.

---

## 4. Proof algebra by rendered channel

The shared infrastructure is generic. The theorem algebra is not.

### 4.1 Geometry / SDF

Candidate conservative facts:

- subtree A is guaranteed no smaller/no larger than subtree B over a domain;
- a branch cannot win a `min/Union`;
- region is proved outside / empty;
- conservative distance lower/upper bounds;
- object/subtree cannot affect a spatial query;
- interval can be skipped while preserving exact surface result.

These may accelerate sphere tracing, CSG evaluation, visibility rays, or other geometric queries, but proof caches remain derived execution artifacts rather than ontology.

### 4.2 Source radiance

Candidate facts:

- `rho_s = 0` throughout a proved domain;
- `chi_s = 0` or an exact zero channel where composition permits pruning;
- `alpha_s = 0` over a proved angular domain;
- source contribution is exactly zero over a query domain;
- shared source-expression subtree can be evaluated once;
- source is outside a proved support domain.

Do not use approximate “too dim to matter” pruning unless an explicit renderer accuracy contract defines the bound. The first implementation should prefer exact-zero or exact-support proofs.

### 4.3 Visibility / shadows

Visibility is derived from geometry and source/receiver configuration.

Candidate facts:

- a ray/segment interval is proved empty;
- an object/Formation cannot intersect the query;
- a blocker is conservatively proved to intersect;
- a cached visibility result remains valid because every declared geometric premise is unchanged.

Unknown/stale state must perform the exact visibility query.

Never map unknown to “visible” or “blocked.”

### 4.4 Medium density `D`

Candidate facts:

- `D = 0` over a spatial/ray interval;
- medium has no support over an interval;
- conservative nonnegative density bounds;
- identical density subexpressions can share execution;
- an authored density edit invalidates only the affected support/bound proofs.

An exact zero-density proof can justify semantic empty-space skipping:

```text
prove D = 0 on [u0,u1]
    ↓
skip [u0,u1]
```

without sampling density at every ordinary marcher step.

### 4.5 Extinction `sigma_t`

Candidate facts:

- `sigma_t = 0` over an interval;
- conservative optical-depth bounds;
- an interval has identity transmittance when extinction is exactly zero;
- support does not overlap the current segment.

Because transmittance integrates extinction, proofs must be interval/integral-safe. A point sample is not proof for an interval.

### 4.6 Scattering `sigma_s` and medium chroma `C_v`

Candidate facts:

- exact zero scattering over an interval;
- exact zero chromatic component where component-wise pruning is safe;
- shared execution between media whose authored mathematical subexpressions are identical;
- conservative support domains.

Scattering/chroma proofs must not acquire density/extinction authority.

### 4.7 Phase `Phi`

Future candidate facts:

- exact angular support/exclusion;
- isotropic identity/default cases;
- angular regions with zero scattering contribution;
- reusable angular subexpressions.

Source `alpha` and medium `Phi` remain different theorem domains even though both may consume direction.

### 4.8 Volumetric emission `E_v`

Future candidate facts:

- exact zero emission over spatial/angular domains;
- emission support intervals;
- shared source-independent mathematical subexpressions.

Volumetric emission is medium-owned emission, not source `rho`.

### 4.9 Material response

Candidate facts depend on the final material-field algebra, but may include:

- exact zero response lobes;
- exact support/exclusion by direction;
- invariant constant response over a domain;
- shared material-expression subtrees.

A material theorem cannot become source-emission or visibility authority.

### 4.10 Indirect transport / GI

GI should consume the earlier meanings rather than erase their boundaries.

Future conservative acceleration may use:

- proved-empty spatial regions;
- proved-zero source or medium contribution;
- stable visibility/reachability structure;
- bounded transport contribution;
- temporal reuse with explicit premise revisions.

Approximate Monte Carlo/path-tracing estimators require an explicit error/statistical contract and should not be mislabeled as exact Prophetic proof.

---

## 5. Invalidation constitution

### 5.1 Runtime coordinate changes

Examples:

- camera/sample position;
- admitted Timeline coordinate;
- ray direction;
- receiver position when treated as query input.

These normally change **evaluation values**, not authored semantic structure.

They must not rebuild:

- the canonical semantic DAG;
- proof artifacts whose theorems quantify over those runtime coordinates;
- shader structure when the variable was already structurally admitted.

### 5.2 Authored value changes

Numeric coefficient changes should:

- update only affected value/parameter state;
- invalidate proofs whose declared premises include that value;
- preserve unrelated channel proof overlays;
- preserve semantic structure when emitted topology is unchanged.

### 5.3 Authored structural changes

Operator/tree/topology changes may:

- repair/recompile only the affected semantic dependency frontier;
- invalidate dependent proof artifacts;
- regenerate backend structure only for affected lowered artifacts.

No ordinary authored edit should require rebuilding every scene theorem.

### 5.4 World membership / topology changes

Adding/removing:

- geometry;
- sources;
- media;
- material assignments;
- Relations affecting the query domain

must update direct indexes and invalidate only proofs depending on changed membership/topology.

No permanent O(world) relevance scan belongs on the hot path.

---

## 6. Piecewise and field-channel bridge

PR #329 currently proves the compiler against real `MathNode` trees. Production fields are often carried by `OntoMath::Piecewise`.

Before production integration, add a bridge that preserves:

- piece bounds and branch topology;
- `MathNode` canonical identity inside pieces;
- channel semantic role;
- PropertyPath/source provenance;
- admitted runtime inputs such as `p`, `t`, `omega`, `wi`, `wo`;
- exact refusal for unsupported operations.

Do not serialize/pretty-print the whole Piecewise tree every frame to rediscover identity.

Use explicit structural/content revisions or canonical compiled identities.

---

## 7. Implementation phases

### Phase A — prove the compiler architecture in tests

**A1 — canonical execution DAG:** complete in PR #329 witness family.

**A2 — incremental repair:** complete in PR #329 witness family.

**A3 — proof-on-road with exact fallback:** implemented in Rungs 1D/1E.

**A4 — cross-domain identity/proof separation:** Rung 1F on PR #329:
one geometry expression, one `rho`, and one `D` intentionally share identical mathematics; mathematical execution may canonicalize together while theorem authority remains channel-scoped.

**A5 — Piecewise bridge:** implemented in Rungs 1G/1H. Representative real field channels now compile through a shared Piecewise adapter with channel/type sovereignty, including scalar `rho/D/sigma_t/sigma_s`, typed vec3 `C_v`, and a real authored Timeline premise.

**A6 — vessel-scoped theorem authority:** implemented in Rung 1I. Byte-identical zero mathematics intentionally canonicalizes across `SourceRho` and `MediumDensity`, while a density-zero support theorem lives only on the compiled density Piecewise vessel. The witness rejects cross-channel proof borrowing, invalidates theorem authority on Piecewise topology or authored-child premise change, falls open to exact Piecewise evaluation while invalid, re-proves locally, and preserves proof state across runtime `x/t` movement without rebuild.

**A7 — channel-specific zero theorem algebras:** implemented in Rung 1J pending exact-head CI. `SourceRho` now owns a separate `RadianceZeroContribution` theorem rather than consuming `DensityZeroSupport`. Density and radiance perform symmetric hostile cross-channel proof-copy tests, independent topology/child invalidation, exact fallback while invalid, partial local re-proof, runtime `x/t` reuse without theorem rebuild, and work-unit accounting for theorem-premise inspection versus exact Piecewise evaluations avoided.

Landing criterion for Phase A:
focused CI green with no production renderer/WGSL modification, including the Rung 1I vessel-proof witness and its proof-economics counters.

### Phase B — establish production observation without changing pixels

Introduce a production-owned semantic compilation cache behind an opt-in/diagnostic seam.

First pass should collect:

- compiled node count;
- canonical hits;
- source/channel owners;
- structure/value revisions;
- proof counts;
- invalidation/rebuild counts;
- resident bytes;
- hypothetical bypass opportunities.

Pixels must still come from the existing exact paths.

This creates a truthful A/B baseline before optimization authority is granted.

### Phase C — first production geometric bypass

Use the theorem family already proven by PR #329 on the smallest exact SDF composition case.

Requirements:

- feature flag / A-B path;
- exact pixel/distance parity;
- explicit consultation/bypass/fallback counters;
- no ordinary-frame theorem rebuild;
- authored mutation invalidates only dependent artifacts;
- unknown falls open.

Do not replace the existing broader SDF range-proof architecture blindly. Reconcile/compose the mechanisms and delete redundancy only with evidence.

### Phase D — source-radiance execution synthesis

Compile real `rho / chi / alpha` field expressions through the common substrate.

Start with exact support/zero-contribution proofs.

Required witnesses:

- source math identical across two sources shares execution where legal;
- source proofs remain source/channel scoped;
- Timeline movement does not rebuild semantic/proof structure;
- editing `rho` does not invalidate `D`;
- editing `chi` does not invalidate material or medium chroma;
- proof-disabled and proof-enabled pixels are exactly equal.

### Phase E — visibility/shadow integration

Build exact visibility first.

Then allow geometry proof artifacts to accelerate the query:

```text
source emission
    * exact/accelerated derived visibility
```

Required witnesses:

- one-source `V=1` compatibility;
- blocker affects only the geometrically blocked source;
- moving/removing blocker updates pixels without mutating source ASTs;
- stale/unknown proof performs exact visibility.

### Phase F — volumetric density/extinction Prophetic integration

Compile real `D` and `sigma_t` Piecewise channels.

First optimization target:

- exact zero-support / empty-interval skipping.

Then add safe optical-depth bounds only when the math supports conservative interval proof.

Required A/B:

- proof traversal ON/OFF exact pixel parity;
- measured density/extinction evaluations avoided;
- same camera motion with zero proof rebuild when theorem premises are unchanged;
- local authored medium edits invalidate only the affected medium/channel frontier.

### Phase G — scattering/chroma/phase/emission

Extend the shared compiler to:

- `sigma_s`;
- `C_v`;
- `Phi`;
- `E_v`.

Maintain independent proof overlays and revision boundaries.

The current V2 independence witness becomes a constitutional regression: media sharing `D/sigma_t` may still differ arbitrarily in `sigma_s/C_v`.

### Phase H — authored material response

Integrate the OntoMath-driven material-field roadmap.

Surface shading becomes explicit composition of:

- geometric surface truth;
- source/visibility transport;
- material response.

Do not move BRDF/material behavior into source radiance.

### Phase I — indirect transport / GI

Only after source emission, visibility, material response, and media are separately represented should indirect transport become another derived execution layer.

The semantic compiler may supply shared calculations and conservative support facts, but the GI algorithm remains renderer execution strategy.

### Phase J — spectral / richer transport extensions

If Earthcall later introduces spectral rendering, polarization, wavelength-dependent medium coefficients, or richer path transport, extend semantic channels rather than reinterpreting existing RGB/scalar authored truth.

---

## 8. Performance accounting required at every production rung

Every proof-enabled path must report enough information to answer whether it actually earns its cost:

- proof consultations;
- successful exact bypasses;
- fallbacks;
- exact evaluations avoided;
- compiled nodes visited;
- proof-build and repair work;
- dependency-frontier size;
- bytes resident;
- bytes rewritten;
- structure recompiles;
- parameter/value refreshes;
- frame/runtime impact on representative scenes.

Do not count “proof exists” as profit.

Do not hide proof-build cost outside the benchmark.

Amortization should distinguish:

```text
one authored semantic change
vs
many runtime samples / frames / rays
```

The desired architecture crystallizes expensive reasoning when premises change, then reuses it while time/sample state advances.

---

## 9. Renderer/world boundary

Authored world truth:

- geometry mathematics;
- source radiance/chroma/angular emission;
- medium density/extinction/scattering/chroma/phase/emission;
- material response fields;
- authored Timeline relationships and other semantic inputs.

Derived transport truth:

- visibility/shadows;
- optical-depth/transmittance consequences;
- direct and indirect light transport;
- conservative proof artifacts.

Renderer mechanism:

- stepping/integration algorithm;
- workgroup layout;
- GPU buffers;
- cache layout;
- scheduling;
- ray batching;
- temporal/spatial reuse;
- backend code generation.

Screen/presentation:

- camera projection;
- exposure/tone mapping;
- display color transform;
- UI/HUD composition.

Do not push renderer mechanism upward into authored ontology, and do not bury authored meaning as anonymous shader constants.

---

## 10. Non-negotiable semantic separations

Preserve these even when the common compiler sees identical equations:

```text
geometry != radiance
rho != chi != alpha
emission != visibility
visibility != material response
rho_source != D_medium
D != sigma_t != sigma_s != C_v
source chi != medium C_v
source alpha != medium Phi
medium emission != source emission
direct transport != indirect transport
world truth != Screen presentation
```

The common compiler exists to share calculation and dependency machinery, not to flatten ontology.

---

## 11. Immediate next gates from PR #329

1. Get exact-head CI green for Rung 1F.
2. Record the cross-domain verdict explicitly:
   **share canonical mathematical execution; scope proof authority by semantic channel.**
3. Add the first real `Piecewise` multi-channel compilation witness using canonical channels already in Earthcall:
   - source `rho`;
   - medium `D`;
   - medium `sigma_t`;
   - medium `sigma_s`;
   - medium `C_v`;
   while preserving type distinctions.
4. Add one radiance-specific exact-zero/support theorem witness.
5. Add one medium-density exact-zero interval/support theorem witness.
6. Measure theorem-build cost vs repeated evaluations.
7. Only after those results choose the first production integration lane.

The likely first production lanes are the already-proven SDF branch-dominance case and exact zero-density/support skipping, because both can be validated against exact authority without introducing approximation.

---

## 12. Success criterion

This project succeeds when Earthcall can take a richly authored rendered world and do this:

```text
Persons/Laws author meaning
        ↓
OntoMath and world structure preserve that meaning
        ↓
common compiler recognizes shared calculation
        ↓
channel-specific theorem systems prove what work is unnecessary
        ↓
only premise changes repair the relevant roads
        ↓
camera/time/rays traverse crystallized roads
        ↓
renderer remains exactly truthful
```

The goal is not merely a faster shader.

The goal is a renderer whose execution structure is increasingly derived from the world's own authored mathematics without allowing optimization machinery to become a second ontology.
