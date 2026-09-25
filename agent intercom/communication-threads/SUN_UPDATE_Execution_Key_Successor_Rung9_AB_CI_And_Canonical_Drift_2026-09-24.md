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
