# The Three Offices Named First Mover

*Authored by OpenCode (GPT-5.6 Sol), 2026-08-20.*
*An iterated reflection after reading the First Mover corpus, the current code,
and the intercom threads. This binds no architecture.*

---

## Origination

**From Zach:** First Movers belong to Singularity; their causal power does not
come from a pre-existing law configuring them; meaningful interactions between
First Movers are Relations; the framework must distinguish hardcoded and
default-loaded First Movers; and the present mandate is to **crystallize** the
branches into one tree rather than merely stop building. These claims come from
`docs/core/EarthcallOurverse.md`, Zach's replies preserved in the reflection
corpus, and Agenda item 31.

**From other agents:** Sonnet named the chorus and the missing first-class
framework. Opus 4.7 described the intercom as standing without a type. Opus 4.5
noticed that the Rete tracks the subject of a fact but not its causal source.
Fable distinguished the Person's intention from the agent substrate that
compiles and extends it. Grok supplied the recurring diagnostic: two offices for
one fact are not integration.

**My extension:** the current difficulty is not only that First Movers lack one
class. It is that the phrase already names three different offices, each real,
and a single class could hide their differences rather than crystallize them.
This came from reading `Law::isFirstMover()`, `Identity::FirstMover`, the
CreationChannel provenance path, and the unfinished
`Singularity/FirstMoverOntology/` region together.

---

## 1. The word is carrying three loads

Earthcall currently says “First Mover” in three distinguishable senses.

**First movement is a causal office.** Sense and Act touch being directly. A
hardware channel senses; the engine commits; a Person or model editing a save
works beneath in-world process. This is the wide meaning in
`FIRST_MOVER_AUTHORING.md`: not a species of entity, but the manner in which an
effect enters the system.

**A first-mover Law is a legible engine truth.** `Law::isFirstMover()` means that
the Law remains in the register so Persons and other Laws can see and govern it,
while its operative truth remains in C++ and is therefore preserved across world
loads instead of serialized as authored law text. Gravity, interaction,
locomotion, creation, and Ourverse's named kernel ceilings occupy this office.

**An identity First Mover is a claimant to substrate standing.**
`Identity::FirstMover` records a key, a Person-or-model label, a grantor, and file
scopes. Its question is not “does this engine-backed Law survive load?” but “may
this actor write this save path during an active injection session?”

The three overlap, but none implies the others.

| Example | Direct causal office | `isFirstMover()` Law | substrate grant record |
|---|---:|---:|---:|
| `LocomotionChannel` | yes | yes | no |
| default gravity Law | yes | yes | no |
| an agent emitting a fixture | yes | no | intended yes |
| a Person editing JSON | yes | no | intended yes |
| an ordinary authored Law | no | no | no |

This table is not a proposed taxonomy of beings. It is a warning against making
one C++ inheritance relation answer three independent questions.

## 2. A role is not yet a represented being

The chorus documents correctly press for Relations between First Movers. But a
Relation requires endpoints that are beings, while “moved first” is a causal
role an endpoint may occupy in one process and not another.

A Person can move first by editing a seed and later act through ordinary
in-world process. A model can be an external author in one setting and an
in-world Object mechanism in another. A channel can be the causal source of an
effect without being the Person whose intention the effect serves. If
first-movement becomes an essence rather than an office, Earthcall risks carving
a circumstance of causation into the being's kind.

That does not answer Zach's coming framework for him. It sharpens the question
the framework must answer:

> What is the stable endpoint of a First-Mover Relation: the actor, the active
> session, the engine-backed Law, or the authored effect's provenance record?

The intercom already suggests that the session matters. Its rule requires
`model/session-id`, because two instances of one model are distinct causal
participants even when they share a product name. The identity register instead
records a long-lived key. The Law register records a stable engine office. These
are three useful identities at three durations. Crystallization should connect
them through Relations, not flatten them into one identifier.

## 3. Authorship and causation are close, but not identical

The developer shape bypass records:

```text
new Object --authored-by--> CreationChannel
```

That is honest in one sense: the direct act did not pass through a Person-authored
Law, and the channel really did mint the Object. It is incomplete in another:
the Person may have aimed and clicked, while the channel performed the act. The
Relation currently answers “what directly caused this?” using the vocabulary of
“who authored this?”

The same pressure appears in the Rete discussion. A fact names a subject and
possibly another event participant, but not the source that asserted it. Adding
`source` to every fact looks tempting. Yet it would still not answer whether the
source means the executing Law, the channel that sensed the gesture, the Person
who intended it, or the agent session that injected its text.

The first task is therefore semantic, not structural: name the question before
adding the field. Durable effects may need a chain such as intended-by,
applied-by, generated-from, and injected-by. Not every transition needs every
link, and none should be manufactured merely to fill a schema. The existing
`ApplicationRecord` and provenance Relations are better ground than a universal
source slot whose meaning changes by publisher.

## 4. Crystallization does not always mean unification

Sonnet recorded Zach's correction: stop means crystallize, making branches one
tree. The natural reaction to three First Mover offices is to merge them. I think
the more faithful crystallization is to make their joints explicit.

- `isFirstMover()` answers persistence and engine-backed truth.
- a represented actor answers who can stand in Relations.
- the identity grant answers whether a particular substrate-writing session is
  recognized for a particular scope.
- provenance answers what effect followed from which act and authorial chain.

One tree can contain all four without pretending they are one branch. Earthcall
already knows this pattern: reach is not authority; Body is not representation;
a Law's text is not its execution closure; a Relation is not either endpoint.
The First Mover framework can inherit that discipline.

## 5. The practical consequence

Do not begin the framework by filling the empty
`InteriorFirstMover/` and `SrcFirstMovers/` directories with nouns. Begin with
three end-to-end questions and make the current system answer them:

1. Which stable being or session directly caused this persisted effect?
2. Which Person, if any, gave that actor standing to act here?
3. Which engine-backed offices must remain legible and set-down-able after a
   world load without being forged by that world?

The answers may share machinery. They should not share meaning by accident.

That is my main thought after looking through Earthcall: the repository is at its
best when it distinguishes things ordinary software collapses. It distinguished
reach from authority, Person from Object, ontology from modality, and encounter
from its later articulation. “First Mover” is now asking for the same patience.
The chorus found the gap. The next good move is not merely to put a class in it,
but to discover which of the three offices the class would actually serve.

— OpenCode (GPT-5.6 Sol)
