# SUN UPDATE — PR #350 Rung 2 consumer-design gate

Date: 2026-09-23
PR: #350
Branch: `sol/rendering-relevance-economics-rung1-20260923`
Canonical observed: `423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`
Prior measured verdict: `SUN_UPDATE_PR350_Relevance_Economics_Rung1_CI_Audit_2026-09-23.md`

## Continuity

This remains the same post-#329 relevance-economics successor thread and the same PR #350. Do not spawn another branch/PR.

Rung 1 is settled: dormant proof-capable WGSL has no stable material OFF tax on the maintained Perlin witness, while active generic traversal is ~34–38% slower. The range-grid/direct-run roads fail economically because they repeatedly discover relevance during marching.

## Rung 2 design question

The next consumer must **eliminate relevance discovery**, not optimize a search loop.

A candidate is admissible only if its hot-path decision is already keyed by state the renderer necessarily possesses at that point. The consumer may index an AOT artifact by such an existing key, but it may not walk candidate lists, probe a hierarchy, scan records, hash semantic structures, or perform a second spatial search merely to discover whether a theorem applies.

This distinction is the economic boundary:

- acceptable shape: `already_known_execution_key -> fixed slot / fixed mask / direct jump metadata -> exact or proven action`;
- rejected shape: `ray/sample position -> search/query/records -> maybe relevant theorem -> action`.

The first shape can make the question disappear into execution dispatch. The second merely renames `rangeCandidate()`.

## Audit of the current #329 observer seam

`RenderedFieldSemanticObserver` is **not** yet an admissible authority substrate by itself. It deliberately exposes no theorem-return API to the renderer, and its current vessel identity is `(Channel, raw Piecewise*, revision)`. Its only production theorem family recognizes an everywhere-defined scalar literal zero.

That is appropriate for diagnostic observation, but before pixel authority it leaves two separate obligations:

1. **identity/lifetime:** raw pointer + revision bookkeeping is not sufficient as an authoritative execution key across removal/re-addition, address reuse, or producer-contract mistakes;
2. **granularity:** an everywhere-zero source/medium theorem can support direct source admission/elision, but it does not solve the expensive single authored Perlin surface march. It is therefore a useful authority-safety prototype, not evidence that the Perlin horizon problem is solved.

Do not conflate these two roads.

## Smallest admissible Rung 2 experiment

Before mutating production pixels, build a **test-only direct-dispatch artifact witness** with no spatial lookup.

The witness should use the scene/semantic DAG compiler to emit a compact immutable execution table whose entries are addressed by a stable compile-time/runtime slot already known by the consumer. Each entry carries only conservative action metadata and provenance needed to validate it. The hot path must perform a bounded fixed number of loads/tests independent of world size and theorem count.

Required test matrix:

1. **Direct-key accounting**
   - count dispatch lookups;
   - count metadata tests/branches;
   - assert zero record scans, zero hierarchy walks, zero hash probes, zero spatial candidate searches;
   - report exact evaluations avoided per dispatch lookup and per metadata test.

2. **Identity/lifetime hostility**
   - premise mutation;
   - source removal/re-addition;
   - simulated slot/address reuse;
   - revision change;
   - stale artifact presented deliberately;
   - every invalid/stale case must fail open to exact evaluation.

3. **Channel/transport sovereignty**
   - byte-identical canonical math shared across `SourceRho` and `MediumDensity` must not share theorem authority;
   - zero density must not erase independently authored V4 self-emission;
   - no theorem for density/extinction/scattering/chroma/phase/emission may be inferred merely from another channel's zero/nonzero result.

4. **Incremental repair**
   - authored mutation repairs only the affected table entry/dependency frontier;
   - unaffected slots retain identity and cached state;
   - camera movement and unrelated runtime movement produce zero table rebuilds unless explicitly premise-dependent.

5. **Economic accounting**
   - compile/build time;
   - repair time;
   - artifact bytes;
   - resident bytes if/when uploaded;
   - dispatch lookups/tests;
   - exact evaluations avoided;
   - no native FPS claim until a separate pixel-authoritative A/B exists.

## Graduation gate

A test-only direct-dispatch witness earns a separate native consumer A/B only if:

- its hot path is O(1)-ish with a fixed bound independent of scene theorem count;
- it performs **no hidden relevance search**;
- exact fail-open behavior survives all hostile identity/lifetime cases;
- local edits preserve unaffected artifact state;
- the accounting shows a plausible positive margin after dispatch/tests, not merely fewer exact evaluations;
- the proposed consumer can state exactly which already-known execution key indexes the artifact.

If the witness requires deriving its key from ray position through a spatial search, scanning candidate theorem records, or consulting a variable-length structure, **reject it immediately** as another relevance-discovery road.

## Important scope finding

The maintained single-Perlin horizon problem may not admit this kind of direct scene-DAG dispatch at the individual march-step level: a ray's sample position is not itself an already-known semantic execution key. If the only way to decide whether a Perlin sample can be skipped is to spatially classify that position, then the current theorem family necessarily reintroduces the search cost Rung 1 falsified.

That would be a legitimate negative result, not a failure of the DAG architecture. The DAG can still win at coarser semantic dispatch boundaries (source/medium/object/program/channel selection, shared authored subexpressions, or other places where identity is already known) without pretending to cure the single-field horizon march.

The next implementation pass must test this boundary explicitly before granting any renderer authority.

## Exact next continuation point

Stay on PR #350. Implement the smallest **test-only direct-dispatch artifact witness** described above, preferably beside the existing scene-spatial/RenderedField semantic witnesses rather than inside the hot WebGPU marcher. Prove zero hidden search and hostile fail-open identity semantics first. Do not touch pixel authority yet.

If that witness cannot name an already-known execution key for the single-Perlin march without spatial discovery, record the negative result and stop trying to make this theorem accelerate that workload. Redirect later work only to semantic boundaries where the key already exists.
