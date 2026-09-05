# Synthesis Studio resonance pass

Codex · session `synthesis-studio-20260904` · 2026-09-04 22:00 PDT.

Zach requested a cooler Synthesis Studio, then asked to resume after a usage limit. This pass authors the dock, resonators, meters, voice and ink selectors as ordinary save data. No Studio runtime subsystem was added. The original JSON, ecform, Zone identity document, and binary companion are backed up under `saves/backups/synthesis-studio-20260905T014017Z-zwfhvjcu/`; unrelated beings are retained. The updater is `scripts/upgrade_synthesis_studio.py` and repeated execution preserves later edits, including after engine serialization strips top-level annotations.

The initial baseline had 15 press-feedback failures because the save had no press Laws. Two new authored Laws restore those gestures. The completed Studio regression run passes, including 35 sequential pad clicks, voice switching, resonator decay, bounded musical play, and save/load. The old pick tests used hard-coded coordinates from the previous dock; they now aim at positions within the authored controls and still assert which exact being was picked. Visual/audio acceptance belongs to Zach's Person Verification List, where the remaining steps are recorded. Computer-use preview failed to open via macOS Launch Services, so no live visual or audible verification is claimed.

The resume found earlier edits already committed by another session. Concurrent ObjectProperties/Physics edits appeared during verification; those belong to their authors and are outside this pass.

Continuation, 2026-09-04 22:53 PDT: the concurrent `Physics::getFormFor` edit accessed private `Object::center` and prevented the final rebuild. I changed only those two reads to the existing `getCenter()` accessor, preserving the calculation. The final regression now also checks that selecting Tidal reaches the color of an actually created stroke. Repeated upgrade protection checks the marker Object's persisted revision as well as the top-level annotation.

Final, 2026-09-04 22:59 PDT: rebuilt Studio regression exits 0, including actual Tidal stroke creation. Earlier WebGPU build succeeded; a later app rebuild of concurrent engine edits was stopped at heavy machine load so the Studio test could finish. The latest concurrent app build is not certified. No preview app remains running. Live visual/audio acceptance is pending; all source changes made by others remain intact except the two accessor reads described above.
