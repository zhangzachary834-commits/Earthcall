# Ray-Domain Transport Compilation

**Origin:** Zach's Ray Transport Canon (revived Original Prism Sun, 2026-10-16): compile surviving world mathematics into the smallest lawful one-dimensional transport program per camera-ray segment. The V5 sample loop remains the exact fallback.

**Status (2026-10-16):** Rungs 0 and 1 are built as Kernel/OntoMath infrastructure with CPU witnesses. **No renderer consumer exists.** Full contracts, evidence and continuation: [plan](../../../../../plans/RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md).

## Done
- [x] Rung 0 — `OntoMath::restrictToLine` (`src/Singularity/OntoMath/LineRestriction.*`): exact polynomial subset, named refusals, symbolic ray coefficients, proved exact vanishing. Witness `ontomath_line_restriction_test` (57/57, Linux).
- [x] Rung 1 — `Rendering::VolumeRayFormCache` / `planVolumeRaySegment` / `executeVolumeSegment` / `affineSegmentTransmittance` (`src/Singularity/Screen/VolumeRayTransport.*`): exact-empty, constant closed form of the accepted lattice, affine transmittance. Witness `volume_ray_segment_transport_test` (49/49, Linux) against a CPU lattice mirror and the real WGSL emitter's flags.

## Open, in recommended order (pitfalls inline)
- [ ] **Rung 1b — GPU consumer for structurally constant, unlit media.** Classify per revision rather than per ray (exact zero direction components are measure-zero at pixel centres). Eligibility joins the module identity. Before retention you need native pixels vs the current lattice (not byte-identical, so state the ~1e-6 relative T outcome at 8-bit), zero-proof and cutoff interplay, ABBA cost, and device reconstruction. A scene with one enabled incident source gains nothing.
- [ ] **Rung 2 — radial-piecewise segmentation.** The census puts 12 of 32 real media in clamp/abs/min/max + Length/Distance/Sqrt over polynomials, and 0 in clamp/abs alone. Boundaries are polynomial roots in s. Widen outward and never inward, or a medium goes silent.
- [ ] **Rung 1c — constant σ with polynomial source.** The arithmetico-geometric closed form cancels catastrophically for thin media, so it needs a stable evaluation or a certified threshold.
- [ ] **Rung 3 — ray-domain sharing** keyed on (form, binding identity). Source, occluder and receiver coordinates are never shared with medium coordinates.
- [ ] **Rung 4+ —** affine-argument transcendentals, polynomial σ, certified quadrature/tails. Approximation is a separate office and needs an author-visible policy.
- [ ] Reduce `ScalarForm::plus` quadratic re-normalization before any live consumer lowers heavy polynomials (`(x+y+z)^6` takes ~66 ms in Debug).

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-16.*
