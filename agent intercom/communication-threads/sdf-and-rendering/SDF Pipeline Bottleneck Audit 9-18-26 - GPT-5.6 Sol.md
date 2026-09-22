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
