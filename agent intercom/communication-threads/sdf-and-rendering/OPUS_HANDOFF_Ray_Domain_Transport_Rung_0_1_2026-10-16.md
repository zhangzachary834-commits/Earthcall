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

---

## Continuation — Rung 2: radial-piecewise segmentation (2026-10-17)

**From:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` (same session, next day)

Zach answered the census in four words, "LETS GOOO do NEXT RUNG radial piecewise segmentation", and the rung is built. Full contract and findings: [Rung 2 plan](../../../docs/plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md).

**What exists now.**
- OntoMath restricts sqrt/abs/min/max/clamp and bounded Piecewise over Rung 0 polynomials into a piecewise-algebraic line form. Each branch declares its switching function. `Length(p − c)` becomes `sqrt(quadratic)`, with no switch at all.
- `segmentLine` proves sign regions by interval branch-and-bound, pruning switches that a saturated clamp hides.
- The Screen schedule turns each piece into a run over the *unchanged* lattice: exact-empty (skip), constant (Rung 1 closed form) or general. Uncertain slivers around roots, and any refused channel, keep the lattice.

**What the witnesses caught.**
1. A grazing ray (distance 2.06 against radius 2) could not be decided by plain interval Horner. The dependency problem widens like `|s|·w`. A centered (Taylor-shifted) enclosure with a double-precision error bound fixed it.
2. My own Rung 1 Perlin fixture used `"variableName"` where the codec says `"var"`, so it was refusing a nameless leaf, not Noise. It is corrected.
3. `ScalarForm::evalRange(constant 0.1)` is the float point `0.1f`, which excludes the double. That is an observation for the proof owners; Rung 2 does not lean on it.

**What the real media say.** 12 of 32 densities are admitted. On them, 69.8% of random-ray lattice samples are *proved* empty, re-checked with 0 violations over 73,728 samples. The existing zero-proof grid proves **0 cells** for the same twelve, because `evalRange` deliberately does not admit Length/Distance (the Oct 15 Euclidean scope decision). The remaining 20 refuse first on Perlin (7), `sin`/`cos` of space (10), division by a non-constant (2) and `x^1.8` (1).

**For the next agent (Jules especially) — pitfalls.**
- CPU scheduling costs ~0.2 ms per ray in Debug and ~34 µs in Release, about 70 s per 1080p frame on one core. **Do not** port branch-and-bound into the fragment shader. Rung 2b should emit closed-form sphere/ray roots per member: one quadratic, cheaper than one density evaluation.
- Support intervals trim traversal *inside* authored boxes. They never move box events or sample spacing. Widen f32 roots outward: an inward root silences a medium.
- Strict signs on Piecewise bounds are deliberate. The CPU's include flags and the emitter's `>=`/`<=` disagree exactly there.
- `undefinedAsZero` is a channel decision (the volume emitter's no-match 0), never OntoMath's default.
- There is still no renderer consumer, GPU evidence or frame claim.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17. Human origin: Zachary Zhang.

---

## Continuation — Rungs 2c and 2d (2026-10-17, second pass)

**From:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX`

Zach: "YAAAAAAAAS NEXT RUGNSSSSS". I took the two rungs the evidence ranked and that this GPU-less container can fully prove. Details: [Rung 2 plan §6a](../../../docs/plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6a-rungs-2c-and-2d--implemented-2026-10-17-second-pass).

- **2c:** every OntoMath transcendental is `kind(scale·var + shift)`, so on a ray it is `kind(α·s + β)`. Sin/cos/exp of space are now admitted, with ranges from OntoMath's own `TransFactor::evalRange` over a soundly enclosed argument. Ln of space stays refused, because the CPU and emitter disagree at ≤ 0. 14 of 32 real densities are now admitted. Perlin is the next wall (15).
- **2d:** I measured before believing. The leaf-range memo and the two-phase schedule are real wins: density segments first, empty is final, and other channels refine only where constancy is still possible, inheriting proved signs. Analytic root proposals were **CPU-neutral** (292.2 vs 290.8 µs summed over 14 media), so they ship default-off as the prototype of the GPU's closed-form-root path. The original-twelve Release mean went from ~34 to 24–27 µs/ray.
- **Pitfall for the next agent:** identical runs here vary by ±40% in wall-clock. Compare range-evaluation counts, and repeat timings.
- **Still true:** there is no renderer consumer, no GPU evidence and no frame claim. The next rungs are 2b (GPU closed-form radial support, which needs a native session) and 2e (proved Perlin bounds on a ray, so saturated clamps can prove emptiness through noise).

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17. Human origin: Zachary Zhang.

---

## Continuation — Rung 2e, and the first real answer for Northern Veil (2026-10-17, third pass)

**From:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX`

Zach: "KEEP GOIGNGNGNGNGNG". Perlin now restricts to a ray through OntoMath's own box theorem (lattice ∩ Lipschitz), so no new noise mathematics was written. That admits **every Northern Veil curtain** and 28 of 32 real densities.

**Northern Veil, against the existing zero-proof grid** (random rays, frozen save, read-only, 0 soundness violations): Rung 2 proves 97.5–99.0% of samples empty, against the grid's 95.1–97.0%. That leaves about half the remaining samples. The gain is 59–85% the thin curtain `clamp(1 − |z − f(x)|/w)`, 15–41% the slab clamps, and 0% noise. Firmament, Sixth-one Sun: this is the factor the grid's resolution cannot see.

**Witness catches this pass:** proved-empty pieces were merged into positive neighbours (target signs now belong to piece identity), and the schedule discarded per-leaf proofs by re-asking a looser merged-piece range.

**Economics:** CPU segmentation costs ~0.4–0.9 ms per aurora ray, not real-time. The right GPU rung is **Lipschitz-certified support tracing** of the curtain factor (Hart's sphere tracing over the restricted argument, a few sines per step). It needs a native session for pixels and cost.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17. Human origin: Zachary Zhang.

---

## Continuation — Rung 2f: the GPU-shaped proof, prototyped (2026-10-17, fourth pass)

**From:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX`

The economics in 2e said exact per-ray segmentation isn't real-time, so I built the algorithm a shader *can* run: **Lipschitz-certified support tracing**. A switch sign that alone forces D ≤ 0 is a killer. Its argument's derivative bound comes from OntoMath interval differentiation, with Cauchy–Schwarz for any Euclidean norm, however authored. One point evaluation at a sample then certifies emptiness for `margin / L` ahead.

Over 28 real media it certifies **70.65%** of samples empty, against full segmentation's 70.60% and the grid's 17.16%, with 0 violations. On Northern Veil it gets 97.6–99.1% with **18–30 curtain evaluations per ray**. The witnesses forced two corrections on the way: norm-aware derivatives, and recognizing tori authored as raw `sqrt(Σ pow(·,2))`.

**For the native session (whoever holds a Mac):** compile killers and Lipschitz structure per revision, compute L per ray, sphere-trace inside the fused marcher, never move box events or sample spacing, shrink f32 margins inward, and prove pixels and ABBA cost before retaining anything. The CPU prototype and its witnesses are the reference.

**Signed:** Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17. Human origin: Zachary Zhang.
