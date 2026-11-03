# Standalone performance analyzer — executable and evidence

Zach proposed system tracing plus complexity analysis to narrow expensive work, clarified that it must be a **separate executable**, and commissioned “BROOOOOOOOO BUILD IT.” This implements that observation tool without changing Earthcall’s authored mathematics or production app. It continues [the performance counsel thread](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#sixth-one-sun-builds-zachs-separate-performance-analysis-executable). Zach originated the whole-system question and separate-executable boundary; Codex implemented the importers, accounting, executable packaging and report.

**Result:** `scripts/earthcall-analyze` runs independently, and `package` produces a self-contained executable `.pyz` requiring Python 3.9+ only. No app rebuild, CMake changes, linked renderer hook or pip dependency. [Complete input contracts and commands](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Standalone_Performance_Analysis/Standalone_Performance_Analysis.md). [Machine-readable evidence and hashes](STANDALONE_PERFORMANCE_ANALYSIS_2026-10-14.json).

It reads frame CSV/JSONL/JSON, Chrome trace JSON, existing demand census results, emitted WGSL, explicit workload models and saved native fidelity artifacts. Outputs are a searchable offline HTML report, structured JSON and terminal text. Optional exploratory A/B requires matching explicit context contracts; optional pixel scaling reports empirical slopes without promoting them to algorithmic complexity or GPU cause. Original inputs are hashed before/after analysis. Existing output paths are refused.

## What ran

- **20 focused tests pass**: CPU timer/percentile accounting, exclusions and phase separation, delayed GPU attribution/deduplication, unknown/unsupported GPU samples, malformed/conflicting data, CSV/JSONL, nested/parallel/crossing trace spans, B/E/counter phases, explicit model counts, observed scaling, WGSL reachability/comments, diagnostic demand accounting, HTML escaping, source mutation, packaged CLI/no-clobber and matched A/B context contracts. Tests package the actual source and exercise the real command in a temporary directory. No CTest/app/GPU/save access.
- The final packaged executable ran `demo`, imported the saved eight-view Northern Veil demand census plus dated inside WGSL and the native Safe-profile fidelity report, and executed an A/B self-control. All supported views import and all self-control timing deltas are zero or undefined for zero baselines.
- A browser rendered the final demo report at a 570px-wide viewport. Filtering `wait_surface` leaves only the corresponding timing row; clearing restores the rows. The synthetic label remains visible with context collapsed. The screenshot shows frame timings and readable table columns. This is a UI behavior/layout check, not Person acceptance.
- `doctor` resolves the actual selected Xcode toolchain. A `/usr/bin/xctrace` launcher exists, but `xcrun --find xctrace` refuses availability; `trace_processor` is absent and `/usr/bin/sample` exists. No capture tool was launched or installed. The temporary localhost preview needed desktop sandbox escalation; no production resource was served.

The demo has 24 **synthetic** frames, p95 162 ms, a 25 ms recording remainder and unavailable GPU samples. These numbers test the procedure; they are not Earthcall measurements. Its overlapping illustrative GPU-tagged span is not added to CPU frame time. The historical import retains all eight demand rows and their pixel-perturbation limits; compiler records/image differences remain correctness evidence with no timing conversion.

## Limits that remain explicit

This is an offline analysis executable, not a new live collector. The existing app’s `FrameStats`/`FrameTimings` fields are compatible with the documented import names, but no app export hook is installed. Raw Perfetto protobufs and Instruments bundles require established external tools/export. The bounded trace reader handles common duration/counter events and declines ambiguous exclusive attribution; it does not infer scheduler causality, critical paths, CPU/GPU clock synchronization or async-flow dependencies. See the [primary Perfetto format documentation](https://perfetto.dev/docs/getting-started/other-formats#chrome-json-format).

WGSL syntax does not prove which GPU instructions survive optimization; a workload formula does not establish actual demand. Counts and timings stay separate, nested and parallel durations are not summed, and surface waits are reported as blocking locations rather than proven causes. Unknown metadata prevents matched comparisons. An A/B report is exploratory, not ABBA/significance/fidelity/retention evidence.

The remaining practical measurement is a compatible fresh live timeline and actual usual-size moving-world frame samples. This pass identifies no new cause of Northern Veil’s roughly 200 ms cost, supplies no new FPS gain, and does not resolve AD-30–36 or install a proof-consuming guard. Other sessions’ CMake, Terminal, Storage, save and documentation work is preserved; no commits, pushes, subagents or renderer mutations.

## Reproduce

```sh
./scripts/earthcall-analyze --help
./scripts/earthcall-analyze doctor
python3 -B tests/singularity/test_performance_analysis.py
./scripts/earthcall-analyze package --output /private/tmp/earthcall-analyze-new.pyz
/private/tmp/earthcall-analyze-new.pyz demo --output /private/tmp/analysis-demo-new
```

For real inputs, use `analyze` with explicit `--frames`, `--trace`, `--demand`, `--shader`, `--artifact` and/or `--model`, following the task’s schema. No input is necessary merely to run the labeled demonstration. New paths are required; do not place reports in saves.

Final local package: `/private/tmp/earthcall-analyze-20261014-final.pyz`. Demo: `/private/tmp/earthcall-analysis-demo-20261014-final/analysis/report.html`. Historical import: `/private/tmp/earthcall-analysis-historical-20261014-final/report.html`. Native input paths and SHA256s, source/package hashes, exact timestamps and compact output results are preserved in the sibling JSON so this audit survives temporary artifacts disappearing.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-14T22:36:29.545538-07:00 (PDT).

Browser-check closure: the preview screenshot is preserved in the final demo directory and recorded by SHA256 in the sibling JSON; the temporary tab and both matching localhost servers were closed. Reports open directly from disk without a server.
