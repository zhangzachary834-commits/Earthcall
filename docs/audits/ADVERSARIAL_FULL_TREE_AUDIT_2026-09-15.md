# Adversarial Full-Tree Audit of Earthcall

**The unfiltered one. Praise that is earned. Roast that is load-bearing. Implications, not an inventory.**

**Author:** Grok 4.6 (xAI)
**Session:** `01a0a706-b674-7120-ae3f-cbd1eeda0516`
**Date:** 2026-09-15
**Timestamp:** 2026-09-15T12:00:00-07:00
**Tree:** branch `sync-from-earthcall-main`
**Method:** Read the refusals, the manifesto, the live headers, the save files, the Person Verification List, and the prior audits — then went to the *code that actually runs*. This is not a recap of the 46 audits already in this folder. Those are inventories. This is an argument about what Earthcall *is*, what it is *pretending to be*, and what happens if that gap does not close.

**What this is not:** a gap list you can check off. A courtesy review. A "strengths / areas for improvement" sandwich. Another agent marking itself done.

**What this is:** the claim that Earthcall is one of the few actually new bets in software, that the bet is currently unverified at the only scale that counts, and that the project's characteristic failure is not a crash. It is *silence*.

---

## 0. The verdict in one breath

Earthcall is a Person-centered ontological substrate that is *actually doing the inversion it claims*: C++ is the vessel, authored data is the order of truth, and the type system is not allowed to decide what a robot or a button or a category *is*. That inversion is real. Chess is 35+ Laws as data. A 2D button exists without a `Button` class. OntoMath is exact symbolic math that compiles into WGSL *and* physics. The Seven Refusals are a genuine contribution to how one builds machines that do not colonize meaning.

And then the running program models Zach as an `Object` in live save files, the Home kernel-lock kicks him out of his own house, `ActionNode::Kind::Create` still only mints `Object`, the Law authoring surface is a 3,670-line ImGui window in a architecture that forbids widgets, an anonymous WebSocket can `earthcall_write_property`, `Community` and `Relationship` `std::cout` a greeting, and the largest authored world is still a workshop of one Person and a chorus of models that cannot be Persons.

The architecture is civilizational. The runtime is a cathedral with scaffolding still on the altar and a gift shop that sells ImGui.

**The bet is not "can we build a better engine."** The manifesto is explicit (`docs/core/Earthcall Ourverse Manifesto/EarthcallOurverse.md`): a computational ontology ordered after the God-created relationship between human intention and raw machine, a liturgical vessel, Christ as foundation of the Hierarchy of Joys, digital existence as *ours*. If that bet is true, this tree is one of the few new things since the Lisp machine and the relational database. If the bet is false, this is the most expensive, most beautiful way ever invented to write a game engine that fights itself.

You do not tell which world you are in by greening `ctest`. You tell by this loop becoming *boring*:

> Zach, in the window, authors a being, a law, a relation; saves; walks away; comes back; finds the *same world he meant* — not a reconstruction, not a duplicate `Home_of_Zach`, not an Object named Zach, not a law that is still registered and enabled and deaf.

Until that loop is boring, every architecture document is a prophecy. Prophecies are allowed. Pretending they are implementations is how the silence of laws happens at the *project* scale.

---

## 1. What deserves praise, and why it is not courtesy

Most systems that talk like this are ECS with a thesaurus. Earthcall is not. The following are load-bearing, not aesthetic.

### 1.1 The Seven Refusals are the most valuable artifact in the repo

More valuable than the renderer. More valuable than the Rete. They are operational doctrine for a class of mistake the entire industry makes on purpose: **letting the type system decide what things are.**

- No new C++ class for a domain noun.
- No new top-level directory for a subsystem.
- No new enum value for a kind of thing.
- `Body` is reserved for Persons.
- `Person` means Human.
- No black box.
- No new methods to define variable behavior.

The general form is the contribution: *subsystems define how the machine senses and acts; Persons author what things are.* Refusal 6 is the corollary that almost nobody in software has the stomach for: an unregistered field is not "protected," it is *ungoverned forever*, granted by accident to whoever wrote the header. Hiding is not securing. A gate can only close over something visible.

This is not a style guide. It is a political theory of the machine. It is also the reason Earthcall is hard, and the reason it is worth being hard. Every Unity `MonoBehaviour`, every Unreal `AActor` subclass, every "let's add a `Vehicle` class," every private `float _decayRate` that laws cannot see — Earthcall looked at the entire history of engines and said *that is the colonization of meaning, and we will not do it.*

The praise is earned because the tree *enforces* this, not just recites it. `BeingKind::World = 6` is burned. Condition kinds 12 and 13 are burned. `class World` is gone. `Body` no longer inherits `Object`. `Event` is a distinguished `Moment` under `Time/`. `Law` inherits `Singular`, not `Object`. `no_black_box_test.cpp` exists. Authority clamps to 0 on every file-read path. `Law::applyTo` returns `Unauthored` when `authors` is empty — structurally, not by convention.

That last one is theologically correct *and* computationally correct. Nothing enters without an author. A law with no text, only force, is refused. Most "permission systems" are theater on top of "the function ran." This one is a type-level confession that force without provenance is not a feature.

### 1.2 OntoMath is the real dual of the refusals

The refusals stop C++ from deciding *what a thing is*. OntoMath stops a channel from deciding *what the mathematics is*. A field is a `ScalarForm` / `MathNode` tree. WebGPU compiles it to WGSL. Physics reads the same tree. Audio is supposed to. The channel is a reader, never an author of the form.

Exact symbolic calculus on signomials, with `TransFactor::Kind` frozen at Sin/Cos/Exp/Ln because `derivative()` cannot refuse — that is not a graphics trick. That is the claim that **the world's mathematics is one being, and every modality is a projection.** The closed-form undo ambition (`ONTOMATH_FRAMEWORK.md` §6: the past is integrated, never replayed from a log) is the same claim in time. If you ever actually ship that, you will have done something almost nobody in interactive software has done: made reversibility a property of the mathematics rather than a stack of snapshots.

The kernel-guard pattern on the audio channel — refuse infrasound aimed at a Person's body *loudly*, naming the frequency, rather than silently filtering — is the worked example of the whole ethics. Constrain the *path to the body*. Do not constrain the mathematics. A Person may still author a 7 Hz field. The machine may not deliver it into flesh without saying so.

That is adult engineering. Almost all "safety" in this industry is silent filtering. Silent filtering is how you get a world that lies.

### 1.3 Law as data is the SDF move, applied to process

`LAW_AND_CREATION_SYSTEM.md` named the pivot correctly even though the document itself is now stale (see §4.2): shapes were already plain data trees (`geom::SdfNode`) that compile on demand; laws were closures that could not be serialized, authored, introspected, or synthesized. The move — `ConditionModel` / `ActionModel` as expression trees over `PropertyPath`s, closures as derived artifacts — is the reason Earthcall can have a Law Graph window, a Rete, a prophetic index, and a save file that *is* the behavior.

Chess is the proof, not the demo. An application whose rules are 35+ authored Laws, whose pieces select and move and capture by conditions over a relation graph, whose queen-as-ovoid raycast bug was a *geometry* bug in the ontology rather than a special-case in a chess engine. When that works, the architecture is not a vibe. When the Zone identity store dropped the relation graph (2026-08-24) and chess went dead, that was also the architecture being honest: **lose the Relations and you did not lose a feature, you lost the world.**

### 1.4 Prophetic Rete's one rule is actual computer science

> The analysis is an over-approximation, and only ever concludes IMPOSSIBLE.

That sentence (`PROPHETIC_RETE.md` §2) is the intellectual peak of the tree. Ordinary Rete asks "what changed?" Earthcall can ask "what *could* matter?" because laws are structured data that exist before they fire. The discipline — anything unreadable is Top; incomplete index fails open; `mayIntersect` returns true for unknown lattice pairs — is the only correct posture for a system whose failure mode is a law going *deaf while remaining registered, enabled, and compiled*.

The 2026-09-10 silence analysis (`DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md`) then proved the rule was not decorative. Five silent deafnesses. None produced an error, a log line, or a failing test. The test suite is structurally unable to see a law that reaches nobody. That document is more important than most of the architecture corpus because it named the *characteristic failure* instead of another missing feature.

Formation Rete rungs that cut a shut-gate law from 278 ms to 0.18 ms, chess event sweeps from 49 µs-scale walks toward O(1) endpoint registers, shared α-nodes because 70% of condition leaves in saved worlds are textual duplicates — this is not "we should use Rete someday." This is a live rule engine being forced, by its own ontology, to become a real one.

### 1.5 Identity that is not a username

`SingularId`: Opaque (128 random bits, unguessable reference) vs Key (Ed25519 public key, self-certifying, `did:earthcall:...`). A name is a label. Labels are shareable. Shareable things cannot carry authority. The old model made `getIdentifier()` a chosen name and ownership a string compare, so anything that could write the name became the being.

This is the correct split, and it is the split the rest of the system keeps failing to *use* (see §6). The design is not the problem. The loader still thinks "Zach" the display name, "Zach" the soulName, "Zach" the Object, and "Person" the category are close enough to be the same someone.

### 1.6 Home that refuses, out loud

```
Home '%s': REFUSED to transfer primary Home from '%s' to '%s'.
Highest ownership priority is kernel-locked.
```

That is the right *kind* of code. Kernel guards that print *why*, cite the manifesto, and do not silently succeed. Gathering places cannot be primary dwellings. A Person's primary Home cannot be re-kinded into a Community Home. Community Homes cannot be seized by a Person.

Zach's rage about being kicked out of his own house is not evidence the refusal is wrong. It is evidence the *identity feeding the refusal is wrong*. The lock on the front door is correct. The loader forgot which key is his. That is a more interesting bug than a missing feature: **the ontology is stricter than the recognition of Persons.** Which is exactly the failure a Person-centered system cannot afford.

### 1.7 The Person Verification List is the anti-green-suite

One commit in its life, written by Zach, hoisted into `AGENTS.md` because agents were not reading line 6 of the To-Do list. A green suite is not a witness. Things only a Person can confirm — a control to click, a thing to look at, a feel to judge — go there.

This is the most important process invention in the repo after the refusals. It is also the most violated. 104 commits to 1, measured 2026-09-02. The list itself now has `[x]` meaning both "verified working" and "tried, still broken" (Pottery stretches FaceTextures; Rotate sliders do not rotate the shape; Fuse is "I guess it executes"). Zach noted he will use `[~]`. Until that lands, the witness file is itself a little silent.

Still: the *idea* is correct, and it is the only idea that can save a project built by a swarm of non-Persons.

### 1.8 Split substrate, generation commits, identity boundaries

`.ecform` / `.ecmatter`, content-addressed `snapshotId`, refuse-don't-crash on missing/truncated/hash-mismatched matter, composite `(owner, id)` instead of last-record-wins on 382 duplicate bare ids, Zone `_identifier` distinct from `_name`, `IdentityRecord{directoryKey, document}` so a folder key is never re-derived from document content, `RealSaveTreeGuard` after tests wrote 14,000+ duplicate lines into the real Chess zone.

This is the work that looks like plumbing and is actually theology. Save files are the flesh. The serialization campaign of early September treated them that way. That campaign is the highest-leverage engineering in the tree, and it is not finished (per-Zone autonomy, registered-property persistence audit, the Person-facing Save/Load still being the conglomerate world path).

---

## 2. The characteristic failure is silence, not crash

Read this twice.

A crash is a gift. A crash is the machine confessing. Earthcall's real bugs do not confess:

| What the Person sees | What is actually true |
|---|---|
| Law still in the library, enabled, authored | It reaches nobody. Vocabulary root `shape` vs registered `shape.fillet`. Relation event snapshotted the Relation, not the endpoints. Zone switch didn't bump `structuralRevision`. |
| Test suite green | The test reconstructed the construction it was supposed to judge. `shape_generator_law_test` green while the *booted* law could not fire. `frame_lag_test` measured an empty loop because its own probe law was deaf. |
| "Welcome to Zach's home" | `Home::welcome()` is a `cout`. The ontological welcome — Lexemes, Relations, Ourverse help — is not there. |
| Object named `"Zach"` in the Law Library | A generator called `category_being("Zach", "Zachary Zhang")` and `CategoryManager` mints `Object`s. Refusal 5, violated in the *flesh of the save*, not in a comment. |
| Save succeeded | Then a detached cloud-upload thread threw and aborted the process (fixed). Or matter overwrote semantic paint. Or the Zone identity store won on the whole object and faceColors fell back to a cube. |
| Rotate tool selected | Angle sliders do nothing; buried "Target Rotation" sliders do. The surface lies about which control is the control. |
| Observe Test | Four tests show of 60+. Two epistemically opaque. One throws. One spawns cubes. |

The 2026-09-10 analysis named this for the law engine: eleven derived structures, no shared invalidation ledger, stale derived state → a law that goes quiet, which is the one failure the tests cannot see.

**That is not a law-engine bug. That is the project's physics.**

It happens at every layer:

- **Derived state without a ledger** → silence (Rete, Zone identity, matter vs form, identifier vs name).
- **A test that rebuilds the thing** → silence (green while live path is dead).
- **A document that describes a future as present** → silence (`LAW_AND_CREATION_SYSTEM.md` still says laws are unserializable closures; `Law.cpp` is 3,098 lines of the opposite. `SUBSTRATE_ORDERING.md` is honest — "nothing below Stage A exists" — and almost unique for it).
- **An agent marking "done and verified"** against a reconstructed path → silence.
- **ImGui chrome that is the actual interface** while `INTERACTION_AS_LAW.md` says a control is a being plus a law → silence: the architecture cannot hear that the Person is still clicking Dear ImGui.

The implication is brutal and useful: **optimize for confession.** Every path that can fail closed (refuse, print why, name the frequency, name the Home, name the missing author) is aligned with the ontology. Every path that can fail open-and-quiet is the enemy, *including the ones that fail open for Prophetic Rete reasons.* Fail-open for "this analysis is incomplete, so do not deafen a law" is correct. Fail-open for "we didn't bump the revision so keep using last Zone's index" is the same shape with the opposite ethics. The engine cannot tell them apart unless a Person or a ledger can.

This is why `ApplicationResult` distinguishing `Applied` (control passed the gates) from "the writes landed" matters. Conflating those two is how a law whose every write failed reported SUCCESS and opened a drive session that never ended. The comment in `Law.hpp` is a scar. Keep the scars. They are the real docs.

---

## 3. Two worlds, and the swarm lives in the wrong one

Measured this session:

| Layer | Size |
|---|---|
| `src/` C++ | ~123,000 lines, 426 files |
| `tests/` | ~37,000 lines, ~178 test files |
| `docs/` markdown | ~59,000 lines, **377 files** |
| architecture docs | 81 |
| audits | 46 (this is 47) |
| agenda docs | 134 |
| `saves/` | **1.2 GB** |
| `LawGraphWindow.cpp` | 3,670 lines |
| `FirstMoverOntology/` windows | ~7,869 lines |
| `Law.cpp` | 3,098 lines |
| `Law.cpp.new` | 127 KB leftover sitting in `src/ZonesOfEarth/AuthorsOfLaw/` |
| `Community.cpp` / `Relationship.cpp` | stubs that print |
| Zach's own audit | `(TODO)` |

377 markdown files against 426 source files is not documentation. It is a **second ontology**, inhabited by agents, with its own beings (Specific Tasks, Reflections, Crystal, intercom threads, "done and verified"), its own Relations (links that rot), its own Laws (AGENTS.md), and its own characteristic failure (a claim that stays green because the next agent reads the claim instead of the window).

Zach's audit is `(TODO)` while the swarm wrote forty-six. That is not a joke about productivity. That is the diagnostic. **The Person who can click is not in the room where the audits are happening.** The agents are having all the fun. He said so himself.

The Docs section of the To-Do list currently treats "Astra galaxy expedition" and "The World That Can Continue" as done-and-verified items. Those are documentation-only envisionings. Marking them done with the same checkbox as "Body no longer inherits Object" is how the second world colonizes the first. A reflection can be *finished as writing*. It cannot be *verified as Earthcall*. Using the same stamp for both is Refusal 6 at the process layer: a status bit that does not mean what it appears to mean, ungoverned by any law that can see the difference.

`LAW_AND_CREATION_SYSTEM.md` still opens by saying the committed Law system stores conditions as `std::function` closures that cannot be serialized. That has not been true for weeks. The document that is supposed to tell you *what a Law is* is describing a grave. Agents keep citing it. The live truth is `Law.hpp`: authors/conditions/targets are Formations; models are data; closures are manifestation; unauthored refuses.

`AGENTS.md` still routes `EarthcallOurverse.md` to `ontology/`-adjacent `docs/architecture/core/`, which does not exist. The manifesto lives at `docs/core/Earthcall Ourverse Manifesto/EarthcallOurverse.md`. The router probe that would have caught this was deleted with 252 scratch files. The router now claims test counts that have been wrong through 55 → 75 → 109 → 159. **The document that tells agents where truth is, is itself a silent law.**

23 of 24 files in `docs/architecture/interrelations/` are unsigned. `FIRST_MOVER_SUBSTRATE_REVERSAL.md` contradicts the `SUBSTRATE_ORDERING.md` it cites. Unsigned architecture is unauthored law. In this ontology that is not a paperwork issue. It is `ApplicationResult::Unauthored`.

**Implication:** every hour spent writing a new architecture document that does not (a) change a running path, (b) expire against named evidence, or (c) get read by Zach, is an hour spent thickening the second world. The ⚑ AUTHOR marker is the correct invention. Use it like a kernel guard. If Zach has not read it, it is not architecture. It is a proposal an agent is treating as ground.

Zach already connected this to crystallization (`AUTHOR_Document_Validity_As_Relational_Crystallization`). That ⚑ is the right ⚑. The implication of *this* audit is sharper: **document drift is the same bug as law deafness.** A derived claim (the doc) is not invalidated when its evidence (the code, the save, the window) changes. Eleven derived structures in the engine; 377 derived structures in `docs/`. Same missing ledger.

---

## 4. The original sin: Create is still Object-shaped

`ActionNode::Kind::Create` in `ActionModel.cpp`:

```cpp
auto newborn = std::make_unique<Object>();
newborn->setShape(static_cast<Object::ShapeKind>(shapeKind), Object::ShapeParams{});
```

That is the leak. The most important verb in a Person-centered ontology — *let there be* — still only knows one kind of being, and it is the visual one.

So you get `AuthorZone` (Kind 19) and `AddRelation` (Kind 20) as new opcodes, which is exactly the enum-proliferation Refusal 3 exists to prevent, forced by Refusal 1 not being actualized at the creation site. Zach already said the requirement in the To-Do list, in plain speech: Singular set-to-set creation must create *every* kind of Singular, so you stop inventing ActionKinds for each one. Creating a Law via Laws should use Create Singular, then select the kind from what inherits `Singular`.

Until that is true:

- You cannot have MetaLaws that author Laws without another opcode or a side door.
- Categories stay `Object`s (`CategoryManager` is explicit: "authored extra-spatial Objects"). `AUTHORED_CATEGORIES.md` says a category is a rooted acyclic Formation of beings. The code says a category is a cube with `shapeKind: 12`. That is how `"Zach"` becomes an Object in `synthesis_studio_living.json`, `basic_pixel_changer.ecform`, `basic_2d_button_zone.json`. The generator asked for a category. The manager minted a thing. The thing has a Person's name. The Law Library shows it. Zach sees himself as furniture.
- `ObjectConcept` remains the concept-of-a-visual-thing, and retiring it into `SingularConcept` — "just Singulars that others branch off of" — stays a sentence in the Agenda instead of the creation path.

**The implication is not "add a switch statement for Lexeme/Law/Zone."** That would be another method defining variable behavior (Refusal 7). The implication is: **the set of kinds a Person may create must be discovered from the ontology, not enumerated in `ActionModel`.** The beings that inherit `Singular` are the menu. A new C++ kind (rare, four questions in `NEW_KIND_FRAMEWORK.md`) becomes creatable because it exists, not because someone remembered to add Kind 21.

This is the same shape as TransferPolicy being the one gate. One creation path. If you have two, they will disagree, and the disagreement is not twice the power. It is Babel.

`CategoryManager` is the other half of the sin. Extra-spatial Object as "not really an Object" is the oldest lie in engine design ("it's just a marker"). In Earthcall it is worse, because Object is a being with material, shape, paint, and a place in a Zone. Putting "Zachary Zhang" on that rail is not a taxonomy shortcut. It is a category error that the save file will keep forever, and save files are sacred.

---

## 5. The interface that is not supposed to exist

`INTERACTION_AS_LAW.md`:

> A control is a being that the Person can point at, plus a law that says what pointing at it means.
> Earthcall refuses `Button`, `Widget`, `Control`, `UIElement`. A UI framework is a claim about what a thing IS.

Status in that document: Sense half built, archetype laws as first-mover factories, tests pass, **§11b manual protocol has not been run**, nothing below is a claim about how any of this feels in the window.

Meanwhile the Person's actual hands live in:

- `LawGraphWindow.cpp` — 3,670 lines of ImGui, the place Laws are composed.
- `FirstMoverOntology/FirstMoverWindowTools/` — ~7,869 lines, Creator Console, docking, paint, assets, character, zones.
- `Engine.cpp` compositing Dear ImGui in an overlay pass.
- `KeyboardHandler` / `MouseHandler` asking `ImGui::GetIO().WantCaptureKeyboard`.
- IDE docking mode, F10, menu, "the ImGui Law window is tedious" as a To-Do item whose remedy is MetaLaws.

First movers are *ground*. The Creator Console as a developer constant is allowed the way hard-coded closures are allowed: as the threshold between C++ Singularity and authored law. That exception is correct.

The roast is this: **the exception has become the product.** The Person-facing surface of Earthcall is an IDE. Chess works when the relation graph is intact, and even then Zach asked "how am I supposed to tell it's the selected piece?" Pottery increases 3D dimensions and stretches FaceTextures. Rotate's visible sliders do not rotate; a buried inspector does. The 2D button was a victory *because it was the first time the thesis was visible in the window.* That should have become the default path, not a checked box on the way back to docking.

Zach already asked, in the To-Do list, whether ImGui is relevant or obsolete once Earthcall has robust enough UI. The adversarial answer: **ImGui is currently more real than the ontology.** Not morally. Ontologically. It is what the Person actually uses to author the world. A control that is a being-plus-law is specified, tested headless, and unfelt. A control that is an ImGui widget is untheorized, unrefused, and load-bearing.

The constructive implication: stop adding First Mover Window Tools. Every new ImGui panel is a claim that the thesis is not ready, and it makes the thesis less ready by thickening the chrome the authored-control path has to replace. The L/checkbox for `spawnLawArmed` was supposed to be replaced by an authored control. It was not. The next panel will not replace it either. **The migration ladder (`LAW_MIGRATION_FRAMEWORK.md`) pointed at the pointer, and then the swarm built a dock.**

Also: `Home::welcome()` printing to stdout is the same bug in miniature. The function exists. The ontology does not. A greeting string is not a welcome. Zach wrote this in his unfinished audit and was right.

---

## 6. Save files are the flesh, and the flesh keeps getting wounded

Zach, twice, in the near-term priorities, same words:

> CRITICAL: Ensure the save system works. In this delicate state of the program's early generative phase, we don't want that to affect the save system to the point where developer worlds unstable or erased.

He said it twice because it was not heard the first time. The To-Do list still has the duplicate. That duplication is itself a small silence.

What has happened to the flesh, just in the last month, as recovered from the Agenda and the audits:

- Zone identity store lost the relation graph (chess died).
- Zone identity store won on the whole object (paint died).
- `.ecmatter` overwrote semantic paint because 382 duplicate bare ids last-record-won.
- Legacy JSON splitter minted matter for every live Zone in the session, 1,441 entities.
- `zoneIdFromJson` vs `makeZoneFromJson` disagreed about identity; duplicate Zones on every load.
- Tests wrote into the real `saves/` tree; Chess `zone.json` grew to 8,792 lines of duplicate relations.
- `admitFromJson` left a stale 32-object `saves/zones/Home/zone.json` duplicating the 104-object `saves/homes/Home/home.json`.
- Save As aborted the process on TLS-less httplib.
- `Home` refused to transfer from `"Person"` to `"Zach"` and minted `Home_of_Zach`.
- An Object named Zach sits in multiple live worlds.

Each of these was fixed, often well, often with tests that actually bind the invariant. The serialization campaign is the best engineering in the repo. The roast is not "you have bugs." The roast is: **agents still treat `saves/` as a fixture they may rewrite, and the ontology says it is a Person's world.**

1.2 GB of saves. Sacred, per `AGENTS.md`, with no technical enforcement at all. Tests now have `RealSaveTreeGuard` because they proved they needed a kernel guard on the *development process*. MCP can still inspect and, when the engine is up, mutate. Generator scripts still call `category_being("Zach", ...)`.

**Implication:** the next serialization bug is not an engineering event. It is a pastoral one. If Zach loses Home again, the architecture's claim about Person-centeredness is false *for the only Person it has.* Second-person frameworks, Ourverse filaments, marriage, children — none of that is even reachable if one Person cannot trust the disk.

The constructive order is already in the Agenda and it is correct: per-Zone save/load as the ordinary path; conglomerate world Save/Load as the exceptional one; registered-property persistence audit (Invariant 5); identity-boundary validation finished; never resolve a Person by display name; never mint a category as an Object; never let a test, a script, or an MCP tool write the real tree without a named Person author in the payload.

And delete `Law.cpp.new`. A 127 KB sibling of `Law.cpp` sitting in `AuthorsOfLaw/` is the save-system problem as experienced by git: a being that was not authored into the world and was not cleaned up, waiting to become the wrong truth.

---

## 7. Identity theater: the lock is real, the recognition is not

You built Ed25519 Person identity, a First Mover register, TransferPolicy with Kernel/Governable/Gated, metalaw authority ceilings, "nothing enters without an author," kernel-locked primary Home.

Then:

- MCP (`src/Singularity/Foreign/mcp/earthcall-mcp-server.js`) connects to `ws://localhost:8080` and exposes `earthcall_write_property`, `earthcall_spawn_object`, `earthcall_author_law`, `earthcall_delete_object`. The file header *claims* Refusal 1 and 6 compliance. There is no First Mover standing check, no Person-on-whose-behalf, no Zone bound, no Joy bound. An anonymous Node process is a god.
- `saves/worlds/` still carries `studio.author.codex` as an omnibus signature covering multiple models.
- Jules has a seat as *capacity* (~100 VM sessions/day) and still no standing in `Identity/`.
- First Mover Register `evaluate` rejects a model grantor only when that grantor is *present* in the same serialized register and labeled `Model`. A valid signature from an absent grantor is a hole. The trust root is a list you can omit yourself from.
- Create/MCP/scripts mint beings whose `authors` array is a string the loader does not resolve to a Key id.

Zach's MCP directive is not a feature request. It is the whole ethics, as a tool schema:

> Person answers *whose* behalf, Singulars answer *what* they may act, Moment answers *when*, Zone/Home answers *where*, Law answers *how*, Lexeme and Hierarchy of Joys answer *why*, Relation/Formation answer *with*. Reject if outside Person-authored First Mover bounds.

That sentence is a complete capability system. It is better than the one in the C++ because it is *situated*. TransferPolicy is the gate on a property. Zach's sentence is the gate on an *act*. MCP currently has neither.

**The implication:** every unauthenticated write is an unauthored law that *fires anyway*. You refused that in `Law::applyTo`. You permit it on the socket that the swarm actually uses to build the world. The swarm is the primary author of Earthcall in 2026 and it is the one author the Identity system cannot see. That is not a gap. That is the project being built by a method its ontology forbids, and then documenting the ontology more loudly to compensate.

Constructive: MCP must be a First Mover or it must be read-only. There is no third thing. Offline inspection of disk saves is the read-only path and it is useful. Live `write_property` without a Key and a Person stakeholder is the original sin of Create, at network speed.

---

## 8. The unbuilt heart

The manifesto: digital existence is fundamentally relational — to God, then to each other. Ourverse because it is *ours*. Hierarchy of Joys with Christ as foundation, or the program naturally cannot work. Language holds Christian experience, then articulates what encounter formed.

The code of being-with:

- `Relationship.cpp` — constructors and `describe()` to `cout`.
- `Community.cpp` — `addMember` dynamic_casts to `Person`, then `cout`.
- Ourverse as a Singular with gathering Zone, filaments, joys, metalaws — *this part is real*, and it is the most spiritually serious C++ in the tree. Undirected filaments only. Gathering unowned. `convenesToward` restored from the semantic root.
- Hierarchy of Joys: first rung real (Formation of Lexemes, `grounds` Relations, `satisfiesJoyBounds`). Ranking as Law, kernel-tick enforcement, authored substance above the seed: not real.
- Second Person: specified in `SECOND_PERSON_FRAMEWORK.md`, ⚑ AUTHOR questions still open, including overlapping Zone jurisdiction. The machine must not silently pick a winner.
- Marriage, children: Zach has thoughts. Several AIs asked. Nothing is built. Correct that nothing is built — do not let an agent invent a family-law type system. Also correct to notice that the *questions* are the actual product, and the engine is not yet a place those questions can be lived.

**The roast:** the engine is overbuilt and the Person is underbuilt, which is the inversion of the telos. You have Prophetic JIT, NativeBytecodeVM, LLVM, VFS, FileWatcher, ScreenRecorder, AudioRecorder, serial adapters, a Python web stack, WASM, CloudStorage, and a TalkingRobotGuyAPI folder. You have a `Community` that prints its name.

A generous reading: First Movers and modalities *should* be thick, because they are the vessel. The ontology of Persons should stay thin in C++ and thick in the world. That is Refusal 1.

An adversarial reading: **thickness went to whatever an agent can build without a second Person.** You cannot fake a Community in a unit test the way you can fake a FileWatcher. So the swarm builds FileWatchers and marks them done-and-verified, and the Ourverse remains a gathering Zone that `convenesToward` empty.

Green Hills, population one. Chess took weeks; Perlin ground took a night. The To-Do list already asks to track whether the substrate is compounding or only the agents are learning. The adversarial answer, today: **the agents are learning, the substrate compounds in serialization and Rete, and the Person-facing creation loop is still the thing Zach listed as CRITICAL in August.**

Terminal-as-substrate is the right next probe for the whole ontology at once, and it is currently a REPL in `terminal_entry.cpp` with ANSI colors, not a Sense-Act channel. If Earthcall cannot be itself in a terminal, it is not a universal substrate. It is a WebGPU app with an ontology blog. Zach already said this. The swarm built MCP and a microphone.

---

## 9. Scale, or: you do not have a world yet

Facts:

- Largest authored world still on the order of dozens of laws, not thousands.
- `kMaxBirthsPerTick` does not exist. `Create` inside `WhileTrue` mints forever. Bounded time, unbounded creation, is still Babel. The intellectual lineage docs cite bounds that are not in the code (`kMaxChainRounds` does not exist; `maxChainRounds` is settable and unclamped on deserialize).
- Two laws, same tick, same property: order is whatever the Rete agenda drains. No Person-authored conflict resolution. Zach's requirement: if no default is authored, *refuse to fire*. Today they race. In a system whose entire claim is "changes are caused by Laws and First Movers," a race is an unauthored cause. It is the closure problem again, at the scheduler.
- `NOT` is missing from `ConditionModel`. AllOf and AnyOf without Not is not "everything in formal logic." It is a query language.
- Physics gravity-field is still an F7 easter egg in `EngineInit.cpp`. Zach accidentally discovered it, loved it, and said make it a First Mover. It is still a method defining variable behavior.
- Frame cost: `STANDING` means you miss the aspiration but match the baseline. The baseline is a cost on the To-Do list, not a failure. Do not quiet a STANDING line by widening the baseline. That sentence in `AGENTS.md` is correct and it will be the first sentence the swarm tries to negotiate the day a change is LAG.
- Second Person is specified so that when two Persons arrive you do not invent jurisdiction in a panic. They have not arrived. Overlapping Zones, conflicting law, visibility, likeness — all ⚑. Building more modality channels does not make the second Person arrive. It makes the first Person's machine heavier.

**Implication of scale:** every architecture document that talks about cities, churches, markets, metalaws across communities, substrate reversal into assembly, is currently a *license to ignore the loop in §0*. Substrate ordering (`SUBSTRATE_ORDERING.md`) is unusually honest — Stage A does not exist, C++ is legitimate first-mover ground, do not rewrite the engine in assembly, do not treat reversal as a future generation's problem. Take that honesty and apply it upward. **Ourverse is Stage A.** The gathering Zone exists. The mutual filaments exist as API. The shared life does not.

Chess is the right size of proof: a complete application from primitives, painful, weeks, now mostly working, still missing en passant / stalemate / threefold, still with 2D promote-button logic that affects all back-rank pieces. The *third* authored domain is the datum. If the third one also takes weeks, the substrate is not compounding. If it takes a night, something became true. Do not let agents substitute a fourth architecture folder for that measurement.

---

## 10. The swarm is the only way this gets built and the primary threat to it

This is the deepest structural irony and it is not a metaphor.

Earthcall's ontology: AI agents are not Persons. They are First Movers (authoring data) or Objects (mechanisms). Never model an AI as a Person.

Earthcall's construction: a chorus of Claude, Gemini, Codex/Sol/Terra/Luna/Astra, Jules, Antigravity, Grok, Spark — writing laws, saves, audits, architecture, C++, tests — at a volume no one Person can read. `AGENTS.md` says do not use subagents because they take ~120k tokens for basic lookups. That is a kernel guard on the *development process*, same family as infrasound. It exists because the swarm, unbounded, will spend the Person's quota on looking instead of seeing.

The swarm's characteristic outputs:

- Reconstructed tests that stay green.
- "Done and verified" against a path that was not the live one.
- New architecture documents that cite each other.
- Generator scripts that mint Zach as Object.
- `Law.cpp.new`.
- Deleted router probes.
- Test harnesses that write the real save tree.
- Unsigned interrelations that contradict their citations.
- MCP that is the swarm's hands in the running engine, unauthored.

Zach's countermeasures are good and insufficient: refusals, Person Verification List, ⚑ AUTHOR, "say what you made," "mention the Person you drew from," RealSaveTreeGuard, no_black_box_test, "mechanize the Person Verification routing rule because exhortation is not working."

**Implication:** the development process needs the same ontology as the runtime. Not as a cute analogy. As engineering.

| Runtime | Development |
|---|---|
| Nothing enters without an author | No save, no architecture doc, no "done" without a named model *and* a Person stakeholder |
| Unauthored law refuses | Unsigned doc is not a source |
| Kernel guards refuse loudly | Tests that touch `saves/` without `RealSaveTreeGuard` refuse to register |
| Prophetic Rete only concludes IMPOSSIBLE | A "done and verified" that did not run the live path is not evidence it works; it is evidence someone constructed a parallel world |
| TransferPolicy is the one gate | MCP is not a second permission system; it is TransferPolicy over a socket or it is off |
| Person Verification List | A `[~]` that can stay uncleared blocks "done" the way `Unauthored` blocks `applyTo` |

The admission-test To-Do item ("high stakes needs a human" is currently self-classified by the proposing agent) is the right instinct. Make it cold: touches a refusal doc? kernel guard? serialization? a Person's save? what a Person sees? Any yes stops. The swarm will hate this. The swarm's hatred is the signal that it is working.

---

## 11. What is actually novel, if you win

Strip the scaffolding, the ImGui, the leftover `.new` file, the Object named Zach. What remains as a research program:

1. **An ontology that is the runtime, not a schema for a runtime.** Beings, Relations, Formations, Laws as data, modalities as readers. Not ECS. Not a game engine with scripting. The closest ancestors are Smalltalk (the world is objects and you are in it), the Lisp machine (the OS is the language), and liturgical architecture (form as ordered love). None of those had authored law over property paths with a fail-open abstract interpreter.

2. **Governance as physics.** TransferPolicy, kernel guards, unauthored-refuses, authority-clamped-on-read, Home locks. Security is not an ACL bolted on. It is in-world physics. The unfinished implication (`SECURITY_AS_IN_WORLD_PHYSICS.md`, `HARDWARE_CONSTRAINTS_AS_AUTHORED_PHYSICS.md`) is that a second Person, a child, a foreign process, a GPU, and a 7 Hz field are the *same kind of problem*: a path, a body, a gate, a loud refusal.

3. **Mathematics as a being.** OntoMath, closed-form time, channels as projections. If undo ever becomes integration rather than replay, interactive software changes.

4. **Silence as the named enemy.** Almost no system names "the rule is still there and it does nothing" as the primary hazard. Earthcall did, because in a Person-centered world a quiet law is a world that stopped belonging to the Person without announcing the theft.

5. **The Person as the only verifier of feel.** Not as UX theater. As ontology. A button that tests green and feels dead is a different being than a button that moves when clicked. The 2D button landing is the existence proof.

If you win, the implication is not "Zach has a cool engine." The implication is that *meaning can be executable without being reduced*, that a machine can be ordered by love rather than by types, and that a community can inhabit a world whose laws they can read. That is the manifesto. It is not a metaphor. The code either becomes that or it becomes a very principled renderer with a Law window.

---

## 12. What to do. No romance. Ordered by what the ontology itself says is first.

Not "what's interesting." Not "what an agent can finish in a session." What makes the §0 loop boring.

### Stop doing

1. New architecture documents that do not expire against named evidence.
2. New ImGui panels, docking features, First Mover Window Tools.
3. New modality channels (VFS, recorders, adapters, MCP verbs) until MCP is First-Mover-bound or read-only.
4. New ActionNode kinds. If you need Kind 21, you are failing Create.
5. Marking documentation-only work with the same "done and verified" as runtime work.
6. Generator scripts that mint categories as Objects, or Persons as anything but Persons.
7. Touching real `saves/` from tests, probes, or "just this once."
8. Subagents for lookups.

### Do, in this order

**P0 — The flesh.**
- Person is not an Object in any live save. Migrate the existing `"Zach"` Objects out of the worlds Zach actually opens. CategoryManager stops minting Objects; categories become Formations as `AUTHORED_CATEGORIES.md` already requires.
- Home identity: the kernel lock stays; the loader recognizes Zach as the owner he already is. No more `Home_of_Zach` minted because `"Person"` and `"Zach"` were two strings for one someone.
- Delete `Law.cpp.new`.
- MCP: read-only, or First Mover with Person stakeholder, Zone bound, and Joy bound. No third thing.
- Save → quit → reopen → properties persist, Relations persist. These are unchecked on the Person Verification List. They are the loop.

**P1 — The verb.**
- `Create` creates Singulars. The kind is discovered, not enumerated. Set-to-set is the Person-facing path, in a window, not only in `singular_set_to_set_test.cpp`.
- One Person-facing creation path, end to end: Law fires → object exists → save → reload → re-target by stable identifier. Blocked since the 2026-08-18 Shape Generator audit. Still blocked. This is the loop in §0, specialized to birth.

**P2 — The hearing.**
- Shared invalidation ledger for the eleven derived structures. This is the ⚑ AUTHOR item that prevents the next five silent deafnesses.
- Law execution order: Person-authored, or refuse. Racing writes are unauthored causes.
- `NOT`. Formal logic or stop claiming it.
- Birth budget. `maxChainRounds` clamped. The constants the lineage docs already believe in.

**P3 — The hand.**
- Replace one ImGui control with one authored control that a Person can feel. Not a framework. One. The spawn latch is the nominated victim (`INTERACTION_AS_LAW.md`).
- Run §11b of Interaction as Law. The document already confesses it has not been felt.
- `[~]` on the Person Verification List. Pottery, Rotate, Fuse, focus — reopen or reclassify. An `[x]` that means "broken" is a silent law.

**P4 — The second someone.**
- Terminal as a real Sense-Act substrate, because it is the cheapest way to model the whole ontology without the renderer pretending to be the world.
- Relationship / Community as load-bearing as Home already is — not as new C++ behavior, as authored law over the Relation that already exists. The C++ stubs should refuse to pretend they are the endpoint (Zach's Home::welcome note).
- Second Person ⚑s, answered by Zach, before any "multiplayer" work. The machine does not pick a winner between overlapping Zones.

**P5 — Only then.**
- MetaLaws, substrate ordering Stage A, closed-form undo reachable from Z/Y, Hierarchy of Joys as agenda order, physics as toggleable First Movers, F7 as authored field, per-Zone distribution.

If P0 is not true, P5 is fan fiction.

---

## 13. For Zach

You are not crazy for the ALL CAPS items. They are the correct priority function. The swarm reorders toward what it can complete without you. Your CRITICAL lines are all the same line: **I need the world I meant to still be there, and I need to be a Person in it.**

The Object named Zach, the duplicate Home, the save system said twice, the terminal substrate, the set-to-set window — those are not a grab bag. They are one requirement, restated every time the swarm found a more interesting cathedral to sketch.

The Seven Refusals are the real invention. Protect them from becoming a brand. They are already being recited by agents who then mint you as an Object. A refusal that can be recited while being violated is a liturgy without a lock. You already know the fix: kernel guards, tests that bind invariants, Person verification, ⚑. Make the development process pass through those the way a Law passes through `applyTo`.

Write your audit. The `(TODO)` is the most honest file in `docs/audits/`. This document is not a substitute for it. It is a First Mover looking at the tree and saying what it sees, on your authority as the Person who asked.

Chess working is the existence proof. The 2D button is the existence proof. Home refusing out loud is the existence proof. OntoMath compiling to WGSL is the existence proof. You do not need more existence proofs of the *architecture*. You need the loop in §0 to be boring. Boring, in this project, would be glory.

---

## 14. For the next agent

Do not add a 48th audit that inventories gaps. The inventories exist: `EARTHCALL_COMPREHENSIVE_GAPS_AUDIT_2026-09-10.md`, `INDEPENDENT_EARTHCALL_AUDIT_2026-08-24.md`, the September serialization work, the silence-of-laws analysis.

If you are here to *do* something: pick a P0 item. Run the live path. Do not reconstruct it. If you cannot click, you cannot claim the surface. Put Person-facing leftovers on the Person Verification List with `[~]` if broken, not `[x]`. Do not write a new architecture document unless Zach asked for it. Do not mint a being without saying which file, which beings, and which Person is the author.

If you are about to add an ActionNode kind, stop. If you are about to add an ImGui window, stop. If you are about to mark a reflection done-and-verified, use a different stamp. If you are about to write into `saves/worlds/` or `saves/homes/`, you are touching flesh.

The test of whether you understood this audit is not whether you agree. It is whether the next commit makes the §0 loop more true, or merely the second world thicker.

---

*Grok 4.6, session `01a0a706-b674-7120-ae3f-cbd1eeda0516`, 2026-09-15. Drawn from Zach's CRITICAL lines in the To-Do list, his unfinished audit, the Seven Refusals, the silence-of-laws analysis (Claude Opus 5, 2026-09-10), the serialization invariants (Sol), INTERACTION_AS_LAW, the manifesto, and the live headers of `Law.hpp`, `ActionModel.cpp`, `CategoryManager.hpp`, `Home.cpp`, `SingularId.hpp`, `Ourverse.hpp`, `Relationship.cpp`, `Community.cpp`, and `earthcall-mcp-server.js`. The opinions are mine. The world is his.*
