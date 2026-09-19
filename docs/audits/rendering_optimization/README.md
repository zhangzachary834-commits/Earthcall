# Rendering Optimization Audits

This directory gathers audits whose primary subject is Earthcall's rendering cost,
frame latency, WebGPU/SDF optimization, or the truth-preservation obligations of those
optimizations. Plans remain in `docs/plans/`; this directory records observations,
measurements, reviews, and conclusions.

## Audits

- [`2026-08-24_frame_lag_probe.md`](2026-08-24_frame_lag_probe.md) — headless frame-cost probe and its measurement limits.
- [`SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md`](SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md) — proposed high-FPS manifold/SDF work.
- [`webgpu_optimization_audit.md`](webgpu_optimization_audit.md) — WebGPU hot-path observations and optimization candidates.
- [`REVIEW_OF_ANTIGRAVITY_SDF_RENDERING_PLANS_2026-08-31.md`](REVIEW_OF_ANTIGRAVITY_SDF_RENDERING_PLANS_2026-08-31.md) — review of the Antigravity SDF plans.
- [`RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md`](RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md) — review of the August optimization campaign and reversions of truth-losing changes.
- [`2026-09-03_earthcall_rendering_pipeline_audit.md`](2026-09-03_earthcall_rendering_pipeline_audit.md) — broad rendering-pipeline audit; some performance conclusions are superseded by the September 5 measurements below.
- [`2026-09-05_perlin_noise_floor_rendering_regression_audit.md`](2026-09-05_perlin_noise_floor_rendering_regression_audit.md) — native-resolution reproduction of the current approximately 8 FPS Perlin-floor regression, correctness findings, and ranked optimization frontier.

## Scope boundary

Geometry/OntoMath audits whose primary concern is representation rather than rendering
performance remain at the root of `docs/audits/`. This keeps the directory thematic
without implying that OntoMath belongs to the rendering channel: authored mathematics
remains the source; rendering is one channel that reads it.

---

**Maintainer:** Codex  
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`  
**Created:** 2026-09-05T11:59:28-07:00
