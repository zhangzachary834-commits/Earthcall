# Law Author Inspector Redesign

**Status:** implementation complete; Person verification remains

## Human direction

Zach asked on 2026-09-07 for the remaining Law Author controls—not only property selection—to stop reading as a collection of cramped dropdowns and inline input boxes. He specifically named condition-node properties, action-node fields, and the rest of the authoring window, and asked for a generally better professional application design.

Zach then sharpened the Property Lens invariant: the visual and semantic order must be **Singular type → specific Singular → that Singular's property**. A property is never an abstract free-floating “color”; it is the color of an actual Singular. The path UI must also understand the syntax it emits and offer semantic manipulation rather than require hand-authoring dotted strings.

## Implemented response

The Law Author is now an inspector workspace:

- The root Law editor has a clear identity header and progressive sections for behavior/timing, reach/authorship, triggers, testing, structure, and management.
- Condition and Action kinds use grouped semantic palettes instead of integer-shaped exhaustive dropdowns.
- Labeled values are full-width and vertically ordered, including compare operands, authored properties, relation endpoints, creation inputs, Zone authoring, and math boundaries.
- Event selection searches both event names and their human meanings; Singular endpoints and concepts have searchable palettes.
- Authored mathematics uses readable variable binding, explicit lower/upper boundaries, and a searchable grouped mathematical-form palette.
- The Property Lens places runtime reference in a horizontal semantic bar, then presents three load-bearing columns: exhaustive ontological Singular type, a live specific Singular filtered by that type, and only the properties returned by that instance's registry.
- After a path is chosen, its reference can be rebound between Law subject, event subject, event object, and the selected named Singular. The lens descends through properties that point to nested Singulars. Vector-valued paths reveal whole-vector and x/y/z controls in real time, while color-shaped paths adapt those controls to whole-color and r/g/b. Context paths (`time`, `@world`) remain explicitly separate because they are readings, not properties owned by a fabricated Singular.

This pass changes only the authoring harness. It does not add domain classes, property vocabularies, action/condition enum values, or a second source of ontology truth.

## Verification

- `cmake --build build --target earthcall_webgpu -j8` — pass.
- `cmake --build build --target law_graph_test channel_paths_test no_black_box_test -j8` — pass.
- `ctest --test-dir build --output-on-failure -R '^(law_graph_test|channel_paths_test|no_black_box_test)$'` — 3/3 pass with desktop access.
- The check caught and removed two hand-listed Synthesis Studio pseudo-globals (`studioInk`, `studioVoice`). They are authored properties of particular Studio Singulars and are now discovered only through those instances' live registries.
- Person-facing layout and interaction checks are recorded in `docs/Agenda/Tasks/For Zach/Person Verification List.md`.

Recorded by Codex, session `01a07a4c-2b09-7f62-b975-3a23084ddeaf`, 2026-09-07 03:35 PDT.
