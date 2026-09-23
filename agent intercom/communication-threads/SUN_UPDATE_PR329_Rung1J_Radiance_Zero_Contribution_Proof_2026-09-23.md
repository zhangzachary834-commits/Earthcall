# SUN UPDATE — PR #329 Rung 1J Radiance Zero-Contribution Proof

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1I_Vessel_Scoped_Proof_Authority_2026-09-23.md`. Do not restart prior rungs.

Rung 1I introduced vessel-scoped `DensityZeroSupport` authority over real `OntoMath::Piecewise` while allowing byte-identical `SourceRho` and `MediumDensity` mathematics to share canonical calculation IDs.

Rung 1J implements the next named gate: radiance gets its own theorem algebra rather than borrowing density semantics.

## Implementation

Code commit:

`d47bfd22bebac86926d5a8133c3a408bc9b21046`

The existing rendered-field Piecewise witness now defines:

```
ProofKind::DensityZeroSupport
ProofKind::RadianceZeroContribution
```

Both theorem families can derive from canonical scalar-zero mathematics, but they are different proof kinds with different channel authority.

No production renderer/WGSL path changed.

## Radiance theorem

A `SourceRho` Piecewise may receive `RadianceZeroContribution` only when the bounded witness can prove one or more scalar constant-zero pieces.

The proof record remains vessel-owned and declares:

- `Channel::SourceRho`;
- scalar `ValueKind`;
- `RadianceZeroContribution` theorem kind;
- exact Piecewise topology key;
- canonical compiled child math premises;
- authored source-node premises;
- exact zero piece indices;
- proof generation.

The radiance query consumes only that exact theorem kind/channel pair.

## Symmetric cross-channel refusal

Rung 1I already copied a valid density theorem onto a radiance vessel and required exact fallback.

Rung 1J performs the symmetric hostile test:

1. build a valid `RadianceZeroContribution` theorem;
2. mechanically copy it onto a `MediumDensity` vessel;
3. invoke the density support query;
4. require zero proof bypasses;
5. require exact Piecewise fallback.

Thus:

```
shared zero math
!= shared theorem authority
```

in both directions.

## Runtime reuse

A fully-zero source-radiance Piecewise is queried across multiple spatial samples and Timeline values.

The witness requires:

- proof build count unchanged;
- Piecewise topology build count unchanged;
- exact-zero radiance bypass remains valid.

Runtime `x/t` movement therefore does not rebuild the radiance theorem when its theorem quantifies over those runtime coordinates.

## Radiance-local invalidation

The source-radiance Piecewise split is then changed.

Recompiling the stable radiance vessel invalidates only the radiance theorem through its declared topology premise.

The density vessel's topology/proof generation remains unchanged.

While radiance proof state is invalid, the support-enabled query falls open to exact `Piecewise::evaluate()`.

Local re-proof restores radiance bypass.

## Radiance child edit + partial theorem

The left radiance child is then mutated from constant zero to constant four.

Recompiling invalidates the radiance theorem.

The density theorem remains untouched.

Exact fallback now proves the changed left source contribution is nonzero while the untouched right piece remains exactly zero.

Re-proof narrows the theorem:

- left radiance piece: no zero-contribution authority -> exact fallback;
- right radiance piece: exact zero contribution -> bypass.

This mirrors density's safe partial re-proof without sharing theorem meaning.

## Economics accounting

The witness now records deterministic semantic work units:

- theorem builds;
- proof invalidations;
- consultations;
- bypasses;
- exact fallbacks;
- proof refusals;
- authored theorem-premise inspections;
- exact Piecewise evaluations avoided.

The current witness also requires:

`exactEvaluationsAvoided > proofBuilds`

for its repeated-query corpus.

This is not a wall-clock performance claim. It is the first bounded amortization witness distinguishing rare theorem construction from repeated runtime queries.

## Canonical/base state

The code head is based on current canonical:

`e4eeee50d1c6982d262c676a0d9d8b55212f959f`

PR #329 was 0 commits behind and mergeable when Rung 1J was implemented.

The branch had already reconciled the 15 canonical commits containing current volumetric/phase/renderer work and unrelated Person serialization fixes.

## CI state

Rung 1I/Rung 1J exact execution evidence is still pending.

A prior Rung 1I run was superseded by documentation commits after successfully compiling broad focused witnesses but before the SDF proof job executed this Piecewise test.

Rung 1J code head `d47bfd22` triggered workflow #2690, but this handoff/plan-only commit supersedes that code head.

Judge the newest exact-head workflow whose tree contains `d47bfd22`.

The required direct gate remains:

`SDF range-proxy verification (macOS) -> Run CPU SDF proof witnesses -> rendered_field_piecewise_synthesis_test`

Do not call Rung 1I or 1J execution-green until that exact witness completes successfully.

## Successor audit pass — exact economics + CI gate

A targeted successor audit re-read the current PR metadata, this handoff, the Rung 1I/1J portion of `rendered_field_piecewise_synthesis_test.cpp`, and the live exact-head workflow rather than restarting the earlier investigation.

PR #329 remained open, draft, mergeable, and based on `sync-from-earthcall-main` at canonical `e4eeee50d1c6982d262c676a0d9d8b55212f959f` before this documentation-only audit commit.

The audit manually traces every theorem construction and query in the deterministic witness. If the current code executes as written, the final economics counters should be:

```
proof_builds=6
proof_invalidations=4
proof_consultations=39
proof_bypasses=29
proof_fallbacks=10
proof_refusals=1
proof_premise_inspections=12
exact_evaluations_avoided=29
```

Derivation summary:

- density theorem builds: initial + post-topology re-proof + post-child partial re-proof = 3;
- radiance theorem builds: initial + post-topology re-proof + post-child partial re-proof = 3;
- invalidations: density topology + density child + radiance topology + radiance child = 4;
- theorem-premise inspections: 6 builds x 2 Piecewise children = 12;
- successful bypasses / exact evaluations avoided = 29;
- exact fallbacks = 10;
- the one deliberate builder refusal is density-proof construction attempted on `SourceRho`.

This is now the concrete next regression-hardening target: after CI executes the witness once and confirms these values, pin these exact counters in the test. Do not pin them merely from static reasoning if CI disagrees; investigate the discrepancy first because it may expose hidden work or a mistaken accounting assumption.

The live exact-head workflow observed during this audit was #2692 on `ec76792fd2dc169b004d7e11bfeb35c89ae8d7c4`. Its `SDF range-proxy verification (macOS)` job had successfully checked out and configured the exact head and was actively building `SDF proof and GPU parity witnesses`; the required `Run CPU SDF proof witnesses` step had not yet executed. Therefore the direct Rung 1I/1J witness was still neither green nor failed at audit time.

No semantic code was changed in this audit pass because changing the code while the first exact-head execution gate was actively building would erase the evidence we are waiting for. The pass instead established the exact expected accounting contract and verified that CI is running the correct branch/head and the correct SDF proof job.

## Next gate

After exact-head green:

1. compare the emitted counters against the exact expected tuple above and pin them as regression assertions if they match;
2. compare density and radiance theorem construction/consultation economics separately;
3. decide whether Phase A has enough evidence to open the first production **observation-only** seam (Phase B), without granting optimization authority;
4. if more semantic coverage is desired first, add `sigma_t` exact-zero / identity-transmittance theorem as its own theorem kind rather than treating it as density;
5. keep `sigma_s`, `C_v`, `Phi`, visibility, materials, and GI theorem algebras separate even when underlying math nodes canonicalize together.

Do not productionize a theorem merely because its math is zero.

This Sun role is not finished.
