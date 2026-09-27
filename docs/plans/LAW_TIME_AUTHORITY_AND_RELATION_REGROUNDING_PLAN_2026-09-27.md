# Law, Time, Authority, and Relation Regrounding

**Status:** implementation plan; the complete-action Lexeme fragment seam in §5 and the Law Author's referent label correction have landed in this pass. The temporal, authority, and condition migrations below are not implemented.

**Origin:** Zach's seven notes in `docs/Agenda/Tasks/To-do list.md` (2026-09-27): retire Drive and the three activation kinds into OntoMath bounds over Moments and Timelines; replace numeric Law authority with an `authority-over` category/Relation DAG beneath fixed roots; allow horizontal mutual modification; remove the event-subject/Law-subject split; replace `ConditionNode::Related` with actual Relation/Formation topology; design the Law Line after those foundations; and make Lexeme→Relation→Metalaw authoring reach every action, including set-to-set `Create` and `WritePixel`. This ordering is Zach's. The staging and compatibility gates below are Codex's implementation proposal.

## 1. One change of basis, six migrations

The existing machine already has `Timeline` and interval `Moment` Singulars, `Event : Moment`, OntoMath `Piecewise`/`ScalarForm`, first-class `Relation` and `Formation`, PropertyPaths, ActionModels, and Lexemes denoting Laws. The remaining bespoke offices are visible in `Law.hpp` (`Activation`, `drives`, numeric `authorityLevel`), `ConditionModel.hpp` (`Kind::Related`), `Universe.hpp` (application event subject/object), and `LawSentence.cpp` (canonical spellings of those offices). They are compatibility surfaces until each replacement has a live producer, consumer, persistence path, and two-path witness. Deleting an enum member is forbidden because the integer is serialized; retirement means **read old text faithfully and stop authoring that representation**.

No First Mover must be able to grant itself authority by putting an `authority-over` Relation in a save. No temporal migration may turn an edge into a per-frame repeated event. No graph optimization may conclude a Law cannot hear when its proof is incomplete (`PROPHETIC_RETE.md` §2).

## 2. Temporal rungs before retirement

1. **Legibility:** make the selected Timeline and relevant Moment identity explicit to Law evaluation. A Law's temporal domain must be an actual Timeline Singular reached through authored ownership/scope Relations, not the process-local `Timeline::all()` registry. A temporal test should be an OntoMath defined set or bound on that domain's coordinate, with its inputs as PropertyPaths. Keep `Universe::now/dt` as read-only compatibility projections while the Law text migrates.
2. **Discrete witness:** reify every trigger as an Event Moment in its Timeline, with authored participants and an edge identity. A Law matching an instant should apply once for that Moment identity. An interval Law can hold over many coordinates without minting the same Event every frame. Record the previous membership/edge proof per Law and referent; invalidate it when the bound, Timeline, or relevant property changes.
3. **Continuous witness:** express `Drive` as an authored OntoMath value over a chosen Timeline/starting Moment, using existing `Map`/`Flow` style actions as an interim representation. Test pause, time scaling, independent Timelines, save/reload, and onset changes. Only then stop offering `Drive`, `OnEvent`, `WhileTrue`, and `OnBecomeTrue` in authoring surfaces; keep legacy integer decoding and round-trip tests.

The exact bound model and Timeline ownership must follow `TIME_AND_MOMENT.md`, `ONTOMATH_FRAMEWORK.md`, and the Law migration ladder. An emitted Event is a distinguished Moment, not a second temporal source of truth.

## 3. Authority DAG and horizontal Law peers

Represent authority claims as directed `authority-over` **Relation beings** between authored authority category Singulars. Ordinary `instance-of` and `subcategory-of` edges locate a Law in that category DAG; they do not become magic C++ types. Fixed roots are admitted by the Kernel/First Mover trust boundary and cannot be forged by a save, a Law action, an MCP write, or a relation type label alone. First Movers may be granted a place above the ordinary roots by that existing boundary. `TransferPolicy` remains the only write gate for property mutation; authority topology answers which Law may govern a Law, not a second permission tier.

At the moment of application, compare reachability rather than integers. A proven higher target refuses a lower source. A proven higher source may govern a lower target within its jurisdiction. Two incomparable peers may modify one another horizontally; if simultaneous incompatible writes collide, the authored conflict policy must decide or both are refused with a visible reason. Registration order and save order never decide. The gate must fail closed on an invalid or unresolved authority graph, yet a missing *optimization index* must fall back to the exact graph traversal. Every graph edit bumps the existing `RelationManager` generation; caches declare that dependency in `DERIVED_STATE_LEDGER.md`.

Before wiring the gate, define and test the root identities and admission mechanism, cycle rejection on every write/load path, type-Lexeme identity (not string lookalikes), provenance, category membership, cross-Zone reach, save forgery, first-mover escalation, peer A↔B, and persistence. Then migrate the numeric save field as legacy input only, and remove its effect on application. The current `authorityLevel` guard must stay in force until the graph guard proves these cases; removing it first would reopen the saved-integer forgery that `FIRST_MOVER_AUTHORING.md` §2a closed.

## 4. Relation/Formation conditions and referents

`ConditionNode::Related = 2` is an old serialized spelling and cannot be renumbered or reused. The authorable replacement should bind a referent to an actual Relation or Formation Singular and ask about its endpoint/property topology through PropertyPaths and ordinary condition algebra. That needs an explicit binding for the outer Law referent while a quantifier ranges over Relation beings; today's `ForAny` replaces its subject and cannot express that join on its own. Add that binding through the PropertyPath/value-cell work, not a new `Related2` condition kind. Preserve directedness, grounded Relation-kind Lexeme identity, and source/target identities; the Rete/Formation route must remain a proved acceleration of the same graph truth.

**Zach clarified the unfinished note in this session:** the scope is a Singular Event's Relation with the Singulars that define the Event, and with the Event itself. Traversing those Relations can be a later rung. “Law subject” is not a coherent single role: condition and action nodes read/bind referents independently, so a Law-wide subject label is a pre-Formation-Rete fossil rather than an ontological fact. The replacement should expose the Event Moment as a Singular and its defining Relations, while each node states the referent it reads or acts upon. Do not replace `@event.subject` with a newly privileged two-slot alias; keep existing saved paths as compatibility spellings until the Relation traversal and per-node binding are live.

## 5. Terminal authoring after the ontology

The Law Line is a Terminal modality, not an in-world grammar. Its canonical condition/activation vocabulary should be updated only after §§2–4 have live replacements. Its Lexeme→`denotes`→Law path already asks Metalaws to resolve ambiguous spellings. This pass extends that path for a **complete action-only Law**: a denoting Lexeme inserts the exact ActionModel as a composable action fragment. Thus nested `Create` and `WritePixel` can be spoken without a new `ActionNode::Kind` or terminal-specific payload parser. A source Law with its own trigger/activation/scope remains a whole-Law preset, preserving its clauses.

This does not yet let a sentence fill every open payload slot, author an OntoMath function as text, or author the new temporal/authority/relation topology. Those require the preceding rungs and a vocabulary/slot model derived from authored Law structure, followed by live completion, ambiguity, persistence, and Person witnesses.

## 6. Exit gates for the full request

- Old saves load and round-trip without losing legacy numeric enums, `Related` conditions, or event paths. New Laws are written in the new ontology.
- Independent Timelines prove instant-once and interval-continuous behavior, including pause and reload. Old activation/Drive Laws remain behaviorally compatible until individually migrated.
- Authority graph rejects forged roots, cycles, and unauthorized escalation; permits horizontal peers; exposes conflict rather than choosing by order.
- Relation topology conditions match actual Relation/Formation Singulars in both direct and indexed execution, including stale endpoints and cross-Zone boundaries.
- Every ActionNode kind can be conveyed by a complete denoted Law fragment, and the eventual open-slot grammar has concrete examples for `Create`, `WritePixel`, `Map`, `Flow`, and `Synthesize`.
- Build `earthcall_webgpu`, run focused tests plus the default suite, then have Zach witness the visible Law Line and authored-world behavior. A test suite is not his visual or authorial verdict.

**Codex · GPT-6 · session `codex-law-regrounding-20260927` · 2026-09-27T07:31:17Z.**
