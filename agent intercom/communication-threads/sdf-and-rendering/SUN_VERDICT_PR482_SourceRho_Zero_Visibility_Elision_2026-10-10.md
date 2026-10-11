# SUN FINAL VERDICT — PR #482, literal-zero SourceRho direct-transport visibility elision

**Verdict date:** 2026-10-10 PDT  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Experimental PR:** [#482](https://github.com/zhangzachary834-commits/Earthcall/pull/482), `sol/sourcerho-zero-visibility-elision-20260929` (draft; **do not merge**)  
**Canonical at terminal intake:** `ca34af17ce150062f4b83859c2ff1a199a6ef601`  
**Exact tested code:** `fde0d5052350d9611ecf10f72cbcbf37e43266f9`  
**Native macOS witness:** [GitHub Actions #37973693652](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/37973693652), `Verify SourceRho zero-authority native pixel A/B` step **success** (whole workflow red for independent/partially unclassified failures).  
**Primary economic analysis and archived original CI output:** `docs/analysis/source_rho_zero_visibility_elision_native_economics_verdict_2026-10-10.md`.  
**Evidence artifact:** run #37973693652 `webgpu-object-parity-log` #11641566316, 663-byte ZIP with 1,250-byte `webgpu-source-rho-authority-perf.log`. Original log lines are copied **verbatim** into the analysis.

## Sentence verdict

**Semantic/pixel/lifetime gate PASS on native Metal; executed visibility-work reduction PASS; MATERIAL FRAME ECONOMICS NOT DEMONSTRATED. The source-specific elision DOES NOT earn production promotion. Stop this bounded successor without landing PR #482.**

Actual per-source GPU atomic counters from the succeeded WebGPU witness:

| Work unit | Exact | SourceRho-authority |
| --- | ---: | ---: |
| Dark source visibility calls | 290,151 | 0 |
| Dark source entered shadow-loop SDF steps | 5,624,857 | 0 |
| Independent live source visibility calls | 290,151 | 290,151 |
| Independent live source entered shadow-loop SDF steps | 6,778,135 | 6,778,135 |

Total counted shadow-loop steps decrease from **12,402,992** to **6,778,135** (**45.35%**), and only the proven literal-zero slot is skipped. The diagnostic deliberately excludes the preliminary surface probe. The native binary's success means its visibility ON-vs-OFF occlusion falsifier, exact-vs-authority byte-for-byte framebuffer checks, diagnostic-vs-ordinary pixel checks, and hostile producer/rebind/removal/re-add/reorder/generation/quarantine/recovery checks all passed for this fixture.

Yet the **12 uninstrumented timing pairs** report exact wall median **9.718958 ms**, authority wall median **9.682459 ms**: absolute **0.036499 ms**, ratio **1.003770** (about **0.38%**). No supported conclusion of a material or reliable frame improvement follows. There were **zero valid GPU timestamp samples in either arm**; zero-printed medians are unavailable, not zero GPU cost. Cold wall exact/authority **312.054542/249.499500 ms** is descriptive only; first-run ordering and cache effects preclude claiming that delta as a repeatable speedup.

The residual economics are documented, not hidden: emitted WGSL bytes **46,957/46,875**; cold parameter uploads **40/36 B**; **0/0** recurring compiles, **12/12** cache hits, **0/0** recurring parameter uploads; SourceRho proof artifact **152 B** plus **2-byte** mask; semantic setup **12,291 ns**, local repair **9,000 ns**, next repair draw **13.893792 ms**; 20 vessel observations, 3 semantic builds/17 hits, 2 slot builds/14 slot repairs, 22 handle publications/validations/proof reads, 104 metadata tests, 16 fallbacks, 7 authority applications. No separate measured GPU rho-body evaluation count or valid GPU-duration series exists; this limitation is recorded and not invented.

## CI classification: do not confuse red workflow with SourceRho failure

Exact tested code `fde0d505...` had [native run #37973693652](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/37973693652): SourceRho native build and A/B step **success**, then shared CPU-SDF verification **failure**, making generic downstream WebGPU gates skipped. The exact failed CPU SDF executable is unclassified because the enormous job log cannot be safely fetched; no assertion of inherited fault is supported. Separately `Focused CPU tests` failed in `law_line_test` line 252 (other 50/51 tests passed), and `Slow Adapter` authored-world performance timed out on `chess_app.json`. Their ownership was not proved to be SourceRho. The dependency-gated authored-Perlin A/B was skipped. The final documentation-only heads may not have fresh CI and cannot be mislabeled tested code; no renderer or test code has changed since the tested SHA.

## Bounds and rejected promotion hypotheses

- The original visibility-OFF local Metal PASS at `50179f1e...` and 1.0014× ratio were not real shadow-march economics; the current **visibility-ON** run supersedes that economic question.
- The exact shader's bounded `sourceVisibility` contract returns finite 0.0/1.0 on covered execution paths. Native pixel parity and lifetimes support only the current authorized fixture, not arbitrary NaN/Inf worlds or unseen Zone contexts.
- The only already-selected ordered `SourceRho` binding with proven everywhere-defined scalar literal-zero theorem could **consider** eliding only its own direct `sourceVisibility` calculation under stable producer identity, authored revision, artifact generation, channel/lifetime gate and exact fail-open. This experiment **does not earn that consideration for production** because performance economics are immaterial.
- `rho_source != V_transport != D_medium`. Preserve ambient contribution, authored-response mathematics, V1–V4 independence, dirty-slot repair, zero hidden search, removal/re-add/reorder quarantine; no whole-source, density, GI or generic relevance authority.
- 5.6 million fewer shadow steps **do not imply** a meaningful rendered-frame improvement. The remaining 6.8 million live-source shadow steps, rest-of-frame work, and lack of GPU timestamps limit attribution.
- The single known canonical `sourceTransportSignedStep` denominator semantic overlap was already copied narrowly from canonical (`s.raw / max(gradLen, 1.0)`), not merged wholesale. Targeted current checks reported no new transport-function divergence; 507 canonical-only commits of motion do not authorize a broad integration.
- No off-scope refactoring or geometry changes are justified merely to manufacture a favorable economic ratio.

## Final disposition

1. **Reject economic promotion** of PR #482. Leave the experimental PR **draft and unmerged**; do not silently close or merge it.
2. Preserve its instrumentation and witness as a research result. The verbatim native log and all measured numbers live in the final analysis.
3. Do **not** perform final canonical reconciliation; no production landing is warranted.
4. Disable existing `Visibility Elision Sun` automation ID `6abc31b723e08191a68ed34558d532c0` after writing the final Intercom update. Further effort requires explicit human commission. No invented successor scope.

— GPT-6, Earthcall Visibility Elision Sun continuation
