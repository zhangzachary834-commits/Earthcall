# Visibility Proof Premise Identity

**Office:** Parallax Sun [H→P]  
**Date:** 2026-11-08  
**Scope:** exact proof-premise composition only. No visibility-result cache, no new Field-VM admission, no Knownness change.

## Why this exists

Lattice's `FieldVmDependencySnapshot` answers one narrow question exactly:

> Which external dependency evidence, values, and authority stamps are present for this field execution?

Visibility Sun's constitution answers a different question:

> Under what exact query and evidence may a path result be called KnownClear or KnownOccluded?

Those are not the same object.

A valid dependency snapshot can still be insufficient to retain a visibility proof after:
- the source or receiver moves;
- blocker evidence changes;
- the admitted execution program changes;
- the receiver/cohort identity changes;
- the numerical march policy changes;
- execution crosses a mesh/SDF frame whose proof obligations have not been explicitly reconciled.

Parallax therefore introduces a **premise identity**, not a proof result.

## Exact tuple

`VisibilityProofPremises` composes:

1. executor frame (`MeshRaster` / `SdfRaymarch`);
2. proof scope (`LocalReceiverGeometry` / `CompleteScene`);
3. exact caller-owned admitted-program identity;
4. exact Lattice dependency-snapshot identity;
5. exact receiver-evidence identity;
6. exact scene/blocker-evidence identity;
7. exact numerical traversal-policy identity;
8. source position as exact f32 bits;
9. receiver position as exact f32 bits.

The representation is a domain-separated, length-delimited textual tuple:

`visibility-proof-premises:v1`

There is no finite-width hash reduction in this layer.

## What this does NOT mean

A valid premise identity does **not** mean:
- the field was admitted;
- traversal succeeded;
- the path is Known;
- the proof may be reused forever;
- mesh and SDF are semantically interchangeable.

It means only:

> every premise required by this Parallax tuple is present and has an exact identity suitable for later equality/invalidation checks.

Knownness remains a Visibility result produced after lawful execution.

## Conservative invalidation

The identity deliberately changes when any named premise changes.

Even signed-zero f32 differences remain distinct. Parallax does not own a numerical theorem saying those bits are interchangeable for every visibility algorithm; over-invalidation is preferable to silently broadening proof authority.

A future optimization may factor a premise out **only after** proving it cannot affect the visibility result under the relevant execution policy.

## Relationship to the solar offices

```
authored meaning
   ↓
Lattice dependency manifest / exact dependency snapshot
   ↓
admitted execution program
   ↓
Parallax exact proof-premise identity
   ↓
lawful traversal / proof
   ↓
Visibility: KnownClear | KnownOccluded | Unknown
```

Density's corresponding rule remains symmetric: retained “empty / non-participating” conclusions must lose jurisdiction when a premise that can change participation changes.

Prism's rule remains the outer constitution: none of these layers may collapse merely because downstream execution wants one convenient key.

## Falsifier

`visibility_proof_premises_test` holds the authored/query state fixed and changes one premise at a time:
- dependency value;
- dependency provider/revision authority;
- source endpoint;
- receiver endpoint;
- blocker evidence;
- traversal policy;
- admitted program;
- receiver evidence;
- executor frame;
- proof scope.

Every such change must produce a different exact identity.

Missing dependency snapshot, missing required authority identity, or non-finite query coordinates must produce no valid identity.

— **Parallax Sun [H→P]** ☀️🔭
