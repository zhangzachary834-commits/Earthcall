# SUN UPDATE — PR #329 Rung 1F Cross-Domain Rendered-Field Synthesis

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from:

- `SUN_UPDATE_PR329_Rung1E_Real_OntoMath_Proof_Road_2026-09-23.md`
- `SUN_UPDATE_PR329_Rung1D_CI_BASE_RECONCILIATION_2026-09-23.md`

Do not restart the investigation.

Rung 1E established conservative proof-on-road over the real `OntoMath::MathNode` compiled DAG. Rung 1F answers the next architectural question: can multiple rendered semantic channels share mathematical execution without sharing theorem authority?

## Base before Rung 1F

Canonical advanced two disjoint Object-composition commits during the pass.

PR #329 was reconciled with current default through two-parent merge commit:

`7d9c7b9ec6309fd813579dae5d3c6d1610896072`

Canonical parent:

`c18802e7aecf94f20fff23fed1dc54a65fc62ad4`

The incoming commits touched no PR #329 files.

## Rung 1F implementation

Implementation commit:

`1bf551816782eea4679f5ee5f528a7072017537a`

Modified only:

`tests/singularity/scene_spatial_ontomath_synthesis_test.cpp`

The witness creates three separately authored real MathNode trees representing three different rendered meanings:

```text
Geometry / SDF       F(p)
Source radiance      rho(p)
Medium density       D(p)
```

All three intentionally contain the same mathematics:

```text
(x + 2) * 3
```

The same `OntoSceneCompiler` compiles all three.

Required result:

```text
compiled(F) == compiled(rho) == compiled(D)
```

The three source trees remain distinct authored objects. Only their canonical mathematical execution identity is shared.

## The architectural verdict is now executable

Rung 1F adds a test-only channel proof ledger keyed conceptually by:

```text
(SemanticChannel, compiledNodeId)
```

The semantic channels are:

- `GeometrySdf`
- `SourceRadiance`
- `MediumDensity`

The proof kinds are deliberately distinct:

- geometry distance/support;
- radiance contribution/support;
- medium-density support.

Therefore one canonical compiled calculation node may be referenced by three different theorem records without allowing the theorem meanings to collapse.

The verdict is:

> **Share canonical mathematical execution where exact identity permits it. Scope optimization/proof authority by semantic channel.**

This is the common-substrate / distinct-meaning boundary the prior discussion required.

## Density-only mutation witness

After all three channels share the same compiled calculation, the test mutates only the authored density scale:

```text
D: (x + 2) * 3
        ↓
D: (x + 2) * 4
```

The authored source-parent repair frontier is exactly the density constant + density root.

It explicitly excludes the separately authored geometry and radiance trees.

The result must be:

- density compiled mapping changes;
- geometry compiled mapping remains on the original shared node;
- radiance compiled mapping remains on the original shared node;
- exactly one channel proof is invalidated: medium density;
- geometry proof remains valid;
- radiance proof remains valid.

The evaluated mathematics also separates: geometry and radiance remain numerically equal while density changes.

## Revert witness

Density is then restored:

```text
D: (x + 2) * 4
        ↓
D: (x + 2) * 3
```

The compiler reuses the original shared canonical execution node.

But the density theorem does NOT silently recover merely because its old compiled node ID is shared again.

The density proof remains invalid until explicitly reattached/re-derived.

Geometry and radiance proof records remain valid throughout because their authored premises never changed.

This extends the Rung 1E distinction:

```text
canonical mathematical artifact reuse
    !=
derived theorem validity
```

with the new cross-domain distinction:

```text
canonical mathematical artifact sharing
    !=
semantic theorem sharing
```

## CI evidence

Exact head including Rung 1F and the plan updates:

`02c108e11f4e8e432fd2b58245b4ee3f1e66e2e1`

Workflow:

- Earthcall focused CI run **#2614**
- workflow run id `35806886822`

The **SDF range-proxy verification (macOS)** job completed **SUCCESSFULLY** on that exact head.

Its successful steps include:

- Build SDF proof and GPU parity witnesses;
- Run CPU SDF proof witnesses;
- all downstream SDF/WebGPU parity steps in that job.

The workflow explicitly builds and runs:

- `scene_spatial_synthesis_dag_test`
- `scene_spatial_ontomath_synthesis_test`

Therefore Rung 1F's direct executable gate is green.

At the time this handoff is written, unrelated workflow jobs may still be running. Do not turn their later unrelated status into a claim that the Rung 1F executable did or did not run: the containing SDF job has already completed successfully.

## Full rendered-field plan added

Zach explicitly required that this work stop being documented as an SDF-only path and include volumetric/radiance/density and the rest of rendered transport.

New master plan:

`docs/plans/RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md`

Creation commit:

`7d2eecbd9bec5b9ff0218636c2b074b11a9e0d72`

The plan covers:

- geometry/SDF;
- surface/material fields;
- source `rho`;
- source `chi`;
- source `alpha`;
- multiple sources;
- derived visibility/shadows;
- medium `D`;
- medium `sigma_t`;
- medium `sigma_s`;
- medium `C_v`;
- future medium phase `Phi`;
- future volumetric emission `E_v`;
- multiple media;
- material response;
- indirect transport / GI;
- later spectral extensions;
- Screen/presentation separation;
- channel-specific Prophetic theorem families;
- incremental invalidation and exact fall-open rules;
- staged production integration and A/B economics.

## Older plans linked/corrected

Radiance roadmap integration commit:

`a3e23774e70b03386dec42b4e37a02987c88cc73`

It now links to the full rendered-field semantic-synthesis plan and records the common compiler / channel-scoped proof law.

Volumetric V0 plan continuation commit:

`02c108e11f4e8e432fd2b58245b4ee3f1e66e2e1`

It corrects the historical stale status: canonical has since landed V1 authored extinction and V2 authored scattering + medium chroma.

It records the continuation stack:

```text
V0  D
V1  sigma_t
V2  sigma_s
V2  C_v
V3  Phi
V4  E_v
V5+ multiple media / richer transport
```

and gives the volumetric Prophetic target: conservative whole-interval zero-density/support proofs, followed later by channel-specific extinction/scattering/etc proofs.

## Current constitutional model

Common substrate may share:

- mathematical execution identity;
- common subexpressions;
- dependency/provenance machinery;
- incremental repair;
- value caching;
- proof invalidation infrastructure;
- backend lowering identity.

Authored/rendered meanings remain distinct:

```text
geometry != radiance
rho != D
D != sigma_t != sigma_s != C_v
source chi != medium C_v
source alpha != medium Phi
emission != visibility
visibility != material response
direct transport != indirect transport
world truth != Screen presentation
```

## Next technical gate — Rung 1G

Do not productionize yet.

The next bounded gate is a real **Piecewise multi-channel adapter witness**.

Use actual Earthcall field vessel structure, not only isolated MathNode roots.

At minimum compile representative scalar Piecewise channels for:

- source `rho`;
- medium `D`;
- medium `sigma_t`;
- medium `sigma_s`.

Also include `C_v` only after the adapter handles its vec3 type truthfully; do not coerce it into scalar merely to make the test easy.

Rung 1G must prove:

1. Piecewise piece/bound topology participates in semantic identity;
2. child MathNodes still canonicalize/shared where legal;
3. channel role scopes proof authority;
4. numeric coefficient edits repair value/premise state without rebuilding unrelated channels;
5. piece-bound/topology edits repair only dependent compiled structure;
6. admitted Timeline/runtime coordinates do not rebuild semantics;
7. unsupported type/operation refuses rather than acquiring another channel's fallback;
8. no pretty-print or full-scene serialization is used as a hot-path identity.

After Rung 1G, measure theorem-build cost vs repeated evaluation and choose the first production A/B lane.

No Big Chungus. Re-read live PR head before every write because multiple Suns have already overlapped this branch.
