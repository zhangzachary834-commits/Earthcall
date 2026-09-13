# Relation semantic identity and constitutive opcodes — 2026-09-13

## Human direction

Zach identified the architectural issue that motivated this work, beyond the earlier `Person is not Object` string guards:

1. `Person` and `Object` are already separate C++ branches under `Singular`; later string reconstruction must not bypass that ontology.
2. A Relation cannot derive its meaning from an arbitrary spelling. Instances of one semantic Relation kind must share one meaning, while two independently authored Relation kinds may legitimately share the same human-readable spelling.
3. Therefore Relation-kind semantic identity must follow the unique identity of the kind-being, not its display/string label.
4. Some Relation meanings are constitutive invariants rather than arbitrary author interpretation. Zach specifically proposed an opcode that exposes C++'s inheritance checker as the authored constitutive substance of an `instance-of` Relation.
5. Zach also corrected the earlier Person/Object guard: a mere string/name collision must not be punished. Person identity protection requires provenance tied to the Person's unique identity, not a matching display name.
6. Zach connected the longer-term cross-Zone semantic unification problem to Ourverse: independently authored meanings can remain distinct locally while an Ourverse-level gathering/filament substrate orders them toward shared unity rather than collapsing them by spelling.

## What landed in the integration superbranch

Branch: `integration/unmerged-superbranch-2026-09-12`.

### Relation kind identity

`Relation` already supported a `Lexeme* _typeLexeme`, but the constructors and `setTypeLexeme()` immediately collapsed it to `Lexeme::symbol`. That meant two different kind-beings both spelled `owns` became the same `Relation::type`, the same `Relation::getIdentifier()`, and one merged edge in `RelationManager`.

Grounded Relations now store the type Lexeme's stable Singular identifier in `Relation::type`. `typeLabel()` supplies the human-readable symbol. Legacy string-only Relations remain readable and retain their historical string identity until migrated.

Serialization now writes:

- `type`: readable compatibility label
- `typeId`: grounded semantic kind identity, when present

Hydration resolves `typeId` back to the Lexeme being and restores `_typeLexeme`.

### Parser no longer destroys semantic identity

`SyntacticParser::resolveMeaning()` now returns the meaning `Lexeme*`, not its symbol string. Parsed Relations are constructed through the Lexeme-typed constructor, so the semantic being survives the parser boundary.

### Constitutive opcode substrate

Zach's C++ inheritance idea is implemented without inventing a second type checker. `ConditionNode::matchesKind()` already is Earthcall's dynamic_cast-backed C++ `instanceof` primitive.

A grounded Relation-kind Lexeme may carry authored dynamic property:

- `relation.constitutiveOpcode = CppInheritance`

The relation's endpoint B may carry:

- `cpp.beingKind = ConditionNode::BeingKind::<...>`

`Relation::evaluateConstitutive()` delegates to the existing `ConditionNode::matchesKind(*endpointA, kind)`.

`RelationManager::add()` treats the opcode as genuinely constitutive, not decorative:

- `Holds` -> ordinary graph admission continues
- `NotApplicable` -> ordinary Relation semantics continue
- `Violated` -> refuse the Relation
- `Invalid` -> refuse transparently rather than guess

This means two Relation-kind Lexemes may both display `instance-of`, while only the one whose unique identity carries `CppInheritance` has C++ inheritance truth conditions.

This is deliberately distinct from authored domain classification (`chair --instance-of--> category.chair`). Domain classification is not silently reinterpreted as C++ inheritance merely because its label is the same.

### Person/Object identity guard corrected

The earlier CategoryManager guard treated a registered Person profile filename/display token such as `Zach` as sufficient proof that an Object with identifier `Zach` was counterfeit. Zach correctly rejected this: two beings may share a human-readable name.

The guard now consults the profile's serialized Person-grade `personId` and refuses only an Object attempting to reuse that unique identity string. The direct test now verifies:

- Person display name `Zach` and Object identifier `Zach` may coexist lexically.
- the Person's key-grade unique identity may not be reintroduced through CategoryManager as an Object.

The historical `basic_pixel_changer` world test was also corrected so it no longer encodes the false rule that the spelling `Zach` itself reserves Person identity. That save remains migration evidence; it has an ambiguous legacy author token and was NOT rewritten.

## Regression witnesses

`relation_retry_lexeme_test` now covers:

- grounded Relation type uses Lexeme ID, label remains available separately;
- same spelling + different Relation-kind IDs stay distinct and do not merge;
- serialization round-trips `typeId` and restores the type Lexeme;
- `CppInheritance` evaluates Holds for an Object against an Object descriptor;
- the same constitutive Relation evaluates Violated for a Lexeme against that descriptor;
- another same-spelled authored `instance-of` kind without the opcode is unaffected.

`logos_modality_test` now requires parser-produced Relations to keep their meaning Lexeme and `typeId` through serialization/hydration.

Astra's `scratch/probes/language_meaning_probe.cpp` was updated because its old diagnostic deliberately reproduced the now-fixed collision and string-typed parser behavior. It now witnesses distinct semantic IDs and grounded parser types instead of treating the bug as expected current behavior.

Focused GitHub CI now builds/runs `relation_retry_lexeme_test`, `logos_modality_test`, and `person_not_object_test` alongside the existing focused witnesses.

## Deliberate compatibility boundary / remaining work

This is the first coherent rung, not a claim that every historic Relation in Earthcall has been migrated.

Still outstanding:

1. Many legacy Relations are created from raw strings and therefore still use compatibility string identity. They should migrate incrementally to grounded Relation-kind beings rather than be bulk-reinterpreted by spelling.
2. Consumers such as parts of Physics, Law/Rete indexing, UI category rendering, Ourverse filament code, and generators still name legacy relation labels directly. Do not mechanically replace those with one global Lexeme: each semantic kind needs explicit identity/provenance.
3. The historical author-smuggling pattern (`"Zach"` as an Object/category-side referent so Law author lookup can find a name) remains migration debt. The name guard was intentionally weakened because it was ontologically wrong; the correct cure is to migrate author references to actual Person/First Mover identities with provenance.
4. No checked-in save was rewritten in this work. Save files are sacred and need owner-authorized migration.
5. Ourverse-level cross-Zone gathering/unification of Relation-kind semantics is not implemented here. Zach's insight is recorded as the architectural direction: do not collapse meanings by label merely to obtain global sameness.
6. `cpp.beingKind` currently addresses the existing irreducible C++ ontology through the already-serialized `ConditionNode::BeingKind` substrate. Do not extend that enum for domain nouns; authored categories remain authored beings and Relations.

## Architectural sentence

A Relation's spelling may tell a Person what it is called; its kind-being identity tells Earthcall which meaning it is, and any constitutive opcode on that kind-being tells the engine what invariant operation must actually be true for an instance of that Relation to exist.
