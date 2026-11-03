# Law Time, Authority, and Relation Regrounding

Zach's 2026-09-27 To-do notes call for one connected migration: temporal bounds over Moment/Timeline, an `authority-over` Relation DAG, horizontal Law peers, explicit referents, actual Relation/Formation topology conditions, and a Law Line whose vocabulary follows those foundations. [The implementation plan](../../../../../plans/LAW_TIME_AUTHORITY_AND_RELATION_REGROUNDING_PLAN_2026-09-27.md) records the dependency order, invariants, compatibility gates, and remaining checks.

## Current implementation

- Complete action-only Laws denoted by a Lexeme can now contribute their exact ActionModel as a composable Law Line action. `Create` with children and `WritePixel` are covered by focused grammar checks; a live denoting Relation and terminal-spoken Law are exercised end to end.
- The Law Author Property Lens now calls an unqualified path's binding “Current referent” and labels the two old Event slots as participants A/B, so its UI no longer claims a single “Law subject.” That label change did not alter path binding.
- EventBus occurrences now have stable identities independent of verb, participants, and second; Rete keeps the Event Moment alive while a triggered Law reads its registered properties through `@event.*`. Law Line completion offers those properties from Event's property registry. Historical Event properties are read-only during application; the saved participant paths remain compatible.
- Numeric authority, activation enums/Drive, `ConditionNode::Related`, and event-subject/Law-subject compatibility paths remain live. This task is **partial**, and the plan does not count as implementation of those migrations.

## Next work

Place Event occurrences in their Timeline through authored Relations, and make their defining Singular Relations traversable from each node's referent. `Core::Event::Custom` still lacks an Event Moment; Event load/round-trip is still absent. Then establish temporal bounds and the authority graph source of truth with save/load and anti-forgery checks, replace the authorable Related/referent form, and revise the CLI vocabulary. Keep legacy serialized integer values readable and preserve inhabited saves through targeted patches. Zach clarified that the scope is the Event Singular, its defining Singulars, and their Relations; condition and action nodes bind their own referents, so no single “Law subject” should be promoted into the new ontology.

**Second pass evidence:** `event_test` and `event_interest_filter_test` exercised occurrence identity, property reflection, legacy paths, direct Event write refusal, and triggered application. `law_line_test` and `law_line_zone_test` exercised completion, including live Zone vocabulary from the Event registry. All four focused tests passed; `earthcall_webgpu` built. Person observation remains open. `continuous_law_test` compiled, but the headless macOS XPC/GLFW session stalled before its first assertion, so it gives no runtime verdict here.

**Codex · GPT-6 · session `codex-law-regrounding-20260927` · 2026-09-27T07:31:17Z.**
**Second pass: Codex · GPT-6 · session `codex-law-regrounding-20260927` · 2026-09-27T17:10:24Z.**
