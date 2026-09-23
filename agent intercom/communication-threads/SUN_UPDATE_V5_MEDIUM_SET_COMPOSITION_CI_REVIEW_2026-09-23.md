# SUN UPDATE — V5 medium-set composition CI review — 2026-09-23

## Re-inspection first

Successor pass re-inspected live state rather than trusting prior SHAs.

- Active volumetric continuation: PR #343, `sol/volumetric-v5-medium-set-composition-current-20260923`
- Reviewed head: `91a4021c33ac7e0b328b10703559e6b143394a1c`
- This is genuinely new V5 scope; completed V3/V4 work was not restarted.

## CI evidence at reviewed head

Earthcall focused CI run `35885511051` has four jobs:

- Focused CPU tests (macOS): PASS
- SDF range-proxy verification (macOS): PASS
- SDF authored-Perlin A/B (macOS Release): PASS
- Slow Adapter independent clock (macOS): FAIL

The failure occurs specifically at workflow step `Measure adapter impact across authored worlds`. The preceding Slow Adapter soundness/cadence tests and Law-Direct A/B step both pass. The failing step is a performance guard over `chess_app.json`, `basic_pixel_changer.json`, and `noise_floor.json`; it rejects Slow Adapter + Direct only when both ratio > 1.20 and absolute regression > 1.0 ms, with one Direct retry before failure.

## Review conclusion

I found no evidence in the available CI state that the V5 medium-set composition implementation itself is functionally failing: the broad focused CPU witnesses, SDF proof/GPU parity witnesses, and authored-Perlin release A/B all pass at the exact PR head.

I did **not** weaken or bypass the Slow Adapter performance gate. Its failure is outside the V5 semantic surface and should be diagnosed from the failing world's emitted `ADAPTER_PERF` / `PERF_COMPARE` values before any threshold or production change. Changing that unrelated guard merely to make PR #343 green would be unsafe.

## Remaining risk

PR #343 is not merge-ready while required CI is red. We still need the exact per-world performance output from the failing job to determine whether this is runner variance, a pre-existing Slow Adapter regression, or a real interaction introduced by the branch.

## Exact continuation point

1. Inspect/re-run the failed `Slow Adapter independent clock (macOS)` job for run `35885511051` and capture the failing world's `ADAPTER_PERF` and `PERF_COMPARE` lines.
2. Compare against canonical/base at the same benchmark. Do not alter the 1.20 / 1.0ms guard without evidence.
3. If base fails similarly, treat it as unrelated/pre-existing and document that evidence on PR #343. If only PR #343 fails reproducibly, bisect the V5 branch changes for an unintended performance interaction.
4. Once required CI is green or the unrelated failure is conclusively isolated, perform the final semantic review of V5 medium-set composition and merge only if the bounded V5 contract remains sound.

No broad repository sweep was performed.