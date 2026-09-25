# SUN UPDATE — Rung 9 A/B CI in flight; canonical advanced

Date: 2026-09-24
PR: #369
Branch: `sol/already-known-execution-key-consumer-20260924`
A/B code head under test: `4910fefcbde402b2d2a65a260a7c7d6518d6a371`
Intercom predecessor head: `9765e0fe30f25aee156d7aecf50e471a7c56b516`
Focused CI: #3258 / `36103064608`

## Fresh-start audit

This continuation re-read the successor handoff, the Rung-9 A/B update, live PR #369, live canonical, and exact-head CI before making any new claim.

Live canonical has advanced to `8980f160c6583732dd4acdd3af7a24060b1a69ce`. A compare against the active successor branch reports the branch 4 commits behind canonical and 33 commits ahead. PR #369 remains open, draft, mergeable, and clean.

Do not mistake the PR API's event-era base SHA for current canonical: the live compare is the authority for drift.

## Exact-head CI state

At this writing #3258 is still in progress on the actual A/B code head `4910fefc...`.

The three visible jobs are:

- Focused CPU tests (macOS): building focused regression witnesses;
- SDF range-proxy verification (macOS): building SDF proof/GPU parity witnesses;
- Slow Adapter independent clock (macOS): beginning checkout/configuration.

Therefore there is not yet a truthful `ALIGNED_AUTHORITY_AB` timing verdict to record.

## Boundary remains unchanged

No production authority was added in this continuation. The only authority remains the bounded native A/B harness. Production `authorityBypassesApplied` remains required to stay zero.

## Exact next continuation point

1. Re-read live canonical first; it may advance again.
2. Read #3258 on `4910fefc...`.
3. If the SDF CPU witness completed, retrieve the exact `ALIGNED_AUTHORITY_AB` output and record exact/aligned ns, ratio, build ns, residency, proof reads, metadata tests, fallbacks, and authority decisions.
4. Reconcile the successor branch with then-current canonical before final closure; preserve ancestry and do not snapshot-overwrite newer canonical work.
5. Run exact-head CI on the reconciled head if reconciliation changes code ancestry.
6. Only after the A/B result and reconciled exact-head evidence are settled, write the final analysis + final Intercom verdict and disable the successor automation.

Do not broaden theorem scope while waiting for measurement.


## 2026-09-25 continuation — previously blocked retention write landed

The earlier native run #3258 / `36103064608` completed green and executed the aligned authority A/B witness, but the decisive `ALIGNED_AUTHORITY_AB` line existed only in the oversized job stdout and was not retained as a small artifact. That made the numerical timing verdict unrecoverable through the available connector without guessing.

The previously blocked CI-only repair has now landed on this same successor branch at `7aafec4092d983657ac59d1d6410e7297bf25a3f`.

The SDF CPU witness now:

- tees `rendered_field_piecewise_synthesis_test` output to `aligned-authority-ab.log`;
- extracts only the `ALIGNED_AUTHORITY_AB` line into `aligned-authority-ab-result.txt`;
- uploads that tiny result as the `aligned-authority-ab` artifact with 7-day retention;
- preserves `pipefail`, so test failure cannot be hidden by `tee`;
- leaves production authority unchanged.

This is evidence plumbing only. It does not alter authored mathematics, theorem semantics, channel authority, V1–V4 independence, or the production zero-authority boundary.

### Exact next continuation point

Wait for exact-head CI on `7aafec4092d983657ac59d1d6410e7297bf25a3f`, retrieve the tiny `aligned-authority-ab` artifact, record the actual timing/build/residency numbers, then re-read current canonical and reconcile ancestry before final closure. Do not infer the timing ratio from the earlier green run; measure it from the retained artifact.


## 2026-09-25 continuation — retained exact-head A/B result recovered

Exact-head focused CI #3281 / `36165550485` completed green on successor head `6d962cbaba60554882cf9998f412fd3c6045bee2`. The retained `aligned-authority-ab` artifact successfully captured the decisive native A/B line.

Measured witness:

- iterations: 200,000
- exact authored evaluation: 374,692,792 ns
- aligned proof-read path: 14,772,083 ns
- ratio, exact / aligned: 25.364926x
- artifact build: 1,750 ns
- logical resident bytes: 70
- proof reads: 200,000
- metadata tests: 800,000 (four fixed provenance tests per read)
- proof fallbacks: 0
- aligned authority decisions: 200,000
- production authority bypasses: 0

### What changed in the economic hypothesis

PR #350 rejected generic per-sample proof consultation because relevance discovery itself was expensive: the Perlin road had to rediscover which theorem applied to a spatial sample. This successor witness removes that hidden relevance search by consuming a semantic execution identity that the renderer already selected independently: an aligned source binding slot.

Under that deliberately narrow condition, proof consultation is not merely non-regressive in this harness; the measured aligned path is about 25.36x faster than repeated exact evaluation of the authored zero Piecewise expression. This does **not** establish universal Prophetic profitability, and it does **not** grant production pixel authority. It establishes a narrower result: when semantic execution identity is genuinely already known, the economics can differ radically from the rejected spatial relevance-search road.

The production boundary remains unchanged. `authorityBypassesApplied == 0`; exact authored mathematics remains sovereign outside the bounded benchmark authority arm. Channel sovereignty and independent V1–V4 semantics remain intact.

### Canonical drift and exact next continuation point

Do not close or promote from this pre-reconciliation measurement alone. Current canonical has advanced beyond the successor ancestry and includes an independent workflow edit. Re-read current `sync-from-earthcall-main`, reconcile ancestry without snapshot overwrite, preserve both workflow intents, then run exact-head CI again on the reconciled successor head. Only after that green reconciled run may the bounded successor receive its final measured authority verdict and final `docs/analysis` / Agent Intercom closure.


## 2026-09-25 continuation — canonical ancestry reconciled

The successor branch has now been ancestry-reconciled with live canonical `32c8b396275b64354c910bd8666a4800cdb75eec` by merge commit `0142b56a12ea8ca269c9c751d39d281db173cdbb`.

This was not a snapshot overwrite. The merge commit has the prior successor head `8c87fd96ddc57eb05668fded0cfe8424cb1a15c5` as first parent and canonical `32c8b396275b64354c910bd8666a4800cdb75eec` as second parent. A merge-base check after the write resolves to that canonical SHA, establishing that the captured canonical head is now true branch ancestry.

Targeted three-way audit found exactly one path changed by both sides since their previous merge base: `.github/workflows/earthcall-ci.yml`. The reconciliation preserved both intents:

- canonical's `save_system_error_test` focused-CI coverage;
- successor's `rendered_field_direct_dispatch_test` coverage;
- successor's retained `aligned-authority-ab` result artifact and pipefail-safe capture.

All other successor-owned files were overlaid unchanged onto the canonical tree because the targeted changed-path intersection showed no other two-sided path overlap. Canonical-only files therefore remain canonical-owned in the merged tree.

Production authority remains zero. This reconciliation changes ancestry and CI composition only; it does not promote the benchmark authority path into rendering.

### Exact next continuation point

Run/read exact-head CI on the reconciled successor head (including this documentation continuation). If canonical advances again before closure, re-read and reconcile only the new delta. If reconciled exact-head CI is green and the retained A/B remains valid, write the bounded final `docs/analysis` verdict and final Agent Intercom verdict; do not invent a new rung.
