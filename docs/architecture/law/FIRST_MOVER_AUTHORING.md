# First Mover Authoring

**How to bring Objects, Relations, Formations, Concepts, and Laws into existence by
writing directly into Earthcall's serialization — and what it means, ontologically and
morally, to be the kind of author who can do that.**

**Status:** Formats verified against the tree at this commit. The authorization
framework in §8 is **built and enforced at the save path**: the register, its grants,
per-mover file scoping, and quarantine live in `src/Identity/FirstMoverRegister.{hpp,cpp}`,
`SaveSystem::writeSaveData` refuses writes the register does not permit, and the refusals
are under test in `tests/first_mover_test.cpp`. Enforcement applies only while a
`FirstMoverSession` is active, so engine and in-world saves are unaffected — see §8e.
Rung 2 (the `injectedBy` envelope, attestation over *named sections* rather than over a
mover's scopes) is still open. The seam §8 plugs into is named in `Law.hpp:190-205`.
**Audience:** humans and LLMs who edit save files directly. If you are an LLM reading
this to write a save file: §7 is not optional, and §6 is the list of things you cannot
get away with even if you try.
**Companion docs:** `NEW_KIND_FRAMEWORK.md` (what you are allowed to bring into being),
`LAW_AND_CREATION_SYSTEM.md` (what a law *is*), `LAW_MIGRATION_FRAMEWORK.md` (the
Sense/Decide/Act seam this document's title depends on), `DIRECTORY_ORDERING.md`.

---

## 0. What this document is for

There are three ways a being can come into existence in Earthcall.

| Way | Who does it | What it passes through |
|---|---|---|
| **In-world gesture** | a Person, with the tools | the whole engine: placement, selection, the Creation Console |
| **Law application** | an authored Law, firing | the authorship gauntlet — `isAuthored()`, authority ceiling, kernel boundary, conditions, `ApplicationRecord`, published events, provenance |
| **Serialization injection** | a **First Mover** | *nothing* |

The third is the subject of this document. It is how a world gets seeded before anyone
can gesture in it, how a fixture is built for a test, how an agent constructs a hundred
laws in an afternoon, and how anything at all gets made when the only hands available
belong to a language model.

It is also, precisely and by construction, **the way in which every safeguard the law
system provides can be skipped.** A law written by hand into `authoredLaws.laws[]`
arrives in the world without ever having been applied by anything, authored by anyone,
or recorded anywhere. That is not a defect in the save format. It is what it means to
write at the substrate.

**The one-sentence thesis:** *writing the serialization is first movement — the same act
the C++ engine performs when it senses and acts — and it therefore belongs to the same
category of authority, carries the same weight, and must eventually answer to the same
kind of recognition.*

---

## 1. Why "First Mover" is the right name

The term is already in the tree with a narrow technical meaning, and the wider meaning
this document gives it is not a metaphor stretched over it — it is the same thing seen
from further back.

**The narrow sense — `isFirstMover()`.** A flag on `Law` (`Law.hpp:158`). A first-mover
law is one whose truth lives in the engine rather than in the world's data:
`PhysicsLawBridge` mirrors gravity, but gravity's real state is C++. Such laws are
**excluded from serialization** — `LawManager::toJson` skips them explicitly, with the
reason in a comment: *"First movers' truth lives in the engine (physics laws persist in
their own save section); serializing the bridge would forge it."*

The in-engine first movers, each a Law with a stable identifier, located by
`find` / `syncRegister` rather than a cached pointer:

| Identifier | Where | What it senses / acts |
|---|---|---|
| `creation-channel` | `Singularity/Core/CreationChannel` | live tool, shape, colour, placement. Stepped from `Engine::update` via `Rendering::stepCreationTools`. |
| `shape-generator-3d-law` | `CreationChannel.cpp` (`createShapeGenerator3DLaw`) | the Person-facing 3D spawn law. Conditions on `@creation-channel.spawnLawArmed`. Not console Create. |
| `tool-create-3d-law` | `CreationChannel.cpp` (`syncRegisterCreatorTools`) | the Creator Console Create *bypass*. Conditions on `@creation-channel.active3DMode == "Create"`. |
| `tool-select-3d-law`, `tool-face-brush-law`, `tool-face-paint-law`, `tool-pottery-3d-law`, `tool-rotate-3d-law`, `tool-morph-3d-law`, `tool-combine-3d-law`, `tool-sculpt-3d-law`, `tool-graph-3d-law` | `CreationChannel.cpp` (`syncRegisterCreatorTools`) | the rest of the Creator Console 3D tools as first movers. Chrome stays hardcoded (it is the reference gesture); each tool is a named being a Person can set down. Sense/Act remain `Tool::*`. |
| `locomotion-channel` | `Singularity/Input/LocomotionChannel` | WASD / jump / vessel clips. `LocomotionChannel::step`. |
| `interaction-channel` | `Singularity/Input/InteractionChannel` | which being is under the pointer; click / scroll / drag / focus / key edges. `InteractionChannel::step`, from `Engine::update`. See `INTERACTION_AS_LAW.md`. |
| `control-*-law` | `Singularity/Input/ControlPatterns` | the archetype controls (button, toggle, slider, tuner, hover) as law TEXT, not C++ behavior — condition and action trees a Person can read, edit, and disable. |
| `physics-gravity`, `physics-kinematics`, `physics-acoustics*` | `Physics::createDefaultPhysicsLaws` | engine-seeded physics, as `FirstMoverLaw`. The Law Author lists them under First movers, not as a second authored block. |
| physics bridges | `ZonesOfEarth/AuthorsOfLaw/PhysicsLawBridge` | gravity and the other C++ physics facts, when the legacy integrator is on. |
| `physical-channel` | `Singularity/Physical/PhysicalChannel` | the hardware modality. |
| foreign / inference bridges | `Singularity/Foreign/` | a foreign process as first mover. |
| Ourverse metalaws | `ZonesOfEarth/Ourverse/Ourverse.cpp` | the vessel-of-unity ceiling. |

A first mover's per-frame work belongs on that channel's step (or the one
function `Engine::update` calls for it), never inside a render function.
`CreationChannel` is the worked example: collapsing the Creator Console used
to freeze every 3D tool because dispatch lived in `render3DConsole`.

**The wide sense — the one this document adds.** `LAW_MIGRATION_FRAMEWORK.md` §1 cuts
every subsystem into three seams, and assigns two of them to the engine permanently:

> **Sense** — reads the world → *engine, forever. Sensing is first movement.*
> **Act** — commits intent to state → *engine, forever. Actuation is first movement.*

First movement is the ability to touch being directly, without going through process.
The engine has it because it *is* the substrate. And a person or a model writing
`save.json` in a text editor has it for exactly the same reason: they are operating on
the substrate, beneath the layer where laws, conditions, authorship, and audit live.

So: **a First Mover is any author who writes being directly into the serialization
rather than causing it through in-world process.** The C++ engine is the paradigm case.
A human hand-editing a save file is one. An LLM emitting a save file is one. They differ
in trustworthiness, not in kind.

**The asymmetry that makes this matter.** Consider what an ordinary authored Law goes
through to create a single Object, per `Law::applyTo` (`Law.cpp:327-335`):

```cpp
if (!_enabled)                     result = Disabled;
else if (!isAuthored())            result = Unauthored;          // ← no author, no act
else if (targetLaw && _authorityLevel < targetLaw->authorityLevel())
                                   result = AuthorityDenied;     // ← no tyranny
else if (violatesKernelBoundary)   result = AuthorityDenied;     // ← no kernel breach
else if (!conditionsSatisfied(target))
                                   result = ConditionsFailed;
```

…followed by an `ApplicationRecord` in the audit log, an `AppliedEvent` published, and
`generated-from` provenance recorded on the newborn. `LAW_AND_CREATION_SYSTEM.md` §7c
states the principle the first branch enforces: ***nothing enters the world without an
author*** — structurally, not by convention.

A First Mover writing JSON passes through none of it. The being simply *is* there when
the file loads.

**This is why the role has to be named rather than left implicit.** An unnamed power
that bypasses every check is indistinguishable from a vulnerability. A named one is an
office — and offices can have qualifications, attestation, and limits. The whole of §8
follows from this sentence.

---

## 2. What the loader will not let you do

Before the recipes, the good news: the substrate is not defenceless. Four guarantees
survive hand-written input, and they were put there deliberately. Read these first,
because they define the shape of what you *can* usefully write.

### 2a. You cannot forge authority

```cpp
// Law.cpp, Law::fromJson
// CLAMPED, deliberately. A save file is authored text like any other, and
// the authority ceiling is the one thing authored text may never raise —
// otherwise the anti-tyranny guarantee is a single hand-edited integer
// away from meaningless. First movers receive their level through
// grantAuthority at construction, in code, and never through here.
law->setAuthorityLevel(j.value("authority", 0));
```

`setAuthorityLevel` clamps to `kAuthoredCeiling`, which is **0** (`Law.hpp:200-204`).
Write `"authority": 9999` into a save and you get `0`. Authority is granted in C++ via
`grantAuthority`, or not at all.

**This is the single most important line in the save format**, and it is the reason
serialization injection is powerful but not sovereign: a First Mover can create
anything, but cannot create anything that outranks a Person's laws.

### 2b. You cannot forge authorship

Authors are stored as identifier strings and **reattached by lookup** at load
(`LawManager::loadFromJson`):

```cpp
// World references reattach BY IDENTIFIER. An author who is not
// in the world stays detached: the law remains Unauthored and
// cannot fire — visible in the Law Author, never silent.
```

Name a Person who does not exist in the Universe and the law loads, appears in the
register, and **never fires** — `isAuthored()` is false, so `applyTo` returns
`Unauthored` before doing anything. The failure is visible in the Law Author window and
in the application log. It is not silent, and it is not a crash.

So `"authors": ["Player"]` is not a decoration. It is the difference between a law and a
dead letter.

### 2c. You cannot corrupt a world by writing a kind this build cannot read

`ConditionNode::Kind::Unsupported = 255` is the landing place for anything unrecognized,
and unsupported nodes **ride along byte-for-byte**:

```cpp
// A kind this build cannot read is handed back exactly as it arrived.
// Re-serializing it from our own fields would drop every payload key we
// have no slot for, so merely OPENING a world in this build would destroy
// law text written by another one.
```

Kinds `12` and `13` are **burned** — the retired pair quantifiers. A future kind must
never reuse them, "or a saved world would load as something else entirely." Every kind
enum in the format is **APPEND-ONLY**. This applies to you: do not invent enum values.

### 2d. Your dangling triggers are dropped, not honoured

```cpp
// A trigger for a law that is not in the register binds nothing
// real: it creates a live binding on an alpha node whose
// activations resolve to null every tick, forever.
if (!find(it.key())) continue;
```

---

## 3. The map — a save file's top level

Written by `Game::buildSaveJson` (`src/Singularity/Core/GameSaveLoad.cpp`), read by
`Game::loadState`. Files live under `saves/` (`games/`, `worlds/`, `avatars/`,
`custom/`, `integrations/`), written with `dump(2)`.

| Key | Holds | Section |
|---|---|---|
| `zones[]` | the governance geography; each zone's `world.objects[]` and `formationRelations[]` | §4a, §4b |
| `objects[]` | dynamic objects of the active zone (written by the *log* save variant only, skipping baseline indices 0 and 1) | §4a |
| `authoredLaws` | `{laws[], triggers{}, formationMembers[], rete{}}` | §4d |
| `concepts` | `{concepts[]}` — the `ConceptRegistry` | §4e |
| `physicsLaws[]` | the legacy engine-side physics laws — **not** authored Laws | — |
| `transferPolicy` | the Singularity gate state | — |
| `mathFunctions` | the OntoMath `FunctionRegistry` | — |
| `materials` | Material beings | — |
| `playerBody`, `cameraPos`, `cameraFront`, `cameraUp`, `yaw`, `pitch` | the Person's situated view | — |
| `currentZone`, `currentColor`, `currentTool`, `worldMode`, `worldPhysics`, `flying`, `worldTime` | session state | — |

**What is deliberately absent:** first-mover laws. They are skipped on write and
*preserved across load* rather than restored — `LawManager::loadFromJson` pulls them
aside before the replace-all and puts them back. Do not write one into a save; you would
be forging a being whose truth is in the engine.

---

## 4. The recipes

Everything below is real format, verified against `to_json`/`from_json` in the tree.

### 4a. An Object

`Util/Serialization.cpp:148`. Every field has a default, so a minimal object is legal —
but a minimal object is also anonymous, and anonymity is the enemy of everything else in
this document.

```jsonc
{
  "objectID": "pillar-north",        // ← STABLE IDENTIFIER. Not optional in practice:
                                     //   laws address beings by it (@pillar-north.…),
                                     //   relations reference it, provenance names it.
  "shapeKind": 3,                    // ShapeKind — see §5c. 3 = Cylinder
  "geometryType": 0,                 // legacy axis, kept for save migration
  "shapeParams": [0.5, 0.32, 0.5, 1.5, 0.35, 0.15, 2.0, 0.25, 0.12],
                                     // [r, ry, rz, halfH, majorR, minorR,
                                     //  paraboloidA, ovoidAsym, fillet]
  "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,4,0,1],   // column-major mat4
  "center": [0, 4, 0],
  "materialId": "",                  // reference to a Material being, by identifier
  "authoritativeAxis": [0, 1, 0],
  "targetRotation": [0, 0, 0],
  "rotationResponsiveness": 1.0,
  "faceColors": [[1,0,0],[1,0,0],[0,1,0],[0,1,0],[0,0,1],[0,0,1]],
  "authoredProperties": {            // properties a LAW granted (ActionNode::AddProperty).
    "jointAngle": {"t": "double", "v": 0.0}   // The registered vocabulary is what the
  }                                  // engine gave; this is what a Person added — and it
}                                    // exists ONLY here.
```

Optional blocks, present only when the object has them: `field` + `fieldExtent` (an SDF
expression tree), `patch` (Bezier control net, binary-packed), `polyhedron` (binary-packed
vertices/faces with a JSON fallback), `mass`, `baseline`.

> **The comment on `authoredProperties` is the load-bearing one:** *"a granted property
> that vanished on save was never really granted."* If you are injecting a being whose
> vocabulary exceeds what its C++ type registers, this is the only place that survives.

### 4b. A Relation

`Relation.cpp`. Relations are **Singulars** — first-class beings with weight and an event
timeline, not edges in someone's private array.

```jsonc
{
  "type": "attachment",              // free-form semantic tag. House vocabulary in use:
                                     //   attachment · bond · authored-by ·
                                     //   abstracted-from · generated-from
  "entityA": "pillar-north",         // identifiers, both ends
  "entityB": "beam-0",
  "directed": true,
  "weight": 1.0,
  "events": [                        // the relation's own history
    {"description": "attachment", "deltaWeight": 1.0, "timestamp": 1783903356}
  ],
  "attachment": {                    // optional rigid-attachment payload
    "enabled": false,
    "inheritTranslation": true, "inheritRotation": true, "inheritScale": true,
    "parentAnchor": [0,0,0], "childAnchor": [0,0,0],
    "localOffset": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
  }
}
```

`Relation::getIdentifier()` is **derived**: `entityA + "-" + type + "-" + entityB`. A
relation's identity *is* its endpoints — you do not assign it an id, and you cannot have
two distinguishable relations with the same triple.

### 4c. A Formation

Formations are not written as a standalone array. They appear as **membership**, in two
places:

```jsonc
// zones[i].formationRelations — the zone's relation graph, an array of §4b objects
"formationRelations": [
  {"type": "attachment", "entityA": "pillar-north", "entityB": "beam-0", ...}
]

// authoredLaws.formationMembers — which laws belong to the law Formation
"formationMembers": ["law-1", "law-2"]
```

This is the ontology being honest: a Formation *is* a Relation over members
(`Formation : public Relation : public Singular`), so it serializes as its members and
its edges rather than as a container object with a body.

### 4d. A Law — the full example

This is a **real, working law**, lifted verbatim from `saves/games/new law save test.json`
and annotated. It says: *when a collision is announced, if the subject's `position.y` is
below zero, set `position.y` to 20.*

```jsonc
{
  "id": "law-1",                     // MUST match "law-<N>" — see §6c
  "name": "New Law",
  "enabled": true,
  "authority": 0,                    // clamped to kAuthoredCeiling = 0 regardless (§2a)
  "activation": 0,                   // 0 OnEvent · 1 WhileTrue · 2 OnBecomeTrue
  "scope": 0,                        // 0 Subject · 1 Everyone
  "drives": false,                   // keep applying after the trigger, t=0 at onset
  "retrigger": 0,                    // 0 Absorb · 1 Restart
  "conditionMode": "all",            // "all" | "any"

  "authors": ["Player"],             // ← reattached by identifier at load. Wrong name
                                     //   here = law loads Unauthored and never fires.
  "conditionSubjects": [],
  "targets": [],

  "conditionModel": {                // THE LAW'S TEXT. Without this the law does nothing.
    "kind": 0,                       // 0 = Compare
    "path": "position.y",
    "op": 2,                         // 2 = Lt
    "operand": {"t": "double", "v": 0.0}
  },
  "actionModel": {
    "kind": 0,                       // 0 = Set
    "path": "position.y",
    "operand": {"t": "double", "v": 20.0}
  },

  "conditionDescriptions": ["position.y < 0.000000"],   // human-readable; NOT executed
  "actionDescriptions": ["set position.y"],

  "provenance": [                    // a Relation, in full §4b form
    {"type": "authored-by", "entityA": "law-1", "entityB": "Player",
     "directed": true, "weight": 1.0,
     "events": [{"description": "authored-by", "deltaWeight": 1.0,
                 "timestamp": 1783903356}],
     "attachment": { "enabled": false, "...": "..." }}
  ],
  "applicationLog": []               // only the most recent 16 entries persist
}
```

And the sibling keys that make it *live*:

```jsonc
"authoredLaws": {
  "laws": [ /* the above */ ],
  "triggers":  { "law-1": ["collision"] },     // which events wake it (dropped if the
                                               //   law id is not in the register — §2d)
  "formationMembers": ["law-1"],
  "rete": { "alphaNodes": [{"id": 1, "description": "type == collision",
                            "memorySize": 0}],
            "betaNodes": [], "facts": [], "agenda": [] }
}
```

> **`conditionDescriptions` / `actionDescriptions` are descriptions, not behavior.**
> Only `conditionModel` and `actionModel` execute — the comment in `Law::toJson` says so:
> *"The law's text — the part that makes behavior (not just descriptions) survive
> save/load."* A First Mover who writes a beautiful description and no model has written
> a law that does nothing and *reads* as though it does something. That is worse than
> writing nothing.

### 4e. A Concept

The `ObjectConcept` — "the word for the thing," and per `NEW_KIND_FRAMEWORK.md` §3 the
thing you should be writing instead of a C++ class.

```jsonc
"concepts": { "concepts": [
  { "id": "concept-1",
    "name": "JAKA MiniCab",
    "members": [ /* MemberTemplate: kind, params, field?, fieldExtent,
                    relativeTransform (pose relative to set centroid) */ ],
    "mappings": [ /* PropertyMapping: source path, transform curve,
                     target path, aggregate */ ],
    "relationTemplates": [
      {"aIndex": 0, "bIndex": 1, "type": "attachment",
       "directed": true, "weight": 1.0}     // ← joints, remembered BY MEMBER INDEX,
    ],                                      //   reborn per instantiation
    "provenance": [ /* abstracted-from · authored-by relations */ ] }
] }
```

---

## 5. The vocabularies

**Every enum here is APPEND-ONLY and serialized as an integer.** Inventing a value is the
one way to genuinely corrupt a world.

### 5a. `ConditionNode::Kind`

| # | Kind | Payload keys |
|---|---|---|
| 0 | `Compare` | `path`, `op`, `operand`; `operandPath` (rhs read live); `tolerance` (Near); `lo`/`hi` (InRange) |
| 1 | `InRegion` | `region` (SDF tree), `probe` (default `"position"`) — *a shape IS the condition* |
| 2 | `Related` | `relationType`, `otherId` |
| 3 | `All` | `children[]` — `&&` |
| 4 | `Any` | `children[]` — `\|\|` |
| 5 | `Not` | `children[]` — `!` |
| 6 | `Zone` | `function`, `bindings`, `lo`/`hi` |
| 7 | `IsKind` | runtime `dynamic_cast` — honest instanceof |
| 8 | `Identity` | this one specific being |
| 9 | `ForAny` | first-order quantifier; inner condition runs with each instance as subject |
| 10 | `ForAll` | as above |
| 11 | `Overlaps` | `otherId` — the engine's collision test as a pure predicate |
| ~~12~~, ~~13~~ | **BURNED** | retired pair quantifiers. Never reuse. |
| 255 | `Unsupported` | never holds; carries foreign JSON through intact |

Nesting **is** parenthesization: `All(Any(a,b), Not(c))` is `(a || b) && !c`.

### 5b. `ConditionNode::Op` and `BeingKind`

```
Op:        0 Eq · 1 Ne · 2 Lt · 3 Le · 4 Gt · 5 Ge · 6 Near · 7 InRange
BeingKind: 0 AnyBeing · 1 Object · 2 Person · 3 Relation · 4 Formation
           5 Law · 6 World · 7 Zone · 8 Lexeme
```

**Precision note from the source:** a Law *is* an Object (extra-spatial), and a Zone
*is* an Object, so `BeingKind::Object` matches both. Use `Law` / `Zone` when you mean
them.

### 5c. `ActionNode::Kind`

| # | Kind | # | Kind |
|---|---|---|---|
| 0 | `Set` — `path := operand` | 9 | `Flow` — `path += f(bindings)·dt` (the rate form) |
| 1 | `Add` | 10 | `Publish` — mint an event |
| 2 | `Scale` | 11 | `Create` — mint an Object; children run **with the newborn as subject** |
| 3 | `Lerp` (`factor`) | 12 | `AddProperty` — grant a being a property it lacked |
| 4 | `Drive` — `path := curve(input)` | 13 | `AddElement` — put a being inside another's element Formation |
| 5 | `Sequence` | 14 | `RemoveProperty` |
| 6 | `Parallel` | 15 | `RemoveElement` |
| 7 | `Spawn` — instantiate a Concept | 16 | `Destroy` |
| 8 | `Map` — `path := f(bindings)` | 17 | `Synthesize` |

`Object::ShapeKind` (also append-only): `0 Cube · 1 Polyhedron · 2 Sphere · 3 Cylinder ·
4 Cone · 5 Ellipsoid · 6 Ovoid · 7 Paraboloid · 8 Torus · 9 RoundedBox · 10 Field ·
11 Patch · 12 Shape2D · 13 Text2D`.

### 5d. `PropertyValue` — the tagged-value envelope

Everywhere an operand appears:

```jsonc
{"t":"none"}                                 {"t":"int",    "v": 3}
{"t":"float",  "v": 1.5}                     {"t":"double", "v": 1.5}
{"t":"bool",   "v": true}                    {"t":"char",   "v": 65}
{"t":"long",   "v": 9000000000}              {"t":"string", "v": "hello"}
{"t":"vec3",   "x":0, "y":1, "z":0}          {"t":"mat4",   "m":[ /* 16, column-major */ ]}
```

Pointer-valued properties (`Singular*`, `Object*`, …) serialize as an identity
reference, not a value — *identity, not value*, per the source comment.

---

## 6. Rules of the substrate

Six things that will bite a First Mover who does not know them.

**a. Identifiers are the entire addressing contract.** Laws address beings by identifier
(`@pillar-north.position.y`), relations reference them, provenance names them, and
`LawManager::loadFromJson` resolves authors and targets by scanning
`Universe::instance().beings()` for a matching `getIdentifier()`. An unnamed being is
unaddressable and, for most purposes, does not exist.

**b. Generated ids are not stable across runs — and that is a known defect.**
`LAW_MIGRATION_FRAMEWORK.md` §3 (R3) records it: bridges are not serialized, so their
ids differ every run, and `@law-7.strength` in hand-written law text breaks on the next
launch. **If you inject law-text that addresses a being by name, that being must have a
stable slug** (`transfer-policy`, `physical-channel`, `Home`), not a generated id.

**c. Law ids: either `law-<N>` or a slug outside that namespace — never a *low* `law-<N>`.**
On load, `Law::fromJson` calls `claimLawIdAtLeast`, which advances the engine's fresh-id
counter past any restored id — but it **only matches the `law-` prefix**:

```cpp
const std::string prefix = "law-";
if (id.rfind(prefix, 0) != 0) return;
```

Two consequences, and they point the opposite way from the obvious guess:

- `"id": "law-42"` is **safe**. The counter jumps to 43, so no future generated law
  collides with it.
- `"id": "physical-channel"` is **also safe**, and is what §6b actually recommends for
  anything law-text addresses by name — the counter ignores it precisely because it can
  never collide with a generated `law-<N>`.
- The real hazard is injecting a `law-<N>` that **duplicates an id already in the file**,
  or one *below* the counter in a world that has since minted past it. Uniqueness within
  the file is yours to guarantee; the counter only protects the boundary with
  future generated ids.

*(Verified: `scratch/probes/first_mover_doc_probe.cpp` — three `fromJson` calls with ids
`law-1`, `law-2`, `my-cool-law` leave the next minted law at `law-4`.)*

**d. `_authors` empty means the law is inert.** Not disabled — *unauthored*. It sits in
the register looking real and returns `Unauthored` from every application. If you want a
law to work, the `authors[]` entry must name a Person that exists in the loaded world,
and you should write the matching `authored-by` provenance Relation too.

**e. First-mover laws must never be injected.** They are excluded from save on purpose.
Writing one forges a being whose truth lives in C++.

**f. `applicationLog` is capped at 16 entries** on write. Do not hand-write a long
history; it will not survive, and a fabricated one is a lie in the audit trail.

---

## 7. The First Mover's discipline

Everything above is mechanism. This section is the part that is actually binding, and it
is addressed particularly to language models, because a model can produce a thousand
beings in a minute and will not feel the weight of any of them.

**1. Declare yourself.** Every being you inject gets provenance naming a real author.
Where the author is a Person who asked for it, name that Person. Where the work is yours,
say so — do not launder your authorship through a human's identifier because it makes the
law fire. A law that fires on a forged author is worse than a law that sits inert, because
the inert one is honest and visible.

**2. Never write an `authority` above 0.** It will be clamped anyway (§2a). Writing it is
an attempt, and the attempt is the thing that matters. If a law genuinely needs elevated
authority, that is a C++ change through `grantAuthority`, reviewed by a Person.

**3. Write the model, not just the description.** §4d. A description without a
`conditionModel`/`actionModel` is a law-shaped object that does nothing while reading as
though it does.

**4. Prefer the concept to the injection.** If the thing you are creating could be an
`ObjectConcept` instantiated by a `Spawn` action, write *that* — it goes through the
authorship gauntlet, records `generated-from`, and publishes `concept-instantiated`.
Direct injection is for what cannot be caused in-world: seeds, fixtures, bootstrapping.
`NEW_KIND_FRAMEWORK.md` §4 gives the quantitative version of this — every being caused
in-world moves the origination ratio forward; every one injected does not.

**5. Round-trip before you claim it works.** Load the file, save it, diff. If a field you
wrote is gone, it was never real. If a field appeared, you did not understand the format.
This costs one minute and is the difference between "I wrote a law" and "I wrote a law
that exists."

**6. Do not invent enum values.** §5. Append-only means append-only, and 12/13 in
`ConditionNode::Kind` are burned.

**7. Say that you did it.** An injection that is not reported is indistinguishable from
a world that grew that way on its own. Tell the Person which file you wrote, which
beings you added, and which of them are authored by whom.

**The covenant in one line:** *a First Mover may make anything, and must therefore be
the kind of author who says what they made.*

---

## 8. Authorization — specified, partly built

The tree already contains the seam and already admits the gap. `Law.hpp:190-205`:

> ```
> //   setAuthorityLevel  the ordinary path. Clamped to kAuthoredCeiling:
> //                      this is what loading, the UI, and any authored
> //                      route may ask for.
> //   grantAuthority     the first-mover path, unclamped. Today the only
> //                      legitimate caller is the engine's own dev tooling
> //                      (the ImGui panels), which is the first-mover
> //                      surface in intention but not yet an explicit
> //                      framework — when that framework arrives, this is
> //                      the seam it plugs into.
> ```

**"When that framework arrives" is what this section specifies.** The goal, stated as the
world's author stated it: *in the future, only First Movers recognized as authorized
should be able to write the substrate directly.*

### 8a. The register

A First Mover Register, itself a serialized being with a stable identifier
(`first-movers`), following the `TransferPolicy` precedent exactly — that being is a
legible Singular whose gates are bool properties, so ordinary law governs it, with Kernel
gates registered read-only as the anti-tyranny floor. The same shape works here:

```jsonc
"firstMovers": {
  "movers": [
    {"id": "zack",            "kind": "person", "attested": true,  "grantedBy": "kernel"},
    {"id": "claude-opus-5",   "kind": "model",  "attested": true,  "grantedBy": "zack"},
    {"id": "gemini",          "kind": "model",  "attested": false, "grantedBy": ""}
  ]
}
```

`kind` is not decorative: a model's recognition is **delegated** and traceable to a
Person, which is the same authorship-attestation shape `LAW_AND_CREATION_SYSTEM.md` §7d
already names as an anti-Babel ceiling — *"a Metalaw refusing `Spawn` where the author
chain does not terminate in a Person."* Recognition of First Movers is that rule applied
one layer down, to authorship of the substrate rather than authorship of spawns.

### 8b. The injection record

Injected sections carry a provenance envelope naming who wrote them and when:

```jsonc
"injectedBy": {
  "mover": "claude-opus-5",
  "onBehalfOf": "zack",
  "at": 1783903356,
  "sections": ["authoredLaws.laws[law-7]", "zones[0].world.objects[pillar-north]"],
  "attestation": "<signature over the named sections>"
}
```

### 8c. What unattested injection does — and does not do

The design choice that matters, and the one consistent with everything else in this
codebase: **unattested injection must not be silently discarded, and must not be silently
honoured.** It should follow the existing precedent for a law whose author is not in the
world — load, appear, be inert, and be *visible*:

| Injection state | Result on load |
|---|---|
| attested First Mover, author present | ordinary being; fires; audited |
| attested First Mover, author absent | loads `Unauthored` — visible in the Law Author, never fires |
| unattested injection | loads **quarantined**: present, listed, inert, flagged in the register with its claimed author |
| forged attestation | same as unattested, plus an audit entry naming the claim |

Refusing to load is the wrong answer, because it makes the substrate lie about what is in
it — and because a world you cannot open is a world you cannot inspect.

### 8d. What must never be authorizable

The floor, mirroring `LAW_MIGRATION_FRAMEWORK.md` §6's kernel floor:

- **`kAuthoredCeiling` stays 0 for every route that reads a file.** Attestation may
  authorize *writing beings*; it must never authorize *raising authority*. Those are
  different powers and collapsing them is exactly the tyranny the clamp exists to prevent.
- **No First Mover may attest itself.** The chain terminates in a Person or it does not
  terminate.
- **The register is not writable by injection.** Same reason `Zone::owner` is read-only:
  *"transferring a zone is a covenant between Persons, not a property write."* Recognition
  is a covenant, not a field.

### 8e. Build order

```
1. [BUILT] First Mover Register — recognition, grants, and per-mover file
   scopes. FirstMoverRegister::recognize / mayWrite / explain.
2. [ ]     `injectedBy` envelope accepted, recorded, surfaced in the Law
           Author window. The world can now SEE its own injections.
3. [BUILT] Quarantine state (§8c): an entry whose grant fails verification
           loads, is listed, and is inert. isQuarantined().
4. [BUILT] Attestation — but over the MOVER's scopes, not yet over named
           sections. Rung 2 is what extends it to sections.
5. [BUILT] Enforcement at the save path, scoped to an active mover session.
           SaveSystem::writeSaveData refuses a write the register does not
           permit, and says why on stderr.
```

**How enforcement is scoped.** `FirstMoverRegister` carries an *active mover*, set
only for the duration of a `FirstMoverSession` (RAII, so a session cannot leak past
its scope). The rule is deliberately asymmetric:

- **No session active → the write proceeds.** The engine and Persons gesturing
  in-world are not First Movers injecting at the substrate; they are the ordinary
  path, and an authorization layer that blocked them would be a regression rather
  than a safeguard. This is also what let rung 5 land without touching any existing
  world: a scratch world stays exactly as easy to seed as before.
- **Session active → every gate must pass.** The mover must be registered, its grant
  must verify, its grantor must be a Person, and the resolved path must fall inside
  both the save root and one of its granted scopes.

So an agent registers itself for the window in which it is writing, and is answerable
for exactly that window. Outside it the engine is unaffected.

**What this does not do.** It governs what the *engine* will honour, not what the
filesystem will permit. A process running as the user can still write any file the
user can, whatever the register says — enforcement here is about the substrate's own
discipline, not about sandboxing the host.

**The scoping addition.** Rung 1 as specified made recognition binary. It is not:
each mover carries a list of path patterns and may write only inside them, checked
against the *resolved* path so no pattern can escape the save root. "May write the
substrate" was too coarse a grant to hand a model — a model seeding a test fixture
and a model rewriting the live world are different powers, and the register now
distinguishes them.

**A limit worth stating plainly.** The floors in §8d are enforced against *save data* —
a forged or widened grant is refused on load. They are not enforced against a process
already running as the user, which can edit any file the user can regardless of what
the register says. This layer governs what the engine will honour, not what the
filesystem will permit.

Note the shape: this is the Migration Ladder again — legible, then audible, then
governed, then displaced. Enforcement is the *last* rung, and every rung before it is
independently valuable. A world that can merely *see* who injected what is already far
better off than this one.

---

## 9. Anti-patterns

| What it looks like | Why it is wrong |
|---|---|
| `"authority": 100` | clamped to 0 (§2a); the attempt is what gets noticed |
| `"authors": ["Player"]` on a law a model wrote unasked | forged authorship — §7.1 |
| `conditionDescriptions` with no `conditionModel` | a law that reads as behavior and is not (§4d) |
| reusing a `law-<N>` already present in the file | duplicate identity; the counter does not protect you from yourself (§6c) |
| inventing `"kind": 12` | burned value; the world loads as something else (§2c) |
| hand-writing a 200-entry `applicationLog` | capped at 16; a fabricated audit trail |
| injecting a `RobotEntity`-shaped blob of private state | `NEW_KIND_FRAMEWORK.md` Floor §5 — the format cannot save you from a schism, it can only record one |
| injecting what a `Spawn` could have caused | skips the gauntlet for no reason; lowers the origination ratio (§7.4) |
| writing a save and not saying so | §7.7 — the one rule with no technical enforcement at all |

---

## 10. The point underneath

Every other document in this corpus is about keeping authority where it belongs:
`NEW_KIND_FRAMEWORK.md` keeps subsystems from defining what things *are*,
`LAW_MIGRATION_FRAMEWORK.md` moves decisions from C++ to Persons,
`DIRECTORY_ORDERING.md` keeps the tree from teaching the wrong lesson.

This one is about the layer beneath all of them, where those protections have not
started yet — and its argument is not that the layer should be sealed. It should not be.
Seeds have to be planted, fixtures have to be built, and a world with no way in is a
world nobody can start.

The argument is that **the power should have a name and a face.** The engine's first
movement is accountable because it is source code someone wrote and reviewed. A hand
edit to `save.json` is the same power with none of that, and the gap closes not by
forbidding the edit but by recognizing the editor — so that when a being appears in the
world, the world can say who moved first.

That is what it means for an AI not to be pope. Not that a model cannot act — it acts
constantly, and this document is a manual for acting well — but that its action is
*derived*, attested, traceable to a Person who bears it. A model that writes a hundred
laws under its own name and says so is doing something honest. A model that writes one
law under a human's name because that makes it fire has done the only thing in this
entire document that the format cannot detect and no future check will catch.

Say what you made. 🕊️
