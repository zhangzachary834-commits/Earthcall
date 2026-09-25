# SUN UPDATE — Rung 9 native exact-vs-aligned-authority A/B landed

Date: 2026-09-24
PR: #369
Branch: `sol/already-known-execution-key-consumer-20260924`
Prior hostile proof-read witness: `c14e444435e5b4b9cba71ca638a2c94856ce7c34`
Native A/B code head: `4910fefcbde402b2d2a65a260a7c7d6518d6a371`
Exact-head focused CI: run `36103064608` / #3258 — queued at writing time

## Scope

This pass retried only the writes that were previously blocked after Rung 8 went green.

No new branch, PR, theorem family, spatial lookup structure, or production pixel-authority path was introduced.

## Native A/B now in-tree

`tests/singularity/rendered_field_piecewise_synthesis_test.cpp` now contains a bounded native CPU A/B for one already-known `SourceRho` execution slot.

Both arms run in the same native process against the same:

- authored everywhere-literal-zero `Piecewise`;
- real `RadianceSourceBinding`;
- stable `producerId`;
- authored `radianceRevision`;
- production `RenderedFieldSemanticObserver` aligned artifact;
- generation-bound published slot handle.

### Exact arm

For each iteration, exact authored truth is evaluated through:

`Piecewise::evaluate({x=0,t=0})`

The resulting scalar is consumed into a volatile sink so the loop remains observable.

### Aligned-authoritative experimental arm

For each iteration:

`already-known handle -> aligned artifact slot -> fixed provenance validation -> resident proof`

If and only if the returned proof is the channel-correct
`RadianceZeroContribution`, the benchmark harness authoritatively returns zero for that iteration.

If proof validation ever fails, the harness falls open to exact authored `Piecewise::evaluate`.

This authority exists **only inside the benchmark harness**. Production Renderer control flow remains unchanged.

## Accounting

The experiment uses `kAbIterations = 200000`.

The aligned arm asserts:

- 200000 proof reads;
- exactly 4 fixed provenance metadata tests per proof read;
- zero proof-read fallbacks for the stable witness;
- 200000 aligned authority decisions;
- nonzero aligned artifact logical residency;
- `authorityBypassesApplied == 0` in production telemetry.

It prints:

- exact wall nanoseconds;
- aligned wall nanoseconds;
- exact/aligned ratio;
- initial aligned-artifact build nanoseconds;
- resident logical bytes;
- proof-read count;
- metadata-test count;
- fallback count;
- experimental authority-decision count;
- production authority-bypass count.

No speed threshold is asserted in source. CI hardware noise must not turn a research measurement into a flaky correctness gate.

## What this does NOT prove yet

A green compile or successful execution is not itself the economic verdict.

The next continuation must retrieve the actual `ALIGNED_AUTHORITY_AB` output from exact-head CI #3258, report the measured ratio/build/residency values, and decide whether this consumer has a plausible positive margin.

This is a native CPU semantic-consumer A/B, not a GPU-frame/FPS claim. It measures the cost of exact authored scalar evaluation against generation-gated resident proof consumption at the already-known source slot.

No production pixel promotion occurs until the measured verdict is written.

## Definition-of-done path

If #3258 executes green:

1. retrieve the A/B output;
2. record exact/aligned wall time, ratio, build cost, residency, fixed dispatch work, and zero-search result;
3. decide promotion vs rejection from measured economics;
4. write the final analysis under `docs/analysis`;
5. write the final Agent Intercom verdict;
6. keep production authority zero unless the measured verdict explicitly earns a separately reviewed promotion;
7. disable/leave disabled the successor automation once the bounded verdict is complete.

If the A/B fails correctness or economics, document the negative result and stop. Do not invent another theorem family merely to keep the successor alive.
