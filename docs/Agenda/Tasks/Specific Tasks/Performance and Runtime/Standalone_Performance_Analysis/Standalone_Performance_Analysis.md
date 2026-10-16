# Standalone performance analysis

**Human commission:** Zach proposed a system-tracing and complexity-analysis procedure to narrow expensive work, explicitly clarified “a separate executable,” then commissioned “BROOOOOOOOO BUILD IT.” Codex connected existing timing/demand/compiler evidence into an offline tool. The app and authored world acquire no subsystem, policy or new state.

**Implemented:** [earthcall-analyze](../../../../../../scripts/earthcall-analyze), a directly executable Python command, with a self-contained executable zipapp packaging command. Python 3.9+ and its standard library are sufficient; no pip package, CMake edit, app link, runtime hook, GPU or network is needed. This is an **offline analyzer**, not an automatic live collector or a complete native profiler. The source is under `scripts/performance_analysis/`.

## Run it

From the checkout root:

```sh
./scripts/earthcall-analyze doctor
./scripts/earthcall-analyze demo --output /private/tmp/analysis-demo-new
./scripts/earthcall-analyze package --output /private/tmp/earthcall-analyze.pyz
/private/tmp/earthcall-analyze.pyz --help
```

`demo` creates clearly labeled synthetic inputs and an `analysis/` report. Open `report.html` locally; its search box filters all tables. `report.json` preserves machine-readable summaries and SHA256 input provenance; `summary.txt` is the terminal-friendly view. Output directories/packages must be new: existing results are never overwritten. The tool writes only the requested new outputs and verifies its input hashes before writing a report. Place outputs outside saves/live data.

The `.pyz` works from another directory without the checkout. It contains only the analyzer package and uses the selected Python interpreter. `package` must run from the source checkout, not from inside an already packaged executable. It packages the current source; rebuild it after code changes.

## Inputs and interpretation

Supply any useful combination; none is manufactured when absent:

```sh
./scripts/earthcall-analyze analyze \
  --frames /path/frames.csv --trace /path/trace.json \
  --shader /path/emitted.wgsl --entry fs \
  --context /path/context.json --output /private/tmp/analysis-run-new
```

- **Frames:** CSV, JSONL, JSON array or `{"frames":[...]}`. Timings are milliseconds; missing metrics are unknown. Use actual captured frame data from a separate harness/export, not manually guessed fluctuating UI numbers. The analyzer does not currently export `g_frameTimings` from the live app.
- **Trace:** Chrome trace JSON array or `traceEvents` object. `X`, thread-local `B/E`, `C` and thread-name metadata are supported. `ts`/`dur` are microseconds regardless of `displayTimeUnit`. Nonnegative timestamps and finite durations are required. Unsupported phases are counted explicitly; flows/async/scheduler semantics are not reconstructed. Non-nested overlapping spans invalidate exclusive attribution on that track. Per-track interval unions avoid nested double counting; cross-track durations are never summed into frame time. A CPU span remains wall-clock, not CPU utilization. GPU-tagged spans do not prove hardware provenance or establish clock synchronization.
- **Demand:** `--demand /path/results.json` imports the existing `scripts/northern_veil_fragment_demand.py` output without running/changing it. Selected-fragment counts are diagnostic, especially when instrumentation changes pixels. Global/proof/factor accounting is checked. Counts are not milliseconds, whole-frame extrapolations, or baseline-exact calls.
- **WGSL:** syntax-level function/call-site inspection with nested-comment removal and optional repeated `--entry` reachability roots. It is not a WGSL compiler, call-frequency profiler, loop expander, instruction-cost estimator or semantic-equivalence checker. Generic constructor syntax is not expanded. Emitted call sites can disappear under backend optimization.
- **Model:** `--model /path/model.json` describes explicit workload multipliers and assumptions. It reports modeled visits and parameterized `O(...)` expressions, not a theorem about the whole program, a calibrated timing estimate or measured demand.
- **Native artifact:** `--artifact /path/report.json` imports saved compiler-profile fidelity reports. Compiler-record counts and image differences remain historical correctness evidence, never compilation/GPU times.
- **Context:** optional JSON description/provenance, kept verbatim and escaped in HTML. Multiple input types are independently summarized; a combined report does not establish that they describe the same program, clock, scene or run.

Trace JSON is an existing [Perfetto-supported exchange format](https://perfetto.dev/docs/getting-started/other-formats#chrome-json-format). Existing Perfetto/Xcode tools remain the capture/large-trace infrastructure; this tool does not reimplement native collection. Raw `.pftrace` protobufs and `.trace` bundles require external tooling/export before import. `doctor` checks tool availability and resolves Xcode tools through the selected toolchain; a `/usr/bin/xctrace` launcher alone is not availability. It launches no capture and installs nothing.

## Frame schema

Context columns: `run,phase,zone,view,width,height,backend,build,route,clock_policy`. Use `startup`, `edit`, `warm` or another explicit phase; the tool never guesses phase from frame index. Unspecified context is `unknown` and prevents matched comparisons. Dimensions are physical integer pixels. `frame_id` must be unique within a run when supplied. Optional `occluded=true` and nonusable `surface_status` exclude records. Accepted status labels: `success`, `optimal`, `suboptimal`; missing status is unknown, not certified success.

Supported timing names mirror `FrameTimings`: `total_ms,input_ms,locomotion_ms,creation_ms,interaction_ms,zone_ms,zone_ground_ms,zone_rot_ms,zone_auto_ms,zone_phys_ms,laws_ms,language_ms,audio_ms,render3d_ms,wait_surface_ms,wait_submit_ms,imgui_ms`, plus `volumeCpuPrepareMs`. Nested timings are listed, never pooled. `recording_remainder_ms` is derived only when all three enclosing/wait values exist; negative remainder refuses inconsistent input. It is a wall-clock remainder, not isolated encoder CPU execution.

Counter names mirror `Renderer::FrameStats`: `drawCalls,uniformBytesWritten,pipelineSwitches,sdfProgramCompiles,volumeProgramCompiles,volumeLowerings,volumeRepairedNodes,volumeParameterBytesUploaded,volumeProofBytesUploaded,volumeZeroProofBuilds,volumeZeroProofCellsProven,volumeZeroProofCellsTotal`. Counts require nonnegative integers; totals are only across samples supplied, not extrapolated to omitted frames.

GPU durations: `gpuMainPassMs/gpuMainPassTimingValid` and `gpuVolumePassMs/gpuVolumePassTimingValid`, with `gpuTimingSampleFrame`. Optional `gpuMainPassTimingSupported=false` vetoes them. Unsupported, invalid, nonpositive or missing-source-ID values are unavailable rather than zero cost. Repeated source-frame IDs are deduplicated; conflicting repeats refuse. The source frame's context is found by `(run,frame_id)`; when absent, GPU context stays unknown. Delayed GPU durations are not assigned to the current CPU frame's view/phase.

Example CSV header/record:

```csv
run,phase,zone,view,width,height,backend,build,route,clock_policy,frame_id,total_ms,render3d_ms,wait_surface_ms,wait_submit_ms,volumeLowerings,volumeParameterBytesUploaded
run-1,warm,example,fixed,2560,1440,Metal,measured-head,fixed,fixed-time,1,50,45,35,0,0,0
```

**Those example values are illustrative, not measurements.** Percentiles use linear interpolation at `(n-1)*p`; sample count, mean, min and max accompany p50/p95 in JSON. A finite p95 under 33.3 ms is only an observed sample property, never the live milestone or Person acceptance.

Filter only frame records with repeated `--frame-filter phase=warm --frame-filter zone=...`; this intentionally does not filter other independent inputs.

## Complexity and scaling

Example model:

```json
{
  "dimensions": {
    "pixels": {"value": 100, "meaning": "assumed covered pixels", "source": "illustrative assumption"},
    "samples": {"value": 96, "meaning": "samples per pixel", "source": "illustrative upper bound"}
  },
  "terms": [{"name": "density", "factors": {"pixels": 1, "samples": 1},
             "unit": "expression visits", "cadence": "frame",
             "assumptions": ["No proof skips, clipping or early exit."]}]
}
```

`--pixel-scaling total_ms` fits the observed log-log slope of median wall-clock duration versus framebuffer pixel count. It requires at least two distinct sizes in otherwise complete matching labeled contexts, including build, phase, camera/view, route and clocks. Duplicate sizes/runs are refused instead of silently pooled. The fit does not establish sampling/thermal/coverage equivalence, causation or asymptotic complexity. Resolution experiments remain diagnostic; they do not authorize lowering the production image resolution.

## Exploratory A/B

`earthcall-analyze compare baseline/report.json candidate/report.json --output /private/tmp/comparison-new` matches complete frame contexts, except intentionally different `run` and `build`. Both reports must have identical `context.comparison_contract` objects containing `backend,zone,width,height,route,clock_policy,sampling,surface_policy,device,configuration`. Put relevant fixture/parameter/fidelity settings in that contract; their actual truth remains the capture producer's responsibility. Use explicit run pairs; multiple runs per comparison context refuse. Missing/different contexts refuse. Output contains median/p95 changes, **not** significance, ABBA evidence, a fidelity verdict or automatic retention. Repeated controlled runs and exact pixels remain independent gates.

## Verification and remaining work

Run `python3 -B tests/singularity/test_performance_analysis.py`. The focused test is independent of CMake, app and GPU; this avoids touching concurrent CMake work. Integration tests package the executable, run its real CLI from a temporary directory, inspect the actual report and verify no-clobber behavior. Other witnesses cover delayed timestamps, invalid values, nested/parallel/ambiguous traces, escaping, changed inputs, demand accounting, syntax reachability, matching contracts and empirical scaling.

The executable was also run on the saved eight-view Northern Veil census, its dated inside WGSL and the full-lattice Safe-profile rejection report. It preserves diagnostic limits and reports missing fresh frame/GPU timing. This verifies import/report behavior, not new renderer performance. Demo search behavior and layout were exercised in a browser. [Durable execution record](../../../../../audits/STANDALONE_PERFORMANCE_ANALYSIS_2026-10-14.md).

Remaining: acquire a compatible fresh live CPU/GPU trace and actual native-size movement samples; add capture adapters when justified by available tools; extend raw trace support through established processors. No current 200 ms bottleneck cause is claimed by this implementation. Author decisions AD-30–36 and the hold on new proof-consuming guards remain untouched.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-14T22:36:29.545538-07:00 (PDT); exact execution timestamps and hashes in the linked audit.
