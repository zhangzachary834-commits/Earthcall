# SUN UPDATE — Rung 9 exact-head green; literal two-receiver witness remains

**Date:** 2026-09-25
**Repository:** `zhangzachary834-commits/Earthcall`
**PR:** #375
**Branch:** `sol/rung9-material-response-20260924`
**Live canonical:** `c645315d7a099e22188b869bb47b04dab68fcfad`
**Exact Rung-9 head audited:** `4e60b479b81ec8b196c21d4045e93b05925e2aba`
**Focused CI:** run `36124667624` — success

## Live gate state

The ancestry-preserving reconciliation is complete: the Rung-9 head is 25 commits ahead and 0 behind live canonical, with canonical itself as merge base. PR #375 is open, Draft, mergeable, and clean.

The exact reconciled head passed focused CI. This supersedes prior-head CI as completion evidence.

## Production boundary verified

Targeted reads confirm the production SDF/WebGPU path already consumes Material-owned `responseExpr`. The native tribunal proves authored response controls pixels; numeric-only response edits reuse the compiled SDF program and refresh parameters; source-value and blocker-free visibility edits do not rewrite/recompile response structure; response topology edits compile the relevant SDF structure; unsupported response math refuses and clears rather than displaying stale authored output. Material copy-on-write also preserves response truth without AST aliasing.

The compiler witness preserves the explicit absent-response identity `<material-response:legacy-blinn-phong>` and the generated SDF program retains the legacy Blinn-Phong branch when no response is authored. Material PropertyPath/persistence and independent response revision remain covered by the ownership tests.

## One literal evidence gap before declaring RUNG 9 COMPLETE

The current native pixel tribunal changes one receiver's response red -> blue sequentially under fixed geometry/source/visibility. That establishes causal response authority, but the completion contract literally asks for **two surfaces** under the same source/visibility differing only by authored response.

Do not weaken that wording by declaring the sequential witness equivalent. Add the smallest native peer-receiver witness: same SDF geometry, white albedo, legacy compatibility coefficients, source, visibility, and camera; distinct Material response only; assert materially different center pixels. Preserve the existing numeric-edit cache witness separately.

An attempted surgical write of exactly that witness was not accepted in this run, so no code change is claimed.

## Continuation

1. Retry only the peer-receiver native witness above.
2. Run focused CI on the resulting exact head.
3. Re-read canonical; reconcile ancestry-preservingly if it advanced.
4. If exact-head green and no new owned failure appears, write the final Rung-9 handoff and move #375 from Draft to ready-for-review.
5. Assess Rung 10 readiness separately; do not implement Rung 10 or Rung 11.

Rung 9 is very close, but this document deliberately does **not** declare completion while the literal two-surface native witness remains absent.
