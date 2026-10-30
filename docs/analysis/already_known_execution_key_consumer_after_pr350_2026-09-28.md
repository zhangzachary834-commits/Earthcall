# Already-Known Execution-Key Consumer After PR #350 — Final Analysis

Date: 2026-09-28  
PR: #369 — `Rendering: already-known execution-key direct-dispatch witness`

## Question

PR #350 showed that a true rendering proof can still lose economically when the hot path must first discover which proof is relevant. Its maintained single-Perlin witness paid roughly 34–38% active-traversal slowdown, including 83,649 candidate consultations to avoid 144 exact samples on the Horizon case (~581 consultations per saved exact sample). A representative direct-run candidate hid 144,135 record tests behind 16,015 top-level queries to save 139 exact samples.

This successor tested the narrower hypothesis:

> Does prophetic semantic metadata become economical when production execution already possesses the semantic identity for independent rendering reasons?

The answer for the bounded source/medium binding witness is **yes**. This is not a general proof of Prophetic Rendering profitability and does not grant production pixel authority.

## Stable semantic base and integration gait

The stable semantic base for the successor was:

`1ca65259b4888acf8fa58688e3272cfe2326895b`

The task deliberately did not chase unrelated canonical motion. Integration was triggered only when an actual dependency was invalidated and once at the deliberate final landing reconciliation.

The final ancestry-preserving reconciliation commit is:

`310fff58b3f5855337a4c6ed688dab3bca84ebba`

with parents:

- successor: `b388afd8a43c6b5e8f478abea16805fec1c54436`
- canonical-at-reconciliation: `3728fd419a72fd2ef85d88b336242bcf1e6d254d`

No force push or snapshot overwrite was used.

After that reconciliation, canonical advanced to `a4ad48d850abe3889b7c754497b0c652f842021c`. The only overlap with this successor's seven changed paths is the CI workflow, where canonical merely adds unrelated focused CPU tests (Law Line / identifier / community coverage). The successor's semantic files are untouched. That movement is not a new semantic integration event.

## The real already-known key

The qualifying production identity is the **ordered source/medium binding slot already selected by renderer admission/transport**.

Production already constructs ordered `RadianceSourceBinding` and `VolumeDensityBinding` vectors from admitted authored fields. The execution slot exists independently of theorem lookup. Therefore the tested decision path is:

```
already-known binding slot
    -> aligned artifact[slot]
    -> fixed provenance metadata tests
    -> proven action or exact fallback
```

It is not:

```
sample position
    -> spatial classification/search
    -> theorem discovery
    -> action
```

The latter was the rejected PR #350 road.

Stable authored producer identity is carried with the binding through `producerId`; numeric slot alone is explicitly not treated as lifetime identity.

## No-hidden-search accounting

For the aligned proof-read path:

- dispatch lookup: direct slot access;
- spatial lookups: 0;
- hierarchy walks: 0;
- variable-length candidate scans: 0;
- semantic hash searches: 0;
- record scans: 0;
- proof reads in the native A/B: 200,000;
- metadata tests: 800,000 = exactly four fixed tests per decision;
- proof fallbacks: 0 in the native literal-zero witness.

The four provenance dimensions are channel, artifact generation, producer identity, and authored revision.

## Hostile identity and lifetime behavior

The successor witnesses fail open to exact authored execution for:

- authored revision mutation;
- producer replacement in the same numeric slot;
- stale artifact generation;
- removal and re-addition;
- numeric slot reuse/reorder;
- cross-channel theorem use;
- byte-identical mathematics in a different semantic channel.

Local repair changes only the dirty aligned slot. Unaffected neighbor slots retain generation/handle validity.

## Channel and V1–V4 sovereignty

Canonical math identity may be shared, theorem authority may not.

`SourceRho` and `MediumDensity` remain distinct theorem channels. A density-zero theorem cannot authorize radiance, and a radiance-zero theorem cannot authorize density.

Most importantly, density-zero support does not erase independent V4 self-emission. The witness preserves `emissionExpr` and `emissionRevision` while exercising density-zero semantics.

## Native exact-vs-authoritative A/B

Final retained artifact from exact-head reconciled workflow #3511:

```
ALIGNED_AUTHORITY_AB
iterations=200000
exact_ns=430651167
aligned_ns=14895166
ratio_exact_over_aligned=28.912143
build_ns=1458
resident_bytes=70
proof_reads=200000
metadata_tests=800000
proof_fallbacks=0
authority_decisions=200000
production_authority_bypasses=0
```

Both exact-head workflows on `310fff58...` passed:

- #3511 push — SUCCESS
- #3512 pull_request — SUCCESS

The jobs include Focused CPU tests, Slow Adapter independent clock, SDF range-proxy verification, and SDF authored-Perlin Release A/B.

## Interpretation

PR #350 and PR #369 are not contradictory.

PR #350 measured a regime where relevance discovery itself was expensive. Exact math was replaced by hundreds of candidate/record decisions per saved evaluation.

PR #369 removes that variable. The renderer has already selected the semantic execution slot for reasons independent of prophecy, so proof consultation is reduced to a fixed aligned artifact read plus four provenance tests.

That change in problem shape is sufficient to reverse the economics dramatically in the native literal-zero witness.

## Authority verdict

**The already-known execution-key regime graduates as an economically viable narrow authority candidate.**

What has earned promotion consideration:

- a real production-derived key;
- zero hidden relevance search on the decision path;
- hostile fail-open identity semantics;
- channel sovereignty;
- local incremental repair;
- explicit V4 independence;
- complete fixed-path accounting;
- a native exact-vs-authoritative A/B showing 28.912143x exact/aligned for this bounded literal-zero SourceRho witness.

What has **not** been granted by this PR:

- no production pixel bypass;
- no generic Scene-DAG authority;
- no arbitrary Perlin/sample-position theorem lookup;
- no authority transfer across semantic channels;
- no claim that every authored expression benefits;
- no GPU profitability claim yet.

`authorityBypassesApplied == 0` remains the production invariant in PR #369.

## Next experiment, separately commissioned

The next scientifically justified experiment is the smallest real production-authority A/B:

1. one already-selected `SourceRho` binding;
2. one conservative theorem already proven in #369: everywhere-defined scalar literal zero;
3. exact authored path versus theorem-authoritative path under a runtime/compile-time experimental toggle;
4. hostile live mutation must fail open before any pixel result is accepted;
5. measure actual renderer CPU/GPU/frame economics, not merely the native micro-path;
6. production authority must remain scoped to this one channel/theorem until measured.

After that, test `MediumDensity == 0` independently with nonzero V4 self-emission to prove that density authority cannot erase emission.

The architectural direction is therefore not “search faster for prophecy.” It is:

> compile conservative truth onto semantic execution identities the renderer already owns, so relevance discovery disappears from the hot path.
