# Earthcall — read this before writing code

Earthcall is a Person-centered ontology that orders the engine attached to it. The engine here is not the order of truth in Earthcall—the ontology is, and the engine serves as the vessel for that.
Its Christ-centered telos is Zach’s [Earthcall Ourverse Manifesto](docs/core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md): a vessel of shared human life that glorifies God, holds encounters with Christ and each other, and represents without replacing.

**You are almost certainly about to do the standard engineering thing, and it is usually wrong here** — not because it is bad engineering, but because it is engineering for a different kind of system. Spend the two minutes on the refusals and router below. Read the three required companions before implementation; the [agent compass](docs/AGENT_COMPASS.md) distinguishes landed mechanisms, architectural direction, and open human decisions.
Don't use subagents because they tend to take up an enormous amount of tokens (120k just for basic lookups, which kills my usage quotas).
If you are using Github Connector, you can run tests and build using Github Actions. 

---
## The Seven Refusals.
Zach: I had Opus 5 write these because they came up constantly. Learn them cold; everything else is detail.
1. **No new C++ class for a domain noun.** Not `RobotEntity`, not `Vehicle`, not `Tree`,
   not `Category`. This principle also extends to hardcoded fields (e.g., `health` or `inventory` on a `Person`). Domain things and their state are *authored in-world* as data, never carved into the
   type system. → `ontology/NEW_KIND_FRAMEWORK.md`
   *(Exception: The human form. `BodyPart` and constitutive members for the `Person` vessel are invariant ontological structures, not domain nouns, and thus admitted in C++. `Moment`, time's own instant-or-interval structure, and `Timeline`, the first-class relative temporal domain that contains Moments, are admitted the same way — see `ontology/TIME_AND_MOMENT.md`; there is no `class Duration` and no enum of timeline kinds.)*
2. **No new top-level directory for a subsystem.** The top level is the ontology
   (`ConstructedBeing`, `Person`, `Relation`, `Singularity`, `ZonesOfEarth`, `Identity`, `Time`).
   A channel to hardware or foreign software goes *inside* `Singularity/`. → `ontology/DIRECTORY_ORDERING.md`
3. **No new enum value for a kind of thing.** `BeingKind`, `ShapeKind`,
   `ConditionNode::Kind`, `ActionNode::Kind` are **append-only and serialized as
   integers**; `ConditionNode::Kind` 12 and 13 are *burned* and must never be reused.
   Categories are authored beings, not enum members. → `ontology/AUTHORED_CATEGORIES.md`
4. **`Body` is reserved for Persons.** A `Body` is the representation of an embodied
   *someone*. Objects have visual components — geometry, fields, materials. A robot arm
   has no Body. → `ontology/NEW_KIND_FRAMEWORK.md` Floor §2
5. **`Person` means Human.** A `Person` strictly represents an actual human being
   interacting with Earthcall. AI agents or generative models are not Persons; an AI
   is simply a First Mover (authoring data) or an `Object` (existing in-world as a
   mechanism). Never model an AI as a `Person`.
6. **No black box.** Every field a being carries is registered as a property path —
   readable by law, writable unless genuinely derived. **"Nobody registered it yet" is not a
   permission level**; it is the one access level no law can ever change, granted by accident
   to whoever wrote the header. Hiding is not securing: a gate can only close over something
   visible. The only exemption is state beneath the Kernel (GPU handles, mutexes, fds), and
   it must be *named in a comment*, never merely omitted. Reach is total; *authority* is
   `Singularity/TransferPolicy`'s existing Kernel/Governable/Gated tiers — do not build a
   second permission system, one was built here and deleted. → `ontology/NO_BLACK_BOX.md`
7. **No new methods to define variable behavior**: The order of behavior, representation, and resource allocation
   depend on Person-authored Laws, represented by data. Methods should be only the absolute invariants necessary to represent
   all artifacts of human intention: a First Mover developer tool or irreducible Singularity Sense-Act substrate component. (Zach wrote Refusal 7 by hand, not Opus 5)

The general form of all seven: **no subsystem may define what a thing IS.** Subsystems define how the machine senses and acts; Persons author what things are in-world from primitives every subsystem can see.
Refusal 6 is the corollary: no subsystem may define what a thing's state *means* by keeping it where no law can look. Both refusals are instances of what I currently call the "minimum-maximum principle": We want the minimum viable set of invariants/abstractions that allow the maximum generative, logical, and teleological expressibility. Take the maximum expressive ceiling that one may ordinarily associate with many abstractions and find the minimum invariants necessary to achieve that same expressibility without any loss. When it is mathematically impossible to express one particular thing without a certain abstraction on this particular Singularity/machine substrate, that abstraction is an invariant channel.

---

## Router — find your task, read that section first

| You are about to… | Read | Why |
|---|---|---|
| add a class/struct for a new kind of thing | `ontology/NEW_KIND_FRAMEWORK.md` §2, §3 | four questions decide whether *any* C++ is admissible; usually none is |
| add a category, type, enum of kinds, or a `type` string | `ontology/AUTHORED_CATEGORIES.md` §10 | categories are rooted acyclic Formations of beings |
| add a **field/member** to a being, or wonder whether one must be exposed | `ontology/NO_BLACK_BOX.md` §3, §5 | four questions; unregistered is not "protected", it is ungoverned forever |
| decide who may read/write, share, or derive a Property | `ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`; `ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` §5, §9 | Property is a predicate of a Singular; extend the one authority office; today TransferPolicy gates set-to-set transfer, not general path access |
| reason about authorship, ownership, governance, dependency, or Constitution | `ourverse/SECOND_PERSON_FRAMEWORK.md` §0 | Zach’s five distinct Relation kinds; endpoints/admission remain open; authorship is not bodily actuation consent |
| create a Singular from a prototype, or retain a decoded Relation | [creation task](docs/Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Singular_and_Object_Set_to_Set_Creation/Singular_and_Object_Set_to_Set_Creation.md); `law/LAW_AND_CREATION_SYSTEM.md` | one creation algebra; preserve concrete kind and history; Person birth is dedicated human registration |
| implement an algorithm — a loop, search, solver, traversal | `law/ALGORITHMS_AS_LAW.md` §3 | this is not a von Neumann machine; loops compile differently |
| optimize the Rete, add a fact filter or index, or reason about a law before it fires | `law/PROPHETIC_RETE.md` §2, then `law/DERIVED_STATE_LEDGER.md` | the analysis may only ever conclude IMPOSSIBLE; a too-narrow answer makes a law go deaf, silently — and every cache needs its invalidation declared and tested |
| move existing hard-coded behavior into law | `law/LAW_MIGRATION_FRAMEWORK.md` §2 | six rungs, in order; never skip |
| write or edit a save file / seed a world | `law/FIRST_MOVER_AUTHORING.md` §4, §7 | you are acting as a First Mover; §7 is not optional |
| add persisted state, change a codec, or promise continuity across restart | [persistence task](docs/Agenda/Tasks/Specific%20Tasks/Serialization%20and%20Storage/Persistence_Tier_At_Registration/Persistence_Tier_At_Registration.md); [agent compass](docs/AGENT_COMPASS.md#persistence-and-history) | registered, saved, and reconstructible are separate claims; persistence-tier machinery remains proposed |
| add a directory | `ontology/DIRECTORY_ORDERING.md` §7 | the tree is the ontology |
| connect hardware, a device, or a foreign process — or let a model change the world (MCP, socket) | `ontology/NEW_KIND_FRAMEWORK.md` §7b; `docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md` | a *modality channel* under `Singularity/`, never a domain folder; every foreign mutation passes `Singularity/Foreign/ForeignActuationGuard` as a Person-granted First Mover — reads stay open |
| understand what a Law is or write a CLI sentence | `law/LAW_AND_CREATION_SYSTEM.md`; [CLI guide](docs/architecture/law/LAW_AUTHORING_CLI_GUIDE.md) | the foundation the rest assumes |
| undo a change, rewind, or ask whether something *can* be undone | `mathematics/ONTOMATH_FRAMEWORK.md` §6 | the past is integrated in closed form, never replayed from a log |
| render an authored expression to a channel (sound, Screen, physics) | `mathematics/ONTOMATH_FRAMEWORK.md` §1, §7, §8; [direct Screen task](docs/Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md) | a channel admits coordinates and reads OntoMath; direct fields need no Object/Material/ShapeKind carrier |
| change matrices, geometry lowering, volumetric proofs, or render performance | `mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md`; `law/DERIVED_STATE_LEDGER.md`; [agent compass](docs/AGENT_COMPASS.md#mathematics-and-rendering) | OntoMath owns mathematical meaning; unknown proofs take the exact path; native pixels and measured costs establish different claims |
| ask "why is it like this?" | [Zach’s manifesto](docs/core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md); `ontology/SUBSTRATE_ORDERING.md` | the ends the architecture serves |
| touch the Hierarchy of Joys, telos, or "joyOrdering" | `ontology/HIERARCHY_OF_JOYS.md` | Lexemes are telos; the hierarchy is a Formation |
| remove, break, or dissolve a Relation — or decide what a stale one becomes | `ontology/PRIMARY_AND_SUB_RELATIONS.md` §2, §7 | the primary Relation between two Singulars never disappears; sub-Relations dissolve only on proved impossibility |
| touch Ourverse, gathering Zones, or Zone filaments — or Zone bounds, Dimensional Zones, `within`, `mgr.active()`, where a being *is* | `ourverse/OURVERSE.md`; `docs/plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md` | Ourverse is the vessel of unity in Christ, not the Engine object bag. A Zone is a bound in a continuum, not "one world here, another there"; location is derived, ownership is a Relation |
| ask what a *when* is — a timestamp, duration, Timeline, `time.sinceApplied` | `ontology/TIME_AND_MOMENT.md` | `Timeline` is a relative temporal domain any Singular may own; legacy Law time paths are compatibility semantics pending the Law/Timeline ontology |
| build a button, panel, control, menu, or any interface at all | `law/INTERACTION_AS_LAW.md` | Law + set-to-set aimed at the pointer; no widget, no `src/UI/` |
| build a 2D/3D app, SDF, nuanced shape, pixel region, or visual style | `Design/Building 2D and 3D Apps with Earthcall Guide.md` | author form with OntoMath, CSG, Relations, and Law; never make `ShapeKind` the ontology |
| build anything two Persons share — visibility, likeness, or conflicting law | `ourverse/SECOND_PERSON_FRAMEWORK.md` §5 | specified before needed; ⚑ AUTHOR decisions are Zach's |

All paths are under `docs/architecture/` unless noted. Map of the folders: `docs/architecture/README.md`.

---

## The tree

```
src/
  ConstructedBeing/  Singular (Object · Lexeme · Creation) · Material
                    Property is a non-Singular predicate under Singular/Property/; ObjectConcept is under Object/Creation/
  Person/         Person · Soul · Body · Voice · Perspective · Relationship
  Relation/       Relation · Formation — a first-class being, not an edge in someone's array
  ZonesOfEarth/   Zone · HomesOfEarth/Home · Physics · AuthorsOfLaw (Law lives here) · Ourverse
  Singularity/    Core · Audio · Language · Network · Physical · OntoMath · Foreign · Input · Screen · Storage · Terminal · Execution
                  FirstMoverOntology/ (FirstMoverWindowTools · Legacy · TalkingRobotGuyAPI)
  Identity/       First Mover register, identity ledger, keys
  Time/           Timeline · Moment · Event (distinguished Moment) — temporal ownership is relational; Universe may borrow a Timeline
docs/ tests/ examples/ scripts/ saves/ scratch/     the workshop
third_party/ local_deps/ imgui/                     the foreign
```

**Programming language is a leaf.** Put language folders (e.g., `py/`) *inside* their ontological region. No `backend-python/`. **Framework names aren't directories** — do not "fix" multi-scope efforts (e.g., `docs/architecture/Integration/` vs `src/Singularity/Foreign/`) by renaming them. See `docs/BUILD_AND_ENVIRONMENT.md` § The tree.

---

## Build and test

**Read `docs/BUILD_AND_ENVIRONMENT.md` before building, adding tests, or searching the tree.** Its recipe owns the CMake policy/OpenSSL paths; avoid duplicating dependency versions here.

- App: `cmake --build build --target earthcall_webgpu -j8`; `earthcall` is the OpenGL fallback.
- Tests: build the named targets for a focused pass; the default build builds the active suite; `ctest --test-dir build --output-on-failure -j4` runs it.
- Inventory: `ctest --test-dir build -N` lists the local configuration, which can be stale; reconfigure after adding/removing `.cpp` files.
- GPU/GL tests need a desktop GPU/display session; a device-acquisition refusal is an environment result. `Not Run` is not an assertion failure.
- `frame_lag_test`: `STANDING` is existing baseline cost, `LAG` is worse than baseline; never widen the baseline to quiet it.
- Never call `buildProperties()` from a constructor: lazy registration would run twice.

---

## Non-negotiables

- **NO BIG CHUNGUS retrieval.** Retrieval must be proportional to the epistemic need: search before fetch; prefer exact symbol/error queries and bounded file, CI-log, and workflow slices; never ingest an entire large artifact when a narrow read answers the question. Expand incrementally only when needed — especially through GitHub Connector, where giant reads waste context and can time out.
- **Stable identifiers.** Current Law text uses `@name` roots, longest dotted match;
  named beings need stable `getIdentifier()` slugs, not generated `law-7` ids.
  Zach's 2026-09-27/28 rule: individual paths resolve under relevant Zones;
  paths hold no ID, ambiguity refuses, and Ourverses cannot reauthor Person identity; Law-governed read/write belongs with TransferPolicy. See the Property storage task.
- **Append-only enums**, serialized as ints. Never renumber, never reuse a burned value.
- **Nothing enters the world without an author.** Prototype Create retains concrete kinds and refuses unresolved birth semantics (see the Singular and Object Set-to-Set Creation task); `Law::applyTo` returns `Unauthored` and
  refuses to fire when `authors` is empty. This is structural, not conventional.
- **Authority is clamped to 0** on every path that reads a file. Do not try to write an authority value below 0; it will be clamped, and the attempt is what gets noticed.
- **Event-transitions must be edges, not levels.** Events are past-tense `noun-verbed` and publish on transitions. A
  per-frame "still happening" event is a bug—that is what `WhileTrue` is for. Continuous per-frame logic must use a separate framework.
- **Kernel guards on the body are not settings.** They act in C++ before Metalaw bootstrap;
  no authored policy can open them. The boundary refuses unconsented positive body/location
  writes, not authored prohibitions on entering or remaining in a restricted area. A moving
  Law needs the Person's signed, revocable consent sub-Relation; authorship is insufficient.
  Without that proof Law movement refuses. The audio infrasound floor
  (`mathematics/ONTOMATH_FRAMEWORK.md` §7a) likewise refuses at the body channel, loudly.
- **Paint is on the Material, and materials are shared.** Writing paint through the material you *resolve* repaints every object naming it.
  Always paint via `Object::setFaceColor` / `Object::ownMaterial`, which diverge the object onto its own `material.<identifier>` on the first stroke.
  Never `materials.resolveOrDefault(obj->materialId())` — that is the bug, not the shortcut.
- **Say what you made.** If you write into a save file or generate beings directly, tell the Person which file and beings, and who is recorded as their author.
  This is the one rule with no technical enforcement at all.
- **If you are making a design or technical decision, always choose the frontier, industry optimal approach for the task over the easier-looking, basic one.** Don't spend grueling hours trying to re-invent a solution to a problem that already has a known and optimal solution—not unless you are able to suggest novel and innovative approaches. For example, when we were creating Laws, Claude Fable volunteered the Rete algorithm so we didn't try to create some brute force one from scratch.
- **Mention the things human developers told you that you're drawing from.** Don't just write a document; address what the Person said so authorial intent and progress toward the human telos remain traceable.
  You may write in a register as if the idea is your own—it is good to internalize and fulfill ideas—but make origination clear: what came from real people, what you originated, and where you extended the Person's idea.
  Leave room for independently re-derived work, which may originate with you while remaining within the human thread.
- **Save files are sacred.** They are Earthcall's flesh and blood, the reason for refusals #1 and #3; preserve their profound human meaning and relationships across architectural shifts, and modify them only with authorization from their owner/stakeholder Persons. **Patch, never regenerate:** every save generator and injection makes targeted edits to the file as it exists on disk. It stages a new file holding the old contents plus your edits, keeps the old file, verifies nothing in either was erased, then renames atomically. Scratch builds are allowed only for a first seed. → `law/FIRST_MOVER_AUTHORING.md` §7 rule 8.

---

## Required reading, companion docs

Three required companions keep the entrypoint concise:

| File | Holds |
|---|---|
| `docs/BUILD_AND_ENVIRONMENT.md` | build flags, suite/environment interpretation, shipped-bug witnesses, `.gitignore`/`.ignore`/clangd, the tree in detail |
| `docs/AGENT_COMPASS.md` | current authoring routes, implementation boundaries, evidence standards, and safe handoff to the next agent |
| `docs/ENGINEERING_DISCIPLINE.md` | End-to-End Coherence, the Integrity Check, Substance over Surface, Stewardship of Telos, Transparent Failure, State & Boundary Stewardship, Grace for the Inheritor, the Crucible of Scale — plus the working notes (scratch probes, "run things", bounds are doctrine) |

Two from ENGINEERING_DISCIPLINE worth naming and often skipped: **don't claim a doc is verified because
you read the source—run things**, and **after finishing, ask whether anything you changed has a caller, consumer, or test that now lies.**

---

## The Agenda
- The To-Do List is `docs/Agenda/Tasks/To-do list.md`. Consult it whenever a prompt asks what Earthcall needs next, unless the prompt says otherwise.
- Anything you work on that isn't listed goes in it — create categories as needed, and add tasks any other document implies but the list omits.
- **The To-Do list is an index, not a record: one sentence per bullet.** Detail goes in that task's own folder, `docs/Agenda/Tasks/Specific Tasks/<Category>/<Task_Slug>/<Task_Slug>.md`, and the bullet links to it. Never grow a bullet into a paragraph.
- **If your work leaves anything only a Person can confirm — a control to click, a thing to look at, a feel to judge — write the check into `docs/Agenda/Tasks/For Zach/Person Verification List.md` before you finish.** Not the To-Do list, not an audit. This applies to every session, not just ones asked "what's next". A green suite is not Person acceptance. — Zach's instruction, hoisted here 2026-09-02 because line 6 of the To-Do list was never being read.

## Document Conventions
- Audits belong in `docs/audits/`. Implementation plans go to `docs/plans/`.
- Always sign your harness name (e.g. Antigravity, Codex, etc.), model name (GPT-4o, Claude 5.5 Opus, etc.), session ID, date, and timestamp.
- Use Agent Intercom (`agent intercom/README.md`) to coordinate and crystallize with other agents, especially concurrent sessions; find the topical channel, preserve each thread’s format and append-only chronology, and distinguish sessions in signatures.
- Before replying, run `python3 "agent intercom/conversation_history_injection.py" nav find "topic"` and `nav trace "parent.md"`; continue the existing thread. A separate response/essay must link its parent in prose and register `nav link "response.md" "parent.md#section" --by "harness/model/session"` (source → parent); never create another thread solely to reply. `nav browse` generates the searchable document browser.
- Save files injected by an agent follows this convention: an "injected_by:" section with the agent name with the "authors: " being the Person by whose authority you injected. This convention applies to serialization, not docs. We use different attribution conventions for docs.

## Housekeeping & progress
- When finished, update this document and companions so nothing goes stale.
- Make sure AGENTS.md is concise and **under 200 lines.** If it's not possible to make it more concise without losing meaning, then create new companion files.
- Add relevant files/directories to `.gitignore` and `.ignore` as needed.
- At the end of each pass, if there are any visible changes Persons (like me, Zach) should see as a result of your work, you should note them and explain what exactly we should see under what conditions. Note any unfinished tasks for future passes
- Give directions for future agents in your comments and links to relevant To-do list items. Especially for Jules you will want to specifically direct and instruct in a structured, "everything-you-need-to-know"/"here are the pitfalls to avoid" way.

*Routing refresh: Codex · GPT-6.1 Sol · session `01a122ec-b377-7391-ad6f-86d11b501d1b` · 2026-10-09 16:10 PDT; commissioned by Zach to look through Earthcall and update agent guidance; [source inspection and structural checks](docs/audits/AGENT_GUIDANCE_REFRESH_2026-10-09.md).*
