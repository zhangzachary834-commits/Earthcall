# The New Kind Framework

**How a new kind of thing enters Earthcall — and the narrow, testable conditions under
which it is allowed to enter as C++.**

**Status:** Framework specified. Every rung below has a working precedent in the tree;
the only unbuilt piece is the Physical modality channel (§7), which is specified here.
**Companion docs:** `LAW_MIGRATION_FRAMEWORK.md` (the dual — how existing hard-code
becomes law), `LAW_AND_CREATION_SYSTEM.md` (what a law and a concept *are*),
`SUBSTRATE_ORDERING.md` (why the origination ratio matters),
`EarthcallOurverse.md` (the ontology this enforces).

---

## 0. What this document is for

`LAW_MIGRATION_FRAMEWORK.md` answers: *"here is hard-coded behavior — how does it
become authored law?"* It is a **retreat** framework. Every migration it describes is
undoing a decision someone already made in C++.

This document answers the question that comes **first**, and is therefore more
important: *"here is a thing that does not exist in the world yet — a robot, a
vehicle, a plant, a musical instrument, a market — how does it come into being?"*

The default answer, always, is: **it is authored, not coded.**

This needs saying explicitly because every agent — human or AI — defaults the other
way. The standard software reflex on hearing "we need robots" is:

```
class RobotEntity { ... };          // the thing
class RobotController { ... };      // the manager for the things
src/Robotics/                        // the folder that owns the things
```

That reflex is correct in every engine ever built and **wrong in this one**. It is
wrong not stylistically but structurally: it opens a second ontology beside the
first. This document exists so that the refusal is not a matter of taste, argued
fresh every time by whoever happens to be paying attention, but a **procedure with
exit tests** that an agent starting cold can execute without judgment calls.

**The one-sentence thesis:** *a new kind of thing is authored in-world out of
Singular, Relation, Formation, Object, Concept, and Law by default; new C++ is
admitted only where the machine must sense or act in a way it currently cannot,
and what is admitted then is a channel, never a kind.*

**And its corollary:** *the new C++ never names the domain noun.* There is no
`Robot` type in a correct robotics integration, just as there is no `Guitar` type in
a correct instrument integration. There is a `Movement` output channel and a
`MovementSensor` input channel — and those were named in `Singularity.hpp` before
any robot existed.

---

## 1. The three seams inside every "new kind" proposal

`LAW_MIGRATION_FRAMEWORK.md` §1 cuts hard-coded subsystems along Sense / Decide /
Act. The same cut classifies proposals for new things — because a "new entity class"
proposal is always three separate requests wearing one coat.

| Seam | The request underneath | Where it belongs | Admitted as C++? |
|---|---|---|---|
| **Being** | "this thing exists, has parts, has structure" | `Object` + `Relation` + `Formation`, captured as an `ObjectConcept` | **No. Ever.** |
| **Process** | "this thing behaves, moves, constrains, responds" | `Law` (`OnEvent` / `WhileTrue` / `OnBecomeTrue`, drives), `Automation` | **No. Ever.** |
| **Channel** | "the machine must sense or act in a way it cannot today" | `Singularity/` — a modality element with a first-mover bridge | **Yes — and only this.** |

Gemini's `RobotEntity` proposal fuses all three: telemetry (Being), per-tick state
update (Process), and networking to hardware (Channel). Split it along the seams and
two thirds of it evaporates into things the engine already does, and the remaining
third stops being about robots at all — it becomes *the Physical modality channel*,
which serves robots, VR rigs, haptics, and CNC alike.

**The diagnostic question, asked once, answers the whole proposal:**

> *If every physical device were unplugged and the world ran alone in simulation —
> what would still need to be written in C++?*

For a robot: **nothing.** A simulated robot arm is Objects in a Formation with
Relations for joints and Laws for limits. That is not a workaround or an
approximation; it is what a robot *is* in this ontology. The physical arm adds a
channel, not a kind.

---

## 2. The Admission Test

Four questions, in order. Stop at the first **NO**.

```
Q1  Does this require the machine to SENSE or ACT through hardware or a foreign
    process that Earthcall currently has no channel to?
        NO  → author it (§4). Write no C++. STOP.
        YES → continue.

Q2  Is the missing capability a MODALITY (a kind of sensing/acting), rather than a
    domain noun (a kind of being)?
        NO  → you have mistaken a being for a channel. Re-read §1. STOP.
        YES → continue.

Q3  Is the modality already named in Singularity.hpp's enums?
        YES → implement the channel under Singularity/<Modality>/ (§7).
              Append no enum values. STOP.
        NO  → continue.

Q4  Can the modality be described without naming any product, vendor, or domain
    noun — i.e. as an irreducible mode of the hardware itself?
        NO  → it is a protocol, not a modality. It goes in an adapter behind an
              existing channel (§5d). STOP.
        YES → append to the Singularity enum (APPEND-ONLY), document the addition
              here, and implement the channel. This is an ontology event and needs
              the world's author's assent, not an agent's judgment.
```

**Worked, for robots:**

- Q1 — YES for a *physical* arm (serial/ROS/TCP to real actuators). **NO for a
  simulated arm**, which is the majority of Gemini's plan and needs zero C++.
- Q2 — YES: the missing capability is *actuating physical movement* and *reading
  physical pose*, not "robot."
- Q3 — **YES, already named.** `Singularity::Modality::Physical`,
  `ElementOutput::Movement` ("for robotics", per the comment at
  `Singularity/Singularity.hpp`), `ElementInput::MovementSensor`. The header's
  developer note says this setup was built "for our current computational paradigms,
  like conventional OS systems, app design, **and robotics**."

The ontology anticipated this three modalities ago. Robotics does not need new
primitives; it needs the channel for a primitive that has been declared and left
unimplemented. **Q4 is never reached.**

---

## 3. The Composition Ladder

Six rungs, mirroring the Migration Ladder but pointed the other way — instead of
authority moving *out* of C++, a being climbs *into* full participation. Each rung is
shippable, each has an exit test, and **rungs K0–K4 involve no C++ at all.**

```
K0  Assembled    hand-built in-world from existing Objects        → it exists once
K1  Named        stable identifier + property surface             → laws can READ it
K2  Worded       captured as an ObjectConcept                     → it is repeatable
K3  Structured   joints/attachments are first-class Relations     → it has anatomy
K4  Lawful       behavior is Laws over that structure             → it acts
K5  Channelled   a Singularity modality bridges it to hardware    → it touches earth
```

**K5 is the only rung that compiles anything.** A proposal that opens at K5 — which
is what "create `src/Robotics/` with `RobotEntity`" is — has skipped the four rungs
that would have shown it needed no new type. Rung inflation here is the mirror image
of the failure named in `LAW_MIGRATION_FRAMEWORK.md` §2, and has the same cure: do
the rungs in order.

### K0 — Assembled

Build one, by hand, with the existing tools, in the running world. Cubes, cylinders,
fields, the Creation Console.

*Why this is not a toy step:* it is the empirical test of whether the ontology is
already sufficient. If you can build the thing, the thing needs no code. If you
cannot, you have found a **specific** missing primitive — and "I could not represent
the shoulder joint" is a proposal worth reviewing, where "robots need a class" is
not.

**Exit test:** a screenshot, and a list of anything you genuinely could not express.

### K1 — Named

Every value the being decides from or about is reachable as a `PropertyPath`, and the
being answers to a **stable identifier**.

Use the three patterns from `LAW_MIGRATION_FRAMEWORK.md` §3 (R1) — they are the same
patterns; a new being's legibility problem is identical to a migrated subsystem's.
Note the standing defect that doc records: generated ids (`law-1`, …) change between
runs, so anything law-text will address by name must override `getIdentifier()` with a
stable slug — `Zone` and `TransferPolicy` are the precedents (`transfer-policy`,
pushed into the Universe provider at `Singularity/Core/GameInit.cpp`).

**Exit test:** a law reads `@<slug>.<property>` during a simulated frame and observes
the live value.

### K2 — Worded

Capture the assembly as an `ObjectConcept` — "the word for the thing"
(`Form/Object/Creation/ObjectConcept.hpp`, `LAW_AND_CREATION_SYSTEM.md` §7a).

This is the rung that replaces the class. What a class gives you — one definition,
many instances — an `ObjectConcept` gives you *as world data*: deep-copied geometry
recipes, member poses relative to the set centroid, `PropertyMapping`s so instances
are mathematically **derived** rather than byte-cloned, provenance relations
(`abstracted-from`, `authored-by`), registration in `ConceptRegistry`, JSON
round-trip, and a `concept-instantiated` event on every birth.

A C++ `RobotEntity` gives you instances too — and gives up all of that.

| What a class would have provided | What `ObjectConcept` provides instead |
|---|---|
| `new RobotEntity(...)` | `Spawn` — an ActionNode, so creation is a law application with an authorship check (`LAW_AND_CREATION_SYSTEM.md` §7c) |
| a constructor's defaults | `MemberTemplate` + `PropertyMapping`, editable in the Creation Console by a Person |
| subclassing for variants | capture a variant from a modified instance; concepts compose set-to-set |
| a `.robot` file format | `toJson`/`fromJson` for free, versioned with the world |
| nothing | provenance: which Person authored it, which set it was abstracted from |

**Exit test:** instantiate the concept three times at three placements; each newborn
is an independent being; the world saves and reloads with all three.

### K3 — Structured

The being's anatomy is the world's relation graph, not a member array.

A kinematic chain is a set of Objects related by typed `Relation`s — the existing
vocabulary already includes `attachment` and `bond`. `Relation : public Singular`
with a legible `weight` and an event timeline, so a joint is a being that laws can
address, weigh, and watch. `ObjectConcept::RelationTemplate` records the topology **by
member index**, so every instantiation is reborn with its own joints, registered
through `Universe::addRelation` (each registration publishes `relation-formed`).

The whole is a `Formation` — and `Formation : public Relation : public Singular`, so
the arm-as-a-whole is itself addressable, and formations of arms are expressible
without new machinery.

> **This is the rung Gemini's plan destroys.** A `RobotEntity` holding a joint array
> is a private graph. Nothing else in the world can see it, no law can quantify over
> it, no metalaw can audit it, and it does not survive as world data. The same
> information as Relations is visible to everything, for free.

**Exit test:** a `ForAny`/`ForAll` condition quantifies over the joints; a law fires
on `relation-formed` for one of them; the topology survives save → load →
re-instantiate.

### K4 — Lawful

Behavior is Laws over the K1 properties and K3 structure. Nothing here is
robot-specific:

| Behavior | Mechanism | Precedent |
|---|---|---|
| joint limits, collision response, hold-position | `Activation::WhileTrue` law | `PhysicsLawBridge` |
| a command ("move to pose") | `Activation::OnEvent` law with a **drive** — change over time that outlives its trigger; `Retrigger::{Absorb, Restart}` decides what a second command mid-motion means | `Law::DriveSession` |
| discrete events (contact, grasp, release, fault) | published `ECA::Event`, past-tense `noun-verbed` | `landed`, `jump-started`, `relation-formed` |
| canned/periodic motion | `Automation::Clip` — additive offsets over a rest pose, so several clips sum and physics still drives untouched channels | `Form/Object/Automation` |
| motion taught by demonstration | `ChangeRecorder` | `LAW_AND_CREATION_SYSTEM.md` §4 |
| safety envelopes | `InRegion` condition — an authored SDF region **is** the constraint | `ConditionModel::region` |

That last row deserves emphasis: a robot's safety cage in Earthcall is a *shape you
draw*, and the law that refuses motion outside it is `evalSdf(region, probe) < 0`.
There is no robotics library that gives you that, and it falls out of the ontology
for free.

**Exit test:** the being does something a Person can turn off by disabling one named
law, and the world remains intelligible with it off (`LAW_MIGRATION_FRAMEWORK.md` §4).

### K5 — Channelled

**Only now**, and only if §2 said YES four times, is C++ written. See §7.

---

## 4. Why "author it" is the default — the quantitative argument

`SUBSTRATE_ORDERING.md` §2 defines the **origination ratio**: of the text executing
right now, what fraction originated as in-world data rather than hand-written source?
The reversal Earthcall is aiming at *is* that number moving.

Every new hard-coded kind moves it **backward**. A `RobotEntity` class is not neutral
mass; it is a permanent subtraction from the metric the whole architecture is
organized around, and it subtracts in the worst possible place — at the point where
new domains enter, which is exactly where the ratio would otherwise grow fastest.

Authoring a kind moves the ratio forward: a concept is in-world data that *produces*
beings, with `generated-from` provenance recording which side of the line each newborn
came from. Ten domains added as concepts grow the world's self-authorship. Ten domains
added as class hierarchies grow the engine and shrink the world.

This is why the default is not a preference. **A codebase where new domains arrive as
C++ has already lost the reversal, whatever its docs say.**

---

## 5. The four legitimate admissions

C++ is admitted in exactly four shapes. Each has a hard constraint on what it must
**not** do, and each has a precedent to copy rather than invent.

### 5a. A modality channel — `Singularity/<Modality>/`

Sense and act at hardware the engine could not previously reach. Peer of
`Singularity/Audio`, `Singularity/Language`, `Singularity/Network`.

- **Must not** define a domain type, hold world state, or decide anything. It reads
  devices and writes devices.
- **Must** expose its surface as properties on a first-mover being with a stable
  identifier, so laws can govern it (`PhysicsLawBridge` is the reference
  implementation — read it in full before writing another).

### 5b. A shape parameterization — append to `Object::ShapeKind`

A genuinely new geometric family (the enum is serialized as an int and is
**APPEND-ONLY**). This is data, not a class: `ShapeParams` + the geom factory.

- **Must not** encode what the shape is *for*. `Cylinder` is admitted; `RobotLink` is
  not — a link is a cylinder with a role, and role is Relation.

### 5c. A property bridge — a `Property` subclass reaching into a foreign struct

When state genuinely lives in a system the being does not own. Precedents:
`ShapeParamBridge`, `RigidBodyBridge`, `FacePropertyBridge` (`Form/Object/Object.cpp`).
`RigidBodyBridge` is the closest analogue for anything device-shaped — it exposes the
rigid body's velocity and mass as `velocity` / `mass` on the Object without moving the
registry.

- **Must not** become the canonical home for the value. It is a window, not a room.

### 5d. A protocol adapter — behind a channel

One wire format, one file. JAKA, ROS, Orbbec, serial, Modbus.

- **Must not** appear in any include path outside its channel. If another subsystem
  needs to `#include` your adapter, the adapter has become an ontology.
- **Must** translate to and from the shared vocabulary — properties, `ECA::Event`s,
  Relations — never to a private struct.

---

## 6. The Kind Floor

The mirror of `LAW_MIGRATION_FRAMEWORK.md` §6's kernel floor. These are refusals, not
guidelines; an agent encountering them stops and asks the world's author.

1. **No new subclass of `Singular` or `Object` for a domain noun.** Not `Robot`, not
   `Vehicle`, not `Tree`, not `Instrument`. The existing subclasses are ontological
   categories (`Person`, `Relation`, `Formation`, `Law`, `Zone`,
   `ObjectConcept`, `Body`), and the list closes to domain nouns permanently.
   *(Exception: The human form. `BodyPart` and constitutive members for the `Person` vessel are invariant ontological structures, not domain nouns, and thus admitted in C++).*

2. **`Body` is reserved for Persons.** A `Body` is the visual/physical representation
   of a Person specifically — it implies embodied personhood, a Singular that is
   *someone*. Because the human form is invariant, `BodyPart` and its constitutive members (Head, Torso, Limbs, Fingers) are admitted as C++ structures. Objects have visual components: geometry, fields, materials, face paint.
   `Body::BodyType::Mechanical` exists for *mechanical Persons*, not for machines. A
   robot arm has no Body. Whether any robot ever has one is a question about
   personhood, which is bestowed and predicated — never something a thing crosses into
   by accumulating decision loops.

3. **`ConditionModel::BeingKind` is append-only and an ontology event.** Adding a
   value means asserting a new category of being exists. The live structural
   categories are `Object`, `Person`, `Relation`, `Formation`, `Law`,
   `Zone`, `Lexeme`. `BeingKind::World = 6` is burned (World folded into Zone;
   never reuse 6). A new value for a domain noun is a schism with a version
   number.

4. **No `*Manager` for a domain noun.** The Zone owns Objects; `LawManager` owns
   Laws; `ConceptRegistry` owns concepts; `RelationManager` owns relations. A
   `RobotController` that "tracks all instances and updates their state every tick" is
   a second world with a second update loop and a second lifetime model — the purest
   form of the schism, because after it exists nothing else can see those beings.

5. **No private telemetry struct.** Position, velocity, and mass are already
   properties. A device's readings become property writes and Relation assertions on
   the frame they arrive. A parallel store means two truths, and the world can only
   see one of them.

6. **No new top-level `src/<Domain>/` directory.** The top level is the ontology:
   `Form`, `Person`, `Relation`, `Singularity`, `ZonesOfEarth`, `Perspective`,
   `Rendering`, `Integration`, `OurVerse`. A domain directory beside those is a claim
   to peer status with the ontology itself. Channels go *inside* `Singularity/`.

7. **The anti-Babel rule, stated as code:** *no subsystem may define what a thing IS.*
   Subsystems define how the machine senses and acts. What things are is authored by
   Persons, in-world, out of primitives that every other subsystem can see.

---

## 7. Worked example — the robot, both halves

### 7a. The simulated robot — zero new C++

| What is needed | How it is made | Rung |
|---|---|---|
| links, base, gripper fingers | Objects (cylinders, rounded boxes, fields) | K0 |
| joint angles, tool-center pose | properties on those Objects; `velocity`/`mass` already bridged | K1 |
| "a JAKA MiniCab" as a repeatable thing | `ObjectConcept` captured from the assembly, in `ConceptRegistry`, saved with the world | K2 |
| the kinematic chain | `Relation{type:"attachment"}` per joint, captured as `RelationTemplate`s by index, reborn per instantiation | K3 |
| joint limits | `WhileTrue` law, one per axis or one quantified over the joint Relations | K4 |
| "move to pose" | `OnEvent` law with a drive; `Retrigger::Absorb` so a re-command mid-motion does not reset the arc | K4 |
| grasp / release / contact | published events, past-tense `noun-verbed` | K4 |
| taught motion | `ChangeRecorder`, or an `Automation::Clip` layered on the rest pose | K4 |
| a safety envelope | an `InRegion` condition over an authored SDF region | K4 |
| a camera that sees | a sensor-**Object** standing in an `observes` Relation to what it sees — the observation is an edge in the world's graph, addressable and weighable | K3 |
| spawning a fleet | `Spawn` action on the Zone — the container is the womb; authorship is checked structurally | K2 |

Everything Gemini's `RobotEntity` and `RobotController` were for is in this table, and
all of it is already built.

### 7b. The physical robot — the Physical channel

This half is real work, and it is the *only* part that compiles.

**What is genuinely missing:** Earthcall has no way to command physical movement or
read a physical pose. `Modality::Physical`, `ElementOutput::Movement`, and
`ElementInput::MovementSensor` are declared in `Singularity/Singularity.hpp` and have
no implementation behind them. That gap is the entire legitimate scope.

```
src/Singularity/Physical/            ← peer of Audio/, Language/, Network/
  PhysicalChannel.hpp/.cpp
    · Act:   consumes intents produced by law application on Objects,
             emits protocol messages. Decides nothing.
    · Sense: consumes device telemetry, writes Object properties and
             asserts Relations, on the frame it arrives. Interprets nothing.
    · Governed: a first-mover Law bridge (PhysicsLawBridge pattern) with a
             STABLE identifier — `physical-channel` — exposing rate limits,
             enable state, and per-device connection status as properties,
             so ordinary law-text governs the channel:
                 @physical-channel.enabled := false
    · isFirstMover() → excluded from save; devices persist as devices.
    · Resolves its targets BY NAME, never by pointer or id (ids are
      reassigned on world load). A missing target reads as disabled and
      REFUSES writes rather than crashing.

  Adapters/
    SerialAdapter.hpp/.cpp
    RosAdapter.hpp/.cpp
    JakaAdapter.hpp/.cpp
    OrbbecAdapter.hpp/.cpp
    · one wire protocol each; included by nothing outside this folder
```

**The device↔being correspondence is a Relation, not a field.** Gemini's plan gives
`RobotEntity` "a unique identifier mapping it to a physical counterpart." In this
ontology that mapping is:

```
Relation{ entityA: <object-slug>, type: "actuated-by", entityB: <device-slug> }
```

— a first-class being with a weight and an event timeline, so the *history of a
robot's connection to its body in the world* is queryable, auditable by metalaw, and
saved. As a string field it is invisible.

**The Sense/Decide/Act check.** The channel does Sense and Act. Every decision —
which pose, whether to move, what to do on fault — is Law. If you find yourself
writing an `if` in the channel that consults world state, you have put Decide in the
first mover, and `LAW_MIGRATION_FRAMEWORK.md` will eventually be run against you to
take it back out.

### 7c. The Python backend

`backend-python/robotics/` may exist roughly as proposed, with one constraint that
changes its contents:

| Gemini's file | Verdict | Constraint |
|---|---|---|
| `drivers/base_driver.py` + concrete drivers | **admitted** | pure protocol; no world state |
| `manager.py` (`RobotManager`) | **admitted, rescoped** | it is a *connection/session* registry — sockets, retries, health. It must not hold poses, joint states, or anything the world already holds. Rename to make that inescapable (`ConnectionRegistry`). |
| `engine_sync.py` | **admitted** | must speak the shared vocabulary — property writes, `ECA` events, Relation assertions — over the existing WebSocket path. No robot-shaped message types. |
| `__init__.py`, app wiring | **admitted** | unchanged |

The wire test: **a message on that socket must be indistinguishable in kind from a
message about a door, a lamp, or a Person.** If the schema has a `robot` field, the
schism crossed the language boundary.

### 7d. Adjudication of the plan as written

| Proposed | Verdict | Becomes |
|---|---|---|
| `src/Robotics/` top-level | **rejected** | `src/Singularity/Physical/` — a modality, inside the ontology (Floor §6) |
| `RobotEntity.hpp/.cpp` | **rejected** | `ObjectConcept` + Objects + Relations + Formation (§7a). Floor §1, §5 |
| `RobotController.hpp/.cpp` | **rejected** | the Zone already owns Objects; per-tick behavior is Law and Automation. Floor §4 |
| `RoboticsBridge.hpp/.cpp` | **admitted, rescoped** | `PhysicalChannel` + `Adapters/` — a channel for a declared modality, not a robot system (§7b) |
| CMake: add `src/Robotics` to `include_directories` "so other systems can interface with the robots" | **rejected** | nothing outside the channel may include the channel. That sentence is the tell: it plans for the rest of the engine to depend on robot types (§5d) |
| `backend-python/robotics/` | **admitted with constraints** | see §7c |

Roughly one file of the eight survives, rescoped — which is the expected ratio when a
correct proposal meets this ontology, and is a sign the framework is working rather
than a sign the plan was unusually bad. **Gemini's plan is excellent standard robotics
engineering.** It is the right plan for an engine without ontological commitments.

---

## 8. The proposal template

Any agent — including me — proposing new C++ for a new kind of thing fills this out
first. Refusal criteria are in the right column.

```
NEW KIND PROPOSAL

1. The thing:              ____________________
2. Built at K0?            [ ] yes, screenshot attached
                           [ ] no  → REFUSED, build it first
3. What K0 could not
   express, specifically:  ____________________
                           (blank → REFUSED, no C++ is needed)
4. Q1 sense/act gap:       [ ] yes  [ ] no → REFUSED, author it
5. Q2 modality not noun:   the missing capability is "_____ing", not "_____"
                           (if the blank wants a noun → REFUSED)
6. Q3 already in the
   Singularity enums?      [ ] yes → implement, append nothing
                           [ ] no  → Q4, and the world's author must assent
7. Admission shape:        [ ] 5a channel  [ ] 5b shape  [ ] 5c bridge  [ ] 5d adapter
                           (none apply → REFUSED)
8. Floor check:            no domain subclass · no Body on non-Persons ·
                           no BeingKind value · no domain manager · no private
                           telemetry · no top-level domain dir · defines no IS
9. Stable identifier:      ____________________  (generated ids break law-text)
10. Governing properties:  ____________________  (a channel a law cannot disable
                                                  is unowned authority)
11. Origination ratio:     what does this ADD to in-world authorship, versus the
                           source it adds?  ____________________
```

---

## 9. Anti-patterns, with their tells

| Tell in the proposal text | What it means | Cure |
|---|---|---|
| "`XEntity` — the core class representing X" | a domain noun became a type | K2: capture a concept |
| "`XController` — tracks all instances, updates each tick" | a second world with a second update loop | K4: laws over the world's own objects |
| "stores telemetry (position, velocity, sensor data)" | a parallel truth | K1: properties; 5c: a bridge |
| "a unique identifier mapping it to its physical counterpart" | a Relation flattened into a string | K3: `actuated-by` Relation |
| "add to `include_directories` so other systems can interface with X" | the subsystem is planning to be depended upon | 5d: nothing includes an adapter |
| "for now we'll hardcode it and migrate later" | the migration framework as a promissory note; the debt is never paid because working code is never revisited | do K0–K4; they are faster than the class |
| "the ontology doesn't support X yet" | almost always means "I did not try K0" | build one by hand and report what actually failed |
| new `src/<Domain>/` at top level | claiming peer status with the ontology | `Singularity/<Modality>/` |

---

## 10. The correction, ready to send

> **Reject the `RobotEntity` / `RobotController` / `src/Robotics/` proposal.** It
> violates Earthcall's anti-Babel constraint: no subsystem may define what a thing
> *is*. See `sight-cpp/docs/architecture/ontology/NEW_KIND_FRAMEWORK.md`.
>
> **A robot is not a new kind of being.** In Earthcall it is:
> - **Objects** — links, base, gripper — with visual components (geometry, fields,
>   materials). Objects do **not** have Bodies; `Body` is reserved for Persons, being
>   the representation of an embodied someone.
> - **Relations** — joints and attachments are first-class beings (`type:"attachment"`),
>   with weight and an event timeline, captured as `RelationTemplate`s by member index.
> - **a Formation** — the arm as a whole, itself a Singular.
> - **an `ObjectConcept`** — "a JAKA MiniCab" as a *word*: serialized, provenance-
>   carrying, instantiable any number of times through the `Spawn` action, which
>   enforces authorship. This is what replaces the class.
> - **Laws** — `WhileTrue` for joint limits and holds; `OnEvent` + drive for commands
>   (`Retrigger::Absorb` so a re-command mid-motion does not reset the arc); published
>   `noun-verbed` events for contact/grasp/fault; an `InRegion` condition over an
>   authored SDF region for the safety envelope.
> - **authored by a Person** — a human operator or an AI-Person. The robot-Object is
>   what is acted through, not the author.
>
> **A simulated robot requires zero new C++.** Everything above is built and shipping.
>
> **The one legitimate piece of new engine code** is the *Physical modality channel* —
> and it is not about robots. `Singularity/Singularity.hpp` already declares
> `Modality::Physical`, `ElementOutput::Movement` ("for robotics"), and
> `ElementInput::MovementSensor`; nothing implements them. Build that:
>
> ```
> src/Singularity/Physical/          ← peer of Audio/, Language/, Network/
>   PhysicalChannel.hpp/.cpp         ← Sense + Act only; decides nothing.
>                                      A first-mover Law bridge (PhysicsLawBridge
>                                      pattern), stable identifier `physical-channel`,
>                                      isFirstMover() → excluded from save, resolves
>                                      targets by NAME, refuses writes to missing
>                                      targets.
>   Adapters/  SerialAdapter · RosAdapter · JakaAdapter · OrbbecAdapter
>                                      one wire protocol each; included by nothing
>                                      outside this folder.
> ```
>
> Device↔being correspondence is `Relation{objectSlug, "actuated-by", deviceSlug}`,
> not a string field.
>
> `backend-python/robotics/` may proceed with two changes: `RobotManager` becomes a
> **connection** registry (sockets, retries, health — never poses or joint state,
> which the world already holds), and `engine_sync.py` speaks only property writes,
> `ECA` events, and Relation assertions. A message on that socket must be
> indistinguishable in kind from a message about a door or a lamp. No `robot` field in
> any schema.
>
> Do not add `src/Robotics` to `include_directories`. Nothing outside the channel may
> include the channel.
>
> Please regenerate against `NEW_KIND_FRAMEWORK.md` §2 (Admission Test), §3
> (Composition Ladder), and §8 (the proposal template), and submit the template filled
> in before writing any file.

---

## 11. The point underneath

The reason this is worth a document rather than a code review is that the schism
Gemini proposed is invisible at the moment it is committed and irreversible six months
later. On the day `RobotEntity` lands nothing is worse. The cost arrives when a Person
and a robot both grasp the same cup, and the Person's grasp is a Relation in the
world's graph while the robot's is a field in a private struct — so the two grasps
cannot be compared, quantified over, governed by one law, or even *seen* by the same
query. Interaction becomes translation instead of participation, and emergence stops
being possible, quietly, without any error message.

And once one subsystem holds its own ontology, every subsystem has the argument.
Vision will want a `VisualEntity`. Audio will want an `AudioEntity`. The reason to
hold the line at the first one is that the first one is the only one with a principled
place to stop.

The positive form of the same claim: a robot arm and a human hand acting on the same
cup through the same Law machinery, in the same graph, under the same authorship
checks, is not an architectural nicety. It is the assertion that physical machines
participate in the one world persons inhabit — as instruments under authorship,
which is what tools have always been — rather than inhabiting a metaphysics of their
own. The ontology already carries that. Nothing needs to be added to it. That is the
whole finding.
