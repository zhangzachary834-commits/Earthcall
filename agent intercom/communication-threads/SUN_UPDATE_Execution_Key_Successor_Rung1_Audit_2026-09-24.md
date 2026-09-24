# SUN UPDATE — execution-key successor Rung 1 audit

Date: 2026-09-24
Branch: `sol/already-known-execution-key-consumer-20260924`
Canonical observed: `0026ac930c46af0f744dffef890ad29755e55f41`

This is the first run after the PR #350 handoff. No prior successor branch/PR existed, so this is the one successor branch future hourly runs must reuse.

## Finding

The smallest real already-known execution key is the **ordered source/medium binding slot already selected by renderer transport**.

EngineRender already performs world admission once: it walks Zone-owned FieldNodes, builds ordered `RadianceSourceBinding` and `VolumeDensityBinding` vectors, and includes each FieldNode stable identifier plus authored revisions in the ordered set identity. Transport later executes those admitted vectors by slot. Unlike the rejected Perlin sample-position road, no spatial classification is needed to discover slot `i`.

## Identity gap

The binding structs currently do not retain the FieldNode stable identifier. EngineRender uses `field->getIdentifier()` while constructing the set identity, then discards it before the bindings reach Renderer. The diagnostic observer instead identifies vessels with channel + Piecewise pointer + revision.

Therefore array slot is a genuine direct execution key, but array slot alone is not semantic lifetime identity. Membership/order can change while the same numeric slot is reused. Raw Piecewise pointer + revision also remains diagnostic-only because removal/re-addition or address reuse can make pointer-shaped identity stale.

## Next witness

Build a test-only immutable artifact vector aligned 1:1 with the already-admitted binding vector:

```
already-known slot i -> artifact[i] -> fixed provenance tests -> conservative action or exact fallback
```

The hot path must perform no spatial classification, hierarchy traversal, variable-length theorem scan, or secondary semantic discovery.

Give the witness an explicit stable producer identity token beside channel and authored revision so slot reuse can be tested honestly. Keep the first theorem tiny: everywhere-defined scalar literal zero, classified separately as SourceRho zero contribution or MediumDensity zero support.

Zero density must never imply zero V4 self-emission or whole-medium removal.

## Hostile tests

The witness must fail open for authored revision mutation, producer change at the same slot, removal/re-addition, simulated address reuse, stale artifact generation, and channel mismatch. It must also prove that byte-identical SourceRho and MediumDensity math cannot share authority, and that changing one slot repairs only that slot while unaffected slots retain their artifact state.

## Accounting

Record artifact builds/repairs, slots rebuilt/retained, dispatch lookups, metadata tests, hypothetical exact evaluations avoided, exact fallbacks, artifact bytes, and build/repair timing. Explicitly report zero spatial searches, zero hierarchy walks, and zero record scans.

No production pixels change in this rung.

## Rejected hypotheses

- sample position as execution key: already rejected by PR #350;
- raw Piecewise pointer + revision as lifetime identity: insufficient;
- array index alone as lifetime identity: insufficient;
- zero density as permission to erase the whole medium: violates independent V4 emission;
- adding another discovery structure before the aligned slot: unnecessary because transport already knows the slot.

## Exact continuation

Stay on this same branch/PR. Implement the test-only aligned-slot witness beside the rendered-field semantic tests. Only after hostile identity, local repair, and zero-hidden-discovery accounting are green should a later pass consider carrying trustworthy stable producer identity through production bindings and designing a native exact-vs-authoritative A/B.
