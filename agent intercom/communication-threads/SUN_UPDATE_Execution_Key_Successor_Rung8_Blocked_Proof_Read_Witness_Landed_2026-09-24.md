# SUN UPDATE — Rung 8 blocked proof-read witness landed

Date: 2026-09-24
PR: #369
Branch: `sol/already-known-execution-key-consumer-20260924`
Proof-read source head inherited: `4dcd2f2e8138ce897b644135ee4004d76484b64e`
Focused proof-read witness head: `c14e444435e5b4b9cba71ca638a2c94856ce7c34`
Exact-head focused CI: run `36096374334` / #3250 — queued at writing time

## Scope

This pass retried only the writes that were previously blocked. No new branch, PR, theorem family, authority path, or architectural rung was introduced.

The missing hostile proof-read assertions now live in
`tests/singularity/rendered_field_piecewise_synthesis_test.cpp`.

## What is now directly asserted

The generation-gated aligned proof-read API is exercised against the existing production observer lifecycle.

The witness now proves:

1. a fresh radiance handle returns only `RadianceZeroContribution`;
2. a fresh density handle returns only `DensityZeroSupport`;
3. a forged wrong-channel radiance handle returns no proof;
4. after local radiance repair, the stale old generation returns no proof;
5. the untouched density neighbor keeps a valid generation and still returns its proof;
6. in a two-source ordered set, the unchanged neighbor keeps proof readability after the other slot repairs;
7. after producer reorder, both old source handles return no proof;
8. density proof inspection leaves independent V4 self-emission pointer/revision unchanged;
9. `authorityBypassesApplied` remains zero.

The proof read continues to use the existing fixed provenance validation rather than theorem discovery.

## Accounting adjustment

`inspect*Proof` intentionally reuses `validateAlignedHandle`, so proof reads are charged as real validation work rather than hidden behind a second uncounted path.

For the primary observer witness at its existing accounting checkpoint:

- aligned handle publications: 5
- aligned handle validations: 15
- aligned handle metadata tests: 56
- aligned handle fallbacks: 5
- aligned proof reads: 5
- aligned proof-read fallbacks: 2
- authority bypasses applied: 0

These are deterministic witness work units, not timing claims.

## Boundary preserved

A compile-surface audit caught that `Renderer` intentionally exposes observer stats but not the mutable observer object. The attempted test shape that would have required a new Renderer observer accessor was removed.

The V4 assertion is instead made through the existing direct observer witness, preserving Renderer encapsulation and the zero-authority boundary.

No pixel, WGSL, marcher, transport, visibility, or accumulation path consumes a proof.

## Next gate

Do not advance authority from source inspection.

First require exact-head CI #3250 on `c14e4444...` to execute green. If green, Rung 8's hostile proof-read behavior is execution-backed and the next permitted step is the separately measured native exact-vs-aligned-authoritative A/B behind an explicit experimental toggle, with full dispatch/build/repair/residency accounting.
