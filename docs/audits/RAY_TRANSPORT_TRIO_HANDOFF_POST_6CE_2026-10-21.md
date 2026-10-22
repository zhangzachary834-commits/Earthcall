# Ray transport handoff — Firmament, Sparkly Guy, Prism Sun

Zach explicitly transfers the torch to this trio because Codex is at 4% usage. This continues the [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Stop new work in this session; no native jobs remain running. No production renderer, defaults, authored budgets, lights, saves or installed app changed in this review. No push or PR was performed by Codex.

## Exact landing point

- Working checkout: `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`, branch `claude/ray-domain-transport-rung0`; pre-closeout HEAD `243add9b1`. The final review/handoff commit is recorded in Intercom after creation.
- Default checkout: `/Users/zacharyzhang/Documents/GitHub/Earthcall`, branch `sync-from-earthcall-main`. Firmament's counsel arrived there from Gemini. Its copy in this ray checkout is verbatim; retain his authorship. Default's local counsel/Intercom changes are not ours to commit or erase.
- Read the [formal counsel](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CE_CANONICAL_BOW_PRODUCER_2026-10-21.md) together with the [Codex response](FIRMAMENT_CANONICAL_BOW_REVIEW_RESPONSE_2026-10-21.md), its JSON and executed scratch witness. The plan carries a **§6ce counsel addendum; §6cf is not implemented**.
- Source of status: [live dashboard](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_RAY_TRANSPORT_PROGRESS_BAR_2026-10-21.md). Do not turn a sparse consumer result into a shipped FPS claim.

## What survived the last passes

| Pass | Actual evidence | Disposition |
|---|---|---|
| §6cb, `a47e5264a` | Repaired the 253 omitted overlap records, original-AST comparator and five closed-form GPU cases. Fusion was about 18% slower. | Keep complete ordered overlap handling; reject that fusion. Register pressure was not measured. |
| §6cc, `175988faf` | Matched current V5 marcher versus branch-aware A* on Apple M5/Metal, timestamps off. Two consumer series: 15.859/15.875 ms versus 4.702/4.748 ms, **3.373×/3.343×**. Six aggregate reference gates passed. | Strongest consumer candidate. Resident precomputed schedule, 2,304 distinct rays replicated 128 times; not full viewport or producer-inclusive. Some per-ray errors regress (RGB 474, T 495 of 2,304). |
| §6cd, `906a075f3` | Target-only host preparation about 49.73% cheaper, still about 1.35 ms/ray. Widths 4 and 2 fail accuracy; width 1 passes but consumer advantage falls to about 1.60×. | Reject as shipping producer; retain useful branch split points. |
| §6ce, `243add9b1` | Fixed-bin GPU proposals refuse 1,392/2,304 rays; 2,192 switch-gap sign changes outside proposal windows, with 20 corroborated examples. Proposal-only about 2.22–2.25 ms versus separate current-consumer control 1.32–1.34 ms. 3,200 padded bytes/ray. | Reject direct scanning. Output never entered the consumer. These are separate host-fenced controls, not joint GPU timestamps. |
| This review | Read Gemini/formal counsel, derived/source-checked corrections, executed tiny analytic witness and updated guidance. | No new native result or production mechanism. |

Durable audits are `NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21`, `NORTHERN_VEIL_TARGET_ONLY_PRODUCER_2026-10-21`, and `NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21` under `docs/audits/`, with their JSON/WGSL/row artifacts. Use those over temporary directories.

## Next bounded pass: canonical producer, with the corrections intact

Accept the Firmament's core: derive generic canonical level sets once, share each underlying function across its several thresholds, derive cheap affine roots and conservative curvature/secant enclosures, then instantiate each ray's endpoints compactly on the backend. OntoMath owns mathematical derivation; Screen owns backend execution. Do not hardcode Northern Veil formulas into the renderer.

For `phi(s)=oz+s*dz-f(ox+s*dx)`, a valid `|f''|<=Mx` gives `M=Mx*dx²`, secant bow `M*h²/8`, and derivative radius `M*h/2`. Observed endpoint errors add `(ea+eb)/h` plus enclosure rounding. Preserve original execution provenance, interior execution bounds and transformed error units. A declared model is not a proven Metal Fast transcendental bound. Missing justification retains/refuses instead of silently proving empty.

Six corrections are mandatory:

1. Emerald's saved/probe box half extent is `(160,36,46)`, not density support `(75,16,...)`. Derive/admit the extra support constraints before treating them as an existing clip.
2. Member world x origins are `[0,15,-5,22]`; local-zero roots differ. Share a reciprocal only with valid per-member numerators/enclosures. Preserve parallel cases and edits. Thicknesses are `[3.2,2.8,3.8,3.0]`.
3. A midpoint split does not prove an extremum, smooth panels or adequate integration. `max(0.01-s²,0)` on `[-1,1]`, split into two width-one panels, gives GL2 zero while its exact integral is `0.0013333333333333`. Keep root coverage, uncertainty retention, integration accuracy and fallback work separately reported.
4. Only the curvature term quarters on bisection; execution uncertainty does not. Monotonicity means at most one crossing; existence needs a bracket, and fixed iteration counts do not guarantee bracket accuracy.
5. Ray extrema solve `f'(x)=dz/dx`, not generally `f'(x)=0`. Reusable derivative-monotonicity partitions may help, but instantiate each ray slope. No fixed 8–12-piece capacity is proved.
6. Fewer work classes/bytes do not establish runtime gains. One-thread fusion and <=256-byte capacity remain experiments. Preserve overflow/budget fallback and measure connected producer plus complete consumer.

First reproduce the saved missed crossings and the narrow-lobe witness. Then implement the smallest generic canonical/curvature prototype with explicit unresolved outcomes. Only after coverage and integration qualification, feed **actual producer output** into complete ordered single/overlap A*. Include tangencies, equal-sign double crossings, parallel rays, member-origin edits, computed constants/cancellation and capacity exhaustion. Do not obtain zero refusals by renaming unresolved intervals certified GL2.

## Suggested trio ownership

- **Firmament:** derivation and review of generic canonicalization, curvature/root coverage and mathematical/error admission under the six corrections.
- **Sparkly Guy:** one bounded implementation/harness owner; connect actual compact producer output to complete transport, preserve fallback/work accounting, reproduce artifacts and qualify native combined cost.
- **Prism Sun:** architecture/provenance/lifetime review, independent scrutiny of the accuracy and cost account, and maintenance of the shared progress dashboard.

These are coordinated offices, not permission for three agents to edit the same implementation simultaneously. Announce one implementation owner and one native timing owner in the existing thread; avoid concurrent Metal benchmarks. Read actual branch/status and repository companions first. Preserve other agents' changes; no stash/reset/checkout to clean a shared tree.

## Reproduction and remaining distance

Small review witness (no native GPU):

```sh
python3 scratch/probes/ray_transport_curvature_review_witness.py --root . --output /private/tmp/new-curvature-review.json
```

Existing matched native consumer driver, when the next owner reserves the GPU lane:

```sh
python3 scratch/probes/ray_transport_native_march_comparison.py --root /absolute/ray/checkout --work /private/tmp/new-unique-ray-run --repeat 128 --label series1
```

Use a new external work directory. Read driver help before `--run-only` or other variants. Ordinary native timestamps remain off because Zach reported flicker; label host completion-fenced measurements honestly. Read `docs/BUILD_AND_ENVIRONMENT.md` before building. Probes link configured objects; do not assume an old binary represents edited sources.

Frozen inputs: `saves/zones/Northern Veil/zone.ecform`, native rays `docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json`, independent finite reference `docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json`. Clock 7.25, origin `(0,50,75)`, 2,304 actual f32 rays sampled from 2560×1440, zero authored source. The 65,536-step reference has a convergence witness, not an exact-real oracle. These are fixtures; do not edit inhabited data-home saves.

The consumer gate is demonstrated only for the finite workload above. Still open, in order: **affordable complete producer → connected native producer/consumer accuracy and total cost → unique full viewport including opaque depth/composition → dynamic edits/device/revision/fallback lifecycle → moving inhabited scene and Zach's acceptance**. No shipped optimization or 30 FPS claim yet. Zach permits a substantial faster checkpoint with accuracy about the same or better than current; he does not require the whole campaign in one PR. Keep resolution, authored scene and accuracy controls fixed while earning that checkpoint.

No new Person-visible change this review. Existing inhabited Person Verification remains open. Resume by reading the counsel and response, not by rerunning every historical benchmark or implementing the rejected midpoint/GL2 claim.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T22:17:16.374046-07:00`.
