# Ray-Domain Transport Compilation

**Origin:** Zach's Ray Transport Canon (revived Original Prism Sun, 2026-10-16): compile surviving world mathematics into the smallest lawful one-dimensional transport program per camera-ray segment. The V5 sample loop remains the exact fallback.

**Status (2026-10-16):** Rungs 0 and 1 are built as Kernel/OntoMath infrastructure with CPU witnesses. **No renderer consumer exists.** Full contracts, evidence and continuation: [plan](../../../../../plans/RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md).

## Done
- [x] Rung 0 — `OntoMath::restrictToLine` (`src/Singularity/OntoMath/LineRestriction.*`): exact polynomial subset, named refusals, symbolic ray coefficients, proved exact vanishing. Witness `ontomath_line_restriction_test` (57/57, Linux).
- [x] Rung 1 — `Rendering::VolumeRayFormCache` / `planVolumeRaySegment` / `executeVolumeSegment` / `affineSegmentTransmittance` (`src/Singularity/Screen/VolumeRayTransport.*`): exact-empty, constant closed form of the accepted lattice, affine transmittance. Witness `volume_ray_segment_transport_test` (49/49, Linux) against a CPU lattice mirror and the real WGSL emitter's flags.

- [x] Rung 2 — radial-piecewise segmentation (Zach, 2026-10-17): `OntoMath::restrictAlgebraicToLine` / `LineAlgebraicEvaluator` / `segmentLine`, plus `Rendering::scheduleVolumeRaySegment` / `executeVolumeSchedule`. Witnesses: `ontomath_line_segmentation_test` (37/37) and `volume_ray_radial_segmentation_test` (25/25). Admits 12 of 32 real densities and proves 69.8% of their random-ray samples empty, where the zero-proof grid proves 0 cells. [Plan and findings](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md).

## Open, in recommended order (pitfalls inline)
- [ ] **Rung 2b — GPU closed-form radial support intervals.** One sphere/ray quadratic per member per ray trims traversal inside the authored box. Never move box events or sample spacing. Widen roots outward for f32. Native pixels and ABBA cost are required. Do NOT port branch-and-bound to the fragment shader: CPU scheduling costs ~0.2 ms per ray in Debug.
- [x] **Rung 2c — per-ray affine-argument transcendentals** (sin/cos/exp; Ln stays refused). 14 of 32 real densities are now admitted. See [Rung 2 plan §6a](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6a-rungs-2c-and-2d--implemented-2026-10-17-second-pass).
- [x] **Rung 2d — cost**: leaf-range memo and two-phase schedule kept; analytic proposals measured CPU-neutral and default off. Release mean for the original twelve media is 24–27 µs/ray.
- [ ] **Rung 2e — Perlin bounds on a ray.** 15 real densities stop at Noise. OntoMath already owns a proved value bound (|noise| ≤ 3.81) and Lipschitz bound. A bounded-noise node would prove emptiness wherever a clamp saturates despite the noise. Exactness stays with the lattice wherever noise survives.
- [ ] Outward-round lone constant terms in `ScalarForm::evalRange` (`Term::evalRange` starts from an un-widened float). This is the proof consumers' office; see Rung 2 plan §5.2.
- [ ] **Rung 1b — GPU consumer for structurally constant, unlit media.** Classify per revision rather than per ray (exact zero direction components are measure-zero at pixel centres). Eligibility joins the module identity. Before retention you need native pixels vs the current lattice (not byte-identical, so state the ~1e-6 relative T outcome at 8-bit), zero-proof and cutoff interplay, ABBA cost, and device reconstruction. A scene with one enabled incident source gains nothing.
- [ ] **Rung 1c — constant σ with polynomial source.** The arithmetico-geometric closed form cancels catastrophically for thin media, so it needs a stable evaluation or a certified threshold.
- [ ] **Rung 3 — ray-domain sharing** keyed on (form, binding identity). Source, occluder and receiver coordinates are never shared with medium coordinates.
- [ ] **Rung 4+ —** affine-argument transcendentals, polynomial σ, certified quadrature/tails. Approximation is a separate office and needs an author-visible policy.
- [ ] Reduce `ScalarForm::plus` quadratic re-normalization before any live consumer lowers heavy polynomials (`(x+y+z)^6` takes ~66 ms in Debug).

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-16.*
