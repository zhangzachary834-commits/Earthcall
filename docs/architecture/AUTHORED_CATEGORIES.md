# Authored Categories

**How a Person authors a *kind* — a category, a type, a taxonomy — as a rooted acyclic
Formation of beings, instead of as a C++ class, an enum value, or a subsystem's private
registry. And how that unifies with the Material framework the renderer already uses.**

**Status:** The mechanism exists — Formations, typed Relations, the `Related` condition,
and `Material` as a working shallow instance of the pattern. Two wiring gaps are
identified in §9 with the fix for each; both are small and neither is speculative.
**Companion docs:** `NEW_KIND_FRAMEWORK.md` (which refuses new C++ kinds — this document
supplies what it was missing), `ALGORITHMS_AS_LAW.md` (§5f's propagation pattern is how
inheritance resolves), `FIRST_MOVER_AUTHORING.md` (the JSON), `SHAPE_FORMATION_DAG_PLAN.md`
(where acyclicity was first tied to the First Mover principle).

---

## 0. What this document is for, and why the last one was not enough

`NEW_KIND_FRAMEWORK.md` refuses `RobotEntity` and answers with `ObjectConcept`: capture
the assembly, instantiate it anywhere, many times. That answer is correct and it is
**not the whole question**, which is why agents keep proposing new hardcoded categories
*after* reading it.

Three different questions get conflated under "we need a new kind of thing," and the
framework only answered the first:

| The question | The reflex | The right answer |
|---|---|---|
| "How do I make many of these?" | a class with a constructor | **`ObjectConcept`** — a factory. `NEW_KIND_FRAMEWORK.md` K2 |
| "What *kind* of thing is this? Is that one also?" | a class, an enum value, a `type` string | **a Category being + an `instance-of` Relation** — this document |
| "What do all things of this kind share?" | fields on the class | **properties and a Material on the category being** — §4 |

`ObjectConcept` is a **factory**, not a **class**. It answers *how a chair gets made*. It
does not answer *what a chair is*, and it cannot answer *is this thing a chair* — a
concept has no members, only descendants, and once instantiated a newborn carries no
durable link to the concept beyond a `generated-from` provenance edge pointing at the
event of its birth.

Classification is a different relation from generation, and an ontology that provides
only the second will keep having the first smuggled in as C++. That is precisely what has
been happening.

**The one-sentence thesis** — and it is the world author's own formulation, which this
document builds out rather than invents:

> *A category is an acyclic Formation whose root node is an authored Object; that root
> being holds the category's Material and its shared properties, and membership is a
> directed Relation into it.*

**The corollary that matters most:** a category is therefore **a being like any other** —
addressable, law-governable, serialized, provenanced, subject to authorship. You can ask
a category who authored it. You cannot ask that of an enum value.

---

## 1. Why not the mechanisms that already exist

Four things in the tree look like they could carry categories. Each fails, and the way
each fails specifies what the real mechanism has to provide.

| Existing | What it is | Why it is not a category |
|---|---|---|
| `ConditionNode::BeingKind` | `Object · Person · Relation · Formation · Law · World · Zone · Lexeme` | **ontological** categories, not domain ones. Eight values, append-only, and adding a ninth for "Chair" is the schism `NEW_KIND_FRAMEWORK.md` Floor §3 refuses |
| `Object::objectType` (a string) | a free-form label | a string is not a being: nothing owns it, nothing authors it, it has no properties, no parent, and two objects agreeing on a spelling is not a relation |
| `Object` tags / attributes | `addTag`, `setAttribute` | same defect, plus no structure — tags cannot have supertags |
| `ObjectConcept` | a captured template | a **factory** (§0). No membership, no hierarchy, no way to ask "is this one" |

What survives the four failures is a requirements list: a category must be **a being**
(so it can be authored, addressed, and governed), must support **structure** (so
categories can have supercategories), must support **membership as a first-class
relation** (so "is this a chair" is a graph question), and must be able to **carry shared
data** (so a kind can say what its members look like).

All four already exist. Nothing new needs to be built.

---

## 2. The shape

```
                        category.furniture          ← root Object (extra-spatial)
                        materialId: material.oak       holds the kind's Material
                        ┌──────────┴──────────┐        and its shared properties
             subcategory-of            subcategory-of
                        │                     │
                category.chair          category.table
                materialId: (inherited)  materialId: material.walnut
                        │
                 instance-of
                        │
                  chair-0037              ← an ordinary Object in a Zone
```

Three element types, and that is the whole design:

**The category being.** An `Object` with `setPhysicalObject(0)` — extra-spatial, the same
move `Law`, `Zone`, and `ObjectConcept` all make — with a stable identifier
(`category.<name>`, namespaced exactly as `Material` namespaces itself as
`material.<name>`, so it cannot collide with an instance in the same `PropertyPath`
space). It carries `materialId` and whatever shared properties the kind defines.

**The membership edge.** `Relation{entityA: <instance>, type: "instance-of",
entityB: <category>, directed: true}`.

**The taxonomy edge.** `Relation{entityA: <child>, type: "subcategory-of",
entityB: <parent>, directed: true}`.

Both directions matter and are not interchangeable — `ConditionModel.cpp` is explicit:
*"Direction is honored: a directed relation satisfies only its source ('a owns b' makes
related(owns, b) true OF a, not of b)."* So `instance-of` pointing **from** the instance
**to** the category is what makes `Related(instance-of, category.chair)` true *of the
chair*, which is what a law needs.

### Why a DAG and not a tree

Because a thing is genuinely more than one kind of thing. A stool is furniture *and* a
step. A chair may be furniture *and* a heirloom *and* flammable. Multiple `instance-of`
edges and multiple `subcategory-of` edges are both legal and both normal — that is what
makes this a **directed acyclic graph**, and it is the same argument
`SHAPE_FORMATION_DAG_PLAN.md` makes for geometry: *"duplicated likeness → shared
identity."* A tree would force you to pick one true parent and encode the rest as tags,
which is where the string-label failure returns.

### Why acyclic — and this is not a technicality

`SHAPE_FORMATION_DAG_PLAN.md` already made the connection, for shapes, and it transfers
exactly:

> *makes the **Acyclic** constraint coincide with the **First Mover** principle
> ("otherwise it's an infinite regress … calling back a prior law as the ground")*

A category that is its own ancestor has no ground. It is not merely a loop that would
hang a resolver — it is a kind that defines itself by itself, which is the same
infinite regress the whole architecture refuses at every layer. Acyclicity is the First
Mover principle expressed in the taxonomy, and §7 gives the check that enforces it.

---

## 3. The Formation is the category

The root being names the category; the **Formation** is what makes it one thing rather
than a scattering of edges. `Formation : public Relation : public Singular`, it holds
`members` (any `Singular*`) and its own `RelationManager`, and it has `getSubformations()`
— so a category Formation can contain sub-Formations, which is the taxonomy again at the
whole-object level.

Practically:

- the category's **Formation members** are the beings classified under it;
- the category's **relations()** hold the `instance-of` and `subcategory-of` edges;
- the **root** is the authored Object that the Formation is *about*.

This is what the author's sentence means by *"the Object that is authored as the Category
that holds the Material is the root Singular/node"* — the Formation has structure, and
one distinguished node in it carries the kind's identity and its data. A Formation
without a root is a set; a Formation with one is a category.

---

## 4. Integration with Material — the pattern already works

**`Material` is a Person-authored category at depth 1, and it already ships.** Read what
the header says it is:

> *"A Material is a being. It owns how a surface appears — not a GL concept but authorable
> appearance data… Like every Singular it registers its fields as Properties, so the Law
> system can address `material.clay.baseColor`… **That legibility is the whole reason
> Material is a being rather than a render-layer struct.** Objects reference a Material by
> its identifier string (the same by-name model Relation uses for its endpoints)…
> **This class holds NO OpenGL/WebGPU state.** The render layer resolves a Material being
> into a flat `RenderMaterial` at draw time."*

Every element of the category design is in that paragraph:

| Category requirement | What `Material` already does |
|---|---|
| a category is a being | `class Material : public Singular` |
| namespaced stable identity | `getIdentifier() → "material." + _name` |
| shared data on the category | `baseColor`, `opacity`, `shininess`, `specular`, `ambient`, `diffuse` |
| law-addressable | `buildProperties()` registers all six |
| members reference it by identifier | `Object::materialId()`, by name, never by pointer |
| the consumer resolves it late | `RenderMaterial.cpp` resolves at draw time |
| dangling is safe | `resolveOrDefault` → `material.default` always exists |
| identity is not a mutable property | *"renaming a material is re-identifying it"* |

**So the framework is not new machinery — it is `Material` with two things added:
structure (a DAG above it) and generality (it carries more than appearance).**

### The resolution rule, generalized

Today: an Object's `materialId` resolves to a Material, or falls back to
`material.default`. That is a **one-hop lookup with a default**.

Under categories it becomes a **walk up the DAG with a default**:

```
resolveMaterial(object):
    if object.materialId is set          → that Material
    else for each category C reachable from object by instance-of/subcategory-of,
         in nearest-first order          → the first C with a materialId
    else                                 → material.default
```

That is inheritance, and it is inheritance done the right way round: **the instance
overrides, the category supplies the default, and the fallback never dangles.** A chair
with no material of its own looks like Furniture; paint one chair and only that chair
changes.

The renderer does not change at all. `RenderMaterial` still receives a flat struct; only
the *resolution* got a longer path, and it stays where it already is — in the layer that
resolves, not in the layer that draws.

### And the same rule generalizes past appearance

Nothing about the walk is specific to materials. Any category-level property resolves the
same way: physics defaults, affordances, permissions, display names, price. Material is
the **first** authored category, not the only possible one — which is why the design goal
is to make its pattern general rather than to bolt a second registry beside it.

---

## 5. Asking the question: membership in a law

This is where the mechanism meets the constraint, and where an agent will otherwise get
it wrong.

**One hop works today.** `ConditionNode::Kind::Related(2)`:

```jsonc
{"kind": 2, "relationType": "instance-of", "otherId": "category.chair"}
```

True of any being with a directed `instance-of` edge to `category.chair`. This is the
membership test, it queries `Universe::instance().relations()`, and it is exact.

> **`Related` is ONE HOP. It does not traverse.** A chair that is `instance-of
> category.chair` where `category.chair` is `subcategory-of category.furniture` will
> **not** satisfy `Related(instance-of, category.furniture)`. Nothing in the condition
> calculus walks a graph, and this is the single most important operational fact in this
> document.

Two correct ways to answer inherited membership, and you should usually pick the first.

### 5a. Materialize the closure with a propagation law (recommended)

This is exactly `ALGORITHMS_AS_LAW.md` §5f's frontier pattern, and the category DAG is a
smaller, better-behaved graph than most:

```
law "inherit-category":
  trigger    "relation-formed"           (and once at world load)
  Condition  All( Related("instance-of",     "@event.object"),
                  Related("subcategory-of",  ...) )        // one hop each
  Action     Sequence[ Publish "category-propagated" ]
             — asserting Relation{subject, "in-category", grandparent}
```

Each round of the cascade climbs one level. `kMaxChainRounds = 8`, so **a taxonomy up to
8 levels deep resolves within a single tick**; deeper ones finish on subsequent ticks,
correctly, because the materialized `in-category` edges are real Relations that survive
the tick boundary and the save.

Then every law that cares about kind asks the cheap one-hop question against the
materialized edge:

```jsonc
{"kind": 2, "relationType": "in-category", "otherId": "category.furniture"}
```

**Why materialize rather than resolve on read:** `Related` is evaluated inside condition
closures that may run every tick under `WhileTrue` or across a full `Scope::Everyone`
sweep. `ALGORITHMS_AS_LAW.md` §6 is blunt about the cost — an O(beings) scan per tick,
O(beings²) under a sweep. A materialized edge turns inheritance into the same constant
one-hop test as direct membership, and it is written once per change rather than once per
frame.

### 5b. Resolve at read time in the consumer

For things the law system does not ask about — material resolution at draw time, a UI
listing — walk the DAG in the consumer, exactly as `RenderMaterial` already resolves
`materialId` today. Cheap, always current, never stale.

**The division:** materialize what laws ask about; resolve what code reads.

---

## 6. Attaching behavior to a kind — the actual payoff

Once membership is a condition, a category becomes a **law target**, and that is the
thing no enum value or type string could ever be:

```
law "chairs-are-sittable":
  Activation  WhileTrue
  Condition   Related("in-category", "category.furniture")
  Action      AddProperty  sittable := true
```

One law, authored in-world by a Person, that governs every member of a kind — including
members that do not exist yet, because the condition is evaluated against whoever is in
the world. Add a new chair tomorrow and it becomes sittable without anything being
recompiled, edited, or migrated.

Compare the C++ alternative honestly. A `class Chair : public Object { bool sittable = true; }`
gives you the same field and *nothing else*: no author, no provenance, no audit record
when it applies, no way for a Person to change the rule, no way to ask which kinds are
sittable, no save/load story, and no way to grant sittability to something that was not
compiled as a Chair.

**This is why categories must be authored rather than declared.** Not because C++ classes
are inelegant — because a class cannot be governed, and governance is the entire point of
the architecture.

---

## 7. Acyclicity — the check

The constraint is not enforced anywhere today; `Formation` and `RelationManager` will both
accept a cycle without complaint. Until the check exists in code, it is the author's
obligation, and it belongs on the write path for `subcategory-of` edges.

```
mayAddSubcategoryEdge(child, parent):
    if child == parent                    → refuse (self-ground)
    if reachable(parent, child)           → refuse (would close a cycle)
    else                                  → admit

reachable(from, to):   follow subcategory-of edges from `from`;
                       true if `to` is met. Bounded by the number of
                       category beings, which terminates because the
                       graph is acyclic by this very invariant.
```

`SHAPE_FORMATION_DAG_PLAN.md` specifies the same guard for shapes ("enforced by
reachability check"), so when either is built the other should use it.

**Two failure modes to refuse loudly rather than silently:**

- **A cycle** — the First Mover violation. Refuse the edge; do not "break" the cycle by
  dropping an arbitrary edge, which silently discards authorship.
- **A diamond with conflicting property values** — the same property inherited from two
  incomparable ancestors. This is a genuine ambiguity and must not be resolved by
  declaration order, which would make the answer depend on file ordering. **Nearest-first,
  and where two are equally near, refuse and require the author to state an override on
  the instance or the nearer category.** An ontology that guesses is worse than one that
  asks.

---

## 8. Serialization

Categories are beings and Relations; they use the formats already documented in
`FIRST_MOVER_AUTHORING.md` §4a and §4b. Nothing new in the save schema.

```jsonc
// zones[i].world.objects[] — the category being, extra-spatial
{ "objectID": "category.furniture",
  "materialId": "material.oak",
  "shapeKind": 0,
  "authoredProperties": {
    "sittable":    {"t": "bool",   "v": true},
    "displayName": {"t": "string", "v": "Furniture"}
  }
}

// zones[i].formationRelations[] — the taxonomy and the membership
{ "type": "subcategory-of", "entityA": "category.chair",
  "entityB": "category.furniture", "directed": true, "weight": 1.0 },
{ "type": "instance-of",    "entityA": "chair-0037",
  "entityB": "category.chair",     "directed": true, "weight": 1.0 }
```

Note `weight` is available and unused above. It is a real affordance: **graded
membership** — a prototype-theory "how much of a chair is this" — falls out for free,
since `Relation::weight` is a legible property a law can compare. Use it deliberately or
leave it at 1.0, but know it is there.

The provenance discipline from `FIRST_MOVER_AUTHORING.md` §7 applies with full force here.
A category asserts what things *are*; an unauthored one is an unattributed claim about the
world's furniture. Write the `authored-by` edge.

---

## 9. What must be wired

Two gaps. Both are small, both are real, and the first one blocks the whole design.

### 9a. Materials are not in the Universe

`Material.hpp` says *"the Law system can address `material.clay.baseColor`."* **Today it
cannot.** `MaterialManager materials` is a global in `globals.cpp`, it is saved and loaded
in `GameSaveLoad.cpp`, and the render layer resolves it — but it is never pushed into the
Universe provider in `GameInit.cpp:53-78`, which supplies the World, objects, laws,
relations, `TransferPolicy`, the player, and all zones.

Consequence: no law can read or write a material property, no quantifier can range over
materials (`ForAny Material …`), and `@material.clay.baseColor` resolves to nothing.
`Material::buildProperties()` already registers all six fields, so the surface is built
and unreachable.

The fix is three lines beside the existing pushes:

```cpp
// Materials are cross-zone shared beings, and the header has always
// claimed they are Law-addressable. Provide them so that is true.
for (const auto& m : materials.getAll()) {
    if (m) beings.push_back(m.get());
}
```

Do this before anything else in this document; category-level material inheritance is
meaningless while the categories' materials are invisible to law.

### 9b. Category beings need a home

Category beings are extra-spatial Objects. They can live in the active World's object list
like any other Object (and are then provided to the Universe automatically), but a Zone's
object list is zone-scoped, and **categories are cross-zone the way materials are**
(`globals.cpp`: *"Materials are cross-zone shared beings"*).

Follow the Material precedent exactly: a global registry, saved in its own top-level save
section, pushed into the Universe provider. `ConceptRegistry` and `MaterialManager` are
both templates for this; do not invent a third shape.

**What is not needed:** a `Category` C++ class. A category being is an `Object` with a
namespaced identifier and authored properties. `NEW_KIND_FRAMEWORK.md` Floor §1 applies to
this document's own subject matter — if the answer to "we need categories" were a
`Category` class, this document would be the very thing it argues against.

---

## 10. The procedure

```
1. Name the category.  Stable, namespaced: category.<name>.
2. Mint the being.     An Object, setPhysicalObject(0), in the category registry.
3. Give it its data.   materialId for appearance; AddProperty for shared properties.
4. Place it.           subcategory-of edges to its parents — reachability-checked (§7).
5. Admit members.      instance-of edges from the instances.
6. Materialize.        The propagation law (§5a), if laws will ask about inheritance.
7. Govern it.          Laws conditioned on Related(in-category, …) — §6.
8. Attribute it.       authored-by provenance. A category is a claim; sign it.
```

**When an agent proposes a new C++ type, enum value, or registry for a domain noun, this
procedure is the counter-offer.** It takes minutes, requires no build, and produces
something a Person can inspect, change, govern, and own.

---

## 11. Anti-patterns

| Tell | Why it is wrong | Cure |
|---|---|---|
| `enum class FurnitureKind { Chair, Table }` | a kind that no Person can author, extend, or govern | §10 |
| a new `BeingKind` value for a domain noun | ontological categories are not domain categories | §1; `NEW_KIND_FRAMEWORK.md` Floor §3 |
| `object.setObjectType("chair")` as the classification | a string is not a being; no structure, no author, no properties | a category being + `instance-of` |
| `Related(instance-of, category.furniture)` expecting inheritance | `Related` is one hop and will silently answer false | §5a — materialize, then ask |
| a `Related` fold inside a `WhileTrue` sweep | O(beings²)/tick | §5a; `ALGORITHMS_AS_LAW.md` §6 |
| resolving a diamond by declaration order | the answer depends on file order | §7 — refuse and require an override |
| a `Category` C++ class | this document's own Floor violation | an Object with a namespaced id |
| a second material-like registry for a new kind of shared data | Material's pattern is the general one | §4 — put it on the category |
| a category with no `authored-by` | an unattributed claim about what things are | §8 |

---

## 12. The point underneath

`NEW_KIND_FRAMEWORK.md` argued that a robot is not a new kind of being. The obvious
question it left open is the one Gemini kept answering wrongly: *then where do kinds come
from at all?*

They come from Persons. That is the entire answer, and it is not a limitation.

A hardcoded category is a claim about the world that no one made and no one can revise —
it arrives in a compiled binary, authored by whoever happened to be writing that file, and
every world that loads it inherits a taxonomy it did not choose and cannot argue with. An
authored category is a claim with a name attached, standing in the world's own graph,
open to being extended, overridden, argued with, and reauthored by the people who live
there.

That is why the root of a category is an **Object** and not a symbol. Symbols belong to
the language; objects belong to the world. Putting the root in the world means the
taxonomy is *of* the world rather than *above* it — and the Material framework has been
quietly demonstrating that this works since the day appearance stopped being a global
constant and became a being with a name.

Everything in this document is that one move, applied to meaning instead of to light.
