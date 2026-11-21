# §6dw — actual two-mode admission and controlled pending-stack lowering

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-26T19:00:52.638639-07:00.

Zach said he was away and could not prompt Sparkly: “U GOTTA DO THE NEXT PASS.” Codex therefore continued execution after [§6dv's corrective checkpoint](GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.md), rather than handing off an unexecuted proposal. This pass changes scratch diagnostics only. The scalar representation is **not retained for production**.

## The changed Mode 2 is now exercised

The new driver composes the actual §6du Mode 2/Mode 3 host and clean shader factories with the previously qualified output/ownership/packet instrumentation. It does not substitute a separate renderer. It checks both actual dispatch paths, each with the six temporal pose entries × repeats 1/2/4/9 and all six budget/capacity controls: **60 native cases for the unchanged control**.

The same matrix also passes with the scalar-stack experiment: **60 more native cases**, 120 total. Across the two variants, the entire emitted native control JSON is byte-identical: flags, captured output bits, expected bits and actual terminal endpoints agree. Each case checks 32,784 sampled rays. Exactly-once entry/final publication, mutually exclusive ownership flags, packet parity, terminal-only poison and actual interval capture all remain enforced. The counters are inspected after the final repetition of each case, not exported as a history of every internal repetition. The seven negative mutations were executed in §6dv; this pass's new variant is covered by the 60-case matrix, not a newly claimed rerun of those seven mutations.

This closes the previous changed-Mode-2 matrix gap. It does not establish generic expression or complete-frame admission.

## One controlled storage experiment

Only the two S1 clean WGSL strings change. S2, general producer, Current and the consumers remain byte-identical. Within S1, the dynamically indexed pending DFS stack of 20 `vec4f` cells becomes 20 named scalar/vector slots, read/written through explicit constant-slot switches. Both original seed slots are preserved. Root tests, evaluation order, midpoint formulas, depth bits, work charges, bands/knots/pieces, budgets, capacities, quadrature and whole-ray Current fallback are unchanged. Impossible slot selections conservatively refuse.

The two separately captured clean programs run on Apple M5. Process-local observers bind both S1 pipeline creations to exact complete WGSL hashes; their original pipeline arguments/options are forwarded. Timestamp/counter sampling remain unrequested. The capture compiler plists are decoded from their original NSKeyedArchiver data; these are **static compiler allocation/instruction fields**, not dynamic spill traffic, occupancy or executed-instruction counters.

| S1 compiler row | Original spilled bytes | Scalar-slot spilled bytes | Original temp regs | Scalar temp regs | Original instructions | Scalar instructions |
|---|---:|---:|---:|---:|---:|---:|
| row with 72 uniform regs | 2,000 | 1,456 | 79 | 126 | 5,153 | 14,448 |
| row with 88 uniform regs | 2,000 | 1,456 | 83 | 126 | 6,435 | 15,563 |

This isolates a real response to pending-stack representation: reported spilled bytes fall **544 bytes (27.2%)** on both S1 kernels, while reported temporary registers rise and compiled instruction counts more than double. It does not isolate every component of the original 2,000-byte frame or explain runtime latency by itself. Current remains at the observed 0-byte spill field/2,530 instructions; unchanged S2/general rows remain at 2,000 bytes.

Full captures remain in `/private/tmp/earthcall-6dw-canonical-capture` and `/private/tmp/earthcall-6dw-scalar-stack-capture`. The durable archive includes compiler tables, source, observer/pipeline facts and trace metadata. GPU buffer snapshots are omitted; the archive is **not a complete Xcode-replayable GPU capture**.

## Clean cost screen: stop this representation

Only after the native matrix and physical change did a clean scalar-stack invocation run. It retains two ordered internal blocks (8 observations/mode/repeat/block, 16 pooled), with encode/completion/total samples. It is **one invocation**, not two independently established runs. Its original-pose source/reference checks pass on 32,784 queries and 192 dual-resolution holdouts, with 12,102 protected-affine matches within the inherited harness scope and actual S1 endpoint admission independently provided by the matrix above.

| Repeats | Current mean / median ms | Scalar routed mean / median difference | Scalar terminal mean / median difference |
|---:|---:|---:|---:|
| 1 | 3.031474 / 3.032500 | −0.012% / −0.047% | −0.075% / −0.102% |
| 2 | 5.463935 / 5.286772 | +2.373% / +0.372% | +4.007% / +14.477% |
| 4 | 8.574992 / 7.577750 | +3.498% / +7.520% | +8.704% / +19.971% |

These are matched submit-to-completion host clocks, including connected GPU stages and completion wait, excluding preceding host preparation/initial upload/compilation. They are not native full-frame FPS or pure GPU timestamps. The near-zero first-repeat differences are not a substantial gain; the longer workloads lose. **Stop the scalar-switch representation before a second cost invocation.** No retention claim is made. These comparisons reject the candidate against Current; they are not a fresh same-session canonical-versus-scalar latency ablation, and do not prove the extra static instructions caused the slowdown.

Correctness harnesses also print incidental legacy batch clocks; those are ignored for economics. The current driver clarifies that the console poison message applies only to completed terminal S1, because Mode 2 deliberately materializes packets. That wording-only correction leaves all WGSL and assertion logic unchanged from the executed control sources preserved here.

## Durable reconstruction without duplicated giant JSON

[The index](GPU_RAY_TRANSPORT_6DW_STACK_ISOLATION_2026-10-26.json) and [7,648,602-byte archive](GPU_RAY_TRANSPORT_6DW_STACK_ISOLATION_2026-10-26.zip) use the existing §6dv archive as a checksum-pinned donor. The two new control reports were independently observed and hashed; a delta reconstructs their complete native JSON byte-for-byte. The timing delta preserves each original top-level value's float lexemes, rather than reserializing numbers. Restored report hashes match the actual native files. This is lossless storage of observed results, not using reference data instead of executing the candidate.

The portable replay reconstructs both variants' native controls, checks every case and actual endpoint, replays the clean reference/timing gates, decodes the original compiler tables, checks observed-pipeline WGSL hashes and rejects altered timing/control digests and false zero-spill claims.

```sh
python3 -B scratch/probes/ray_transport_gpu_6dw_review_replay.py --root . \
  --record docs/audits/GPU_RAY_TRANSPORT_6DW_STACK_ISOLATION_2026-10-26.json \
  --archive docs/audits/GPU_RAY_TRANSPORT_6DW_STACK_ISOLATION_2026-10-26.zip \
  --donor-archive docs/audits/GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.zip \
  --output /private/tmp/6dw-replay.json
```

Fresh isolated execution uses `ray_transport_gpu_6dw_stack_isolation.py --root . --build-root <source-matching-checkout> --work <fresh-external-directory> --variant canonical|scalar-stack --run`. Add `--capture` only for diagnostic captures, or `--timing` for clean cost/reference admission; capture and clean timing are mutually exclusive. The captured/timed source and instrumented test source are separately preserved. Engine source tree remains `3603add4018961529a91bc9592d76f9a553500b1`.

## Remaining work

This result gives the council a measured lever, not a winning renderer: lower private storage alone can trade against register demand and compiled code size. Do not keep retiming these 20-way switches. A next representation needs a proved traversal/lifetime invariant that reduces pending state or demanded root work without this switch expansion; retain coverage of multiple/even roots, exact boundary arithmetic, conservative unresolved regions and whole-ray fallback. Any capacity reduction needs a proof from the actual seeding/depth algorithm, not the sampled census. A Newton-only shortcut still cannot establish complete support.

**All three shipping gates remain OPEN:** profitable connected execution, generic OntoMath/full-frame integration, and inhabited FPS/Person acceptance. Both research checkpoints are durable; no optimization is promoted, and the production renderer, default settings, authored saves and installed app remain unchanged. No new Person-only acceptance item is introduced. Native lane is released at the completion post.
