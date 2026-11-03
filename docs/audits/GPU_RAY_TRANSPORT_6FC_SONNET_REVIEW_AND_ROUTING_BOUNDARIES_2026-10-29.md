# §6fc — Sonnet review, corrective cleanup and native routing-boundary witness

Zach said Sonnet returned and Firmament gave counsel, then asked **“how good is current on this exact route the witnesses go on have you tested.”** This reviews Sonnet's `f7d29b45f` and [§6fb report](GPU_RAY_TRANSPORT_6FB_SINGLE_COMPUTE_PASS_2026-10-29.md), and answers [Firmament's routing counsel](FIRMAMENT_COUNSEL_POST_6FA_METAL_TRACE_SINGLE_PASS_AND_MASK_ROUTING_2026-10-29.md). Continue the [existing Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Feature checkout `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`. No subagents.

## Verdict and Current on the actual route

**Retain Sonnet's coalesced execution.** The implementation preserves the ordered dispatch calls and shader text; the tested positive path is sound within its witness scope. Independent replay of the retained native observations confirms all **1,536 measured frames**, actual lane/pass counts, camera/clock/focus, paid activation and queue-drained scope. This is a three-lane rotated screen, **not ABBA**. All eight block comparisons favor coalesced over original routed, without establishing a confidence interval.

| Mean complete ms/frame | Current | Original routed | Coalesced routed |
|---|---:|---:|---:|
| Queued | 62.092139 | 63.360328 | 62.420924 |
| Completion-fenced | 66.493459 | 69.335323 | 66.685363 |

Current therefore **was tested on the same scripted 64-pose camera/time path**, native 2560×1440, with the same copied 24-object/four-curtain scene. Coalesced is 1.48%/3.82% cheaper than original routed by ratios of pooled means, but 0.53%/0.29% above Current. No demonstrated Current win, inhabited FPS or 30 FPS milestone. Preserve §6ey's older, different-protocol −6.17% queued/no-fenced-benefit result as dated evidence; do not combine the series.

**Authored accuracy on the whole route remains unknown.** These timing frames do not contain independent reference RGB/T for all 45°/far-100 rays or clocks. The two captured poses test equivalence and refusal, not closeness to the authored equations. Previous 60° references cannot transfer. No new authored-reference rows were computed in this pass.

I independently decoded all 18 retained PNGs, including CRC/filter checks, and verified **206,438,400 RGBA8 comparison bytes** across 14 equal-image comparisons: original repeat, coalesced, poisoned lanes and forced-Current refusal at poses 0/32. Normal routed differs from Current, so the witness is not an inert lane. The complete publication was read and **FNV64-hashed** in Sonnet's native process. Those digests agree, but raw publication arrays were not retained; this replay does not promote digest agreement into independently replayed raw-publication equality. Three forged sample/mode/focus controls refuse.

The old native binary and receipts bind correctly. One provenance limitation is explicit: Sonnet flipped the header default/comment after testing, then compile-verified it. All other recorded source bytes are recovered exactly from his commit. The precise pre-flip header bytes are unavailable; its measured hash is retained, and the later header is **not** passed off as those bytes. The actual coalesced=true path was exercised explicitly in the native run; the default constructor flip itself was not separately run.

## Corrective changes and verification

1. **Remove ordinary-frame dispatch hashing.** Sonnet's FNV loop ran on all 240 dispatches during ordinary rendering. It is now opt-in through `KernelRoutedTransport::collectDispatchTrace`, written by the existing renderer diagnostic setter. Ordinary frames retain cheap execution counts but have `dispatchTrace=0`; diagnostic A/B still collects the hash. This is Kernel observation state, named beneath Screen, not an authored Property or new permission office. Coalescing stays defaultON and Current fallback is unchanged.
2. **Preserve historical witnesses.** Sonnet extended the hash-bound §6ey tool files. His exact executed versions are archived, his successor harness is now `scratch/probes/ray_transport_6fb_engine.{cpp,py}`, and the original §6ey files are restored byte-for-byte from `6601d2606`. Historical full-source replays still require their historical source identity; restoration is not a claim that old evidence qualifies the new renderer.
3. **Independent host interception.** `ray_transport_6fc_encode.py` extracts the current production `ensure/releaseBuffers/encode` bodies and records actual API dispatch calls and active buffer-binding descriptors. Separate-pass/trace-off, coalesced/trace-off and coalesced/trace-on produce identical 240 logical dispatch records and strip configuration words. Pass counts are 240/1/1; hashes are zero/zero/nonzero. This is CPU interception, not new GPU pixel evidence.
4. The real `earthcall_webgpu` Release target builds successfully after the cleanup. No new foreground Engine run or performance comparison was made after this host-only diagnostic change. Emitted WGSL and production routing kernels are unchanged.

## Firmament's useful lead, and what is not yet proved

The routing limitation is real: `kClassifyWgsl` specializes only mask 1 to S1 and the ordered disjoint mask 3 case to S2. Other masks, including zero, go to fallback. But this was a conservative fast-path restriction, not evidence that production currently renders the wrong image. Hoisting producer bind groups is also plausible: only the invariant first 16 bytes of Config are read by those producers. It remains a separate unimplemented ablation; no 125→38 count or gain is claimed here.

Three counsel claims need tightening:

- The ~17.31 ms consumer versus ~63.55 ms fragment-channel comparison is a **source-order-attributed diagnostic comparison**, not an isolated equal-accuracy integrator benchmark or guaranteed 44.99 ms that another change can recover. GPU activity includes stalls and overhead; names were inferred, not observed shader labels.
- Classification's 64 invocations handle **64 different tiles** (`tileId=gid.x`). Their wave knots generally have different domains. The source does not establish 64 lanes redundantly computing the same tile; within each tile the knots are already shared across its rays.
- **Unconditional empty-tile skipping does not preserve every refusal or raw schedule packet.** This was tested on actual Metal, not merely inferred from source.

### Executed native counterexample

`ray_transport_6fc_empty.cpp` compiles the actual classifier and fallback WGSL. Its only external candidate mutation is `if (activeMemberMask == 0u) { return; }`. One tile has 15 valid finite rays, one invalid-marker control, four constant-wave support fixtures with conservative curvature bounds, and no support intersection. The device must report Metal and ordinary timestamp queries are disabled. No production shader was edited.

| Per-ray producer budget | Original valid refusals / accepts | Skip-empty valid refusals / accepts |
|---|---:|---:|
| 0 | 15 / 0 | 0 / 15 |
| 95 | 15 / 0 | 0 / 15 |
| 96 | 0 / 15 | 0 / 15 |
| 1024 | 0 / 15 | 0 / 15 |

The invalid setup marker survives in all eight cases. Original classification emits one fallback tile; the mutation emits none. Original fallback checks budget zero before geometric elimination and charges 24 units for each of the four proven-empty members, so positive budget 95 also changes the refusal outcome. Even at sufficient budget, leaving setup's all-zero count words changes unused base-offset fields from the original producer's packet.

This **does not show a wrong rendered pixel** or prove the optimization mathematically invalid. A new valid emptiness proof could legitimately avoid work. It disproves the unconditional bit-identity claim and shows why force-refusal behavior and proof admission must be handled deliberately. No consumer or Screen image was executed in this tiny witness. The native process exits 0 in **0.768 s**, within the predeclared 60-second cap. GPU lane is RELEASED; no window was opened.

## Next Sonnet quest

1. Keep the coalesced path and diagnostic-off default. Use the separate §6fb harness for new work; historical tool bytes and actual executed sources are in the archive.
2. Obtain a bounded, observer-controlled census of actual route masks on the frozen target views/clocks before quoting empty/single-member savings. Preserve output controls and distinguish diagnostic cost from timing.
3. Start with **one** guarded routing change. Preserve zero-budget refusal for valid producer inputs. Define whether positive-budget acceptance and unused schedule words must match the original, or whether this is new proof admission requiring independent mathematical evidence. Do not silently equate the two contracts.
4. For arbitrary-member S1/S2, consistently carry actual member IDs through all payloads and consumer masks; prove packing bounds and preserve ordering. Test members 1/2/3, reverse disjoint pairs, overlaps, support slivers, budgets, capacities, poison and partial refusal. Do not simply loosen the classifier's mask checks while producers still assume members 0/1.
5. Gate native whole-output/publication behavior before clean complete costs. If optical outputs change, qualify them against their own authored references; no tolerance widening or sampled-max substitution for a full-population claim. Pure mathematical derivation belongs in OntoMath, execution packaging in Screen.
6. In the accuracy lane, capture the actual target route's ray/depth/parameter identities for both Current and routed before new references. Declare a cohort and CPU cap; a bounded cohort is useful but must stay labeled as such. **Current's full-route mathematical quality is still an open question.**

No broader mask-routing implementation, hoisted bind-group change, lighting/sample/resolution change, generic certification or Person acceptance was added. Physical movement/appearance/inhabited cost remain open.

## Evidence

[Compact result](GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.json) · [lossless content-addressed archive](GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.zip). Archive **1,355,614 bytes**, SHA256 `efc6f8fb3e4f1089ea21a6f061f557fa8a64aa76b1c38e43e8cb49bbf3a77bb9`. Includes the 18 saved captures, raw native timing/receipts, exact executed Sonnet tools, explicit missing-header attestation, review controls, current encode interceptor, standalone native source/results and Release build log. No binaries/objects or live saves are committed. Every payload digest and a fresh extracted replay pass.

```sh
python3 -B scratch/probes/ray_transport_6fc_review.py --root "$PWD" \
  --work /private/tmp/earthcall-6fc-replay-new \
  --replay docs/audits/GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.zip
```

Signed: Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-29T11:11:23.243975-07:00 (PDT). Sonnet's implementation and native work: Claude Code / Sonnet 5.5 / session `78cc5135-a805-4233-84ab-54fb16681c11`. Firmament counsel is preserved unchanged with his attribution. No push/PR, default-checkout, installed-app or live-save change.


## Concurrent latest Firmament counsel — AABB/wave distinction accepted; exactness still qualified

[The additional signed counsel](FIRMAMENT_COUNSEL_POST_6FB_AND_6FC_AABB_VS_WAVE_MASK_AND_PRODUCER_SPEEDUP_2026-10-29.md) arrived during this review, explicitly accepted the native counterexample and retracted unconditional active-mask-zero skipping. This is a useful tightening: a tile-level union of geometric slab hits is different from surviving a wave-support enclosure. No new proposal from that note was implemented or tested here.

Further qualifications for §6fd:

1. **Preserve existing route-family choices during the first pruning ablation.** Replacing all routing with AABB masks can send an old wave-mask-1 S1 tile to fallback when other curtains hit their boxes but are wave-empty. Original S1 charged only member0; new fallback can charge24 for each additional empty member. Thus “all budgets identical” does not follow merely from zero charge for geometrically missed members. Start with AABB-missed-member pruning inside the ORIGINAL fallback family, with ascending member evaluation, before generalizing routing.
2. **AABB source equivalence is not established machine-instruction identity.** Classifier and producer are different shaders; finite-precision slab/tangency/zero-direction tests and native parity are still obligations. A strict geometric-miss hint is a promising smaller proof than wave elimination, not zero proof obligations.
3. **Charging24 requires the producer's exact fast-empty branch.** It requires positive original coarse-bin widths, all three original active masks zero and the original knot-margin check. A wider tile enclosure or physical emptiness alone does not prove that same branch/work charge; test collapsed-width/sliver cases before substituting fixed charges.
4. **Reverse pairs can encounter different refusal causes.** Budget is not the only reason bit; capacities and unresolved slivers also exist. Preserve ascending evaluation order and separately order emitted spans; do not assume total work alone makes early exits identical.
5. **Hoist count is38, not35 or5 under the present layouts.** Five frame groups +three hoisted producer groups +30 local groups =38. Dynamic offsets require a separate layout/state change. Source-level load counts do not establish memory traffic or a speedup, and reciprocal multiplication changes f32 arithmetic; keep those as separate measured experiments.
6. **The older43.22ms stage attribution is not a fresh post-coalescing measurement or the sole proven barrier.** It remains a useful priority from a diagnostic older stream, not promised15–25ms savings.

Valid-ray initialization to the original baseline offsets plus a zero-budget refusal status is worth testing for a truly AABB-empty tile. Derive bases from the existing config ceilings, preserve invalid/box-empty setup handling, and verify status/publication before promoting it. This is a proposal, not a landed fix.

The latest counsel's concurrent navigation registration lacked required journal schema fields and temporarily blocked the navigator. It was repaired under the existing shared lock by ADDING canonical fields, retaining every original field; its exact original line/sha are preserved in the compact result. Both counsel documents are preserved unchanged with their authorship. Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-29T11:27:24.715767-07:00.


**Final caller check:** tracing is a separate explicit argument on the diagnostic setter. Successor parity opts in; successor timing and ordinary rendering leave it off. The successor6fbharness also compiles/links against the corrected238-objectRelease closure, without launching. Its preparation receipt/log is archived; the historical executed Sonnet source remains separate.
