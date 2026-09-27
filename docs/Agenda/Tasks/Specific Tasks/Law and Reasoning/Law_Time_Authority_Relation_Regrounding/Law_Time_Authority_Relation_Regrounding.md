# Law Time, Authority, and Relation Regrounding

Zach's 2026-09-27 To-do notes call for one connected migration: temporal bounds over Moment/Timeline, an `authority-over` Relation DAG, horizontal Law peers, explicit referents, actual Relation/Formation topology conditions, and a Law Line whose vocabulary follows those foundations. [The implementation plan](../../../../../plans/LAW_TIME_AUTHORITY_AND_RELATION_REGROUNDING_PLAN_2026-09-27.md) records the dependency order, invariants, compatibility gates, and remaining checks.

## Current implementation

- Complete action-only Laws denoted by a Lexeme can now contribute their exact ActionModel as a composable Law Line action. `Create` with children and `WritePixel` are covered by focused grammar checks; a live denoting Relation and terminal-spoken Law are exercised end to end.
- The Law Author Property Lens now calls an unqualified path's binding “Current referent” and labels the two old Event slots as participants A/B, so its UI no longer claims a single “Law subject.” The runtime path model is unchanged.
- Numeric authority, activation enums/Drive, `ConditionNode::Related`, and event-subject/Law-subject compatibility paths remain live. This task is **partial**, and the plan does not count as implementation of those migrations.

## Next work

Establish the temporal and authority graph source of truth with save/load and anti-forgery checks, then replace the authorable Related/referent form and revise the CLI vocabulary. Keep legacy serialized integer values readable and preserve inhabited saves through targeted patches. Zach clarified that the scope is the Event Singular, its defining Singulars, and their Relations; condition and action nodes bind their own referents, so no single “Law subject” should be promoted into the new ontology.

**Codex · GPT-6 · session `codex-law-regrounding-20260927` · 2026-09-27T07:31:17Z.**
