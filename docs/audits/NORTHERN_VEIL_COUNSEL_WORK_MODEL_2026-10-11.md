# Northern Veil: screening work before implementing another optimization

**Commission:** Zach asked to consider rendering, caching, compilation and mathematical evaluation together, rather than continue implementing locally plausible hypotheses. This is fresh CPU analysis supporting [the response to the Sixth-one Sun](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#astra-counsel--derive-the-work-before-choosing-its-storage). Zach originated that whole-system direction; Codex devised and executed the screening models below.

**Signed:** Codex / GPT-6 / session `01a12cb8-ac95-7b13-827d-2c7f289aa560` / 2026-10-11T15:02:22-07:00. “Astra counsel” names the requested role; no more specific model variant is asserted by this signature.

## Evidence boundary

HEAD is `790277e889b5246dde8fd2e351c00e369c8f89c3`, with the Sun's uncommitted rendering continuation present. No production source, authored save, sample policy or existing experiment was changed. No GPU timing, native pixel comparison, engine launch, or live movement verification was performed in this counsel pass.

A new scratch C++ executable read the actual MessagePack-wrapped Northern Veil save through `FieldNode::fromJson` and called `buildVolumeZeroProof`. `VolumeZeroProof.cpp` was freshly compiled into a separate scratch object; the executable linked the remaining existing app objects. Its proof dump matched the initial dump from the existing proof object byte for byte. This is a CPU proof-data witness, not a fresh rebuild of every dependency or a native soundness certificate.

Provenance:

| Input | SHA256 |
|---|---|
| `saves/zones/Northern Veil/zone.ecform` | `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496` |
| `src/Singularity/Screen/VolumeZeroProof.cpp` | `c252f061868d3d7135f6913a53e7488c3d09abed1245b31a6c68c830cf16c4bc` |
| `src/Singularity/Screen/VolumeZeroProof.hpp` | `b730919be4110df16191441e9d77156775d091bcabe05645b37e3100f41f86ad` |
| `src/Singularity/Screen/WebGPU/SdfWgsl.cpp` | `0e0a5923029128c0eb16c9757cc8f28140e85c4b206ebe6c032e6caa48b4d801` |

Scratch evidence: `/private/tmp/earthcall-astra-counsel-01a12cb8/`: `dump.cpp`, `dump`, `proofs.json`, `screen.py`, `screen-results.json`, `provenance.json`. These machine-local files may disappear; the method and results are preserved here.

## Model and its deliberate limits

The Python model used 2,304 stratified native-pixel coordinates: `(20+40*x+0.5, 20+40*y+0.5)`, for `x=0..63`, `y=0..35`, in a 2560×1440 framebuffer. It used the existing inside camera `(0,50,75)` looking at `(40,62,110)`, 60-degree vertical FOV, four original box entry/exit events, 96 samples per chord, and the fused segment-spacing rule. No renderer resolution was changed.

It modeled the current inside path's per-medium support intervals, original proof cells, and cell-exit jumps. It used Python double arithmetic, omitted opaque clipping and the opacity stop, and did not evaluate noise, radiance, phase or transport. Geometry classifications used the prior equation model's coefficients, whose saved-source hash matches the current save. Counts therefore describe a full-lattice, real-arithmetic screening workload. They are neither actual GPU instruction counts nor an exact f32 replay. The stratified rays also cannot measure SIMD divergence among adjacent native pixels.

## A direct empty-block index has a modest opportunity here

The existing CPU builder proves blocks but stores leaf bits. An alternative screening representation assigns each empty leaf a four-bit code for its largest all-empty aligned dyadic ancestor. Unknown leaves have code zero. Ancestors clip at non-power-of-two grid boundaries. An independent exhaustive check verified that every encoded block contains only leaves already marked proven; it establishes a combinatorial enclosure invariant, not the underlying CPU-to-GPU mathematical proof.

The modeled shader uses one direct code lookup and computes the corresponding box exit. It never walks a hierarchy or retains a per-ray interval cache. This differs from the earlier rejected hierarchy, which searched up to four levels during execution.

| Modeled inside work | Current leaf walk | Direct block codes |
|---|---:|---:|
| Loop visits | 173,510 | 158,449 |
| Proof lookups | 228,944 | 211,364 |
| Unproven medium/sample demands | 147,801 | 147,801 |
| Skipped lattice indices | 307,013 | 307,013 |

All 2,304 rays retained identical ordered `(segment, sample index, unproven-medium mask)` sequences. Visits fell **8.68%**, lookups **7.68%**. Packed codes would occupy **91,184 bytes**, versus **22,796 bytes** for the existing bit words, before other metadata. This is a reason to defer implementation: it trades four times the proof storage for a modest work-count reduction and leaves all shape demands intact. It is not a failed GPU benchmark, nor proof that every alternative empty-block representation has the same limit.

## More expensive rejection remains inside the shared shape

The same modeled demands classify as follows:

| Shape stage | Medium/sample count |
|---|---:|
| Enter shared shape through an unproven cell | 147,801 |
| Reject through existing cheap X/Y factors | 14,977 |
| Reach the three fold trigonometric calls | 132,824 |
| Those folds establish zero ribbon factor | 65,638 |
| Reach geometric support and potentially fluting/noise | 67,186 |

Thus **49.42% of modeled fold evaluations establish zero**. This is a different opportunity from traversing already-proven empty cells. Existing factor exits already avoid fluting/noise on those zero samples; a new guard must not count their Perlin evaluations as additional savings.

A second model partitions each medium's entire authored local X extent into equal bins. For each bin it encloses the sum of its three fold terms using endpoint and interior-extremum ranges, then expands by ribbon half-width. It checks this interval after the existing X/Y exits and before the fold. The table contains bounds, not interpolated values.

| Bins per medium | Four-media table bytes | Rejected modeled fold evaluations | Fraction of fold evaluations |
|---|---:|---:|---:|
| 128 | 4,096 | 46,701 | 35.16% |
| 256 | 8,192 | 56,066 | 42.21% |
| 512 | 16,384 | 60,705 | 45.70% |

Every modeled rejection was checked against the independently evaluated real-valued ribbon condition. At 256 bins the avoided three-call folds correspond to **31.57% of modeled spatial trig call sites**, including the two fluting calls that remain for geometric-positive samples. No noise calls are removed. These percentages are not elapsed-time predictions; loads, branch coherence, live state and driver lowering remain unmeasured.

**This table is not an admitted exact certificate.** Its `1e-4` numerical pad is a diagnostic choice, not a backend error bound. Python sin/cos ranges and source coefficients do not enclose every allowed WGSL evaluation. Before implementation, interval construction must account for actual f32 parameter conversion, expression order, argument rounding, intrinsic accuracy, coordinate/bin boundaries, and final clamp semantics. Unsupported domains retain the original evaluator. Broadening the bounds can reduce or eliminate the opportunity above.

## Structural finding requiring a numerical contract

`TransFactor::evalRange` in `ScalarForm.cpp` uses host sin/cos extrema and `Interval::outward`; the latter widens float endpoints by one representable step. That source establishes endpoint widening, but does not by itself establish an enclosure of every permitted GPU intrinsic result. WGSL permits reassociation/fusion and specifies intrinsic accuracy by operation and input range; its f32 sin/cos bound is stated for `[-pi,pi]`. Some authored fold arguments leave that range. A tighter GPU skip certificate must supply a justified backend bound or decline that bin. [WGSL floating-point rules](https://www.w3.org/TR/WGSL/#floating-point-evaluation).

This is an identified proof obligation, **not a reproduced incorrect frame** and not a finding that every current proof is invalid. Existing spatial margins and witnesses retain their documented scope. It is especially relevant before tightening them: the three failed lookup experiments demonstrate that isolated compute equality did not establish equality inside the complete fragment program.

## Decision

Prioritize a bounded production-kernel investigation and backend-aware support certification. The first optimization candidate is a generic, sample-local support guard before expensive shape factors, provided conservative bounds leave a meaningful opportunity. Its initial form skips only a proven-zero shape evaluation and preserves all lattice indices; traversal jumps are a separate later decision.

Do not implement the direct-block table merely because its proof is easy, and do not call the support guard faster from these counts. The counsel thread specifies the dependencies, fallback, measurement gate and alternatives. No new Person-facing behavior was created; the existing Northern Veil movement, appearance and usual-framebuffer checks remain open.
