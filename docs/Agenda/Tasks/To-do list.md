## INSTRUCTIONS GUIDE FOR WRITING HERE:
1. Make each point on the list as concise as possible.
2. If you need to write a longer paragraph, put it in a separate file or create a new directory for it inside the Specific Tasks folder.
3. Judge what parts you want to write are more suited for the Agenda and what is more suitable for a separate, dedicated doc that is not inside Agenda.
4. Ensure you always link up relevant docs, files, and classes. Only do so if the directory is already in your context—-do not spend compute trying to search for it.

Near-term priorities (2026-08-14 — from architecture review):
1. **Remove `EventEntity` (Ontological debt / Refusal #1)**: Delete `src/Singularity/Core/EventEntity.hpp` and `.cpp`; translate `EventBus` custom events to Relation-based representations with `type` field; remove `_activeCustomEvents` / `Universe::addActiveEvent` in `Law.cpp`. (Ref: `scratch/audits/audit_report_2026-08-13.md`).
2. **One Person-facing creation path, end-to-end**: Prove full loop (Law fires → object exists → save → reload → re-target by stable identifier) using Shape Generator 3D law seed (`saves/tests/shape_generator_3d_law.json`).
3. **Unseal `Soul` and `Formation` properties**: Expose membership and `relationTypeTag` in `Formation::buildProperties()`; expose `_identity` as read-only `ComputedProperty` in `Soul::buildProperties()`.
4. **Hierarchy of Joys (Operational requirement)**: Add minimal load-bearing constraint (e.g. `Singular::satisfiesJoyBounds()` or First Mover seed law requiring explicit joy ordering).

Architectural Actualization
1. Ensure `Relation` and `Formation` ontology is load bearing.
2. Ensure Singularity-level actions are done through `Relation`s and `Formation`s.
3. Have `Relation` store things with `Singular`'s data structure framework and `Formation`s.
4. Define the Hierarchy of Joys precisely; make it operationally load bearing.
5. Convert SDF and Bezier patch geometry frameworks to Law-adjustable properties; verify via Law tool.
6. Fully realize `Relationship` and `Community` ontologies (replace placeholder stubs).
7. Make `Key` and `KeyBind` `Singular`s and integrate into Earthcall.
8. Make cursor a baseline avatar.
9. Fully realize `Body` and `BodyPart` vision.
10. Define light ontology to expose shaders to Persons.
11. Zone jurisdiction resolution.
12. Fully realize the Ourverse vision.
13. Integrate 2D capability into Singularity by decomposing fundamental parts for Person-authored Laws.
14. ✅ **Singular and Object set-to-set creation** — done and verified (2026-08-16): Exposed all 19 `ActionNode::Kind` values in `LawGraphWindow.cpp`; reworked kind 17 (`Synthesize`) into composed child actions (`Create`, `AddProperty`, `Map`); verified via `tests/singular_set_to_set_test.cpp` (45/45 pass). See [Specific Tasks/Singular_and_Object_Set_to_Set_Creation.md](Specific%20Tasks/Singular_and_Object_Set_to_Set_Creation.md).
16. **Ensure properties are exposed via PropertyPaths (Refusal #6)** — PARTIAL (2026-08-13): Enforced via `docs/architecture/NO_BLACK_BOX.md` and `tests/no_black_box_test.cpp`. Four beings remain on sealed debt register (`World`, `Ourverse`, `Formation`, `Soul`).
17. Finish Law synthesis.
18. **Write Propertypath governance framework ontology** — PARTIAL (2026-08-13): Governance spine established in `docs/architecture/NO_BLACK_BOX.md` §2. Unwritten: member ownership, Zone occurrence, sacredness, ordering, Hierarchy of Joys access, and Metalaws.
19. **Write Kernel boundaries** — PARTIAL (2026-08-13): Channel refusal requirements defined in `docs/architecture/NO_BLACK_BOX.md` §2a/§2b. Unwritten: general boundary doc and implementation of `Singular::satisfiesKernelBounds()`.
20. Implement Voice, Sound, and Audio primitives.
21. Update `README.md`.
22. Finish migrating to WebGPU.
23. 2D objects, `Singular`s, and UI created by 2D system.
24. Human Language-Symbolic processing `Formation`s.
25. Resolve Singularity external app integration.
26. Multi-device Earthcall networking and inter-device paradigms.

Propertypath exposure debt (tracked in `tests/no_black_box_test.cpp`):
1. `World` — `World::buildProperties()` is `{}`; resolve retirement into `Zone` before populating properties.
2. `Ourverse` — `Ourverse::buildProperties()` is `{}`; expose `ownedObjects`, `primaryGatheringZone`, and `laws`.
3. `Formation` — `Formation::buildProperties()` is `{}`; expose membership and `relationTypeTag`.
4. `Soul` — `Soul::buildProperties()` is `{}`; expose `_identity` as read-only `ComputedProperty`.
5. `Object::rotation` — Round-trips lossily via Euler angle conversion; recorded in `kWriteExemptions`.
6. `Perspective` — `Perspective.cpp` is empty; implement constructor or retire stub.

Housekeeping:
0. ✅ **Fill in agent intercom** — done and verified (2026-08-15): Implemented `agent intercom/conversation_history_injection.py` and `agent intercom/README.md`.
0. ✅ **One-click WebGPU launch** — done and verified (2026-08-14): Added root `Run Earthcall.command` invoking `scripts/build.sh webgpu run`.
0a. ✅ **One-click WASM and Python launches** — done and verified (2026-08-14): Added `Run Earthcall WASM.command` and `Run Earthcall Python.command`; fixed import path in `src/Singularity/Foreign/py/app.py`.
1. ✅ **Organize repo upper directories** — done and verified (2026-08-13): Moved root utility scripts to `scratch/` per `DIRECTORY_ORDERING.md`; cleaned abandoned build dirs.
2. ✅ **Organize scratch folder** — done and verified (2026-08-14): Subdivided `scratch/` into `probes/`, `legacy/`, `scripts/`, `fixtures/`, `audits/`, and `experiments/`; added `scratch/README.md`.
3. ✅ **Gitignore cached clang** — done and verified (2026-08-13): Added `.cache/` to `.gitignore` and untracked index files.

Feature-sized (split out of Housekeeping 2026-08-13):
1. ✅ **Restore legacy 3D create tool & author Law counterpart** — done and verified (2026-08-13): Restored `Tool::ShapeGenerator3D` in ImGui as First Mover (`CreationChannel`); authored Law version (`saves/tests/shape_generator_3d_law.json`) activated by `L` key; fixed `Engine::initLogic()` null dereference. See [Specific Tasks/Legacy_3D_Create_Tool_Restoration.md](Specific%20Tasks/Legacy_3D_Create_Tool_Restoration.md).
2. Personally test 3D shape law and other laws.
3. Redesign ImGui developer tool for 2D and 3D objects; turn text features into `Lexeme` writers with preset font First Movers.
4. ✅ **In-world test observation feature** — done and verified (2026-08-13): Created `dump_test_save` helper (`saves/tests/`) and developer UI panel for in-engine test loading.
5. Make `Lexeme` authorable with flexible layout/spacing fixtures.

Bugs:
1. ✅ **The self-lifting floor** — done and verified (2026-08-14): Fixed ungrounded list index fallbacks in `World.cpp`/`Ourverse.cpp`, fixed origin vs. `supportOffset` clamping in `Physics.cpp`, and filtered ground by `baseline` attribute in collision loop; guarded by `tests/ground_plane_test.cpp`. Open sub-issue: `Tool::ShapeGenerator3D` cursor hit fallback on miss. See [Specific Tasks/Self_Lifting_Floor_Bug.md](Specific%20Tasks/Self_Lifting_Floor_Bug.md).

R&D:
1. Determine best ML strategies for Integration classifiers.
2. **Formations with Neuro-Symbolic ML / OntoMath** — PARTIAL (2026-08-13): Implemented `Formation` neural structure with Hebbian learning in `LanguageSystem::tick`. Next step: migrate hardcoded constants to authorable Law rules. See [Specific Tasks/Neuro_Symbolic_Formations_and_ML.md](Specific%20Tasks/Neuro_Symbolic_Formations_and_ML.md).

Basic design before creating fully working products with Earthcall
1. Write foundational design specification.
