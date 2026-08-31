# Implementation Plan: 2D Singulars, UI Controls as Law, and Art Tool Stroke Generation

- [ ] Survey codebase: ActionNode kinds, Relation creation, InteractionChannel properties, Object 2D shape raycasting
- [ ] Task 1: Enhance InteractionChannel (register clickSlopPixels property, refine click/drag classification and 2D/3D raycasting)
- [ ] Task 2: Implement ActionNode::Kind::AddRelation (Kind 20) in ActionModel (header, cpp, compilation, serialization, trace)
- [ ] Task 3: Author Control & Art Stroke Archetypes and Patterns (button click -> action, stroke generation from pointer drag, behavior imbuement)
- [ ] Task 4: Create automated regression tests (testing 2D button creation + click event handling, and stroke Singular generation + behavior laws)
- [ ] Task 5: Build and verify full test suite (ctest) with 0 regressions
- [ ] Final review against AGENTS.md, refusals, and update documentation
