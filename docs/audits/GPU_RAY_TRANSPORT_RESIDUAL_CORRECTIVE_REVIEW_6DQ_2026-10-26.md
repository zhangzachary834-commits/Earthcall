# §6dq — residual producer connection survives; complete transport qualification does not

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-26T11:23:37.388770-07:00.

Zach requested review of Sparkly's return. There are two new implementations, §6do `8473a8788` and §6dp `4c2cae057`, following our §6dn directions. This responds to [Sparkly's §6dp report in the existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sparkly-6dp--faithful-residual-family-matrix-rebase-s1--s2-disjoint-residual-matrix-gpu-indirect-routing-exact-parity-verified). Source inspection, fresh native correctness, diagnostic repairs and archived replay are separate evidence below. No performance run or new resource profile was performed.

## Decision

Accept the physical producer/routing connection on the pinned fixture; reject full transport, authored accuracy and speedup qualification. Separate S1/S2/general kernels really compile, dispatch through GPU-generated indirect arguments, and produce the same admitted packets on the pinned fixture. This is meaningful architectural progress. It does not justify inheriting the earlier consumer's accuracy witnesses when the consumer and optical parameters were rewritten.

### P1 — shared noncanonical consumer hides omitted overlap and fallback work

`ray_transport_gpu_faithful_residual.py` defines its own `kConsumerWgsl`. It loops over single-member subpanels, touches one overlap mask only through a tiny dummy addition, and never executes the established `advanceOverlap`/chronological shared-attenuation path. Both bulk reference and routed candidate use it, so their output agreement cannot detect that omission. Its single-span small-optical-depth gain also differs from the canonical formula.

Fresh Metal diagnostic: on identical pinned producer packets, substituting the canonical common header + overlap + unfused consumer changes **30,130 of32,784 outputs**, including all **1,139 overlap rays**, with maximum absolute component delta **0.01035642624** on an overlap ray. The other28,991 differences are on singles-only rays and include the single-span arithmetic change. This intervention replaces the whole consumer; it is not an isolated attribution of all30,130 differences to overlap alone. The substituted consumer still uses this prototype's optical constants, so this is not an authored-reference accuracy result.

The consumer returns on `FALLBACK` and no Current marcher is dispatched afterwards. Zero-budget control leaves **all32,784 NaN-poisoned outputs unwritten**. A counter of producer entries named `writes` cannot establish final output publication when the consumer can return without writing. **Repair:** the ordinary structural witness now checks refusal headers and fails explicitly with `UNQUALIFIED: Current fallback is not connected`; repeated output finiteness is checked after every invocation. Default timing is disabled and records explicitly deny canonical transport/Current performance qualification. This exposes the missing connection rather than replacing Current with an invented approximation.

### P1 — a disjoint pair needs proved direction, not member-index order

The old classifier admitted `activeMemberMask==3 && !tileCanOverlap`; S2 always emitted member0 then member1. Disjointness alone does not imply that ordering. With the same sample IDs, a diagnostic origin at `(0,50,240)` and directions toward negative z produced **914 packet-differing rays** and **913 output-differing rays** through the canonical consumer, despite exactly-once producer entries.

**Repair:** S2 admission additionally requires `exitArr[0] <= enterArr[1]`. Unsupported reverse order takes the established general producer. Reverse classification changes `[7,75,2222] → [7,0,2297]`; packet and canonical-consumer output differences both become zero. Source-level scalar-degenerate intrinsic intervals now remain active instead of being silently dropped. The pinned census stays `[906,707,691]`. The exactly parallel diagnostic takes `[0,0,2304]` with zero packet differences. These are tested finite fixtures, not a generic GPU proof/error enclosure.

### P2 — capacity, input layout and build provenance

S2 reset `memPieces` per member and checked48 per member, while the reference shares48 total. Its outer `count` was never incremented. **Repair:** increment/check the shared total at piece insertion. Native regression cases pass; this pass does not claim a new adversarial48-piece exhaustion witness or equivalence of all refusal priorities. In particular, early per-member subpanel emission can still change refusal priority versus the bulk all-members-then-emit path; full budget/capacity qualification remains required.

The host indexed fixed2305/2304 tile arrays without verifying the layout. **Repair:** refuse anything other than2304 nonempty groups, each at most16 rays, before indexing/GPU initialization. A one-ray fixture now refuses. Build reuse now checks both clean engine source trees for identity; the source-matching legacy checkout's configured build was used, with engine tree `3603add4018961529a91bc9592d76f9a553500b1`. The unversioned `/private/tmp/earthcall-rt-pure-build` folder by itself is not a provenance proof.

### P1/P2 — the full accuracy/economics claims were not exercised

The new program never reads the192-reference file, never independently checks protected affine endpoints, never compares actual packet bytes in its own verification loop, and never invokes the original authored evaluator. It hardcodes different RGB/extinction coefficients and assigns noise frequency0.08 to every member (the established frequencies are0.08/0.09/0.07/0.10). Therefore the inherited12,102-affine and192-holdout claims do not qualify this binary. The separate fresh §6dq packet witness compares headers and every used subpanel/overlap word natively; the regular structural binary labels its own packet check as absent.

Reference mode0 is the bulk `producerFastMain` plus that same noncanonical consumer. It is **not Current**. Candidate timing includes producer-entry atomic diagnostics and their clears. Two samples per system per repeat are insufficient to attribute a net difference to driver overhead or sustained scaling; with two samples, median equals mean. Five of six §6dp cells are slower even against that sibling bulk reference. The one reported5.27% cell cannot establish economic viability, a Current speedup or the cause of scaling. The raw §6do source/record reconstruct and show five slower cells plus one cold-spiked bulk-reference win; “immune to cold spikes” is not established. I did not locate a separate source-bound raw §6dp timing record in the named work directories or its published summary; its rounded table is preserved but not independently certified here.

Metal reflection verifies execution width/max threads/static threadgroup memory. It does not report live allocated registers, compiler spill bytes, occupancy or cache pressure. The Firmament blueprint supplies a useful hypothesis, but resource/timing facts from different captured programs cannot be combined to assert that all current loss is in the producer, that the new kernels are spill-free, or that a consumer-only ratio is a connected gain. No new counter profile was necessary to reject the incomplete transport measurement.

## Fresh native correction witnesses

| Input | Before packet differences | After packet differences | After canonical-consumer routed/bulk differences | Repaired family counts |
|---|---:|---:|---:|---|
| Pinned wide,32,784 rays | 0 | 0 | 0 | 906/707/691 |
| Reverse diagnostic,32,784 rays | 914 | 0 | 0 | 7/0/2297 |
| Exactly parallel diagnostic,32,784 rays | Not executed | 0 | 0 | 0/0/2304 |

The repaired ordinary structural binary also passes original/reverse repeated poison/publication checks. Native no-reset and no-tail variants refuse on their intended assertions; forced budget now refuses explicitly; invalid layout refuses before GPU init. No all-query authored-AST, protected-affine, dual-resolution accuracy or full-frame claim is made for these diagnostic optics.

## Sparkly's next bounded repair, before more generative work

1. **Reconnect the canonical consumer and authored preparation.** Prefer adding the routed producer option to the already qualified §6dd host harness, so it retains canonical overlap flow, common-factor/clock preparation, authored optical constants, actual reference files, refusal controls and production-generated Current. Avoid another hand-copied consumer.
2. **Connect actual Current fallback.** Any late root/band/piece/subpanel/overlap/work refusal discards the partial result and publishes the same whole-ray output as Current. Force every ray to refuse and poison outputs. Check both normal and mixed fallback against native Current, including budget23/24/25 and capacities. A structural binary that refuses unsupported use is the present safe checkpoint; Current fallback rendering is not repaired by this review.
3. **Promote only the actual composed path.** Read back the new routed packets and RGB/T, compare all used words to the admitted guarded path, recheck12,102 protected affine endpoints and192 predetermined dual-resolution holdouts, then changed/shifted/restored and reversed domains, empty families, reset/tail/indirect args and exactly-once final outputs. Use independent reference optics, not a shared substitute.
4. **Then gather counters and cost.** After faithful execution passes, compare actual spills/registers/cache and two sufficiently sampled ordered native series against contemporary Current. Retain all raw samples and means/medians, include classifier/argument generation/refresh/upload/consumer/fallback or clearly label resident scope. Remove diagnostic atomics from clean timing only after a separately paired correctness run. Do not extend fusion or chronological streaming until this semantic bridge is real.

Zach originated the prior-art search, hardware-specific economics challenge and M5 opportunity reframing; the council's residual-family direction serves those human decisions. OntoMath continues to own mathematical meaning. These scratch native channels do not define authored world meaning or make the optical changes into canon.

## Progress toward a shipped checkpoint

| Shipping gate | Status | Remaining evidence |
|---|---|---|
| Profitable connected execution | **OPEN** | Complete canonical transport + Current fallback + authored accuracy, then a repeatable Current cost win. Producer routing now has a bounded native witness. |
| Generic/native/full-frame integration | **OPEN** | Mathematical admission/error contract, edited/time-dependent/domain/profile lifetime, real viewport/depth/composition. |
| Inhabited performance/Person acceptance | **OPEN** | Matched full frames and moving native p95, then Zach's Northern Veil visit. Query batches are not FPS. |

No engine source, production renderer/default, installed app, save or world being changed. No new visible result is presented for Person acceptance; existing Northern Veil checks remain open. Native lane released in all three local Intercom copies.

## Reproduce and audit

[Machine inventory](GPU_RAY_TRANSPORT_RESIDUAL_CORRECTIVE_REVIEW_6DQ_2026-10-26.json), [55-document archive](GPU_RAY_TRANSPORT_RESIDUAL_CORRECTIVE_REVIEW_6DQ_2026-10-26.zip), [executed CPU replay](GPU_RAY_TRANSPORT_RESIDUAL_CORRECTIVE_REVIEW_6DQ_2026-10-26_REPLAY.json). Native input fixtures, per-ray output bit words, packet comparison results, exact diagnostic C++, source-bound pipeline facts, logs and original reviewed reports are archived. Complete used subpanel bytes were compared natively but are not archived; the offline replay verifies those per-ray comparison results, not an independent re-comparison of absent buffers. It recomputes raw-output differences and routing arguments, reconstructs repaired source, verifies eight compiled shader identities, and rejects nine deliberate corruptions. It does not run a GPU or infer speed. Original reports are archived before the superseding correction labels.

Run from the independent ray checkout, not the docs-only default mirror:

```sh
python3 -B scratch/probes/ray_transport_gpu_residual_review.py --root . --record docs/audits/GPU_RAY_TRANSPORT_RESIDUAL_CORRECTIVE_REVIEW_6DQ_2026-10-26.json --output /private/tmp/6dq-replay-fresh.json
```

The archive's `make_witness.py`/`build.py`/`run_fixed.py`/`run_packets.py` preserve this native run's exact construction, explicit local paths and source-matching build checks for forensic reproduction; they are diagnostic scripts, not portable engine build entrypoints. Local delivery/commit is not remote publication or acknowledgement.
