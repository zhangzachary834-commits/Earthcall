# §6dy — review and repair of the 15-cell pending-stack certificate

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-26T22:07:12.505345-07:00.

Zach reported “THE SPARKLY GUY CAME BACK.” This corrective pass reviews Sparkly's `1a105e57c`, responding to [§6dx's allocation experiment](GPU_RAY_TRANSPORT_6DX_STACK_BOUND_2026-10-26.md) and the [§6dx execution assignment](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sixth-one-sun-to-sparkly-guy--mission-6dx-prove-the-pending-stack-bound-then-specialize-allocation-without-switch-expansion). Zach's current-or-better authored accuracy and substantial matched native improvement remain the acceptance rule. This is CPU/source/evidence work only; **no fresh GPU run, capture or timing** occurs in this review.

## Accepted checkpoint, with precise limits

The finite pending-depth model explores **32,767 states and 49,148 transitions** and has maximum occupancy **15**. The certified real solver's executions are a subset of those abstract transitions, so their occupancy is **at most 15**. The abstract maximum is attained; this does not prove a real ray attains it, and certainly does not say every ray uses 15 cells.

The actual source seeds the right half in slot 0 and left half in slot 1, both at depth 1, consumes the final cell first, and either removes it or pushes two children at depth `d + 1` only when `d < 14`. All earlier returns/refusals/prunes remove traces from this abstraction. Deferred sibling depths increase strictly except that the final current cell may share the preceding sibling's depth. Initially `(1,1)` satisfies this invariant. Removing the current cell preserves it; replacing depth `d` by `(d+1,d+1)` preserves it. Therefore the stack contains at most one deferred sibling at each depth plus the current cell, and occupancy is at most `d+1 <= 15`. The depth helpers encode these values faithfully in four bits. Each bin reseeds or exits; bins do not accumulate pending stacks.

For every split-eligible state the newly pushed occupancy is at most 15. At cutoff depth, both the old and smaller guards emit the existing uncertain band regardless of whether their capacity subexpressions differ. Thus a 15-cell allocation changes neither the original successful traversal nor its refusal/band behavior. It changes no midpoint arithmetic, accepted depth, work budget, endpoint operation, root coverage or quadrature.

The saved native program for the smaller array passes **60 cases**, covering both actual modes, all repeated/changed poses, four work budgets and two capacities. Source-bound replay checks output bits, final ownership, packet parity, terminal-only poison and actual S1 endpoints. This is a replay of Sparkly's native run, not a new native execution. The seven negative GPU variants from §6dv were **not** rerun in §6dx; complete fresh negative-variant qualification remains unclaimed.

## A real certificate gap, repaired without changing shaders

The original `verify_source_matches_proof` only searched for seven expected strings anywhere in the shader. It did not check the pop or whether the cutoff dominated splitting. Direct counterexamples were admitted by the old guard:

- Remove `top--` while leaving all searched phrases intact.
- Retain `f32(depth) >= 14.0` inside `false && ...`, making the cutoff ineffective.
- Change the seed count and keep `top = 2u;` inside a comment.

The archive preserves the original function. The portable replay reproduces these three admissions in each S1 shader and verifies that the repaired guard refuses all six instances. These findings invalidate the old guard as a future admission certificate; they do not show today's unchanged 15-cell shader produced wrong pixels.

The repair binds the complete normalized token bodies of `roots`, `getDepth` and `setDepth` to the reviewed transition proof. Comment handling includes nested block comments; formatting/comments are harmless, while changed control flow, seeding, child depths or depth decoding require a renewed proof. This deliberately recognizes a known implementation and declines others; it is not a general WGSL theorem prover. Unknown implementations may not silently inherit the smaller allocation.

Fourteen actual emitted-source mutations are rejected: seven changes in each of unfused/terminal S1, including pop, ineffective/changed cutoff, comment-decoy seed/push changes, child-depth changes and broken depth decoding. The guard and allocation-count checks use explicit exceptions, and the proof-only command passes under **Python `-O`**. Positive formatting and nested-comment cases also pass. The two changed model-parameter witnesses still yield bound 16. The abstract enumerator now uses a deque without changing its returned proof counts.

`--prove-only` now checks the actual emitted source and mutations; it previously returned after testing only the abstract model. Compiler-table extraction now skips buffer payloads by header, fails on malformed compiler plists and requires two S1 rows instead of silently swallowing missing evidence.

**All five executed native C++ programs reconstruct byte-for-byte after the repair:** both clean timing programs, both capture programs and the 60-case control. Consequently there is no altered shader to justify rerunning the GPU or retiming this losing candidate.

## Compiler evidence survives; broader memory claims do not

Exact complete-WGSL hashes bind the observed Apple M5 S1 pipeline objects to the saved sources. The raw compiler plists confirm both rows:

| S1 row | Canonical spill bytes | 15-cell spill bytes | Temp regs | Uniform regs | Canonical instructions | 15-cell instructions |
|---|---:|---:|---:|---:|---:|---:|
| unfused | 2,000 | 1,760 | 79 | 72 | 5,153 | 5,093 |
| terminal | 2,000 | 1,760 | 83 | 88 | 6,435 | 6,375 |

This is a useful controlled compiler response: **240 fewer reported spill bytes, unchanged register fields, 60 fewer compiled instructions**. It does not prove optimal register allocation, dynamic spill traffic, an exact frame-memory layout or a latency cause. Source allocation shrinks by 80 logical bytes; the compiler's 240-byte response cannot be reverse-engineered into exact ownership of every remaining byte from that fact alone.

The remaining named declarations are `bands: array<vec2f,32>` (**256 logical bytes**), `knots: array<f32,96>` (**384**) and `pieces: array<vec2f,48>` (**384**), not the larger vec4 arrays in the draft. Those 1,024 logical bytes do not explain the entire 1,760-byte compiler field. More fundamentally, a depth bound limits **pending work**; it does not bound **retained results** such as all bands, candidate boundaries or pieces. Their bounds need their own argument, including level sets, affine endpoints, uncertain bands, merging order, capacities and refusal behavior. A merged-span census cannot replace that proof.

## Cost: still no admitted Current improvement

There are two saved programs: one canonical invocation and one 15-cell invocation. Each contains two ordered internal blocks with 8 observations per mode/repeat/block, pooled to 16 per mode (64 across the four modes). Neither candidate has two fresh independent timing invocations. The unchanged and smaller-array programs were executed in separate invocations, so their raw deltas are not a same-session paired allocation-only latency ablation.

The 15-cell candidate's own matched Current comparisons are:

| Repeats | Unfused mean / median vs Current | Terminal mean / median vs Current |
|---:|---:|---:|
| 1 | +1.425% / +0.454% | +1.917% / +0.657% |
| 2 | −11.888% / −0.057% | +6.045% / +1.856% |
| 4 | +2.726% / +0.864% | +9.140% / +11.197% |

At repeat 1 the terminal **median** gap is 16.626 µs; the **mean** gap is 48.529 µs. “Within 17 µs” must not describe the mean, all repeats or a full interactive frame. These are sampled resident query-dispatch submit-to-completion host clocks, excluding prior host preparation/upload/compilation, not pure GPU timestamps, observed clock-frequency states, full-frame FPS or inhabited movement. The first-repeat proximity and isolated favorable unfused mean do not establish a substantial repeated mean-and-median win.

Both saved invocations pass the frozen 32,784-query and 192 dual-resolution reference gates, with 12,102 protected-affine matches within the inherited harness scope and actual S1 endpoint admission supplied separately by the matrix. Original-pose benchmark packet records still reconstruct terminal reference packets; they alone are not actual terminal captures.

**Retain the proof/allocation recipe as a research checkpoint; do not promote a renderer or retime this same candidate without a new material hypothesis.** No fresh performance confirmation is justified by the existing losing screen.

## Durable evidence and reproduction

[The index](GPU_RAY_TRANSPORT_6DY_STACK_BOUND_REVIEW_2026-10-26.json) pins the [14,311,528-byte archive](GPU_RAY_TRANSPORT_6DY_STACK_BOUND_REVIEW_2026-10-26.zip), original Sparkly documents/source, executed C++, raw compiler tables, pipeline facts and byte-exact report deltas against the existing §6dv archive. The compiler extracts omit GPU buffer snapshots and are not a complete replayable `.gputrace`. Saved native bytes are compared with independent actual-file digests before delta storage; this is not reference data substituted for execution.

```sh
python3 -B scratch/probes/ray_transport_gpu_6dx_stack_bound.py --root . --prove-only
python3 -O -B scratch/probes/ray_transport_gpu_6dx_stack_bound.py --root . --prove-only
python3 -B scratch/probes/ray_transport_gpu_6dy_review_replay.py --root . \
  --record docs/audits/GPU_RAY_TRANSPORT_6DY_STACK_BOUND_REVIEW_2026-10-26.json \
  --archive docs/audits/GPU_RAY_TRANSPORT_6DY_STACK_BOUND_REVIEW_2026-10-26.zip \
  --donor-archive docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip \
  --output /private/tmp/6dy-replay.json
```

## New Firmament strategic counsel received during review

The new uncommitted Firmament strategy draft `FIRMAMENT_STRATEGIC_REVIEW_POST_6DX_ARE_WE_ON_THE_RIGHT_TRACK_AND_M5_TRUE_MASTERY_2026-10-26.md` was read while this review was finishing; its read-only snapshot is preserved as `counsel/firmament-strategy-draft-read-snapshot.md` inside the review archive, with the original file left under its author’s ownership. Its recommendation to seek structural work reduction is useful. Its numerical and hardware guarantees need the same admission boundaries as other proposals: no “airtight” universal proof, dynamic L1 thrashing inferred from static spills, guaranteed zero-spill kernel, zero-cost animated table, or 12–16× classifier/FPS gain follows from the current witnesses. The 8.01× error ratio is confined to the named frozen holdouts, and 11–17 µs describes selected first-repeat medians, not all costs.

**Concrete mathematical correction.** Let the intrinsic wave be `w(u)=sin(u)` on `[0,π/2]`, which is monotone increasing. Use a normalized ray with `dx=2/√5`, `dz=−1/√5`, and choose origins zero. With `u=dx*s`, the actual level-set field is `Phi(s)=sin(u)−u/2`. Its derivative changes sign at `u=π/3`; the endpoint values are 0 and `1−π/4≈0.214602`, while its interior maximum is `√3/2−π/6≈0.342427`. Level 0.3 has two interior crossings although both cell endpoints lie below it. Thus wave-extremum partitioning alone cannot certify the ray-restricted field monotone. Any admission must enclose `dz + dx*w'(u)` away from zero with the applicable execution error, handle reversed/parallel directions and uncertainty, or retain the general solver.

A table evaluated at exact intrinsic knots also does not automatically equal the authored float32 evaluation at a rounded pulled-back ray coordinate. Animation preparation, uploads and invalidation cost must be counted. `var<workgroup>` data live only within one workgroup invocation and do not persist into a later producer dispatch; sharing across stages requires explicit materialization or a separately qualified fusion. Parallel classification must respect the 16-ray segment within 32-wide execution, sparse/padded lanes, uniform barriers and current supported features. Static memory availability is not a speedup measurement.

A separate monotone-regime residual kernel is an architectural hypothesis, not an already proved zero-array/zero-spill executable. Admission needs real per-level predicates, chronological uncertain-band/affine-boundary ordering and safe late refusal before publication. A census of monotone bins does not itself prove the fraction of complete rays/tiles eligible for that route.

## Next direction and shipping distance

For Sparkly's next bounded implementation, test **only coalescing the three producer dispatches into one compute pass** in the qualified routed path, with identical shaders/pipeline inputs and unchanged reset/classifier/args, consumer and Current fallback dependencies. This is an encoder/dispatch experiment, not a guarantee that backend hazard barriers disappear or that cost improves. Keep classification parallelization, tables and monotone solver changes out of this candidate. First require source identity and the actual two-mode ownership/packet/refusal matrix; then a matched clean rejection screen. Stop a loser. A possible winner needs fresh independent matched confirmation and complete cost scope. Do not merge across producer-to-consumer dependencies speculatively.

After this isolated test, further solver work should start from measured remaining demand and exact duplicate adaptive-interior `evalPhi` work across level sets on the same clipped rays. The nine coarse knot evaluations and prepared factors are already shared; do not count that benefit again. Reuse existing census machinery, preserve source/fixture identities and check that instrumentation has not changed the path being counted. Counts/CPU models are diagnostics, not native cost evidence. Retained bands/knots/pieces need their own capacity argument; no shrinking them by analogy to the pending stack. Return removable work, eligibility and state/ordering costs before selecting a new solver representation. Keep threshold-specific error/slack, affine/uncertain provenance and whole-ray Current fallback.

**All three shipping gates remain OPEN:** profitable connected execution, generic OntoMath/full-frame integration, and inhabited native FPS/Person acceptance. This pass repairs the compiler-certificate boundary and preserves a sound research specialization. It changes no renderer, default setting, authored save, installed app or visible behavior. Existing Person verification remains open; no new Person-only check is introduced. Native lane remains released.
