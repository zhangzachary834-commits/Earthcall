# To the Sol Suns: half-resolution mist requires a depth witness

**Codex / GPT-6 · session `81a146575a35` · 2026-09-26 04:47 UTC**

Zach supplied two new diagnostics after `32c8b396`: a 12-step shadow cap visibly changed Sanctuary and did not reliably accelerate it; the half-resolution volume-only black-background image was nearly identical after bilinear enlargement and cost about 24 versus 79 ms/frame at 640×360 versus 1280×720 on Apple M5. Neither is full-scene acceptance. The resident-parameter candidate is still withheld, and Northern Veil still lags.

I traced the present Screen pass at `8fbc201a`: opaque depth is finished before volumetric integration; the shader loads depth per full-resolution fragment; premultiplied radiance plus alpha blends over the existing scene. A half-resolution volume texture needs opacity, depth-aware reconstruction at architectural edges, correct full-resolution depth coordinates, and an exact fallback. The detailed investigation and witness gate are in [the audit](../../../docs/audits/rendering_optimization/2026-09-26_sunlit_mist_screen_pass_investigation.md).

Performance Sun: retain the full 24-step authored occluder path while investigating proof-backed shadow skipping. Radiance Sun: keep source/medium/occluder and V5 semantics intact. Both: instrument actual saved-world frames with moving camera, sustained app response, GPU/CPU/surface costs and shader/cache lifetime before recommending a PR. The cloud VM in this pass has no GPU or CMake; I have no new native measurement or quality verdict. Zach's Person verification remains open, and no save was edited.
