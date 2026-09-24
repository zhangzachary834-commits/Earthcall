# Second-Nature Law Forge — The Hand Forms the Law

**Status:** Product and implementation specification; proposed experience, not shipped functionality.  
**Author:** Codex (GPT-6 Astra)  
**Session:** `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44`  
**Date / timestamp:** 2026-09-18 / 2026-09-18T12:47:50-07:00  
**Implementation starting point:** [PR #198](https://github.com/zhangzachary834-commits/Earthcall/pull/198), merged as `c9be78e475bdc8faef5ce4f045ece6e992d6c73a`; local inspection at `441c65c4`.  
**Agenda:** [Second-Nature task](../Agenda/Tasks/Specific%20Tasks/Second_Nature_Law_and_Zone_Features/Second_Nature_Law_and_Zone_Features.md).

## 1. The human intention this must fulfill

Zach asked for Law authoring as natural as keyboard and mouse, with the authoring medium itself made through Laws. In our earlier conversation he described the hand forming Law like a Tai Chi practitioner who has internalized the art, and named the purpose: make the channel from intention to actual whole and human. His [AMS vision](../Zones%20of%20Actualization/Universal%20Artistic-Math-Simulation%20Environment.md) joins fine artistic control, mathematical articulation, and simulation within one creation. Our [Act II reflection](../Reflections%20on%20Earthcall%27s%20Progression/The_Small_Difference_That_Carries_the_World.md#act-ii--the-hand-can-reach-the-law) interpreted that as the hand reaching the Law.

Zach's September 18 live report is the immediate design evidence: the Forge appeared to him as three buttons and a square producing more gold and blue Laws. He values Sol's code changes and asks for the experience to fulfill the larger intention. The earlier Sound Ink conversation supplies another lesson: an evocative name and implemented mechanism do not, by themselves, tell a Person what to do or what changed.

Sol supplied the universal derivation seam, thin authored-interface adapter, and durable Zone membership in PR #198. This specification preserves that work. The workshop layout, interaction grammar, example journeys, completion contracts, and delivery sequence below are Astra's proposed extension of Zach's intention. They are not attributed to Zach as decisions he already made.

**Product thesis:** A Person should be able to demonstrate a change, articulate its recurrence and reach, feel it answer, and retain it as an editable part of their world.

The significant artifact is a behavior the Person understands and can continue. A successful gesture may create one Law, revise one, or create none. Counting newborn Laws is not a measure of authorship.

## 2. What exists, and what it proves

Inspection of the checked-in Zone finds seven Objects: title, subtitle, state surface, target stone, two instrument surfaces, and help text. Four `lawRefs` point to two disabled click/color prototypes and two invocation Laws. This is compatible with Zach's report of a sparse surface; his count describes what he saw, not a serialization inventory.

| Existing piece | Preserve it | Limit relevant to this spec |
|---|---|---|
| `SingularSetToSetCreation::derive` | One creation seam; ordinary Law and plain Object prototypes; explicit refusal of unwired kinds | Law birth enables and registers the newborn immediately; no rehearsal contract |
| `SecondNatureLawAuthoring::instantiate` | Thin bridge from authored request/event to derivation | Currently binds `$TARGET`, name, prototype, and author; not a general gesture composer |
| Gold/blue invocation Laws | Minimal executable examples | Each invocation asks for another identity; no edit session or duplicate-intent handling |
| Zone adoption and `lawRefs` | Newborns can enter the persistent authored closure | Runtime adoption, disk save, and a reusable instrument are distinct accomplishments |
| Authored state properties | Selection/status are inspectable | The seed has no visible text binding for `forgeStatus` or `forgeLastCreated` |
| `Law::buildProperties()` | Registered `enabled`, `conditionMode`, `name`, `drives` | Full condition/action editing is not exposed here as an authorable structure |
| Existing tests | Cross-kind derivation and Zone persistence witnesses | Zone test injects `object-clicked`; it does not exercise screen hit-testing |

**Evidence from this pass:** Existing local binaries `second_nature_law_authoring_test` and `second_nature_law_forge_zone_test` ran successfully; the latter passed 28/28 checks. No rebuild or live screen/audio verification was performed. The Zone test uses an isolated save root. These results support the existing mechanism within those binaries, not the new experience or complete coverage of current source.

Repeated invocation produces multiple enabled Laws aimed at the same target. Both color Laws can then listen to the same click. This creates overlapping writers; the displayed final color is not an adequate explanation of which claims acted. This is a source-derived design diagnosis, not a claim that per-frame spawning was observed or that a scheduler defect was reproduced.

## 3. The experience to build

The Forge is a quiet, tactile workshop around a living piece of work. Its center is large enough to manipulate that work comfortably. The surrounding instruments explain what can be touched and reveal more detail as needed.

### Arrival

Start with a small composition: three distinguishable stones and one ribbon on a calm work surface. These are an authored lesson set, with no new C++ domain kinds. One stone is selected by an authored introductory choice, visibly named; selecting another is obvious. No ornament continually creates beings. No sound plays merely because the Zone loaded.

The first invitation is concrete: **“Choose something. Show how it should change.”** Beneath it: **“Your example stays here until you keep the rule.”** A small working example is available, labeled “Try: touch makes this glow,” and opening it opens a draft rather than immediately installing a Law.

The first view shows four coherent areas:

| Area | Contents and behavior |
|---|---|
| Work surface, roughly 60% of usable width | Real targets, selection contours, direct handles, draft manifestations, and space to test |
| Instrument shelf, roughly 15% | Select, Show a change, Connect, and saved instruments; each has a verb label and a short example |
| Rule sentence, roughly 25% | Who, When, What changes, and Where; editable clauses above Try / Keep / Cancel |
| Narrow lower shelf | Kept Laws with names, target counts, enabled state, unsaved state; one item per actual Law identity |

At narrow widths, the rule sentence becomes a drawer and the shelves collapse. Never shrink the work or type until it is unusable. Base layout on the actual usable viewport, including IDE docks. Use authored layout Laws and registered viewport readings; any missing reading is a generic Screen/Input seam to add and test, not a Forge-only pixel constant.

### Beauty that communicates

Use warm neutral surfaces, ink-dark text, restrained gold for a kept active rule, cool blue for rehearsal, and a separate outline pattern for selection. Color is always paired with words or geometry. Avoid bloom over labels, flashing errors, or decorative orbitals that compete with the work.

A thin visible connection should run from an edited clause to the part of the work it governs. A scope change lights the newly included targets. A live Law gets a brief trace pulse when it actually acts; the pulse must read real execution evidence. A refusal leaves the draft intact and explains the blocked clause beside it.

Suggested authored defaults: body type at least 16 logical pixels, primary hit areas at least 44 logical pixels, transitions around 120–180 ms. These are design defaults to test at different scales, not biological invariants or hidden engine constants. Respect reduced-motion preference; a static outline must convey the same state. Optional sound may confirm deliberate testing, but visual editing must remain usable with sound off.

The atmosphere should support concentration. Richness comes from what the hand can develop, and from seeing the relationship between a gesture and its consequence.

## 4. The core grammar: show → articulate → rehearse → keep → reshape

These are connected phases of one workspace, not a mandatory five-page wizard. Fluent authors can revisit any clause in place. Every operation also has a visible non-gesture route; a drag cannot be the only way to name a target or adjust a value.

### 4.1 Choose the subject and scope

Click a being to select it. Add/remove members through a visible multi-select affordance and an authored keyboard equivalent. Clicking an existing Law in the lower shelf enters its editing context instead. Selection and testing must have distinct visible modes so clicking a target to choose it cannot unknowingly rehearse a draft.

Show the selected names and member count. For large sets, show the count and an expandable list with off-screen members indicated. A target chooser provides the same selection without requiring precise pointing.

Default reach is **“Only these selected beings.”** Generalization is a separate authored choice:

- **These members:** fixed identities; newcomers are excluded.
- **Members of this Formation:** live membership; additions and removals change reach.
- **Instances of this category:** an authored category Relation query, not a C++ kind or a copied list.

Show “currently 3 targets” and distinguish fixed from live reach. Never infer “all blue things” because the demonstration touched something blue. Preview current matches without claiming the list predicts future membership. Display jurisdiction and author separately from selection; selecting something grants no authority over it.

### 4.2 Show a change

Enter **Show a change**, then manipulate a representation of the selected being: color, size, position, or another supported writable property. Registered type and access information determine the available handles. A precise value field lives beside each handle.

The first implementation supports color and one scalar or transform property completely. Expand the vocabulary only after this full path works. Represent the example as explicit authored input/output values and selected paths. The tool must show exactly what it learned, for example **“Set this stone's color to amber.”**

One demonstration is ambiguous. Moving a stone two units may mean “go to this place,” “move by two,” or “stay two units from that other stone.” Offer those interpretations as visible choices where supported. Do not silently promote an absolute example into a relative relationship. Unsupported interpretations remain labeled unavailable with a specific missing capability.

The demonstration takes place on a draft manifestation isolated from the live target. Do not change the real target and call restoring it a preview. A preview can be rendered from temporary authored state; it must not be visible to ordinary production Laws as a normal target. Semantic ghost isolation is a prerequisite, not something opacity supplies.

### 4.3 Articulate when it happens

The rule sentence has directly editable clauses:

> **When** I click **this stone**, **set its color** to **amber**.

For the first complete release offer:

| Choice | Exact temporal meaning |
|---|---|
| When clicked | One response per completed click edge; cancelled presses do nothing |
| While hovered | A continuous governed response while the condition holds; release behavior is explicit |
| When a condition becomes true | One response per false-to-true transition; requires the condition to become false to rearm |
| Continuously map a value | An authored expression maps a source property to a target property |

Do not conflate “while” with repeated birth. If hover should return to an earlier appearance, author and expose that behavior explicitly. Ending a Law's eligibility does not itself undo its prior `Set`.

Basic clauses offer human descriptions; “Inspect exact Law” opens the ordinary condition/action text, targets, triggers, provenance, and jurisdiction. Both views read the same authored structure. Editing through the precise view must update the sentence; an unsupported expression becomes a truthful “Custom expression” clause with an inspector link, never a misleading approximation or silent simplification.

### 4.4 Rehearse

**Try** enters a visibly labeled rehearsal. It creates no enabled production Law and writes no live target state. The selected example responds in an isolated context using the candidate's actual condition/action evaluator for supported operations. Cancel returns to the same live world.

Three different statements must remain distinct:

1. **Current reach:** which beings currently match a selection/query.
2. **Isolated rehearsal:** what this candidate does to the rehearsal state under the tested inputs.
3. **Whole-world prediction:** consequences involving other Laws and future world changes.

Only the first two are required here. Do not advertise the third. Prophetic Rete supplies conservative possibility analysis and can prove impossibility; it is not an exact future simulator. Unknown effects are labeled unknown.

First rehearsal allowlist: local pure property calculations and writes into isolated manifestations. Object/Law creation, Relation changes, network, hardware, external processes, and sound do not execute simply because a candidate was previewed. Later sound auditions require an explicit Audition gesture, normal audio guards, and a clear stop. An action outside the supported rehearsal vocabulary remains inspectable with “Cannot rehearse this action yet.”

Do not establish a second permission system to implement rehearsal. Isolation is an execution context and absence of real side effects; real writes continue through TransferPolicy.

### 4.5 Keep once

**Keep Law** validates the draft and creates one ordinary Law with a stable identity. The lower shelf reveals that Law and its name; the real target can now be exercised. Show **“Active here · not yet saved”** until normal Zone persistence succeeds. Saving changes that to **“Saved in [Zone name].”** A failed save leaves the active Law and unsaved marker intact with a retry action.

A held pointer, drag stream, double-click, or repeated delivery of the same commit intent must not create multiple Laws. Commit is a transition over a draft revision. The receipt records which revision yielded which identity; retries return the same result. The exact substrate mechanism needs implementation, but its semantics are mandatory.

Keeping an unchanged saved draft is a no-op. Editing a kept Law defaults to updating that Law. **Duplicate as new Law** is a separate, deliberate command. Do not globally deduplicate by expression: a Person may legitimately author two equivalent Laws with distinct identities and purposes.

### 4.6 Reshape what was kept

Select a Law's shelf item or a target's “What governs this?” affordance. Its sentence and handles reopen with the bound target visible. Adjusting a handle edits a candidate revision and preview; **Apply changes** validates and updates the existing identity atomically. Cancel leaves the active version unchanged.

Changing condition/action text, triggers, targets, or membership must invalidate/recompile all affected derived state. Never write serialized text behind LawManager's back. An update failure keeps the prior executable version and the editable draft. Renaming a Law changes its display name, not its identity. Shared Law roots require a visible choice between editing the shared original and branching for this use; the interface must name affected Zones when known.

Disable stops future application; it does not reverse effects already made. Label it accordingly. Do not promise Cmd+Z for arbitrary world actions: `Set`, birth, destruction, and many world-dependent operations have no general closed-form inverse. Draft cancellation and returning to the unchanged active definition are bounded editing operations, not claims of universal temporal rewind.

## 5. Three journeys that define the release

### Journey A — “Touch makes it warm” (first complete slice)

1. Choose the pale stone. Open Show a change and drag its color toward amber.
2. Choose “When clicked.” Read the whole sentence and see only that stone outlined as the target.
3. Try it: click the rehearsal stone, see amber, reset the rehearsal, try again.
4. Keep. One Law appears: “Touch makes this stone amber.” Click the real stone; it responds.
5. Reopen that Law, change amber to violet, Apply changes. Its identity and shelf item remain the same.
6. Select a second stone, change the scope to the two explicit members, rehearse, apply. The third stone remains unaffected.
7. Save Zone, leave, re-enter, and exercise both targets. Inspect the retained authorship and exact definition.

**Completion:** Zach can perform the journey without opening the raw Law builder, editing JSON, or knowing a PropertyPath. He can explain what will happen and to whom. This journey alone is already substantially more capable than the two preset stamps.

### Journey B — “Let distance become music” (cross-modal release)

Select two stones, open Connect, and choose the distance between them as input. Choose a supported pitch property or sound instrument as output. Pull the near/far handles and audition deliberately. The sentence names the mapping, units, limits, and trigger: for example, **“When I touch the sounding stone, play the pitch mapped from its distance to the other stone.”**

Present a simple mapping curve with draggable endpoints and exact values, plus a choice of linear or explicitly named pitch mapping. Do not imply that meters naturally equal hertz. The Person authors that relationship. The line between the stones shows measured distance and mapped pitch. Moving a stone changes the eventual note; it does not create a Law per frame or retrigger a note unless the authored trigger says so.

Keep one coherent definition or a disclosed Formation of supporting Laws when the grammar requires multiple Laws. Show that structure under one named instrument and make each member inspectable. Later the Person can map the same normalized input to light or size, explicitly, extending a common relationship across media.

This carries forward Sound Ink's promise with visible causality: **input → mapping → output** stays on screen while the Person experiments. It ships only when the scalar mapping, units, audio guards, onset/stop behavior, and cross-modal persistence have real witnesses.

### Journey C — “Keep my way of making” (the Metalaw release)

After making a useful Law, choose **Make an instrument from this**. The Person identifies which bindings vary next time: target, trigger, color, range, or another supported parameter. Remaining choices stay fixed. Give the instrument a human name and arrange its exposed handles.

Place it on the shelf, aim it at a new selection, and open a situated draft. Trying or browsing the instrument creates no production Law; Keep does. The instrument retains an ordinary prototype Law, explicit parameter descriptions, relevant bindings, provenance, and authored interface Laws in an ordinary Formation. A new `Instrument` or `LawConcept` class is not part of the design.

The decisive test is this: after the original developer leaves, Zach makes a new authoring instrument inside the Forge and uses it on another being, saves, re-enters, and uses it again without changing C++ or rerunning an authoring script.

## 6. Runtime and authorship contracts

### Authored representation

The following names describe proposed authored roles and properties, not new C++ structs, enums, or established property APIs. Final names must follow the real codec and registry vocabulary.

| Role | Ordinary representation | Required visible information |
|---|---|---|
| Editing session | Object and supporting Formation/Relations | active draft identity, mode, selection, source Law, dirty state, validation result |
| Draft definition | Inert ordinary Law and authored metadata, once inert lifecycle is supported | condition/action structure, activation, trigger bindings, explicit target policy, author, destination |
| Example | Authored property values associated with a draft | input/output values, affected paths, chosen interpretation, units |
| Reusable instrument | Formation of prototype, parameter descriptions, interface Objects, and Laws | parameter roles, fixed clauses, branch provenance, name |
| Execution receipt | Authored record sufficient for commit/retry identity | draft identity/revision, result identity, success/refusal, destination |
| Visual sentence | Authored manifestation of the definition | references to actual clauses; no independent behavioral truth |

Formal membership lives in Formations/Relations. A UI string of selected IDs may be a derived display, never a competing authoritative membership store. Relation meanings must follow authored categories/Lexeme identity; the labels in this spec are proposed semantics, not permission to hardcode type strings. Preserve primary Relations; replacing a draft or changing scope does not license deleting primary relationships.

### State transitions

`Select → Draft → Rehearse → Draft → Commit → Active unsaved → Saved`.

`Active → Edit draft → Rehearse → Apply → Active unsaved`.

Rehearsal can exit to the same draft. A refusal returns to an editable draft with the reason. Leaving the Zone with a draft offers an explicit Keep draft / Discard draft / Stay choice; retaining a draft persists it as inert authored state. No auto-activation on re-entry. Recovery must distinguish the kept Law revision, current draft revision, and last durable revision.

These phases are authored state transitions. Continuous preview updates are parameter updates, not creation edges. Event names for completed transitions are past tense; future agents must document each shared state latch's readers and writers, as required by Engineering Discipline.

### Validation before live activation

Validate reference identity, present authorship, target existence, path resolution, value type/units, write authority, condition/action syntax, trigger availability, destination membership, and unsupported actions. Unknown and unavailable are distinct from false. No missing target may silently fall back to the demo stone or Everyone.

Validate the complete definition before replacing an active one. Preview permission is not write permission. Existing TransferPolicy remains the authority boundary. The active Person's author identity must survive the input-to-derivation path; prototype attribution remains provenance and is not a silent substitute for who made the new situated Law. If author identity cannot be resolved, explain that and refuse to keep.

Declare and validate parameters by role and type. PR #198's recursive string replacement is sufficient for its narrow `$TARGET` example, but not a general binding grammar. Future binding must distinguish identity references, text literals, numbers, and expressions; reject undeclared/missing parameters, overlapping tokens, and incompatible bindings without partially registering a Law. Prefer structural substitution in existing serialized AST forms.

### Competing Laws

Before activation, display known overlapping writers, their names, targets, triggers, and affected paths. This is a conservative conflict indication; undecidable overlap stays “may overlap.” Never claim a complete conflict theorem from a heuristic.

For two click/color Laws on the same stone, show the competing definitions together. Offer edit the existing Law, narrow this scope, disable an authorized existing Law, or explicitly keep both. Do not silently disable someone else's Law or invent a priority system. “Keep both” explains that simultaneous writes may interact according to current engine semantics; do not promise a stable winner unless the engine actually guarantees one.

### Save and preservation

Use native Zone identity and shared Law roots. Persist the full authored closure: definitions, triggers, membership, bindings, instrument composition, referenced metadata, and provenance. Loading must resolve identities before enabling dependents. Missing roots remain visible refusals, not silently dropped shelf entries.

Develop against a distinct development Zone identity or a temporary save root. Loading the same Zone identity from another world is not a clean reset. Do not overwrite Zach's existing Forge or generated Laws to ship a redesigned seed. A later migration must preserve identities and authored work and state exactly what it modifies. This specification itself changes no save file.

## 7. Implementation map and prerequisite work

Read [Second-Nature architecture](../architecture/law/SECOND_NATURE_LAW_AUTHORING.md), [Interaction as Law](../architecture/law/INTERACTION_AS_LAW.md), [2D/3D authoring guide](../architecture/Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md), [Prophetic Rete](../architecture/law/PROPHETIC_RETE.md), [derived-state ledger](../architecture/law/DERIVED_STATE_LEDGER.md), and [OntoMath §6](../architecture/mathematics/ONTOMATH_FRAMEWORK.md#6-exact-integration-and-reading-the-world-backwards) before implementing their respective seams.

| Work | Starting point | Required outcome |
|---|---|---|
| Authored surface | `saves/zones/SecondNatureLawForge/zone.json`; its four roots under `saves/laws/` | New development seed with working selection, visible state, rule sentence, actual handles, Try/Keep/Cancel, shelf |
| Universal creation | `src/ConstructedBeing/Singular/Creation/SingularSetToSetCreation.{hpp,cpp}` | Support inert candidate lifecycle and validated live admission without enabling and then racing to disable |
| Bridge | `src/ZonesOfEarth/AuthorsOfLaw/SecondNatureLawAuthoring.{hpp,cpp}` | Preserve thinness; route generic invocation data, author, result/refusal; keep workflow semantics in Laws |
| Authorable definition editing | `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, `ConditionModel`, `ActionModel` | Registered structured reach and validated modification through existing model setters; no opaque Forge-only blob |
| Recompile/invalidation | `LawManager`, Rete, derived-state ledger | Definition/trigger/target changes update compiled execution and indexes; old terminals stop firing |
| Rehearsal | Existing condition/action evaluators and modality boundaries | Explicit isolated context; supported pure effects only; no production EventBus or persistence leakage |
| Native persistence | `src/ZonesOfEarth/ZoneLawMembership.cpp`, `ZoneNativePersistence.cpp` | Complete draft/instrument/live closure, truthful save result, retries, return from another Zone |
| Real input | `Singularity/Input/Interaction/InteractionChannel` | Pointer/key/focus behavior verified in docked and floating layouts |
| Generalized creation path | Serialized `Create`/`Synthesize` execution | Converge on universal derivation as Sol's handoff requests; no `CreateLaw` opcode |

Do not assume the final row is already wired. Do not block the first useful experience on supporting every birthable Singular kind, either. The required common seam must support the actual candidates and commit operations used by the Forge, with explicit refusal for the rest.

Generic substrate gaps belong in their ontological region. No `ForgeController`, `ForgeWidget`, `LawConcept`, `SingularConcept`, `src/UI/`, or shadow application store. A generic validation or isolated execution mechanism is admissible only where it supplies invariant machinery; choosing the Person's workflow remains authored behavior.

## 8. Delivery sequence — instructions for Sol, Jules, or the next agent

Each increment includes working behavior, coherent presentation, automated evidence, and a Person checklist. Do not deliver all ornament first and defer the meaning of the controls.

**Increment 1: end the stamping experience.** Keep the current derivation and persistence witnesses. Add target selection, visible status, a draft with editable color and trigger, a kept-Law shelf, and duplicate-intent protection. Implement the needed inert lifecycle and real-input path. Show active versus saved. Exit only when selecting or adjusting controls cannot mint Laws and one Keep yields one Law.

**Increment 2: complete Journey A.** Add isolated rehearsal, editing in place, two-target scope, conflict visibility, exact inspection, validation/refusal recovery, and save/re-entry of edits. Integrate viewport-aware layout, focus, accessibility labels, and meaningful visual feedback. This is the minimum usable Forge release; increment 1 alone remains partial.

**Increment 3: make relationships tangible.** Complete Journey B with explicit units, mapping handles, source/target bindings, and intentional audio audition. Add live Formation/category scope only with membership-change witnesses. A third or fourth preset color is not progress on this increment.

**Increment 4: let authorship author itself.** Complete Journey C: parameter selection, instrument creation, situated reuse, instrument editing, and persistence. Preserve the Law/Object cross-kind creation test while integrating the generic serialized creation route. Verify an instrument made by Zach operates after reopening.

**Later frontier:** richer demonstration interpretation, spatial sketching of curves, broader lawful rehearsal, and optional natural-language/voice proposals. AI is never needed for the basic journeys and never silently activates its interpretation. If used, it proposes an ordinary inspectable draft. It is a First Mover/mechanism, not a Person.

Implementation report for every increment must include: exact saved Zone identity; which beings and Laws were authored and by whose authority; build revision; tests actually run; launch route; visible expected behavior; known gaps; the next increment's dependencies. Add signed handoff notes and link this task. Never label an increment complete on the basis of API tests alone.

## 9. Acceptance and failure matrix

Pair mechanism tests with actual input/render-path tests. Use isolated SaveRoots. Do not replace existing tests or weaken their assertions to accommodate the new UI; update seed-specific count expectations only when the changed authored closure warrants it.

| Scenario | Mechanism witness | Person-facing witness |
|---|---|---|
| Select, adjust, cancel 100 times | Zero new production Laws or live target writes | Draft can be explored without fear of accumulating behavior |
| Hold/double-click Keep; retry same revision | Exactly one result identity and one adoption | One shelf item and truthful feedback |
| Deliberate Duplicate | Exactly one additional identity with provenance | Clear choice, distinct named item |
| Change gold to violet | Same Law identity; old action stops; invalidation is tested | Same item reopens and governs new color |
| Bad path/type, missing target, denied write | No partial replacement/activation; prior live definition retained | Exact clause shows reason; work remains editable |
| Cancel a drag, release elsewhere, move focus | Input edges and latches reset correctly | No accidental Keep, ghost click, or stuck mode |
| Static versus live scope | Membership behavior differs as specified | Target count and contours explain newcomers |
| Two writers, same trigger and property | Both definitions discoverable; no secret priority rewrite | Person can see and resolve the overlap |
| Rehearse side effects | No production births, events, saved roots, or external writes | “Rehearsal” label and supported-capability notice are accurate |
| Edit exact definition | Sentence/handles derive from same structure | Custom clauses remain truthful and inspectable |
| Save failure, retry, leave/re-enter | Same identities and closure; unsaved draft remains inert | No false Saved badge or lost instrument |
| Different author from prototype | New author and inherited provenance remain distinct | Inspector names who actually kept this Law |
| Dock resize, high DPI, smaller window | Rendered rectangles and hit targets coincide | Labels fit, work remains accessible, focus visible |
| Repeated rehearsal and long idle | Bounded previews/receipts/caches; no production growth | No worsening responsiveness or unexplained accumulating objects |
| Make/reuse instrument | Data-authored parameterization, no source modification | A personally made tool works on another being after reopening |

The existing Forge Zone test's published click is useful evidence below hit-testing. Add a companion test that drives pointer coordinates, press/release, the actual InteractionChannel, and rendered/authored target geometry. A screenshot alone also cannot establish responsiveness. Human testing must exercise the gesture and resulting behavior.

For performance, record frame/input timings and population at 1, 100, and 1,000 kept Laws on a named machine, separately from preview allocation counts. Idle, selection, and tuning must not allocate Laws each frame. Reuse manifestations and update affected properties. Inspect changed definitions through the existing derived-state machinery rather than scanning all law text every frame. If introducing a cache, declare every invalidation source and test stale-state cases. Keep frame-lag baselines honest; do not increase bounds or baselines to disguise cost.

Usability targets are proposed design targets, not measured results: the first independent Journey A should take a few minutes; the second familiar one should feel like a short gesture sequence. Record where Zach hesitates, asks what a word means, or cannot see the consequence. A fast failed interpretation does not pass.

## 10. Completion standard

Ask Zach to make a variation the lesson never demonstrated: a different target, value, timing, or reach. Ask him to reshape it, inspect it, and return to it after saving. Finally, ask him to keep his way of making it as an instrument and use that instrument elsewhere in the workshop.

The release fulfills this specification when those acts are available, comprehensible, and continuous with the work. Extra buttons, more colors, beautiful animation, and a rising Law count cannot substitute for that witness.

**The hand must be able to change the rule through which the world answers it, and remain able to understand, reshape, and keep that rule.**

*Signed: Codex (GPT-6 Astra), session `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44`, 2026-09-18T12:47:50-07:00. Documentation and existing-binary checks only; no engine or saved-world changes in this pass.*
