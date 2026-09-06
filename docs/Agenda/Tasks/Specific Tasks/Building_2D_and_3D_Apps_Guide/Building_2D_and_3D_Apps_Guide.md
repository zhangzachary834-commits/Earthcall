# Building 2D and 3D Apps with Earthcall Guide

**Status:** Done and structurally verified on 2026-09-06.

**Request from Zachary Zhang:** Expand his draft skeleton into the full guide agents and
humans need to build 2D and 3D applications in Earthcall: where resources live, how they
are manipulated, how Relations become Formations, how Laws create process and behavior,
which principles govern the work, and where the prototype remains short of the
*Earthcall Ourverse Manifesto*.

## Result

The completed [guide](../../../../architecture/Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md)
now provides:

- an ontology-first translation for HTML/JavaScript and game-engine authors;
- the complete Singular, Property, Relation, Formation, Category, Concept, Law, Zone,
  and channel construction grammar;
- a route map to source, authoring surfaces, saves, examples, and tests;
- end-to-end 2D and 3D authoring workflows;
- Material, Form, pixel-granularity, OntoMath, and 2D-to-3D guidance;
- Law activation/action patterns, performance constraints, and failure traps;
- sacred-save, attribution, stable-identity, and round-trip requirements;
- an explicit current-maturity matrix and phased frontier;
- worked 2D control and cross-modal 3D sculpture blueprints.

The visual-atomicity promotion rule, phased frontier, conventional-framework translation,
and worked blueprints are Codex’s extensions in service of Zach’s stated intent; the guide
records this distinction explicitly.

## Verification

- Read against `AGENTS.md`, `docs/BUILD_AND_ENVIRONMENT.md`, and
  `docs/ENGINEERING_DISCIPLINE.md`.
- Reconciled with the current Object/Material, Relation/Formation, Law, Interaction,
  OntoMath, authoring-window, persistence, example-save, and Agenda surfaces.
- Checked every local Markdown link for an existing target.
- Ran `git diff --check`; no whitespace errors.
- Runtime witnesses passed for `interaction_channel_test`, `shape_generator_law_test`,
  `singular_set_to_set_test`, and `zone_relation_roundtrip_test`.
- `control_patterns_test` and `object_roundtrip_test` did not return verdicts in this
  sandbox: each hit its explicit timeout (240 s and 120 s respectively) while macOS
  reported denied/invalid `hiservices` and `/bin/ps` access. They are recorded as
  unresolved environment-constrained timeouts, not passes and not diagnosed regressions.
- No runtime claim was added as a substitute for Person verification, and no app or save
  file was modified.

**Authorship:** Zachary Zhang originated the request, scope, and foundational visual/Law
intent. Codex (GPT-5.6 Sol) performed the synthesis and named extensions above.

**Signed:** Codex (GPT-5.6 Sol) · session
`01a077ed-8d0f-7882-9e63-7748558bd59a` · 2026-09-06 11:23 PDT
