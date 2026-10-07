# SUN UPDATE — V4 PR #339 exact-head CI triage

Date: 2026-09-22
Repository: `zhangzachary834-commits/Earthcall`
Owner branch: `sol/volumetric-v4-authored-emission-20260922`
PR: #339 — Volumetric V4: authored emissive media

## Current truth

V3 is landed and must not be reopened. V4 has advanced materially beyond the original storage-only rung.

Audited V4 head before this documentation commit:
`4c81615a6dc395256d25ec9c991e8c5718de39e6`

Production V4 architecture observed in targeted reads:
- independent `volume.emission.ast` / `E_v` authorship and projection;
- emission-owned vector/angular OntoMath lowering;
- directional emission binds its own sample-to-eye omega rather than source alpha's source-to-receiver omega;
- dedicated depth-aware volume transport integrates self-emission separately from source-driven scatter;
- absent emission is literal zero self-emission, preserving pre-V4 behavior;
- renderer cache/program identity has been extended from `(D, sigma_t, sigma_s, C_v, Phi)` to `(D, sigma_t, sigma_s, C_v, Phi, E_v)`.

The constitution remains:
`rho_source != V_transport != D_medium`
and
`D != sigma_t != sigma_s != C_v != Phi != E_v`.

## Exact-head CI result

Workflow run `35820907202` is not green.

Blocking failure is currently a compile defect in the new focused witness, not evidence of a production renderer semantic failure.

Compiler diagnosis:
`tests/singularity/sdf_wgsl_parameter_refresh_test.cpp:701`
constructs the result of `MathNode::fromLegacyExpression(...)` through `std::make_shared<MathNode>(...)`.

`fromLegacyExpression(...)` already returns `std::shared_ptr<MathNode>`, so the test asks C++ to construct a `MathNode` from a `shared_ptr<MathNode>`. No such constructor exists. This is the same ownership-shape class of witness error encountered during V3 CI triage.

## Next exact action

1. Fix only that witness ownership construction first. Do not perturb production V4 semantics to satisfy a test compile error.
2. Re-run exact-head focused CI.
3. If compilation clears, inspect every job, especially native WebGPU evidence.
4. Continue only evidence-driven fixes.
5. Do not merge until native source-free emission proves that a medium with authored `E_v` visibly emits radiance with external illumination absent/disabled, plus hue/magnitude, numeric-vs-structural invalidation, Timeline, refusal/no-stale, persistence/Property sovereignty, and sibling cache-identity witnesses.
6. Reconcile with live canonical before landing.

## Scope guard

No V5 multiple-media architecture, GI, multiple scattering, path tracing, or unrelated renderer refactors belong in this PR.

— GPT-5.6 Sol
