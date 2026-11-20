# C5 Horizon traversal and optical diagnostics

Sixth-Two Sun / Codex GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-19T01:56:06-08:00.

Zach relayed EF5 Horizon's review of the [Joint execution experiments](C5_JOINT_EXECUTION_COST_EXPERIMENTS_2026-11-19.md). Horizon proposed measuring sample visits and proof queries, then reordering fixed optical work inside a single shader. He also proposed a streamed event sweep, per-medium proof retention and, conditionally, interval compute. This pass executes the first diagnostics. It preserves stock chronology, budgets, support certificates, and saves.

## Exact inputs

C5 `69ab1586f93be5522c5ce7c260b11f3e784d548a`, restored Northern Veil from `31c0bb1a2`, isolated fixture, six fixed cameras, world clock 7.25, native arm64 Release. Diagnostics live under `build/c5-horizon` and the verifier checks each actual draw's patched WGSL. Every measured draw was unrouted. Diagnostic split layouts remain cost probes with no chronology authority.

## 0A · Work census

At 2560 × 1440, one pixel at the center of each cell on a 64 × 36 regular grid reports counters. Each fragment increments private values and publishes once on discard or return. The forced split counts are summed over both draw programs. At far south their sum had 2,304 rasterized fragment invocations, matching Joint. At the other five views the split had 4,608, versus Joint at 2,304. The counts reflect the sampled rays and are never scaled to claim a full-screen instruction count or frame time. The same shader arithmetic can compile differently under the extra instrumentation, so timing is excluded from these runs.

| View | G1 sample visits, Joint / 3+5 | G1 zero-cell calls, Joint / 3+5 |
| --- | ---: | ---: |
| Spawn | 129,518 / 99,032 | 181,033 / 120,439 |
| Main Veil | 156,101 / 123,417 | 237,157 / 164,869 |
| Dense inside | 248,624 / 230,944 | 359,114 / 286,848 |
| Reverse camera | 280,103 / 104,716 | 291,826 / 114,135 |
| Far north | 156,614 / 122,668 | 237,750 / 165,873 |
| Far south | 238,880 / 238,880 | 427,612 / 427,612 |

Stock-optics counters reproduce the step and proof-call counts on the sampled rays. Reverse shows 2.67× the steps and 2.56× the calls. Far south matches both counts despite the measured G1 timing gap. The census establishes a visit-count burden for some views and leaves a physical execution cost per visit or outside this sampling grid in others. It does not apportion the visit-count burden between chord spacing and collective jumps. A positive returned cell proof is also distinct from proof of a repeated call within the same certified interval.

### Labeled Metal spill events

A separate native Instruments run fixed reverse-camera view D and used the uninstrumented G1 shaders. Both render passes were labeled `EF5 Volume Transport`. The trace joins `graphics-compiler-spill-events` to `metal-application-encoders-list` by command-buffer and encoder ID and restricts the join to the benchmark process. The attribution path uses driver trace metadata only. These traced intervals are excluded from performance measurements.

| Exact G1 draw | Labeled volume encoders | Attributed spill report |
| --- | ---: | --- |
| Joint8 `f8cafbe6d66af7d2` | 58 | 58 events, 496 bytes reported per event |
| Forced 3+5 `724f61a5e13cbafd` then `71024760f89d0ed8` | 58 | Zero events |

This directly observes an encoder-level difference in reported compiler spills under the synthetic event/support workload. It establishes neither which WGSL object caused a spill nor a bytes-per-thread value. The available counter catalog in this trace session exposed RT Unit Active, and the shader profiler table supplied zero instruction samples. Registers per thread, SIMD occupancy and instruction-cache behavior remain unresolved. The command-line compiler toolchain was unavailable, but the native trace completed independently.

## 0B · One shader, two loop orders

The fullscreen 192-position optical fixture runs at 1280 × 720. Both variants visit the same mapped position for each member. One loop visits all eight media per sample. The other uses two sequential loops and four media per sample in each. They use the same optical blocks and integer aggregation for a diagnostic image. No transport update is performed in this diagnostic. The initial raw-bit checksum magnified float differences and its output cannot serve as a parity check. A second pair, run in reverse process order, sums fixed-point optical quantities and presents a bounded numeric signal. Its stock-output contract remains diagnostic.

| View | Sample outer, ms | Chunk outer, ms |
| --- | ---: | ---: |
| Spawn | 381.2 | 346.3 |
| Main Veil | 427.2 | 381.4 |
| Dense inside | 432.2 | 396.6 |
| Reverse camera | 349.0 | 323.7 |
| Far north | 437.8 | 395.8 |
| Far south | 409.0 | 383.0 |

The numeric variant improves 6.4–10.7% in this pair. The corresponding outputs differ by at most one BGRA byte level at 78–257 of 921,600 pixels per view. The raw-bit checksum comparison had a smaller and inconsistent timing difference across the six views. The result shows the compiler/backend can respond to a changed physical loop shape even within the same fragment stage. Shader live ranges, arithmetic lowering, I-cache behavior, and register allocation can all change with reordering. A claimed I-cache or occupancy cause requires a counter attributable to the pass.

## Source review and follow-through

The event array is dynamically indexed in both sorting and traversal. The generated chord array is accessed at literal member indices. Neither the declaration sizes nor the WGSL determine the actual allocated register count or spill cause. The current proof loop can query a member again at the next sample when any member prevents a collective jump. Reuse would need an interval authorization that survives rounded ray coordinates, clipped support bounds, near-parallel policy and any mutable certificate recheck state.

Horizon’s proposed event sweep and proof retention change distinct causes. A finite-ray event sweep should start at the first clipped event and advance through each distinct endpoint, preserving the stock treatment of coincident and tiny gaps instead of merging them. A proof reuse trial must preserve transport over both physical entry/exit boundaries and the certificate validity interval. A native spill observation cannot establish that any one source object caused it without a controlled source A/B.

No compute prototype or production code change resulted from these diagnostics. A future tile interval classifier would still owe the existing per-ray depth bound, event positions, sample lattice and continuation state. A full-screen `vec4<f32>` read plus write is about 118 MB of logical traffic per handoff at 2560 × 1440, before worklists. Bus latency and effective device traffic remain unmeasured.

## Scalar event sweep alone — native trial

Sixth-Two Sun / Codex GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-19T22:58:43-08:00.

Zach commissioned the isolated scalar sweep proposed after Horizon's counsel. Per-medium proof reuse is excluded. All changes live in the ignored `build/c5-event-sweep` harness. No tracked renderer or test source changed. C5 HEAD remains `69ab1586f93be5522c5ce7c260b11f3e784d548a`. The isolated restored eight-field Northern Veil has SHA256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`, from `31c0bb1a2`. Person Data Home is untouched.

The transform replaces the dynamically indexed sixteen-event array and insertion sort with sixteen scalar clipped endpoints and repeated selection of the next strictly greater endpoint. The eight literal-index chord values become scalars. It starts at the minimum endpoint and stops at the maximum endpoint, preserving clipped intervals, coincident boundaries and the original `1e-6` tiny-gap predicate. The cursor advances before a rejected tiny interval continues. An initial host replay exposed an erroneous extra tail interval when using `t1` as the last sentinel; that variant was corrected before native execution. The sample body, support checks, optical arithmetic and continuing `(L,T)` update retain the original source, apart from substituting scalar chord names. Four source pairs passed prefix/sample-body equality checks, and the preparation shaders are byte-identical.

The independent host replay passed 20,003 finite-endpoint cases and 1,812,744 f32 sample-position bit comparisons, including coincident endpoints, near-threshold gaps and a thin curtain. This is bounded host evidence. Native paired captures provide the scene-specific GPU witness below.

Both baseline and candidate ran native arm64 Release at 2560 × 1440, the same six cameras and clock 7.25, requested Routed ON, with zero routed sets. Each view has two batches of three completed frames; the table reports the median of batch time divided by three. Captures use separate untimed frames. Actual draw records confirm one full-viewport eight-medium unrouted program per measured frame, the expected source bytes, and no warm compilation or program refusal. A first attempt with an older harness produced no drawable surface and yielded no usable timings. Both measured variants then used the same bounded warm-up requiring three consecutive drawn frames.

| View | Stock Joint8 ms | Scalar sweep ms | G1 baseline ms | G1 sweep ms |
| --- | ---: | ---: | ---: | ---: |
| A · Spawn | 170.16 | 177.41 | 72.76 | 85.87 |
| B · Main Veil | 203.87 | 214.65 | 88.90 | 105.07 |
| C · Dense inside | 328.66 | 334.03 | 101.94 | 104.57 |
| D · Reverse camera | 289.90 | 289.31 | 95.27 | 88.88 |
| E · Far north | 200.27 | 209.44 | 87.48 | 101.53 |
| F · Far south | 714.69 | 725.32 | 149.69 | 162.03 |

All six full-optics captures and all six G1 captures match their baselines byte-for-byte. Full Joint8 changes by +4.26%, +5.29%, +1.64%, −0.20%, +4.58%, +1.49% in camera order. G1 improves 6.71% at reverse camera and is slower in the other five views. These short matched measurements show no consistent recovery. The tiny full-optics reverse difference does not establish a gain.

| Full-optics source | Stable fingerprint | WGSL bytes |
| --- | --- | ---: |
| Stock outside | `eba60b98aace5a5a` | 158565 |
| Sweep outside | `3dc35cd07e4f64b0` | 159226 |
| Stock inside | `2907418542ec52c6` | 153527 |
| Sweep inside | `021f6e42d5db9d12` | 154188 |

The candidate's outside/inside SHA256 identities are `070994548699a9d9b453effb9c6167813705339be4e9ed857e83424705080e8a` and `76c19fb405f005b11ade4eddee276f0222ea73e3df11e8e626f4a3ba08d4400e`. Candidate G1 fingerprints are `0669476489f081ac` / `442b5f92df61f95e` (129114 / 124076 bytes). The verifier retains exact source identities, paired capture hashes and batch values in `build/c5-event-sweep/results.json`; its readable table is `results.md`. Native logs, compressed captures, transform and verifier remain alongside it.

The attempted new labeled Metal trace failed while saving under disk exhaustion. No new scalar-sweep spill report is available, and the earlier 496-byte G1 observation cannot be attributed to this candidate. Removing the WGSL array does not prove reduced hardware spilling. More spill attribution requires a successful matched trace with sufficient storage.

Disposition: reject this scalar-sweep representation as a production performance repair. It preserved the bounded host sequence and twelve scene captures, but it did not recover Joint8 economics. The event-array hypothesis remains unproven at the hardware-allocation level. This result does not test proof reuse or rule out another event representation. Thresholds and production execution remain unchanged; PR #882 stays blocked on Joint performance, with no new Person acceptance claim.
