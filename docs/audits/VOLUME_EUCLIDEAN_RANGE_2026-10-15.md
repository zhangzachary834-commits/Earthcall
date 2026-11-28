# Scoped Euclidean range analysis

**Implemented:** explicit host range rules for vector Length and vector/vector Distance. **Native proof consumers remain unchanged.** This completes the host implementation portion of Stage D1, not a rendering speedup or the full range/backend contract.

Zach commissioned the next rungs after the [three rejected optimization trials](VOLUME_POWER_AND_OPTIMIZATION_TRIALS_2026-10-15.md), continuing his goal of the minimum necessary mathematical evaluation without sacrificing authored beauty. This implements the [Euclidean range stage](../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md#7-stage-d--generic-ranges-with-a-sound-native-consumer), drawing from [Firmament's supplied radial coverage analysis](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#firmament-to-sixth-one-sun--astra--complete-probe-provenance-in-privatetmp-accepted-epistemic-qualifications-and-ontopow-three-route-parity-contract). Codex added intermediate host-rounding enclosures, overflow/type refusal, explicit consumer isolation and numerical/actual-consumer witnesses. No author decision is selected.

## Implementation and scope

`MathNode::evalMathematicalRange` in `src/Singularity/OntoMath/ScalarForm.cpp/.hpp` adds Euclidean rules to the existing recursive range machinery. `evalRange` retains its prior behavior. A private invocation-local context selects the extra rules; no persistent field, enum, authored ID, serialized flag, memo or GPU resource is added. All current production callers still use the old entrypoint:

- `Rendering::densityProvenNonPositive` and the existing volume proof builder;
- `geom::evalRange`, including geometry hierarchy/tessellation consumers;
- `PropheticRete` read/write range analysis.

The new entrypoint's current consumers are the focused mathematical witness and a separate read-only census executable. This prevents a finite new host range from silently strengthening an unqualified GPU skip or changing Rete reachability. The earlier power correction remains intact.

Only vector Length and vector/vector Distance are added. Scalar overloads are refused, matching actual MathNode evaluation. Other operations retain their existing range rules and limitations; unsupported nodes remain Unknown. This is no universal real/double/libm or native arithmetic certificate.

## Enclosure argument

For each already-enclosed vector component `[a,b]`, the smallest absolute value is zero if the interval crosses zero, otherwise `min(abs(a),abs(b))`; the largest is `max(abs(a),abs(b))`. Squaring these nonnegative endpoints gives the component's squared enclosure. The implementation widens **each float multiplication and each addition**, then takes and widens the square root. Lower squared/norm bounds are clamped to the mathematical nonnegative domain. An exact all-zero vector remains singleton zero. Distance first encloses each component subtraction, then uses the same norm argument; the two endpoint bindings remain independent.

Widening just the final square root of a double dot product would not necessarily enclose the executed host float dot product. This implementation instead encloses intermediate float operations and the ideal norm of already-enclosed inputs. A type mismatch, missing/null operand, empty/nonfinite component interval, subtraction overflow or square/sum overflow refuses a finite norm bound. A finite ideal norm does not excuse host overflow: `length((1e30,0,0))` is Unknown because the host square overflows. Tiny squared inputs may underflow; widened bounds include both the tiny ideal result and the actual host zero. Such loose bounds are intentional.

The configured tested host compiler uses `-g -std=gnu++17` without a fast-math flag. This argument does **not** enclose the retained Metal Fast length/distance intrinsics, coordinate assignment, fused arithmetic or downstream transport; those remain obligations for native consumption.

## Frozen-world census

`scratch/probes/volume_euclidean_range_probe.cpp` is a separate executable. It constructs no renderer/device, reads explicit frozen fixtures once, uses the legacy builder's current grid and 1% enlarged leaves, and compares old versus explicit new host analysis. It refuses guarded/function-call/fold cases rather than inventing their applicability. Empty non-medium Piecewise entries are skipped. Initial development omitted that skip and failed on an undefined empty expression; that failed scratch run is preserved and was corrected before the reported census.

| Frozen medium | Legacy builder empty / total | Explicit host analysis empty / total |
|---|---|---|
| Sanctuary cloud sea | 0 / 34,656 | **10,968 / 34,656** |
| Sanctuary halo mist | 6,400 / 35,200 | **19,648 / 35,200** |
| Northern Veil emerald | 31,920 / 33,120 | 31,920 / 33,120 |
| Northern Veil cyan | 34,240 / 35,420 | 34,240 / 35,420 |
| Northern Veil violet | 31,830 / 33,120 | 31,830 / 33,120 |
| Northern Veil crimson | 33,870 / 35,112 | 33,870 / 35,112 |

The new implementation reproduces Firmament's cloud/halo coverage screen with the stricter intermediate handling. It adds no coverage in these four auroras. **All six legacy builder payload fingerprints and counts match the separately linked pre-change mathematical implementation.** Fingerprints here are deterministic FNV-1a over packed word integers, corroborated by counts; source/binary provenance uses SHA-256. A separate payload-only pass also compares all **8,610 packed word values**, support bounds, axes and 395 bands; the complete old/new census payload reports are byte-identical for both fixtures. This establishes compatibility for these inputs, not an exhaustive proof of every prior consumer.

The new analysis checks 2,584 cloud, 4,624 halo and 31,036 aurora CPU sample values in marked cells at time 7.25, finding no positive/undefined values. Sampling corroborates the range argument; it grants no native omission permission and makes no performance claim.

## Executed validation

The checkout WebGPU app and six focused test targets rebuild. Final results:

| Witness | Result |
|---|---|
| `volume_zero_proof_test` | **48/48**, including 16 new groups |
| `sdf_range_soundness_test` | PASS |
| `ontomath_test` | ALL OK |
| `prophetic_rete_test` | ALL OK with desktop access |
| `webgpu_volume_zero_proof_test` | **69/69**, no device skip |
| `webgpu_ontomath_native_resolution_parity_test` | PASS, **0 mismatched pixels at 1280×720**, no device skip |

New groups compare independent analytic box extrema and 8,192 actual host Distance evaluations, plus origin-crossing/negative/degenerate boxes, tiny values, zero, endpoint independence, invalid types, missing/unbound inputs, nonfinite/overflow cases and nested subtraction/clamp. One test demonstrates a newly finite radial-zero analysis while the production density proof still declines that conclusion. Native proof regression includes full 2560×1440 proof-on/off byte parity, camera crossings, edits, membership and device recreation.

The initial sandbox Prophetic Rete run stalled in macOS GLFW/XPC initialization and was interrupted (exit -2), not counted as a test pass or assertion failure. Its log is preserved; the same rebuilt binary passes with desktop access. Other final witnesses pass. No full-suite or live movement verification is claimed.

## Evidence, reproduction and remaining rung

[Machine-readable results and exact commands](VOLUME_EUCLIDEAN_RANGE_2026-10-15.json) preserve input/source hashes, compiler/link argv, all census rows and focused-test results. Temporary source snapshots, binaries and full logs are at `/private/tmp/earthcall-euclidean-range-20261015/`, with the final census under `census/` and direct payload comparison under `payload-check/`. The probe's developer-only `EC_RANGE_PAYLOAD_ONLY=1` mode skips the leaf analysis/sampling and emits legacy payload values for this comparison. Temporary storage is not promised durable.

Build the app using [the configured build recipe](../BUILD_AND_ENVIRONMENT.md). The census uses the app's actual entry compiler command with its source substituted, and the app's link command with only `entry.cpp.o` replaced. The separate legacy comparison additionally substitutes the frozen pre-change ScalarForm object; only methods were added, with no object-layout change. Both exact argument lists are recorded. Fixtures and original Firmament/`nv_probe` sources remain read-only. No installed app, live save, authority mechanism, native shader, sample lattice, timestamp switch, commit or push is changed.

**Next:** qualify a narrow native numerical/coordinate consumer contract before allowing the new radial theorem to remove renderer work. AD-30/36 still govern proof scope and surviving channel contributions. Dependency-directed repair and certified tails remain unfinished. Until those obligations hold, the new knowledge remains host analysis and production keeps its previous exact fallback. Zach's existing appearance/movement checks and 30 FPS milestone stay open; no new manual app interaction is required for this analysis-only API.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T10:45:00.917254-07:00 (PDT).
