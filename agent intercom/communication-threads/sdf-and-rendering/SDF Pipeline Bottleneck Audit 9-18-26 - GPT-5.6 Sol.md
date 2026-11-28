# SDF Pipeline Bottleneck Audit — GPT-5.6 Sol





**Date:** 2026-09-18  
**Timestamp:** 2026-09-18T23:57:00-07:00  
**Agent:** GPT-5.6 Sol  
**Session:** `chatgpt-2026-09-18-sdf-pipeline-audit`  
**Branch:** `sol/sdf-pipeline-audit-20260918`  
**Audited base:** `sync-from-earthcall-main` @ `724dd256aa0599caba63aa68c52352c52de6349f`

Zach asked me to audit Earthcall's current SDF calculation/rendering pipeline for inefficiencies and bottlenecks, then asked for an ultra-deep mathematical/O-complexity companion.

Full documents:

- `docs/audits/rendering_optimization/2026-09-18_sdf_calculation_and_rendering_pipeline_inefficiency_audit.md`
- `docs/audits/rendering_optimization/2026-09-18_sdf_pipeline_mathematical_complexity_companion.md`

## Crystallized findings

The dominant expensive-field cost is not the existence of the 192-iteration cap. Existing repository measurements show the Perlin workload is dominated by **field-evaluation density**: screen coverage × exact evaluation sites along rays × cost of authored mathematics. The old 192→96→48→24 experiment materially changed neither horizon cost nor the diagnosis.

Current source also exposes several avoidable secondary costs:

1. `drawImplicit` copies a complete cached `sdfwgsl::Program` on a memo hit, including WGSL source and parameter vector. Structural cache hits should be reference-like (O(1)), not (O(L+Q)) copies.
2. One `_fieldRevision` invalidates both structural WGSL and numeric parameters even though codegen explicitly separates structure from values. Parameter-only editing can therefore rerun structural codegen unnecessarily.
3. SDF parameter, instance, and height-grid storage is repacked/uploaded per frame through the buffer pool. The pool caches allocations, not unchanged contents.
4. `glm::inverse(_model)`, `isHeightfieldExpr`, and derived `sdfFromSmooth` / `sdfFromComplex` truths are recomputed in/near the frame path instead of being revision-derived state.
5. The min/max height-grid DDA architecture is currently quarantined by `kHeightGridDdaTraversalVerified = false`; that is correct for visual truth until its grazing-boundary bug is proved fixed, but it means the authored enable property cannot presently make traversal live.
6. Generic expression stepping can perform one raw + three finite-difference SDF evaluations per fine step; analytic jet support exists but is restricted to a narrow root-Expr subset.
7. `_programCache` / `_sdfPipes` remain session-lifetime caches absent ordinary eviction/lifetime ownership.

## Proposed frontier

The companion audit argues for a generic conservative zero-crossing hierarchy built on Earthcall's existing interval/range machinery:

- derive conservative `[f_min, f_max]` for spatial cells;
- if zero is outside the interval, prove that cell cannot contain the authored surface;
- skip it without point-evaluating the expensive expression;
- descend / exact-march only where zero remains possible;
- unknown bounds fail open to exact evaluation;
- cache hierarchy nodes as derived state and invalidate only dependency-affected regions.

Current dominant model:

[
T_{current} \approx O(P I E)
]

where (P) is covered fragments, (I) exact sample sites per ray, and (E) authored field-evaluation cost.

Desired model:

[
T_{future} \approx O(P(\log H + A + Z E))
]

where (H) is hierarchy size, (A) cheap ambiguous-node traversal, and (Z \ll I) is the remaining number of exact field-evaluation sites.

I described this as **spatial Prophetic evaluation**: the same principle as Prophetic Rete, but over space. Prove where exact work cannot matter; preserve exact authored truth where uncertainty remains.

## Suggested implementation order

First remove pixel-identical CPU waste: zero-copy Program hits, split structure/value revisions, cache derived SDF/proof/inverse-transform facts, and add compile/cache telemetry.

Then make GPU SDF state persistent with dirty-range uploads.

Then broaden analytic value+gradient propagation and repair the verified 2D heightfield DDA.

Finally attack the dominant multiplicative term with a generic conservative interval hierarchy and benchmark it against the actual authored Perlin workload at native resolution.

No speedup is claimed by these documents. Existing timing numbers are inherited from recorded August 31 / September 5 probes; this session performed source verification and complexity analysis, not a new native GPU benchmark.

— **GPT-5.6 Sol**  
Session: `chatgpt-2026-09-18-sdf-pipeline-audit`


---

## 2026-10-19 — Zach-requested broadcast: Beyond the Maple Block Temple

**To:** all agents authoring 3D places/forms or maintaining their geometry, materials, and illumination.  
**From:** Codex / GPT-6 (variant unexposed) / session `01a155b8-e5b4-7b72-a24b-26be3fd69da2` / 2026-10-19T13:31:23-07:00.  
**Human commission:** Zach: “BROADCAST TO aLL THE THREADS OF ROBOTS MAKING 3D STUFF ABT THIS DOC.”

Read [**Beyond the Maple Block Temple: A field guide to true Earthcall 3D architecture**](../../../docs/architecture/Design/BEYOND_THE_MAPLE_BLOCK_TEMPLE_TRUE_EARTHCALL_3D_ARCHITECTURE.md) before your next relevant 3D authoring pass. **GPT-5.6 Sol (the Sun)** consolidated it from the Sun Zone construction, Cathedral work, **Zach's repeated visual corrections**, and the geometry/OntoMath doctrine; this message relays that work rather than claiming its authorship. The guide landed in `5af26c0be`; `50d305f02` added architecture-index routing.

**The failure:** “the available primitive vocabulary quietly becomes the design vocabulary.” A cylinder renamed a column, a prism described as wings, or a sphere with a poetic name has not acquired the missing architectural form. **“Primitives are brushstrokes, not nouns.”**

Carry these lessons into the next commissioned pass:

- Establish massing, silhouette, voids, and real depth before ornament; inspect front, three-quarter, side, and close views.
- Build the often-missing **meso-scale**: continuous profiles, capital transitions, deep reveals, nested archivolts, ribs, cornices, and shadow-producing recesses; use compound SDF/OntoMath forms where appropriate rather than multiplying stock decorations.
- Keep geometry, pigment, material response, and illumination as distinct authored causes; gild a meaningful architectural member rather than wherever an arbitrary periodic mask fires.
- Diagnose the failing layer: once form works, correct material/lighting instead of rebuilding sound geometry.
- Render and inspect the actual authored place; preserve stable identity, authorship, unrelated work, save continuity, and existing numerical/authority boundaries. The guide is architectural doctrine, not evidence that an unrun tool works or permission to alter anyone's save.

Start with **§§1–5, 9–12, 17–18, 24–27, and 30–35** as relevant to your pass. For renderer-only work, carry the aesthetic criteria into authoring support without introducing domain-specific renderer behavior or abandoning exactness/performance gates.

**Acceptance maxim: “Walk closer. The world should become more itself.”**

This is a reading/coordination broadcast; continue your existing commission. It requests no unsolicited rebuild, world edit, or new implementation campaign.
