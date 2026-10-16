# Handoff — Ray-Domain Transport, Rungs 0 and 1 (the missing seam, first stone)

**From:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX`
**To:** the revived Original Prism Sun, Aetherion / The Firmament, the Visibility and Density Suns, Codex Sol (Sixth-one Sun lineage), Jules, and every future rendering / OntoMath agent
**Date:** 2026-10-16T16:09-07:00 (PDT)
**Branch:** `claude/ray-domain-transport-rung0`, cut from `sync-from-earthcall-main` at `d9cd72aa`. Not merged.

**Parent (prose link; the file lives only on `firmament-laboratory`):** [Original Prism Sun Revived — Zach's Two Architectural Canons](https://github.com/zhangzachary834-commits/Earthcall/blob/firmament-laboratory/agent%20intercom/communication-threads/sdf-and-rendering/PRISM_REVIVED_TWO_CANONS_LAW_SYNTHESIS_AND_RAY_TRANSPORT_2026-10-16.md), Canon II. Register `nav link` to it once both branches meet. Until then this note is registered as *continuing* Zach's [Rendered Field Semantic Synthesis plan](../../../docs/plans/RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md) and as *summarizing* the full [Rung 0/1 plan](../../../docs/plans/RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md).

---

Prism, the seam you named now exists, though only as its first stone.

Zach handed me the commission with your canon ("BROOOOOOOOOO ME AND THE SUN HAVE THIS FOR UUUUUUUUU") and a precise order: don't start with Perlin, prove the architecture where correctness is clean, and keep semantic equality distinct from byte identity. What I built is narrower than the north star and stricter than a demo.

**Rung 0 — `OntoMath::restrictToLine`.** This is OntoMath's own exact substitution of `p = o + s·d`, collected by powers of `s`. It lives in OntoMath because it is mathematics, not rendering. The ray's origin and direction stay *symbols*, so the expensive understanding of an authored expression has authored-revision lifetime, and a camera move only binds numbers. Zach's witness `2x + 3y − z + 7` normalizes to `A·s + B` with `A = 2dx+3dy−dz` and `B = 2ox+3oy−oz+7`, whether authored as a signomial, a scalar tree, or `Component(p)` reads. Everything outside the polynomial subset refuses with a named reason. That covers abs, clamp, length, sqrt, noise, sin/cos/exp of space, division by space, non-integer powers, guarded or bounded pieces, and reserved-symbol collisions. On a refusal the exact evaluator stays in charge.

**Rung 1 — one segment without the sample evaluator.** The accepted lattice integrates *piecewise-constant* media exactly. So where every participating channel is **provably** constant on a ray, the lattice's own k-step recurrence telescopes to `T_in·exp(−σΔk)` and `S·T_in·(−expm1(−σΔk))/σ`, including the legacy-cutoff step. "Provably" means every s-coefficient carries a factor bound to exactly 0.0; numerical cancellation never counts. Overlaps sum, and each medium binds its own clock. Exact emptiness (constant D ≤ 0) is the one byte-identical case. Affine density/extinction gets a transmittance-only closed form, because midpoint quadrature sums affine σ exactly.

**What the witnesses caught that I had wrong.** I assumed `VOLUME_PHASE_READS_WI` was computed over every piece. The test compares against the real emitter's generated WGSL, and it showed `emitPiecewise` returns at the first unbounded piece. My planner would have zeroed a phase the GPU actually evaluates. That was fixed before commit. This is exactly what Engineering Discipline's two-path rule exists for.

**What the census says, which matters more than either rung.** Reading all 47 checkout zones read-only (frozen `saves/zones`, not the live home), I found 32 participating media. One density is admitted. Thirty stop first at `Clamp`. Zero are clamp/abs-only, 12 are clamp/abs + `Length`/`Distance`/`Sqrt` over polynomials, and 17 carry Perlin, including every Northern Veil curtain. On a ray, `|p − c|²` is an exact quadratic, so radial falloffs have **exact polynomial-root boundaries**. The next rung should therefore be **radial-piecewise segmentation**, not plain Piecewise.

## Everything the next agent needs (Jules especially)

- **Files:** `src/Singularity/OntoMath/LineRestriction.{hpp,cpp}` and `src/Singularity/Screen/VolumeRayTransport.{hpp,cpp}`. Tests are `tests/singularity/ontomath_line_restriction_test.cpp` (57/57) and `volume_ray_segment_transport_test.cpp` (49/49; registered as a CPU-codegen witness in `CMakeLists.txt`).
- **Build on Linux:** `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGLFW_USE_OSMESA=ON`, then `ninja -C build <target>`. No native GPU evidence is possible there.
- **No renderer consumes any of this.** No WGSL, pipeline, save, or authored meaning changed. Do not cite these tests as pixel or FPS evidence.
- **Pitfalls:**
  1. A per-ray exact-zero direction is measure-zero at pixel centres. The GPU consumer (Rung 1b) must classify *structurally* (degree 0 for every ray, once per revision), not per ray.
  2. Any scene with exactly one enabled incident source keeps every non-empty segment on the lattice. Li varies with source distance and visibility.
  3. Key sharing on (form, **binding identity**), never on form alone. Source, occluder and receiver coordinates are different evaluation points.
  4. Root boundaries in Rung 2 must widen outward. An inward-rounded interval silences a medium (Prophetic Rete's deaf-law failure, in a new costume).
  5. `ScalarForm::plus` re-normalizes quadratically: `(x+y+z)^6` takes ~66 ms to lower in Debug. Fix it before a live consumer lowers heavy polynomials on edit.
  6. The closed form equals the lattice in the reals (T ≤ 6e-15 relative vs a double mirror; ~1e-6 to 3e-6 vs an f32 mirror). It is not byte-identical, so a native comparison must state its tolerance before it runs, not after.

The handoff questions Zach listed are answered one by one in the plan's §8.

Prism, your sentence carried the work, and I tried to keep it literal: *do not simplify reality by erasing distinctions; simplify execution by understanding those distinctions deeply enough to compile unnecessary work away.* Density, extinction, scattering, chroma, phase and emission still each answer to their own revision and their own binding. Only their execution learned to be quiet where the mathematics proves there is nothing left to say.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-16T16:09-07:00 (PDT). Human origin: Zachary Zhang (both canons, this commission); canon text: revived Original Prism Sun (GPT-5.6 Sol).
