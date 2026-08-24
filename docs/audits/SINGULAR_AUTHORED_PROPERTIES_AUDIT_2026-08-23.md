# Authored properties on a base Singular — audit (2026-08-23)

**Asked by Zach:** whether the innate capacity (whether gated or not) exists to
author an arbitrary number of properties of any kind into a base Singular.

**Sources:** `docs/core/EarthcallOurverse.md` (Property — hardcoded vs
Person-originated, and the bounds that "need" to exist), `Singular.hpp` /
`Singular.cpp`, `PropertyValue.hpp`, `ActionNode::AddProperty`,
`TransferPolicy`, `DataStructure.hpp`, Object/Relation/Material/Person
serialization, `LawGraphWindow` AddProperty editor.

**Probe:** `scratch/probes/singular_authored_properties_probe.cpp` — 59/59.
Not a reading of the headers. The probe constructed Relation, Material, Object,
and Person, granted five hundred names, stored every `PropertyValue`
alternative, fired `AddProperty` with TransferPolicy closed, and round-tripped
JSON.

**Companion refusals:** this is not a request for a new C++ class, a new enum
of property kinds, or a second permission system. The question is whether the
substrate already has the slot the manifesto describes.

---

## 0. Verdict

**Yes. The innate capacity exists, on `Singular` itself, ungated.**

A being you can construct — Relation is the thinnest — already carries
`std::map<std::string, PropertyValue> _dynamicProperties`. There is no engine
cap on how many names go in. Every alternative of `PropertyValue` can sit in
that map at runtime. `ActionNode::AddProperty` writes it. `setDynamicProperty`
writes it. A `Set` of a missing single-segment name mints it.
`TransferPolicy` does not consult the write. The manifesto's bounds object
(`DataStructure::writeBounds`) exists and is never evaluated.

`class Singular` is abstract (`getIdentifier` and `buildProperties` are pure
virtual). There is no being that *is only* a Singular. The capacity is
inherited, not a special Object feature. The probe granted properties onto
Relation, Material, Person, and Object through the same two methods.

What fails is not existence. It is *keeping* what was granted, *listing* it,
and *meaning "any kind" the way the manifesto said it*.

---

## 1. What Zach asked, and what the manifesto already claimed

From `EarthcallOurverse.md`, in Zach's words, immediately after defining
Singular:

> There are two kinds of properties:
>
> Hardcoded properties—innate fields that are designed after conceptually
> innate things to something’s being.
> Person-originated properties—members of any primitive, Earthcall-native,
> default, or external class type added to a data structure. This needs
> bounds as we don’t want Objects to simply get bloated with person authored
> properties.
> Bounds—Person-authored constraints on the conditions upon which the list
> of properties may be changed.

And the question sitting under that:

> I’m wondering whether this statement is accurate: “Under the hood,
> hardcoded properties are the OOP paradigm. Authored properties use Entity
> Component System. Hardcoded properties are innate to the Singular classes
> themselves while authored properties are data that operates as properties
> in real time.”

The audit is that paragraph, asked of the tree: does a base Singular already
have an open authored vocabulary, of any kind, in any number — even if some
later gate would close it.

---

## 2. Claim-by-claim

### 2a. The slot is on Singular, not on Object — **held**

`Singular.hpp` carries the map, `setDynamicProperty` / `getDynamicProperty` /
`removeDynamicProperty` / `dynamicProperties()`, and `DataStructure` beside
it. Every inheritor gets them: Object, Person, Relation, Formation, Material,
Zone, Law (Law is an Object).

Verified: Relation, Material, Person, and Object each accepted a grant in the
probe. Relation is the nearest thing the tree has to a "base Singular" you
can actually construct.

### 2b. Arbitrary number — **held, ungated**

`setDynamicProperty` is:

```cpp
void Singular::setDynamicProperty(const std::string& name, const PropertyValue& v) {
    _dynamicProperties[name] = v;
}
```

No count, no size check, no `writeBounds` consult, no TransferPolicy consult.
The probe put 500 names on one Relation; first and last still read. The only
bound is memory. The manifesto's "we don’t want Objects to simply get bloated"
is a wish the tree does not enforce.

### 2c. Any kind — **held inside a closed variant, not as "any C++ type"**

`PropertyValue` is a closed `std::variant` of eighteen alternatives:

| Kind | Runtime store | Survives `propertyValue` JSON |
|---|---|---|
| `monostate` | yes | as `{"t":"none"}` |
| `int` `float` `double` `bool` `char` `long` | yes | yes |
| `std::string` | yes | yes |
| `glm::vec3` `glm::mat4` | yes | yes |
| `PropertyList` / `PropertyDict` (nested, recursive) | yes | yes |
| `Singular*` `Object*` `Relation*` `Formation*` | yes | writes `{"t":"ref","id":...}`, loads as `monostate` |
| `OntoMath::ScalarField` / `VectorField` | yes | writes a type tag with no payload, loads empty |

The probe stored all eighteen on one Relation at once.

What the manifesto named and the variant does **not** admit:

- an external C++ class type that is not one of those alternatives
- `Person*`, `Material*`, `Law*`, `Zone*`, `Body*` as themselves (a Person
  can be stored only by upcasting to `Singular*`)
- a new kind of value without appending the variant (append-only, same
  discipline as the enums)

Nested `PropertyDict` / `PropertyList` make the *structure* of a value
arbitrary, still of those kinds. `PropertyPath` does not walk into a dict:
`bag.leaf` is `NoSuchProperty` even when `bag` is a dict with `leaf` inside.
A dotted *name* as a single flat key (`acoustic.amplitude`) does work —
longest-match in `findProperty` — and that is how chess and sound-emitter
authoring already store namespaced properties.

So "any kind" is true of the closed currency, false of "any type a Person
could name in C++."

### 2d. Gating — **authoring is not gated; transfer is a different door**

Three things look like gates. Two of them are not on this write.

| Mechanism | What it actually governs | Does it stop AddProperty / setDynamicProperty? |
|---|---|---|
| `TransferPolicy` | set-to-set *capture* (`ObjectConcept::captureState`) | **No.** Probe closed `warmth` and AddProperty still granted it. |
| `DataStructure::writeBounds` | manifesto "Person-authored constraints on the conditions upon which the list of properties may be changed" | **No.** The field exists. Nothing reads it. Grant ignores it. |
| `AddProperty` itself | empty name, unproven owner, shadowing a registered first-mover name | **Yes**, those three. Not count, not kind, not TransferPolicy. |

A `Set` of a missing **single-segment** name also mints (`PropertyPath::setValue`
calls `setDynamicProperty`). A missing **multi-segment** path refuses
(`NoSuchProperty`) and does not mint. That is a shape of address, not a
governance gate.

The Law Graph editor is a surface bound, not an innate one: the AddProperty
panel is a 64-character name and an `InputDouble`. Law *text* (JSON
`operand` via `propertyValueToJson`) can grant any round-trippable kind.
The editor cannot.

### 2e. A granted property is as real as a first-mover one — **held for law, not for listing or copy**

`Law::couldApplyTo` treats an authored name as a real requirement. `findProperty`
builds a `DynamicPropertyBridge` so `PropertyPath` reads and writes it.
Arithmetic coercion on the bridge matches the registered path (the
`Map`-into-bool bug recorded on the to-do list).

What the comment on `Singular.hpp` claims and the tree does not keep:

> A law-added property is a real part of the being, so it has to be
> ENUMERABLE (the authoring UI offers it beside the registered vocabulary)
> and PERSISTABLE.

**Enumerable:** `listProperties()` walks `_propertyRegistry` only. An authored
name appears there only after `findProperty` has cached a bridge. The probe:
grant `"secret"`, `listProperties` misses it; call `findProperty("secret")`,
then it appears. Capture (`ObjectConcept::captureState`) *does* walk
`dynamicProperties()` directly, so set-to-set is not blind. The authoring UI
that offers `listProperties()` is.

**Copy:** `Singular(const Singular&)` copies `_telosId` and nothing else.
Relation copy (implicit) therefore drops the authored map and keeps telos.
Object is not copyable (`= delete`), which hides the same hole for Objects.
Zone's user-defined copy also does not take the map.

**Revoke after lookup:** `removeDynamicProperty` erases the map entry and
leaves the cached `DynamicPropertyBridge` in `_propertyRegistry`. The probe:
grant, read through `PropertyPath` (caches the bridge), revoke, re-grant —
the re-grant is refused as "would shadow first-mover" because `findProperty`
still finds the stale bridge. The property is gone and cannot be granted
again on that instance.

### 2f. Persistence — **Object and ObjectConcept only**

`LAW_AND_CREATION_SYSTEM.md`: "a granted property that vanished on save was
never really granted."

| Being | `authoredProperties` in its JSON? |
|---|---|
| Object (`Serialization.cpp`) | **yes** — probe restored double, string, list |
| ObjectConcept | **yes** |
| Relation | **no** — `toJson` writes type/weight/ends/events/attachment |
| Material | **no** — appearance fields only |
| Person | **no** — displayName, personId, pose, body |
| Zone / Formation | **no** path found |

Pointer kinds and OntoMath fields do not round-trip even on Object: the JSON
tag is written, `propertyValueFromJson` returns `monostate` for `"ref"` and
has no `"scalar_field"` / `"vector_field"` branch.

Chess and the sound-emitter live as Object `authoredProperties`. That is why
those worlds survive. A property granted onto a Person, a Relation, a
Material, or a Zone is session-only.

### 2g. The OOP / ECS remark — **directionally right, implementation is a map on the being**

Hardcoded properties *are* the class: `PropertyRef` / `ComputedProperty` over
members registered in `buildProperties()`. Authored properties *are* data
attached at runtime without a new class. They are not an ECS. There is no
component type, no archetype, no sparse set. It is a per-being
`std::map<string, PropertyValue>` plus a lazy `DynamicPropertyBridge` so the
same `PropertyPath` walks both vocabularies. That is enough for the
ontological claim (a Person adds a property the type system never knew) and
weaker than the slogan.

---

## 3. What "base Singular" means here

The question is whether you need a more specific class — Object, Person, a
new noun — before a Person can author properties. You do not.

You also cannot mint a being whose C++ type is `Singular`. The four first-order
kinds (Person, Relation, Constructed Being, first-mover / Singularity
interface) are the instantiable surface. The map is on the ancestor they
share. Authoring into "a base Singular" is authoring into that ancestor's
map, on whichever inheritor you have.

The Object-shaped path is the only one that currently *survives a save*. That
is a serialization gap, not a missing capacity.

---

## 4. What is not yet the manifesto

Zach named bounds so Objects do not bloat. `DataStructure` is the named
place: a bounded bag with a `ConditionNode* writeBounds`. Nothing in
AddProperty, Set, or `setDynamicProperty` asks it. Until a Person can author
the condition under which the list may change, the capacity is open in the
way the manifesto feared, not in the way it wanted.

The Law Graph's double-only AddProperty panel is the other unbuilt half of
"any kind": the machine will store a dict, a vec3, a string; the Person-facing
editor will only type a number.

---

## 5. Implied work (not done this session)

1. Persist `authoredProperties` on every Singular that serializes, or say
   honestly that only Objects keep grants. Relation, Person, Material, Zone
   currently vanish them.
2. `listProperties()` should offer the authored map without requiring a prior
   `findProperty`. The comment already requires this.
3. `removeDynamicProperty` should drop the cached `DynamicPropertyBridge`, or
   `findProperty` should not treat a stale bridge as a first-mover. Re-grant
   after revoke is currently a trap.
4. Singular copy is telos-only. Either copy the authored map or delete copy
   the way Object does.
5. `DataStructure::writeBounds` is the manifesto's bound and is dead. Either
   wire it into grant/revoke, or stop presenting it as the property
   framework.
6. Ref identity in `propertyValueFromJson` (`"ref"` → monostate) and
   OntoMath field payloads.

The probe stays in `scratch/probes/`. It is an audit instrument, not a
regression test. The live guards for the Object half are already
`tests/law/law_creation_test.cpp` and `tests/constructed-being/object_roundtrip_test.cpp`.
