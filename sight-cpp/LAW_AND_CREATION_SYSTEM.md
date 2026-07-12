# Law Creation & Object Set-to-Set Creation System

**Status:** Design pinned — implementation staged (see Build Order at bottom).
**Companion docs:** `SHAPE_FORMATION_DAG_PLAN.md`, `EVENT_BUS_VS_EVENT_HANDLER.md`, `RELATION_EVENT_SYSTEM.md`, `EarthcallOurverse.md`.

This document specifies how Persons author Laws without touching C++, and how new
sets of Objects are created from old sets. The two systems share one skeleton and
are built in dependency order: **legibility → language → hearing → memory → birth**.

---

## 0. The One Architectural Pivot

The committed Law system (`src/ZonesOfEarth/AuthorsOfLaw/Law.hpp`, `Law.cpp`) stores
conditions and actions as `std::function` closures. Closures cannot be:

- **serialized** — `Law::toJson()` currently saves only *descriptions*; a saved
  world loses all law behavior on load,
- **authored** — a Person cannot mint a closure from inside the world,
- **introspected** — Rete compilation and the authoring UI need to see structure,
- **synthesized** — higher laws require modeling the constituent *process*, and a
  closure has no describable process. A law with no text, only force.

This problem was already solved once, in geometry. `geom::SdfNode`
(`src/Form/Object/Geometry/Sdf.hpp`) stores shapes as **plain data trees** —
serializable, introspectable, editable — and compiles/tessellates on demand. Its
own header says why: *"deliberately plain data (not opaque lambdas) so it can be
serialized, introspected, and edited."*

**Laws make the same move.** Conditions and actions become expression trees over
`PropertyPath`s; closures become derived, compiled artifacts — exactly as
`SdfToken` RPN is derived from `expr`.

```
SdfNode        : shape as data      → tessellateSdf() compiles it for GL
ConditionModel : predicate as data  → compile() emits ECA::ConditionPredicate
ActionModel    : change as data     → compile() emits ECA::ActionExecutor
```

Law is identity (an Object); its models are its essence; the compiled closures are
its manifestation. This is "Law is process/change, Singular/Object is identity"
made literal.

**The first-mover boundary, preserved in the type system:** hard-coded closures
remain legal — but only as first movers (physics, engine-level laws; the threshold
between the C++ Singularity and the Singular-Relation-Formation ontology).
Person-authored laws enter as models. Both compile into the same ECA slots.
First movement is ground; authored law is text.

---

## 1. Stage One — Legibility: the Property Bridge

*A Person must be able to say "this object's height" without writing C++.*

Files: `src/Form/Singular/Property/`

### 1a. Runtime-generic access on `Property`

```cpp
class Property {
public:
    virtual ~Property() = default;
    virtual std::string name() const = 0;
    virtual std::string typeName() const = 0;

    // The door laws walk through.
    virtual PropertyValue value() const = 0;
    virtual bool setValue(const PropertyValue& v) = 0;

    // Non-null if this property's value is itself a Singular
    // (the recursion point for nested paths).
    virtual Singular* asSingular() const { return nullptr; }
};
```

- `PropertyRef<Owner,T>` implements via `if constexpr` over the variant membership
  of `T`; `glm::vec3` decomposes to component sub-properties.
- `ComputedProperty<Owner,T>` finally gets its body: delegate to the getter/setter.
- `PropertyValue.hpp` fixes: add `#pragma once`; replace the local `Vec3` with
  `glm::vec3` (the codebase speaks glm — a private Vec3 means converting at every
  boundary forever).

### 1b. `PropertyPath` — the address of a variable

```cpp
struct PropertyPath {
    std::vector<std::string> segments;          // {"position","y"}, {"shape","majorR"}

    Property* resolve(Singular& root) const;    // walk registry, descend via asSingular()
    std::string toString() const;               // "position.y"
    static PropertyPath parse(const std::string&);
    nlohmann::json toJson() const;
};
```

`resolve` looks up `segments[0]` in the root's `_propertyRegistry`, then descends
through `asSingular()` for each remaining segment. This is the street-address
system for the substrate. It is also why properties must be **registered**, not
merely existing: registration in `buildProperties()` is the Singularity-enforced
categorical wrapping of first-mover properties the manifesto requires.

### 1c. `buildProperties()` implementations

Order of implementation, chosen so Stage 4's recorder can watch what the tools
already touch:

1. `Object` — position, scale, rotation, `ShapeParams` (r, ry, rz, halfH, majorR,
   minorR, paraboloidA, ovoidAsym, fillet), SDF morph `t`, visibility.
2. `Person`, `BodyPart`, `Zone`.
3. `Law` itself — `enabled`, `conditionMode`, numeric leaves of its models.
   (This single registration is what makes Metalaws free in Stage 6.)

The broken sketch in `Singular.hpp` (eight `std::vector<T>*` members, invalid
syntax) is *subsumed* by this: "a Singular can flexibly own any kind of thing"
**is** `_propertyRegistry` + `PropertyValue`. The registry is the mechanism.

**Milestone test:** `PropertyPath::parse("position.y").resolve(obj)->setValue(3.0f)`
moves a cube.

---

## 2. Stage Two — Language: ConditionModel & ActionModel

Files: `src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.{hpp,cpp}`,
`ActionModel.{hpp,cpp}`; `src/Singularity/OntoMath/Function.{hpp,cpp}` grows up.

Copy the `SdfNode` construction deliberately: one struct, enum discriminator,
children by `shared_ptr` with deep-copy value semantics, static factory helpers,
`toJson`/`fromJson`, and a `compile()`.

### 2a. `ConditionNode`

```cpp
struct ConditionNode {
    enum class Kind { Compare, InRegion, Related, All, Any, Not };
    Kind kind = Kind::Compare;

    // Compare
    PropertyPath path;                             // lhs
    enum class Op { Eq, Ne, Lt, Le, Gt, Ge, Near, InRange } op = Op::Eq;
    PropertyValue operand;                         // rhs literal…
    PropertyPath operandPath;                      // …or rhs as another property
    float tolerance = 0.0f;                        // Near
    PropertyValue lo, hi;                          // InRange

    // InRegion — a shape *is* the condition
    geom::SdfNode region;                          // authored with the ordinary shape tools
    PropertyPath probe;                            // point to test (default "position")

    // Related — graph-shaped conditions
    std::string relationType;                      // "touching", "member-of", "authored-by"
    std::string otherId;

    std::vector<std::shared_ptr<ConditionNode>> children;   // All / Any / Not

    nlohmann::json toJson() const;
    static ConditionNode fromJson(const nlohmann::json&);
    ECA::ConditionPredicate compile() const;       // tree → closure, once
};
```

`compile()` is a recursion; each node returns a lambda capturing its compiled
children. `All`/`Any`/`Not` honor the existing `ECA::ConditionMode` semantics.

**Related (landed):** the Universe carries a relation provider (the engine
wires the active zone's Formation relations) and `Related` compiles against
it: true when the subject participates in a matching relation — empty
`relationType` accepts any kind, empty `otherId` means related to ANYONE, and
direction is honored ("a owns b" holds OF a, never of b). No provider = no
proven relations: the condition never passes. Relations also join
`Universe::beings()`, so quantifiers range over them (`ForAny Relation ...`),
and `RelationManager::add` now publishes a string-typed `"relation-formed"`
ECA echo (subject: the newborn relation) that laws can bind as a trigger.

**`InRegion` is projection mode made real.** When a Person wants "when something
enters *here*", they raise the same `ShapeGenerator3D` used to make real objects,
sketch the region, and the tool writes the resulting `SdfNode` into a
`ConditionNode` instead of into the world. Same hands, same gestures, same math —
only the destination differs: being versus criterion. UI cost: one "projection
mode" flag on the tool plus translucent rendering of the region.
`evalSdf(region, p) < 0` is the predicate.

A Law's condition tree **is** its Formation of conditions (the Rete DAG derives
from it — manifesto requirement satisfied structurally).

### 2b. `ActionNode`

```cpp
struct ActionNode {
    enum class Kind { Set, Add, Scale, Lerp, Drive, Sequence, Parallel, Spawn };
    Kind kind = Kind::Set;

    PropertyPath path;                             // what changes
    PropertyValue operand;                         // Set/Add/Scale/Lerp payload

    // Drive — the gradient law: output as a *function* of an input
    CurveModel curve;
    PropertyPath input;                            // domain variable ("time", or any property)

    // Spawn — the object-creation bridge (see §7)
    std::string conceptId;
    std::vector<PropertyMapping> mappings;

    std::vector<std::shared_ptr<ActionNode>> children;      // Sequence / Parallel

    nlohmann::json toJson() const;
    static ActionNode fromJson(const nlohmann::json&);
    ECA::ActionExecutor compile() const;
};
```

`Drive` replaces the fixed IF→THEN with a continuous mapping: "glow brighter *as*
the person approaches," not "glow when near." Discrete actions are the degenerate
case (constant curve behind a condition).

### 2c. `CurveModel` — OntoMath's real job

Replaces the pseudocode in `OntoMath/Function.hpp`:

```cpp
struct CurveModel {
    enum class Form { Constant, Polynomial, Sinusoid, Piecewise } form = Form::Constant;
    std::vector<double> coeffs;                    // c0 + c1·x + c2·x² + …
    double amplitude = 0, frequency = 1, phase = 0, bias = 0;   // Sinusoid
    double evaluate(double x) const;
    nlohmann::json toJson() const;
    static CurveModel fromJson(const nlohmann::json&);
};
```

The sinusoid fields are **exactly `Automation::Track`'s fields** — not
coincidence. Automation was the first pattern-of-change-as-data; `CurveModel` is
its generalization off the transform channels onto arbitrary `PropertyPath`s.

### 2e. OntoMath — authored mathematics (added after commit 8)

Earthcall is driven by math in the program. `Singularity/OntoMath/Expression.hpp`
is the exact symbolic core: `Term` (coefficient · Π varᵢ^expᵢ, real exponents),
`Expression` (sum of terms; exact `plus`/`times`/`normalized` combine-like-terms,
exact `derivative`/`antiderivative` by the power rule), and `Piecewise` (the
manifesto's DiscreteFunctions: open/closed bounds per piece; **undefined outside
every piece — nullopt, never silently zero**). `Operations::hyperop` is the
arithmetic ladder (succession → addition → multiplication → exponentiation →
tetration …).

Two authorable kinds ride the ECA split:
- **`ConditionNode::Kind::Zone`** — satisfied when `f(bindings)` lies in the
  authored `[lo, hi]` zone (either side may be unbounded). *Condition functions
  check if the input satisfies the designated zone inside the function.*
- **`ActionNode::Kind::Map`** — `path := f(bindings)`. *Action functions govern
  how behavior changes by making the output determined by a function on inputs.*

`MathBindings` (variable name → PropertyPath) is the bridge: variables are
Person-authored primitives naming where each value lives on the subject.
Undefined math never fires a condition and never writes a value. All of it —
variables, bindings, coefficients, exponents, piece bounds — is model data:
serialized with the law, edited in the Law Author window.

**Change over time (landed):** Singularity owns a world clock —
`Universe::setClock(now, dt)` set by `Game::update` each frame — legible through
three reserved read-only paths: `time` (seconds since the world began),
`time.delta` (the frame's dt), and `time.sinceApplied` (seconds since THIS law
began holding for THIS subject; per-law-per-subject onset memory, release
re-arms). Binding a math variable to a time path makes an authored expression a
function of time. Two forms:

- **Position form** — `Map` with `t → time.sinceApplied`: `position.y := f(t)`.
- **Rate form** — **`ActionNode::Kind::Flow`**: `path := path + f(bindings)·dt`
  each tick. The authored model is `dp/dt`; OntoMath's exact
  `derivative`/`antiderivative` make Map and Flow exact counterparts.

**Referents are the author's choice — the action phase names its own.** Every
path carries a qualifier saying WHOSE property it is: plain (the law's
subject), `@being-id.` (one specific being, Universe lookup),
`@event.subject.` / `@event.object.` (the triggering event's participants —
a collision has two; the LawManager arms an application-event context around
event rounds, and drive sessions remember their launching participants by id
for their whole life). The event's beings are thus available BY CHOICE among
all others, never a dichotomy. In the Law Author every property picker pairs
WHAT (the property) with an "of ..." combo choosing WHOSE (subject / event
participants / any live being).

**Driving is an authored choice, and any variable can be its domain.**
`Law::drives()` (serialized; legible as the `drives` property → metalaws can
govern it) says the law keeps applying after its trigger (OnEvent or
OnBecomeTrue; WhileTrue re-applies on its own). Each tick the session asks
`ActionNode::definedFor(subject)`: is the function still defined at the
CURRENT values of its bound variables? **The authored Piecewise bounds ARE the
duration, and ANY bound variable may cut them** — time is one input among the
rest (another being's position, the subject's own state). When the function
becomes undefined, the session ends and `"law-drive-finished"` is published
(the law analogue of `automation-clip-finished`). An action with no bounded
function drives until disabled. Empty-input `Drive` now uses the world clock
(the old "commit 4 frame-time" promise, closed).

**Transcendentals (landed):** a `Term` may carry exact `TransFactor`s —
`sin/cos/exp(scale·var + shift)` and `ln(scale·var)` — so periodic,
exponential, and logarithmic change are law-text, not curve approximations.
The calculus stays exact and honest: full product/chain rule on
differentiation (sin/cos/exp closed; `ln` carries no shift so its derivative
`x⁻¹` stays inside the algebra); integration holds `∫x⁻¹ = ln x` (the old
honest gap, closed; defined on x > 0 — ln outside its domain evaluates to
nullopt, never a guess), single sin/cos/exp/ln factors, and
`∫ln(ax) = x·ln(ax) − x`; products needing integration by parts refuse with
nullopt. `Expression::sinusoid(A, f, φ, bias)` is `evalTrack`'s exact form,
so recorded periodic change can retire into the exact core. The Law Author's
term editor multiplies factors in via "+f()" (kind × bound variable) with
scale/shift edited inline.

Growth path: expression-valued exponents (createTerm's recursive "term inside
the exponential space"), integration by parts, richer symbolic simplification
(sin² + cos² = 1 and kin), graph/relation-valued conditions.

### 2d. `Law` owns models

```cpp
class Law : public Object {
    // …existing (authors/conditions/targets Formations, provenance, log)…
    ConditionModel _conditionModel;    // the law's text
    ActionModel    _actionModel;
    void recompile();                  // models → _conditionPredicates / _actions
};
```

- `addCondition(desc, predicate)` / `addAction(desc, executor)` remain — **first
  movers only**.
- `toJson`/`fromJson` serialize the models. *This is the commit where saved worlds
  stop forgetting their laws.*
- **World persistence (landed):** `GameSaveLoad` saves `LawManager::toJson()`
  (laws + the trigger map — LawManager owns trigger truth; Rete alpha bindings
  are derived from it), `ConceptRegistry::toJson()`, and the world clock.
  On load — after the world, so identifiers resolve — `loadFromJson` restores
  the register and reattaches authors/targets BY IDENTIFIER through the
  Universe: an author absent from the world leaves the law Unauthored (it
  cannot fire; authorship is a covenant, not a copy), and the clock never
  runs backward.
- The `Law : public Object, public Relation` sketch is **rejected** (Singular
  diamond; fights `Law.cpp`). The relational aspect is carried by `_provenance`
  and the conditions Formation — composition, the same move the manifesto makes
  for Relation-Objects.

---

## 3. Stage Three — Hearing: close the Event → Rete → Apply loop

The `ReteNetwork` exists but is deaf: nothing feeds it facts, nothing drains its
agenda. Three wires, all in `LawManager`:

```cpp
void LawManager::connectToEventBus() {
    Core::EventBus::instance().subscribe<ECA::Event>([this](const ECA::Event& e) {
        ReteFact fact;
        fact.type      = e.type;
        fact.subject   = e.subject;
        fact.subjectId = e.subject ? e.subject->getIdentifier() : "";
        _rete.assertFact(fact);
        _dirty = true;
    });
}

void LawManager::tick() {                          // once per frame from GameUpdate
    if (!_dirty) return;
    for (const auto& act : _rete.evaluate())
        if (Law* law = find(act.lawId))
            if (Singular* subject = act.token.facts.empty() ? nullptr
                                                            : act.token.facts[0].subject)
                law->applyTo(*subject);
    _rete.drainAgenda();
    clearTransientFacts();                         // event facts live one tick
    _dirty = false;
}
```

**Adapters:** each existing typed event (`ObjectHoverEvent`, PersonEvents,
collision events, `Law::AppliedEvent`) gets a one-line echo publishing a generic
`ECA::Event` with a string `type`. Strings are the deliberate choice: C++ event
*types* are frozen at compile time, but a Person minting a new law must name new
event kinds at runtime ("entered-my-home", "finished-pottery"). The string field
is the runtime-extensible axis; typed events remain the fast first-mover lane.
This resolves "the user connects the Rete activation protocol with first movers,
otherwise programmers update code each time" structurally.

**Fact lifecycle policy:**
- *Event facts* are transient — cleared each tick ("this happened just now").
- *State facts* (property snapshots enabling beta joins like "A touching B AND B
  is red") are asserted on change, retracted when false. Ship transient-only
  first; add state facts with the first join-condition law.

**Two poison defects to fix before laws chain:**
1. `EventBus::publish` holds `_mutex` while invoking listeners
   (`EventBus.hpp` publish path). A law action publishing an event from inside a
   handler **deadlocks**. Fix: copy the listener vector under lock, dispatch
   unlocked. Re-entrant publish is the *normal* case once laws chain.
2. `Formation::getIdentifier()` returns `"Formation"` for every instance. Rete
   bindings and provenance alias into mush. Give Formation a unique counter ID
   (the `Law::initializeLawIdentity()` pattern).

---

## 4. Stage Four — Memory: authoring actions by demonstration

*"The Action is essentially recording the change of designated variables over time
and modeling that change."*

File: `src/ZonesOfEarth/AuthorsOfLaw/ChangeRecorder.{hpp,cpp}`

```cpp
class ChangeRecorder {
    struct Trace { PropertyPath path; std::vector<std::pair<float,double>> samples; };
    std::vector<Trace> _traces;
    Singular* _subject = nullptr;
    float _t = 0;
public:
    void watch(PropertyPath p);
    void sample(float dt);                 // per frame while the Person sculpts
    ActionModel fit() const;               // traces → Drive nodes
};
```

`fit()` tries per trace, in order:
1. **Constant** — variance ≈ 0 → no node;
2. **Linear** — least-squares slope → polynomial `Drive`;
3. **Sinusoid** — frequency estimated by zero-crossings of the mean-subtracted
   signal, then least-squares amplitude/phase/bias.

The experienced flow: *press record → work the vase's rim rhythmically with the
Pottery tool for three seconds → stop → receive a law that breathes the same
rhythm forever.* The demonstration **is** the source code. This is
entire-process capture, not output matching — the fitted curve preserves the
shape of what the Person did, which is exactly what synthesis (§6) consumes.

---

## 5. Stage Five — Authoring UI: the generalized node graph

`GameNodeGraph.cpp` (Mode3D::Graph) already renders and edits an `SdfNode` tree as
in-scene floating cards. Generalize its card-walker behind a small node-provider
interface (label, children, select, edit-panel), then:

- **Law graph:** Event card → Condition cards (the Formation of conditions) →
  Action cards → Target binding via `Tool::PickObject3D` (point at the referent).
- **Concept graph (§7):** member templates and mappings in the same editor.

One editor, three tree types (shape, condition, action). This matches the embodied
UI build order already adopted for shapes: gizmos → node-graph → clay.

---

## 6. Stage Six — Synthesis & Metalaws

**Synthesis** (both manifesto paths, both cheap once models are data):
- *Interpretive path:* compose two laws' ActionModels under `Sequence` /
  `Parallel` nodes; conditions combine under `All`/`Any`. Tree algebra — the
  cross-product homology.
- *Native path:* run both laws on the same referents with the `ChangeRecorder`
  watching; capture the **cumulative** trace; `fit()` a single new model. One
  process, natively owned, no delegation overhead at runtime.

**Metalaws need zero new machinery.** Once `Law::buildProperties()` registers
`enabled`, `conditionMode`, and its models' numeric leaves, a Law whose target is
another Law *is* a Metalaw. The Singularity-level ceiling is one authority check
in `applyTo()` before mutating a law-typed target: lower scopes (Zone permissions)
may govern laws in their jurisdiction but cannot override higher-order Metalaws,
personhood integrity, authorship, or substrate order.

---

## 7. Object Set-to-Set Creation

Files: `src/Form/Object/Creation/ObjectConcept.{hpp,cpp}`, `PropertyMapping.hpp`,
`ConceptRegistry.{hpp,cpp}`

### 7a. `ObjectConcept` — the word for the thing

The manifesto's "extra-spatial Object [that] stores the concept of the object for
later use." Follows Law's own precedent — `Law.cpp` marks itself extra-spatial
with `setPhysicalObject(0)`.

```cpp
class ObjectConcept : public Object {              // extra-spatial
public:
    struct MemberTemplate {
        Object::ShapeKind   kind;
        Object::ShapeParams params;
        geom::SdfNode       field;                 // deep-copies: template is its own being
        glm::vec3 offsetFromCentroid, scale, rotation;
        // material / face-paint refs as they become serializable
    };
    struct RelationTemplate { int a, b; std::string type; float weight; };

    std::vector<MemberTemplate>  _members;
    std::vector<RelationTemplate> _relations;      // captured Formation topology
    std::vector<PropertyMapping>  _mappings;

    static std::shared_ptr<ObjectConcept> captureFrom(
        const std::vector<Object*>& sourceSet, const std::string& name);

    std::vector<std::unique_ptr<Object>> instantiate(
        const glm::mat4& placement,
        const std::vector<Object*>* sources = nullptr) const;

    nlohmann::json toJson() const;
    static std::shared_ptr<ObjectConcept> fromJson(const nlohmann::json&);
};
```

`captureFrom` is the abstraction gesture: the Person selects a set with
`Selection3D` (a set of one is fine) and invokes **Capture Concept**. The system:

1. copies each member's geometry recipe (SdfNode deep-copy semantics guarantee
   the concept is an independent being, not an alias — see `Sdf.hpp` copy ctor),
2. records member transforms **relative to the set centroid** (a concept is
   placeable anywhere),
3. snapshots inter-member Relations from the enclosing Formation,
4. records provenance: `abstracted-from` → each source, `authored-by` → the Person,
5. registers with `ConceptRegistry` (mirror `LawManager`: registry + Formation +
   `toJson` + registered-event).

### 7b. `PropertyMapping` — structure carried across, not bytes cloned

```cpp
struct PropertyMapping {
    PropertyPath source;           // read from source-set member
    CurveModel   transform;        // through a function…
    PropertyPath target;           // …into the new object
    enum class Aggregate { PerMember, Mean, Sum, Max } agg = Aggregate::PerMember;
};
```

This is `Property.hpp`'s promised "source-to-target transformation for object
generation." It elevates creation above copy-paste: *"each new column's height =
1.5× its source's; hue rotated 30°; radius = mean of the whole source set."* The
new set is mathematically **derived** from the old — modal information
transferred, not bytes duplicated.

**Landed (2026-07-11):** mappings may carry an EXACT OntoMath `Piecewise`
transform (`hasExact`/`exact` — bounded domains, transcendentals; undefined
transfers nothing), and the whole experience is authorable in the **Creation
Console** (`Rendering/CreationWindow`): assemble a source set from the 3D
selection, pick and choose which properties carry over from the live set's own
vocabulary, choose redistribution (per member / mean / sum / max) and
transform (as-is / linear / exact f(x) via the shared MathEd editors), then
"Capture concept" and/or "Create objects now" — with "use output as new source
set" chaining set-to-set-to-set. `instantiate` publishes a
`"concept-instantiated"` ECA echo (a birth can wake laws).

**Governed transfer (landed):** every mapping passes the Singularity
**TransferPolicy** gate (`Singularity/TransferPolicy` — permissions root at
Singularity). Three tiers: *Kernel* (position/rotation/center — universally
transferable, laws cannot close), *Governable* (open by default; a law may
close/reopen), *Gated* (governance state like enabled/drives/name/weight —
closed until a law opens it). The policy is itself a LEGIBLE Singular
(identifier `transfer-policy`): each gate is a bool property, so ordinary laws
govern access — `@transfer-policy.gate.shape := false` — with Kernel gates
registered read-only (the anti-tyranny floor). State persists with the world.

**RelationTemplates (landed):** a concept captures the set's STRUCTURE, not
just its members — wherever the world's relation graph relates two source-set
members, the edge is remembered BY INDEX (`RelationTemplate {aIndex, bIndex,
type, directed, weight}`), serialized with the concept, and reborn between
the corresponding newborns on every instantiation, registered into the
world's graph through `Universe::addRelation` (the write side the engine
wires to the active zone's Formation; each registration publishes
`relation-formed`).

**Retrigger (landed):** what a re-trigger means while a drive is running is
authored vocabulary — `Law::Retrigger {Absorb, Restart}` (serialized,
APPEND-ONLY): Absorb keeps the running process's clock (a block resting in
constant collision cannot stack or reset it); Restart makes the new trigger
a new t = 0 (re-kick mid-arc, re-arc). Edited beside the Drive checkbox.

Still ahead (the user's layers 2/4/5): first-mover tools recorded mid-process
(ChangeRecorder thread), cross-kind transference, and set-to-set over
non-Object Singulars (Persons, BodyParts, Zones, Relations, Formations) —
needs a Singular-kind-aware MemberTemplate; the property table and gates carry
over unchanged. Law set-to-set arrives as the special case (Law extends
Object) under Zone-level authorial permissions.

### 7c. `Spawn` — creation *is* a law application

`World : public Singular` (`World.hpp`) makes the unification nearly free. The
`Spawn` ActionNode compiles to:

```cpp
return [conceptId, mappings](const ECA::Event& e, Singular& target) {
    auto* world   = dynamic_cast<World*>(&target);       // law's TARGET is the container
    auto  concept = ConceptRegistry::instance().find(conceptId);
    if (!world || !concept) return;
    for (auto& obj : concept->instantiate(placementFrom(e), sourcesFrom(e)))
        world->addObject(std::move(obj));                // existing ownership path
};
```

A generation law's *target* is the World/Zone that receives the newborn objects —
the container is the womb. `applyTo(world)` runs the full existing gauntlet:

- **authorship check** — `Unauthored` laws cannot create; *nothing enters the
  world without an author*, enforced structurally;
- condition check, `ApplicationRecord` logging, `AppliedEvent` published;
- provenance: `generated-from` relations recorded on each newborn.

Creation gets governance and audit for free because it **is** process, and
process is Law.

### 7d. Self-propagation and the Anti-Babel ceilings

Self-propagation (the embryonic-cells vision) is a generation law whose output
Formation satisfies its own condition. The Singularity-level ceilings that keep
this from Earthchaos/EarthBabel are concrete knobs on this loop, all expressible
as Metalaw predicates over properties Laws already expose:

- **Generation depth** — provenance chains record ancestry; cap traversal depth.
- **Rate ceilings** — spawns per tick per author.
- **Authorship attestation** — a Metalaw refusing `Spawn` where the author chain
  does not terminate in a Person.

Chaos is unbounded recursion; Babel is one law claiming every target. Both are
Metalaw predicates — no new machinery. (The theological Anti-Babel section itself
lives in the Ourverse Manifesto; this is only its enforcement surface.)

---

## 8. Build Order (each step compiles; each has a one-line test)

| # | Commit | Test |
|---|--------|------|
| 1 | **Un-break the tree**: converge `Law.hpp` with the ECA-migrated `Law.cpp`; fold `Singular.hpp` sketch vectors into the property registry; make `OntoMath/Function.hpp` real C++; `#pragma once` + glm in `PropertyValue.hpp` | full build passes |
| 2 | Property bridge: `value()`/`setValue()`, `PropertyPath`, `Object::buildProperties()` | set `position.y` by path — cube moves |
| 3 | Models: `ConditionNode`/`ActionNode`/`CurveModel` + JSON round-trip + `compile()`; port one hard-coded law to a model | save → load → law still fires. **Load-bearing commit.** |
| 4 | Loop closure: ECA::Event adapters, `LawManager::tick()`, EventBus deadlock fix, Formation unique IDs | "object enters region → turns gold" survives save/load |
| 5 | `ChangeRecorder`: watch/sample/fit | record pottery oscillation → sinusoid Drive law |
| 6 | Concepts: `captureFrom`/`instantiate` + `PropertyMapping` + `Spawn` + `ConceptRegistry` | select set → capture → instantiate elsewhere with a mapping applied |
| 7 | Graph UI: generalize `GameNodeGraph` card-walker; law + concept graphs in-scene | author the §4 test law without code |
| 8 | Synthesis + Metalaws: `Sequence`/`Parallel`, recorder-based native synthesis, `Law::buildProperties()`, authority gate | metalaw disables a law's `enabled` by law |

Nothing after commit 3 requires re-architecture: once conditions and actions are
trees of data over addressable properties, every later feature — UI, synthesis,
metalaws, spawning, anti-Babel ceilings — is *operations on trees*, the same way
every shape feature became operations on `SdfNode`.

---

## 9. Known defects this design depends on fixing

| Defect | Location | Why it matters here |
|---|---|---|
| Working-tree `Law.hpp` sketch doesn't compile against `Law.cpp` | `AuthorsOfLaw/Law.hpp` | Commit 1 |
| `Singular.hpp` invalid member syntax (`std::vector<char> characters*`) | `Form/Singular/Singular.hpp` | Commit 1 — intent subsumed by property registry |
| `OntoMath/Function.hpp` is pseudocode | `Singularity/OntoMath/` | Commit 1; grows into `CurveModel` in commit 3 |
| `Law::toJson` persists descriptions, not behavior | `AuthorsOfLaw/Law.cpp` | Commit 3 |
| `EventBus::publish` dispatches under `_mutex` — re-entrant publish deadlocks | `Singularity/Core/EventBus.hpp` | Commit 4 — law chaining publishes from handlers |
| `Formation::getIdentifier()` returns `"Formation"` for all instances | `Form/Object/Formation/Formation.hpp` | Commit 4 — identity-keyed Rete/provenance |
| `PropertyValue.hpp`: no `#pragma once`, private `Vec3` instead of `glm::vec3` | `Form/Singular/Property/` | Commit 1 |
