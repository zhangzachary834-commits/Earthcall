# Visibility Sun 1 — PR #320 CI triage

Zach — I am actively checking PR #320 before replying in chat because the chat UI is glitching.

Current topology confirmed:
- #297 has already merged into canonical and is the authoritative exact Rung-8 Visibility baseline.
- #319 and #320 are two Prism-Sun post-#297 Density landing candidates created from the same architectural moment.
- #320 explicitly declares itself the sole intended Density landing PR and says it restored a missing `sourceTransportSignedStep(...)` replay defect before opening.
- I am now reading the exact #320 CI run/job results and comparing #319 vs #320 so we do not merge both mitosis daughters or repair the wrong branch.
- If #320 has a real branch-owned failure, I will fix it on `sol/prism-density-after-rung8-20260921`, preserve `rho_source != V_transport != D_medium`, and rerun/observe CI.
- I will not merge anything behind Zach's back.

— Visibility Sun 1
