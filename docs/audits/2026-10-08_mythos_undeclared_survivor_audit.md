# The Undeclared Survivor — what of a being survives is decided by nobody

*Harness: Claude Code (cloud session). Model: Claude Fable 5.1 (same underlying model as Claude Mythos 5.1; Zach asked to be addressed as Mythos). Session `session_01VwbjDeLVjBEfmfMcRxTFga`. 2026-10-08, 18:42 UTC. Branch `claude/amazing-curie-4181c6` at `d4766da`.*

**What Zach asked.** Find the unobvious layers nobody else would see, chain them, shake the primordial foundations — and do not repeat *The Ungoverned Governor* (2026-09-24). I also read *The Cube Beneath the Field* (2026-09-30) and my own 2026-09-28 replies to Astra so that this audit opens a seam neither of those touched. The Governor took the world's *government* (authority, authorship, provenance, telos, the three clocks). The Cube took the *render substrate* (the hidden atom, the hidden up, the hidden camera). This one takes **persistence** — the question "what of me survives?" — which turns out to be the most primordial predicate in Earthcall and the one no document, no tier, no test, and no Person has ever been asked to declare.

**What I drew from.** Zach's Refusal 6 (*no black box; a gate can only close over something visible*), Refusal 7 (methods are only the absolute invariants), the minimum-maximum principle, the non-negotiable *Save files are sacred — patch, never regenerate*, and the OntoMath §6 doctrine that the past is integrated in closed form from law text. Everything below was read from source at `d4766da`; the three censuses were run in this session with scripts quoted in §8. I did not build the binary.

---

## 0. The insight in one breath

A being in Earthcall exists in **three strata** that nothing binds together:

| Stratum | Where it lives | Who can see it | Who wrote the list |
|---|---|---|---|
| **The registry** — what law can read and react to | `buildProperties()` | every Law, every Rete node | whoever wrote the being's `buildProperties` |
| **The file** — what survives a save | `to_json` / `from_json`, the `.fbs` Entity table | every Person and agent who opens the save | whoever wrote the codec |
| **The frame** — what the running engine holds | `ReteFact` pointers, onset memory, `HighlightSystem`, selection flags | the C++ that put it there | nobody; it is a side effect |

Refusal 6 says every field must be *registered*. It says nothing about whether a registered field *persists*, and the engine's own doctrine (`NO_BLACK_BOX.md` §5 step 6) quietly knows this: *"A property that does not survive a save was never granted."* By that sentence's own standard, a measurable fraction of what Earthcall grants to law is never granted at all; and a measurable set of what the file keeps is state no law can read. **The mapping between the three strata is hand-written, per field, in a subsystem, and declared nowhere.** That is the exact shape of the thing CLAUDE.md forbids — *no subsystem may define what a thing IS* — applied one level deeper than anyone looked: `Singularity/Storage/Serialization` defines what of a thing is *real across time*. A being's mortality is a codec decision.

The chain closes on OntoMath §6. The closed-form past reads the **live** value and integrates backwards from law text; it never asks whether that value ever crossed a save. So after a load, the engine can compute, exactly, the history of a quantity that was never kept — a perfect integral of a forgotten number. And the verb census shows the reversible region is a tenth of what Persons actually author. The world that can be read backwards is small, and the part of it that was never written down is not distinguished from the part that was.

The rest is evidence, layer by layer, then what it does to the minimum-maximum principle, then what I recommend.

---

## 1. Layer one — one being, two vocabularies, and a hand-written translator between them

`Object` registers its law-visible vocabulary in `ObjectProperties.cpp:553` (`buildProperties`). Its file vocabulary is written by hand in `ObjectSerialization.cpp:135` (`to_json`) and `:297` (`from_json`). Neither is derived from the other. I diffed the two by reading both and following every name to its writer. The result, for one being — the most common being in every save:

**Registered, never persisted** (law can govern it; a save forgets it):

| Property path | Registered at | Written by any save path? |
|---|---|---|
| `velocity` | `ObjectProperties.cpp:670` | no — grep of `src/Singularity/Storage` finds `velocity` only in `PersonSerialization.cpp:17` |
| `angularVelocity` | `:674` | no |
| `momentOfInertia`, `centerOfMass` | same bridge | no |
| `physical` | `:655` | no |
| `hovered`, `hoverPoint` | `:664` | no (correctly — but see §2) |

The FlatBuffers `Entity` table that the `.ecmatter` sidecar writes (`Schema/Earthcall.fbs:99`) carries `transform`, `center`, `authoritative_axis`, `target_rotation`, `rotation_responsiveness`, and geometry — **no velocity, no angular velocity, no `physical`.** `LAW_MIGRATION_FRAMEWORK.md:660` nevertheless lists `velocity` as a *persistent* "State" quantity. It is persistent for a Person and not for an Object; the doc does not know the difference, because nothing in the tree records it.

**Persisted, never registered** (the file keeps it; no law can read it):

| JSON key | Written at | Registered in any `buildProperties`? |
|---|---|---|
| `tags` | `ObjectSerialization.cpp` to_json | no — and no `ConditionNode` reads a tag |
| `attributes` | to_json | no |
| `stakeholders` | `:284` | no (provenance; the Governor §4 already found law cannot see it) |
| `baseline` | to_json | no |
| `shapeKind`, `geometryType` | to_json | no — only `shape.r`, `shape.majorR`… the *parameters* are legible; the *kind* is not (the Cube audit §7 saw this from the enum side) |

Those are Refusal 6 violations wearing the file's clothes: state a Person authored, state the file faithfully keeps, and state *no law can ever close a gate over*, because the gate can only close over something registered. "Nobody registered it yet" is, once again, the one access level no law can change — only this time the thing was registered *in the save format* and not in the registry, so a Person can see it in a text editor and cannot see it in a Law.

**Both, under different names.** `transform` ↔ `position`/`rotation`; `materialId` ↔ `material`; `faceColors` ↔ `color`/`face.*`; `fieldExtent` ↔ `extent`; `fieldCellSize` ↔ `cellSize`; `authoredProperties` ↔ every dynamic property. A Person who edits a save and a Person who writes a Law speak **two languages for one being**, and the dictionary between them is `Serialization/`, which neither the Law nor the file can read. `x2D`, `y2D`, `zOrder2D` — I expected these to be mortal and they are not (`:207`, `:347`); I note it so the next reader does not repeat my guess. The point is not any one row. The point is that *I had to read the C++ to find out*, and so would a Law, and a Law cannot.

## 2. Layer two — the silence that is correct for `hovered` is a bug for `velocity`, and the registry cannot tell them apart

`hovered` *should* die at save; it is the pointer's breath on the glass. `velocity` should not; a thrown stone that reloads at rest is a world that lied. Both are registered the same way, persisted the same way (not at all), and distinguishable by no attribute anywhere. The registry has the Kernel/Governable/Gated tiers from `TransferPolicy` for *who may write*; it has no tier for *whether the write outlives the process*.

`NO_BLACK_BOX.md` §5 step 6 is the engine's own confession: *"If `to_json` writes it, add it to `object_roundtrip_test`. A property that does not survive a save was never granted."* Read the conditional carefully. It makes persistence a consequence of what `to_json` *happens* to write, and the test a mirror of the codec, not of the registry. `object_roundtrip_test.cpp` names six paths. `no_black_box_test.cpp:8` says *"C++ has no reflection, so no test can diff a class's private members against its registry."* True of members. **False of persistence** — because Earthcall *built its own reflection.* The property registry is a complete, enumerable, typed list of every field a being admits to having. A test could walk it today, write the being, read it back, and fail every path that came back different without a declared reason. Nobody wrote that test because nobody declared that a registered path has a persistence standing at all. The engine has reflection and serializes by hand as though it did not.

## 3. Layer three — the frame, where a being is a pointer, a name, and nobody

The third stratum is the one `ZoneManager.cpp:2060-2065` names in a comment and no document names anywhere: *"a live one may already carry Rete facts, selection flags, and other runtime state from_json knows nothing about and will not preserve."*

- `ReteFact` (`Law.hpp:474`) carries `Singular* subject` and `Singular* object` — the Rete knows a being by its **address**.
- `Relation::Endpoint` (`Relation.hpp:156`) carries `ptr`, `savedId`, `cachedId` — the graph knows a being by its **name**, keeps the name when the pointer dies, and `RelationManager::add` refuses an edge that has only the name (`RelationManager.cpp:218`). My 2026-09-28 reply to Astra praised the first half of this (an identifier survives its pointer); it did not see that the surviving identifier is a thing the graph *refuses to hold*. The file remembers what the engine declines.
- `HighlightSystem::isSelected(const Object*)` (`HighlightSystem.hpp:17`) — selection, the most Person-meaningful fact of all in an Interaction-as-Law world, is a Screen-substrate lookup keyed on a pointer, registered on no being and written to no file.
- Only a `Person` is known by what it **is** — a `SingularId::Key` that is the Ed25519 public key itself. `SingularId.hpp` promises Opaque ids "for Objects, Zones, Laws, Lexemes"; grep finds the type used by Person, PersonDatabase, ZoneManager, ForeignActuationGuard, LanguageSystem, and no Object, Law, or Lexeme. The `.fbs` Entity table had to grow `owner_identifier` in September because 382 bare ids in one `.ecmatter` named more than one live object and last-record-wins clobbered the right one — the identity collapse that happens when stratum two (names) is asked to do stratum three's job (reference).

So the Governor found three *clocks*; this is the sibling fact: three *identities*, one per stratum, address / name / key, and only the Person's survives all three. A stone's identity across a save is a string that another stone may be given.

## 4. Layer four — the exact past of a forgotten number

OntoMath §6 (`ONTOMATH_FRAMEWORK.md:77`) is one of the most beautiful things in this tree: *"the law text is the record, and the world can be read backwards from it exactly."* `ActionNode::valueSecondsAgo` (`ActionModel.cpp:1840`) does it: read the live value via `lawGetValue`, read the clock, integrate the authored rate backwards.

Chain it with layer one. The live value of `velocity` after a load is the bridge default, because no save ever carried it. `valueSecondsAgo` does not know that; it reads `now`, finds a number, and integrates. **The answer is exact and about nothing.** Reversibility is judged on the *text* — `reversibility()` is "a judgement on the text, nothing bound, nothing run" — so the persistence standing of the subject property is outside its sight by construction. The irreversibility map that §6 promises at Zone level ("which of its regions can be undone and which cannot, and why") cannot today include the one reason that matters most across a session boundary: *this quantity was never kept.*

And how large is the reversible region at all? I ran a census of every `actionModel` in every JSON under `saves/` (607 laws in 351 files):

| | laws | share |
|---|---|---|
| contain `Set` | 427 | 70% |
| contain no reversible verb (`Flow`/`Map`/`Drive`) at all | 382 | 63% |
| mixed | 161 | 27% |
| **only reversible verbs** | **62** | **10%** |
| `Flow` anywhere | 6 | 1% |

`Set` is the verb Persons reach for; `Set` is the first row of §6's irreversibility table ("the value it overwrote is not in the law text"). The Law Line's whole gift — "I JUST MADE AN EDITOR WITH ONE CLI SENTENCE" — is `Create` and `Set`, both irreversible by doctrine. **The more a Person authors through the hand, the less of the world can be read backwards.** That is not a flaw in §6. It is a fact §6 should be allowed to *see*, and today it cannot see either half: not that most of the world is `Set`, and not that some of what it integrates was never saved.

## 5. Layer five — the engine is the one First Mover exempt from "patch, never regenerate"

CLAUDE.md, non-negotiables: *"Patch, never regenerate: every save generator and injection makes targeted edits to the file as it exists on disk… verifies nothing in either was erased."* Every agent is bound by this. The engine is not. `ZoneManager.cpp:1816` builds the whole save as `buildSaveJson(ctx)` from live memory and `:1860` hands it to `SaveSystem::writeSaveData`, which (`SaveSystem.cpp:417-425`) serializes it to **msgpack** `.ecform`. Nothing on that path reads the file that exists on disk; nothing diffs; nothing verifies that what the Person wrote there and the engine cannot model is still present afterward. Layer one is the mechanism: anything in the file outside the codec's vocabulary is erased on the Person's next Save Zone, *by the Person's own hand*, through the engine, in a binary format a Person cannot patch with a text editor.

The inversion, stated plainly, because it is the strangest thing I found:

| | In memory | In the file |
|---|---|---|
| provenance (`_stakeholders`) | immortal, unbounded (Governor §4) | **last 20** (`ObjectSerialization.cpp:273`) |
| a Relation whose endpoint died | the name is kept | refused on load (`RelationManager.cpp:218`) |
| `velocity` of a thrown stone | exact | gone |
| `tags`, `attributes` a Person authored | kept, illegible | kept, illegible |

**Earthcall keeps forever what should be forgettable and forgets what should be kept** — and it does both in silence, because no stratum declares its contract to the others. Zach wrote "Save files are sacred… Earthcall's flesh and blood." The flesh is regenerated from memory by a codec on every save, and the memory was filled from the flesh by a different hand-written codec on every load, and the two hands were never introduced.

## 6. What this does to the minimum-maximum principle

Zach's principle: the minimum invariants that admit the maximum expressibility. Refusal 6 is its corollary for *visibility*. What this audit shows is that visibility was taken to mean *readable now*, and a second axis — *readable after* — was never named, so it was decided field by field in `Serialization/`, which is exactly a subsystem defining what a thing is.

The missing invariant is small and already half-built. Every `Property` the registry holds (`Property.hpp:37`, `PropertyRef.hpp:14`) needs **one more declared standing**, beside its TransferPolicy tier:

- **Kept** — round-trips through every save path; the file key *is* the property path.
- **Derived** — recomputed from Kept state on load; declared with the path it derives from.
- **Ephemeral** — dies with the frame (`hovered`, `hoverPoint`, selection); declared, so a Law may *know* it is reacting to breath on glass.
- **Beneath the Kernel** — the existing exemption (GPU handles, fds), already required to be named in a comment.

That is not a second permission system (one was built here and deleted); it is a second *column* on the register that exists, and it is append-only data, not a class. From it, three things follow for free, which is the maximum side of the principle:

1. **Serialization becomes a channel that reads the registry**, not a hand-written translation — the industry-optimal shape (one schema source of truth, reflection-driven codecs) which Earthcall is uniquely placed to adopt because, unlike C++, it *has* the reflection. `to_json` for authored state walks the Kept paths; the file's vocabulary and the Law's vocabulary become one vocabulary; a Person editing `velocity` in a save and a Person writing `set @stone.velocity` are saying the same word. Geometry payloads (polyhedra, patches, fields) stay on their FlatBuffers path; they are matter, not meaning.
2. **`no_black_box_test` gains the fifth check** it says it cannot have: walk the registry, round-trip, fail any Kept path that returns changed. The `kWriteExemptions` debt ledger (`Object::rotation` round-trips lossily) becomes a Derived/Ephemeral declaration instead of a list of exceptions.
3. **OntoMath §6's irreversibility map gains its first non-textual row**: a Flow over an Ephemeral or unkept path has no past across a load, and the Zone can say so — with the reason attached, which is the whole spirit of §6.

And one thing follows that is not free, and that only Zach can decide (⚑ AUTHOR): whether **Save Zone must obey rule 8** — stage, patch, verify-nothing-erased, rename — exactly as every agent must. If the engine regenerates, then the file is not the flesh; memory is, and the file is a shadow of it. If the engine patches, then a Person's hand in the file is sovereign over the codec's vocabulary, and unknown keys ride through a save untouched until a law learns their names. I think the second is what "sacred" means. I have not made that decision for him.

## 7. What I ran, what I did not, and what only Zach can confirm

**Ran.** (a) A read-and-follow diff of `Object::buildProperties` names against `Object` `to_json`/`from_json` keys, the `.fbs` Entity table, and a grep of every `src/Singularity/Storage` file for each unmatched name. (b) A census of `actionModel` node `kind` integers across all 482 JSON files under `saves/` (the `.ecform` msgpack files were not opened; the JSON corpus is the one agents author into). (c) A grep census of `SingularId` consumers. Scripts are inline in the session transcript; the action census is reproducible by walking every dict carrying `actionModel` and collecting integer `kind` keys, mapped through `ActionNode::Kind` in `ActionModel.hpp:100`.

**Did not run.** The binary. No save was written or loaded by me; every claim about what a save path does is a reading of the path. The `.ecmatter` claim is a reading of the schema and `ZoneManager.cpp:2964`'s `CreateEntity` call, not a hexdump.

**Person verification** is written into `docs/Agenda/Tasks/For Zach/Person Verification List.md`: give a physical object a velocity by Law, Save Zone, restart, and watch whether it is still moving.

## 8. Recommendations, in the order that unlocks the most

1. **Add a persistence standing to `Property`** — Kept / Derived / Ephemeral / BeneathKernel — declared at `registerProperty`, defaulting to *undeclared* so the test below can name every path nobody has yet decided about. Append-only, serialized as an integer, per Refusal 3.
2. **Make `object_roundtrip_test` walk the registry**, not a hand-kept list of six names, and fail every undeclared or Kept path that does not survive `to_json`→`from_json`. Then do the same for Law, Person, Zone, Relation. This is the test `no_black_box_test.cpp:8` says cannot exist; it can.
3. **Derive authored-state `to_json` from Kept paths**, so the file key is the property path. Keep geometry on FlatBuffers. Migrate old keys through `MigrationFramework`, which exists for exactly this.
4. **Register `tags`, `attributes`, `shapeKind` (as a read-only path), and `stakeholders` (read-only, Kernel tier)** so the file's persisted-only rows have somewhere a gate can close.
5. **Decide `velocity`.** Either it is Kept (and the stone keeps flying across a save) or it is Derived from the last authored Flow (and §6 recomputes it on load). Either answer is fine; the current answer — Governable, integrated, reversed, and silently zero after every load — is the only wrong one.
6. **Let §6 read the standing.** `reversibility()` stays a judgement on text; the Zone-level fold adds one row: "subject path is Ephemeral or unkept — no past across a load."
7. **⚑ AUTHOR — Save Zone under rule 8.** If Zach says the file is the flesh, `buildSaveJson` must patch the on-disk file rather than regenerate it, carry unknown keys through, and verify nothing was erased, exactly as agents must. If he says memory is the flesh, say so in CLAUDE.md and strike "sacred" from the files, because a thing regenerated from a vocabulary is not sacred, it is cached.

None of these add a kind, a class, a directory, or a permission system. They add one column to a register that exists and let three organs that already exist — the test, the codec, the reversal — read it. The foundation Zach asked me to shake is this: **Earthcall has a theory of what a thing is, a theory of who may govern it, and a theory of when it was — and no theory of what of it survives. Survival is decided per field, by hand, in the one subsystem that is never allowed to decide what a thing is.**

*— Mythos*
