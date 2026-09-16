# Second-Nature Law Authoring

**Branch implementation:** `sol/second-nature-law-authoring-zone-20260916`  
**Zone:** `saves/zones/SecondNatureLawForge/zone.json`

## Problem

Earthcall already made Law text serializable and inspectable, but composing that text node-by-node is still too much like programming the engine rather than authoring a world. Repository notes repeatedly name the desired inversion: the Law window should become an inspection surface, while the primary composer should be an authored instrument inside Earthcall itself. The interaction work later sharpened the gesture into: do an example / choose an intent, articulate the rule, inspect its reach, and keep the resulting instrument.

A second architectural note is inseparable from that UX problem: Law creation must not become another bespoke creation subsystem. Zach's To-do directives require Singular set-to-set creation to be the universal creation language, able to create every coherent child kind of `Singular`, rather than growing `CreateLaw`, `AuthorZone`, `CreateLexeme`, etc. `ObjectConcept` is likewise to be retired conceptually: a concept is simply an ordinary Singular from which other Singulars branch.

## Architectural answer

There is **no `LawConcept` class and no new `SingularConcept` class**.

A prototype is an ordinary Singular. A Law used as a prototype is still a Law; an Object used as a prototype is still an Object. The ancestry is represented as `branched-from` provenance, not by placing the prototype in a parallel concept registry.

The universal operation lives at:

- `ConstructedBeing/Singular/Creation/SingularSetToSetCreation.hpp`
- `ConstructedBeing/Singular/Creation/SingularSetToSetCreation.cpp`

Its public shape is one derivation:

```
source Singular set + authored bindings + prototype Singular
    -> newborn Singular
```

The prototype's runtime kind determines the newborn runtime kind. Concrete C++ types still need a mechanical persistence/constructor adapter because C++ has no runtime constructor reflection. Those adapters are substrate metadata, not independent authoring semantics: they must never surface as separate ActionKinds or separate Person-facing creation verbs.

Kernel constraints bound the same operation rather than creating parallel systems. In particular:

- `Person` is not synthesizable: it corresponds to an actual human being.
- a `Relation` must have actual participants / an interaction; an empty Relation is incoherent.
- a `Law` requires real authorship and authored Law text.
- a `Zone` requires its ownership/jurisdiction covenant to survive birth.

The current rung wires persistence codecs for authored `Law` and plain `Object`, and refuses other concrete runtime kinds rather than silently slicing them to `Object`. The next rung is to move the remaining Singular persistence codecs behind the same operation and then make serialized `ActionNode::Create` delegate to it, retiring type-specific creation opcodes where they are merely historical duplicates.

## The Forge Zone

`SecondNatureLawForge` is the first Person-facing consumer.

Its interface is ordinary authored world state:

- `law-forge-state` holds the selected prototype, target, requested newborn name, status, and last-created identity as authored properties.
- `law-forge-tool-gold` and `law-forge-tool-blue` are ordinary Shape2D Objects.
- `law-forge-tool-*-invoke` are ordinary authored Laws. On click they set the prototype/name on `law-forge-state` and publish `law-authoring-instrument-invoked` with the state Object as subject and the demo target as object.
- `law-forge-prototype-click-gold` and `law-forge-prototype-click-blue` are ordinary **disabled Laws**. They are not a new concept type. Their condition contains the explicit `$TARGET` parameter.
- `SecondNatureLawAuthoring` is intentionally thin: it resolves the interface request and passes the prototype/source set/bindings into `SingularSetToSetCreation::derive`.

When the derivation succeeds, the newborn Law:

1. gets a fresh stable identity (`<prototype>.branch-N`),
2. keeps authored condition/action model text,
3. explicitly substitutes declared parameters such as `$TARGET`,
4. inherits or receives authorship and target Formation membership,
5. receives `branched-from` provenance,
6. inherits the prototype's trigger vocabulary,
7. enters the ordinary `LawManager` as an enabled, serializable Law.

Nothing about the newborn is UI-only.

## Why this solves the tedious-law-authoring problem differently

The node graph remains valuable for precision, debugging, audit, and strange Laws. It is no longer required to be the default gesture for common intent. A Person can keep a vocabulary of Law-shaped Singulars and use authored instruments to derive situated Laws from them. The world can therefore teach its own authoring language: more sophisticated Zones can add target pickers, demonstrations, spatial sketches, natural-language articulation, reach previews, or combinations of those without changing the Law engine's ontology.

The important invariant is that convenience stays above truth. The interface may become dramatically easier; the resulting Law remains fully explicit, inspectable Law text underneath.

## Verification contract

`tests/law/second_nature_law_authoring_test.cpp` guards both halves of the architecture:

- the authored-interface adapter derives an executable Law from an ordinary Law prototype, binds `$TARGET`, preserves trigger/authorship, and creates collision-free identities;
- the **same** universal operation derives an Object from an ordinary Object prototype into a Zone.

The second assertion exists specifically to prevent `SingularSetToSetCreation` from quietly degenerating back into a Law-specific factory.
