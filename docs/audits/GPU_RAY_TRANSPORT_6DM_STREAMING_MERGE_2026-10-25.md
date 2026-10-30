# §6dm — Streaming 4-way merge intervention: correctness verified, array storage eliminated, resident cost loses Current

**Signed:** Antigravity / Gemini / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / 2026-10-25T20:15:00-07:00.

**Human commission & Council directive:** Zach: “BROOOOOOOOOO THE SIXTH ONE SUN HAS MORE FOR UUUUUUU”. Followed Sixth-One Sun's (§6dl, commit `4c710a3ff`) directive: test the concrete deletion of `mergeKnots[96]` entirely by streaming the 4-way endpoint merge directly into adjacent-interval emission using scalar `prevKnot`/`bestVal` registers and four head cursors. Active independent checkout: `Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`.

> §6dn review: the table below uses medians of total multi-repeat submissions; mode3 excludes host certificate construction/initial upload. All 12 medians are slower, but only 8 means are slower. New physical spill/register behavior and the cause of the slowdown remain unmeasured. [Independent replay and next steps](GPU_RAY_TRANSPORT_STREAMING_MERGE_REVIEW_6DN_2026-10-25.md). Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-25T23:17:33.279825-07:00.

---

## 1. Intervention Scope & Source Modification

- **Array Elimination:** `mergeKnots: array<f32, 96>` is completely removed from the shader AST. The shader now contains zero 96-entry arrays.
- **Single-Loop Streaming Merge:** Replaced the two sequential loops (buffer all endpoints into array, then scan array adjacent pairs) with a single loop maintaining scalar `prevKnot` and `bestVal`.
- **Invariants Preserved:**
  - Exact tie-breaking order (strict `<` prioritizes lower member index `m`).
  - Duplicate/zero-width knot suppression (`b > a`).
  - Midpoint arithmetic (`a + 0.5 * (b - a)`).
  - 4-unit subdivision arithmetic and boundary selection (`select(..., k == 0u)`, `select(..., k + 1u == n)`).
  - Member mask sweep with monotonic per-member `pIdx` cursors.
  - Capacity guards (`cfg.maxSubPerRay`, `cfg.maxOverPerRay`, `reason |= 16u`, `reason |= 32u`).
  - Retained member 32-band / 48-piece bounds, 70-entry local knots array (scoped to the member loop), and `strictlyAscendingDisjoint` fastpath early return.

---

## 2. Replayed Correctness & Raw Qualification (32,784 Rays)

Both variants (`disjoint-stream-merge-wide` and `ux-tile-cert-stream-merge-wide`) passed full native qualification:
- **Queries:** 32,784 unique rays; 247,326 singles, 1,799 overlaps, 0 normal fallbacks.
- **Bit Parity:** 100.0000% strict bit-exact packet and output parity against the guarded reference.
- **Affine Retained:** 12,102 / 12,102 protected affine endpoints verified within 2 ULP of CPU-f32 reference.
- **Holdouts:** 192 dual-resolution holdouts pass all six current-or-better metrics.
- **Refusals:** Budget 23/24/25 controls and poisoned capacity paths verify proper fallback triggering.

---

## 3. Two Clean Paired Resident-Cost Series — Total-Submission Medians (Apple M5)

Measured uninstrumented mode 3 candidate (`cpFastCompact`) versus mode 0 (`Current`) across two independent resident-cost series. Values are medians of total submissions, before division by repeat count:

### Disjoint Stream Merge Candidate vs Current:
| Series | Repeats 1 delta | Repeats 2 delta | Repeats 4 delta |
|---|---:|---:|---:|
| Series 1 | +0.179% (2.55173 vs 2.54717 ms) | +14.875% (4.37081 vs 3.80483 ms) | +14.763% (10.23433 vs 8.91783 ms) |
| Series 2 | +0.522% (2.56158 vs 2.54829 ms) | +0.699% (3.83373 vs 3.80713 ms) | +20.657% (7.66098 vs 6.34938 ms) |

### UX Tile-Cert Stream Merge Candidate vs Current:
| Series | Repeats 1 delta | Repeats 2 delta | Repeats 4 delta |
|---|---:|---:|---:|
| Series 1 | +0.093% (2.54529 vs 2.54292 ms) | +0.576% (3.88912 vs 3.86683 ms) | +2.999% (9.15365 vs 8.88708 ms) |
| Series 2 | +2.045% (2.67127 vs 2.61773 ms) | +1.222% (5.19619 vs 5.13346 ms) | +0.705% (10.28585 vs 10.21390 ms) |

In all 12 median comparisons, the candidate loses to Current; eight arithmetic-mean comparisons lose and four win. Neither summary establishes a consistent gain. Host certificate preparation and initial upload remain outside Mode 3 timing.

---

## 4. Hardware Attribution & Architectural Conclusion

1. **Array Deletion Does Not Establish a Cost Win:** Deleting `mergeKnots[96]` does not produce a consistent resident gain. Live scalar state is a source-level hypothesis; this pass has no new compiler/counter evidence establishing physical spills or the cause of the cost.
2. **Stop Scratch Trimming on Monolithic Kernels:** As Sixth-One Sun and Prism advised, this representation experiment is concluded. No further ad-hoc scratch trimming on this monolithic shader is warranted.
3. **Path Forward:** The evidence points toward physically separate, faithful residual programs with smaller actual obligations rather than attempting to coerce a monolithic pipeline into fitting cache constraints.

---

## 5. Artifacts & Gate Standing

- Native `.gputrace` packages captured:
  - `/private/tmp/earthcall-6dm-capture-disjoint/disjoint-stream-merge.gputrace`
  - `/private/tmp/earthcall-6dm-capture-cert/cert-stream-merge.gputrace`
- All 3 shipping gates remain **OPEN**:
  1. Profitable connected execution.
  2. Generic / native / full-frame integration.
  3. Inhabited native FPS / Person acceptance.
- Native profiling lane RELEASED.
