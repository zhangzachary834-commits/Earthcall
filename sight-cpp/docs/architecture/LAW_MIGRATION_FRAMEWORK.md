# Law Migration Framework

**How hard-coded functionality becomes authored law — repeatably, incrementally, and safely.**

**Status:** Framework specified. Rungs 1–3 already have working precedents in the tree;
rung 4 needs the six pieces of machinery in §5, which are specified here and mostly not
yet built (`LawSynthesis` is the exception — the synthesis path already exists).
**Companion docs:** `LAW_AND_CREATION_SYSTEM.md` (what a law *is*), `EarthcallOurverse.md`
(why), `SHAPE_FORMATION_DAG_PLAN.md`, `EVENT_BUS_VS_EVENT_HANDLER.md`.

---

## 0. What this document is for

`Physics.hpp:11` says it plainly:

> *"This should be retired into a legacy system and physics should become purely
> runtime-created Law Formations."*

That sentence is true of far more than physics. Camera movement, keyboard bindings,
locomotion, tool behavior, UI gating, spawn rules — all of it is currently C++ that
*decides* things the world's Persons should be able to decide. The law system
(`ZonesOfEarth/AuthorsOfLaw/`) is now rich enough to carry that weight: conditions
and actions are serializable trees over addressable properties, with exact
mathematics, quantifiers, folds, drives, events, metalaws, and persistence.

What has been missing is not expressive power. It is **a repeatable procedure**.
Every migration so far has been improvised, and improvisation does not scale to
"all kinds of hard-coded functionality" — especially not when the migrating agent
is an AI that starts each session cold.

This document is that procedure. It gives:

- a **ladder** (§2) — migration is six discrete rungs, not one leap, and each rung
  is independently valuable and independently shippable;
- **recipes** (§3–4) — for each rung, the exact code pattern, with the precedent
  already in the tree to copy;
- **the machinery rung 4 requires** (§5) — phases, jurisdiction, **contention and
  concord**, the intent seam, the parity probe, seed laws — specified as headers you
  can implement;
- **how conflicting claims resolve** (§5.3) — the program *detects* conflict and falls
  back to the first mover; Persons resolve it, through a source of truth that may be a
  law, a Relation between laws, or a Formation of laws;
- **the kernel floor** (§6) — what must never be migrated, and why that boundary is
  a safety property, not a limitation;
- **what a law may author about itself** (§6.1–6.3) — its condition's *domain*, freely
  (feedback is how numeric laws terminate); its *docket* — enabled, authority, claims,
  jurisdiction, tier, budget — never; and the six independent bounds that keep the
  first permission from becoming the second;
- **the ledger** (§7) — a machine-readable manifest so an agent can pick the next
  item without re-deriving the whole world;
- **the procedure** (§8) — twelve numbered steps, executable without judgment calls;
- **three worked migrations** (§9–11) — gravity, camera movement, keyboard — each
  taken from real line numbers in this tree to the seed laws that replace them;
- **anti-patterns** (§12) and **the build order for the framework itself** (§13).

**The one-sentence thesis:** *you do not migrate behavior, you migrate authority —
first make the subsystem legible, then audible, then governable, and only then
displace the decision, one named responsibility at a time, with the old code still
present and able to take back over.*

**And its corollary:** *where authority is contested, the program's job ends at naming
the contest.* Migration hands decisions to Persons; a framework that then adjudicates
between Persons has taken back with one hand what it gave with the other.

---

## 1. The three seams inside every hard-coded subsystem

Before the ladder, the distinction that makes the ladder possible.

Every hard-coded system fuses three things that the law ontology keeps apart:

| Seam | What it does | Who should own it after migration |
|---|---|---|
| **Sense** | reads the world (`glfwGetKey`, collision tests, `getSupportHeight`) | **engine, forever.** Sensing is first movement. |
| **Decide** | turns sensed values into intent (`if W pressed → move forward`, `velY -= 9.81·dt`) | **law.** This is the only part that migrates. |
| **Act** | commits intent to state (`pos += v·dt`, submit draw call, write GL buffer) | **engine, forever.** Actuation is first movement. |

`Game::stepMovement` (`src/Singularity/Core/GameUpdate.cpp:695`) is a perfect
specimen: it senses keys and support height, decides velocity and displacement,
and acts by writing `_camera.pos` — all in one 130-line function with the
decisions expressed as `constexpr float GRAVITY = 9.81f` (`:786`).

**Migration = finding the Decide seam and cutting it out.** If you find yourself
migrating a `glfwGetKey` call or a matrix upload, you have cut in the wrong place.

The reason this matters ontologically: Earthcall's Singularity is the C++ layer
that *first-moves* — it can sense and it can act. Law is process: the shape of
change between sensing and acting. Migrating Sense or Act into law is not
liberation, it is just slower C++.

---

## 2. The Migration Ladder

Six rungs. Each is a complete, shippable state. Each has an exit test that must
pass before the next is attempted. **Never skip a rung** — the rungs are ordered
by what they make possible, not by ambition.

```
R0  Opaque      the subsystem is invisible to law
R1  Legible     its state is registered properties  → laws can READ it
R2  Audible     it publishes events at decisions    → laws can RESPOND to it
R3  Governed    its constants are properties        → laws can TUNE it
R4  Displaced   a seed law makes the decision       → laws OWN it (C++ still present, yielded)
R5  Native      the C++ decision body is deleted    → the law IS the behavior
```

Value accrues at every rung, which is what makes this safe to do incrementally
and safe to stop halfway:

- **R1** alone makes the subsystem *quantifiable* — folds and quantifiers can range
  over it, the Law Author window offers its vocabulary, metalaws can audit it.
- **R2** alone makes the subsystem *composable* — other laws react to it without it
  knowing they exist.
- **R3** alone makes the subsystem *governable* — a Person can change gravity's
  strength by law, and the change persists with the world.
- **R4** is where authority actually transfers, and it is the only rung that can
  break the game. It is therefore the only rung with a mandatory parity gate (§5.5).
- **R5** is bookkeeping — but it is the rung that makes the promise real, because
  code that still exists will be reached for.

**Rung inflation is the classic failure.** An agent that reads `LAW_AND_CREATION_SYSTEM.md`
and immediately tries to express `stepMovement` as an `ActionModel` will produce
something that neither works nor can be debugged, because there is no legible
`velocity` to write, no `grounded` to test, no event to bind, no phase to run in,
and no way to turn the old code off. Rungs 1–3 are what make rung 4 a small change.

---

## 3. Rungs 1–3: the recipes (all three have working precedents)

### R1 — Legible: the Property Bridge

**Goal:** every value the subsystem decides *from* or decides *about* is reachable
as a `PropertyPath`.

**Pattern A — the state lives on an existing being.** Register it in that being's
`buildProperties()`:

```cpp
// src/Person/Person.cpp:31 — the precedent. Note :37-53: the "Law System
// Perception Properties" block is exactly this move, already made for the
// tool/cursor/camera surface.
_propertyRegistry.push_back(std::make_unique<ComputedProperty<Person, bool>>(
    "grounded", this, &Person::propGrounded));            // read-only: no setter
_propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
    "velocity", this, &Person::velocity));
```

**Pattern B — the state lives in a foreign struct the being does not own.**
Write a `Property` subclass that reaches into it. Three precedents in one file:
`ShapeParamBridge`, `RigidBodyBridge`, `FacePropertyBridge`
(`src/Form/Object/Object.cpp:536-580`). `RigidBodyBridge` is the closest analogue —
it exposes `Physics::getBodyFor(obj)`'s velocity and mass as `velocity` / `mass`
on the Object, so collision *response* became authorable without moving the
rigid-body registry.

**Pattern C — the state has no being at all.** Mint one. An extra-spatial
`Singular` (or `Object` with `setPhysicalObject(0)`), with a **stable string
identifier**, pushed into the Universe provider at `src/Singularity/Core/GameInit.cpp:46`.
Precedents: `TransferPolicy` (identifier `transfer-policy`, pushed at `GameInit.cpp:63`)
and all Zones (`:68`). This is how input, the UI, and the camera each acquire a body
in §9–11.

**Naming rules.** Dotted names are registered *flat* — `PropertyPath::resolve`
does longest-dotted-name matching first (`Form/Singular/Property/PropertyPath.hpp:12-19`),
so `key.SPACE` and `face.0.color` are single registry entries, not nested beings.
Use dotted flat names for families of leaves; use nested Singulars only when the
sub-thing is genuinely a being with its own life.

**Read-only is a design statement, not laziness.** Register without a setter
anything a law must never write: derived state (`grounded`, `supportY`), identity
(`name` — see `Person.cpp:35`), and Kernel-tier governance. `Zone` does this
deliberately for `owner` — "transferring a zone is a covenant between Persons,
not a property write."

**Exit test.** A test in `tests/` that resolves the path, reads the live value
during a simulated frame, and — for writable properties — writes it and observes
the engine obey. Add a `make test-<name>` target beside the existing sixteen
(`Makefile:217-268`).

---

### R2 — Audible: the Event Echo

**Goal:** every moment the subsystem *decides something discrete* announces itself
as an `ECA::Event`, so laws can bind to it.

```cpp
// src/Singularity/Core/GameUpdate.cpp:790 — the precedent, already in stepMovement
Core::EventBus::instance().publish(
    ECA::Event{"jump-started", &_player, nullptr, std::time(nullptr)});
```

**Rules that keep the vocabulary usable:**

1. **Past-tense `noun-verbed`.** The existing vocabulary — `collision`, `landed`,
   `jump-started`, `locomotion-started`, `locomotion-stopped`, `object-hover-enter`,
   `person-joined-zone`, `zone-entered`, `relation-formed`, `concept-instantiated`,
   `automation-clip-finished`, `law-drive-finished` — is the house style. Match it.
2. **Edges, not levels.** Publish on transitions. A per-frame "still falling" event
   is a bug: that is what `Activation::WhileTrue` exists for, and it costs nothing.
3. **Both participants.** `subject` is the being the event is *about*; `object` is
   the other party. Persons entering a zone carry the zone as the object
   (`person-joined-zone`) precisely so a law can ask `@event.object.owner`. An event
   that drops its second participant permanently loses expressiveness — the whole
   `@event.object` referent vocabulary depends on it.
4. **No payload beyond the two participants.** `ECA::Event` has no data field
   (`ECA.hpp:24`), and it should not grow one. When the "payload" is an identity
   (which key? which tool?), **make it a being and pass it as the subject.** That is
   the ontologically honest move and it costs zero machinery — see §11.
5. **Publish outside locks.** `EventBus::publish` historically dispatched under its
   own mutex; laws chaining events from handlers is the normal case now. Verify the
   copy-listeners-then-dispatch fix is in place before adding a publisher inside a
   handler (`LAW_AND_CREATION_SYSTEM.md` §9).

**Exit test.** A law created with `laws.createLaw(...)` + `laws.bindTrigger(id, "your-event")`
fires exactly once per occurrence during a simulated scenario, and the
`ApplicationRecord` shows `Applied`.

---

### R3 — Governed: the First-Mover Bridge

**Goal:** the subsystem's *magic numbers* become properties on a Law that lives in
the register. The C++ keeps doing the work; what moves is **governance**.

`PhysicsLawBridge` (`src/ZonesOfEarth/AuthorsOfLaw/PhysicsLawBridge.{hpp,cpp}`) is
the reference implementation and should be read in full before writing another one.
Its shape:

```cpp
class YourBridge : public Law {
public:
    explicit YourBridge(const std::string& engineThingName);
    bool isFirstMover() const override { return true; }   // excluded from save
    static void syncRegister(LawManager& laws);           // called once per frame
private:
    void buildProperties() override;                      // ComputedProperty over
    float propStrength() const;                           //   getters that reach
    void  propSetStrength(const float& v);                //   into the engine
};
```

Four properties of the pattern that are load-bearing:

- **It resolves its engine target by *name*, not pointer or id** (`PhysicsLawBridge.cpp:8-13`),
  because ids are reassigned on world load. A target that no longer exists reads as
  zero/disabled and **refuses writes** rather than crashing.
- **`isFirstMover()` excludes it from serialization** (`Law.hpp:158`). The bridge is a
  runtime being; the engine state it mirrors persists in its own save section. Loading
  a world preserves bridges rather than restoring them.
- **`syncRegister` runs every frame** (`GameUpdate.cpp:674`), so an engine law created
  at runtime gets a bridge the same frame it appears.
- **The bridge is a `Law`, so metalaws govern it for free.** `@<bridge>.strength := 3`
  is ordinary law-text. That is the entire point: gravity stops being a world the law
  system cannot see.

> **Defect this framework requires fixing first.** `Law::initializeLawIdentity`
> assigns generated ids (`law-1`, `law-2`, …; `Law.cpp:44-59`). Bridges are not
> serialized, so **their ids differ every run** — which means `@law-7.strength` in a
> seed law or a Person-authored law breaks on the next launch. Before any R3 bridge is
> depended upon by law-text, **override `getIdentifier()` on first-mover bridges to
> return a stable slug** (`physics-gravity`, `input`, `camera`, `ui`). Stable
> identifiers are the addressing contract; generated ids are fine only for beings that
> are saved with their id.

**Exit test.** A metalaw that writes the bridge property changes observed behavior;
the change survives save → load; the register lists the bridge.

---

## 4. R5 — Native (stated early, because it defines "done")

After the seed law has held jurisdiction through a play-test cycle:

1. Delete the *Decide* body from the C++. Keep Sense and Act.
2. Delete the jurisdiction check that guarded it (the responsibility now has no
   first-mover holder; §5.2 allows a responsibility to be law-only).
3. The seed law loses `isFirstMover()` if it ever had it, and is serialized as
   ordinary world data.
4. The ledger row moves to `Native`, and the parity scenario is retired or converted
   into a plain regression test.

**Do not reach R5 until a Person can disable the seed law and get an intelligible
world.** "Disable the gravity law and the player floats" is intelligible. "Disable
the ground-constraint law and the player falls through the floor forever with no way
back" is not — that responsibility belongs in the Gated or Kernel tier and stays at
R3/R4 permanently. Deletability is a governance question, not a cleanliness question.

---

## 5. The machinery rung 4 requires

Rung 4 is where authored law takes over a decision the engine was making. Six pieces
are needed, listed in dependency order. Five are small and missing; the sixth (§5.3)
is the governance half of the framework and is where most of the design lives, though
its hardest component — `LawSynthesis::compose` — is already written.

### 5.1 Phases — *when* in the frame a law runs

**The problem.** `LawManager::tick()` is called exactly once, at the very end of
`Game::update` (`GameUpdate.cpp:679`), long after `stepMovement` has already
integrated and committed the player's position. A law that writes `velocity.y`
there affects nothing until the next frame — and a law that writes `position`
fights the movement resolve, which is why `stepMovement:706-711` already contains a
"was the player teleported by a Law?" reconciliation hack.

**The fix.** Laws declare which phase of the frame they run in; the engine ticks the
law manager once per phase.

```cpp
// Law.hpp — serialized as int, APPEND-ONLY. 0 keeps today's exact behavior.
enum class Phase {
    Frame     = 0,   // end of update — the historical slot, and the default
    Sense     = 1,   // after input/collision reads, before any decision
    Intend    = 2,   // decisions: write intent quantities
    Integrate = 3,   // decisions: write velocities/rates
    Constrain = 4,   // clamps, floors, ceilings — the last word before commit
    Present   = 5    // after commit: cosmetic, camera follow, effects
};
Phase phase() const;
void setPhase(Phase p);
```

```cpp
// LawManager
std::vector<Law::ApplicationRecord> tick(Law::Phase phase = Law::Phase::Frame);
```

Implementation notes that matter:

- `tick(phase)` **filters both passes by phase** — the Rete agenda drain *and* the
  continuous sweep. Without the filter, moving to six ticks per frame would fire every
  `WhileTrue` law six times per frame, silently multiplying every `Flow` by six.
- The `kMaxChainRounds` ceiling (`Law.hpp:476`) applies **per phase**, not per frame.
- Events published during a phase become facts for the *next* phase in the same frame —
  which is exactly the behavior you want (a jump published in `Sense` is heard in
  `Intend`), and it is bounded by there being finitely many phases.
- Default `Frame = 0` means every existing law and every saved world behaves
  identically after this change. That is the compatibility contract for adding
  phases at all.

Engine wiring, in `Game::update` / `stepMovement`:

```
read input, run collision/support queries      → tick(Sense)
                                               → tick(Intend)      // moveIntent written here
apply intent to velocity                       → tick(Integrate)   // gravity, drag, impulses
integrate position, resolve collisions         → tick(Constrain)   // floors, bounds
commit position, pose body                     → tick(Present)
… rest of update …                             → tick(Frame)       // everything existing
```

### 5.2 Jurisdiction — *who* owns a decision

**The problem this solves is the single most dangerous one in the whole framework:**
two systems deciding the same quantity. Turn on a gravity law while
`stepMovement:788` still runs `_playerVelY -= GRAVITY * dt` and you get double
gravity — and the symptom (falling at 2g) looks nothing like the cause.

**The fix.** Name every migratable decision. A named responsibility has exactly one
**holder** at a time, and the transfer is explicit, runtime-reversible, and legible.

**The holder is a Singular — not necessarily a Law.** This is the load-bearing
correction to the obvious design. A responsibility must have one source of truth, but
"one source of truth" and "one law" are different claims. The holder may be:

- a **Law** — the simple case, one law decides;
- a **Relation** — the pairwise case, `A yields-to B`, where the relation between two
  laws *is* the settlement;
- a **Formation of laws** — the n-ary case, where several laws jointly hold and the
  Formation's own `RelationManager` carries the structure among them.

`Formation::getIdentifier()` is unique per instance (`Formation.hpp:101`) and
`Relation::getIdentifier()` derives from its endpoints and type (`Relation.hpp:90`), so
all three are addressable beings and `holderOf()` can return any of them with no new
machinery. §5.3 is about how a holder of the second or third kind comes to exist.

```cpp
// src/ZonesOfEarth/AuthorsOfLaw/LawJurisdiction.hpp
//
// Who decides what. Every migratable decision in the engine is a named
// RESPONSIBILITY with exactly one holder: the engine's first mover, or a BEING
// that has claimed it (a law, a relation between laws, or a formation of laws).
// The first mover asks before deciding; a claimed responsibility means the C++
// body does not run at all — no fighting, no double integration, and the claim
// can be released at runtime (disable the holder and the first mover takes back
// over on the next frame).
class LawJurisdiction : public Singular {
public:
    static LawJurisdiction& instance();
    std::string getIdentifier() const override { return "jurisdiction"; }

    // Mirrors Singularity/TransferPolicy's three tiers, for the same reason.
    enum class Tier {
        Kernel,      // never claimable: the anti-tyranny floor (§6)
        Governable,  // claimable by any authored law
        Gated        // claimable only when a metalaw has opened the gate
    };

    // Declared once at boot, beside the code that implements the first mover.
    // A first mover MAY name a default resolution discipline for contention
    // (§5.3); Kernel responsibilities MUST.
    void declare(const std::string& responsibility, Tier tier,
                 const std::string& description,
                 Discipline defaultDiscipline = Discipline::None);

    // The first mover's guard. True = the C++ body should run. The subject
    // matters: a zone-scoped claim may hold over one being and not another,
    // so jurisdiction is answered per (responsibility, subject).
    bool heldByFirstMover(const std::string& responsibility,
                          const Singular* subject = nullptr) const;

    // A being claims by listing responsibilities in its serialized `claims`
    // field; LawManager syncs claims each frame from enabled+authored holders.
    // A second claim over the same (responsibility, subject) does NOT pick a
    // winner — it raises a Contention and the first mover keeps deciding until
    // a Concord resolves it (§5.3). The program detects; Persons resolve.
    enum class ClaimResult { Accepted, Contended, RefusedKernel, RefusedGateClosed };
    ClaimResult claim(const std::string& responsibility,
                      const std::string& holderId, Reach reach = {});
    void release(const std::string& responsibility, const std::string& holderId);

    // "" = first mover. May return a law id, a relation id, or a formation id.
    std::string holderOf(const std::string& responsibility,
                         const Singular* subject = nullptr) const;

    const std::vector<Contention>& contentions() const;

    // The table is a legible being: @jurisdiction.<slug>.holder / .tier /
    // .contended / .concord / .defaultDiscipline — so metalaws can audit the
    // transfer of authority, and the Law Author can show who owns what.
    void buildProperties() override;
};
```

Usage at the seam — this is the whole ceremony:

```cpp
// Game::stepMovement, step 4
if (LawJurisdiction::instance().heldByFirstMover("player.vertical-motion")) {
    _playerVelY -= GRAVITY * dt;                 // the legacy body, untouched
}
_camera.pos.y += _playerVelY * dt;               // the ACTUATOR always runs
```

Four properties make this the right shape:

- **Incremental** — `player.vertical-motion` can migrate while `player.horizontal-motion`
  stays first-mover. You never have to move a whole subsystem at once.
- **Reversible at runtime** — disable the holder, and the first mover resumes next
  frame. This is the recovery path when an authored law is broken, and it is why the
  C++ is not deleted until R5.
- **Legible** — `@jurisdiction.player-vertical-motion.holder` is a property, so the
  world can *see* who governs it. Governance that cannot be inspected is not governance.
- **Non-adjudicating** — contention does not resolve itself into a winner. It falls
  back to the first mover, which is the one behavior in the system already known to be
  correct, and waits (§5.3).

Add to `Law`: `const std::vector<std::string>& claims() const;` serialized alongside
triggers, and synced by `LawManager` each frame (same cadence as
`PhysicsLawBridge::syncRegister`). Relations and Formations acting as holders carry
the same field — for a Formation, the claim is the Formation's, not its members'.

**Do not conflate jurisdiction with `Law::authorityLevel`.** They answer different
questions and an implementer will merge them if not warned. `authorityLevel`
(`Law.hpp:160-170`) is the metalaw ceiling: *may this law govern that law?* — a
Singularity-granted, never-law-modifiable bound on who can rewrite whom. Jurisdiction
is: *who decides this quantity?* A high-authority metalaw does not thereby win a
contest over `player.vertical-motion`; it just means nobody below it can edit it. The
ceiling constrains what a Concord may propose; it never selects one.

### 5.3 Contention and Concord — conflict is named, not decided

**The principle: the program detects conflict; Persons resolve it.** Detection is
first movement — mechanical, exhaustive, reliable, and something code is good at.
Resolution is authorship, and a program that silently picks a winner has quietly
legislated. The only automatic resolution permitted is one that was *itself authored
in advance*: a metalaw, or a first-mover default declared alongside the responsibility.

Migrating a subsystem creates contention as a matter of course, so this is not an
edge case bolted onto the framework — it is the framework's governance half.

#### 5.3a Jurisdiction is not only spatial

A being falls under a zone's laws through any of several **bindings**, and it may be
under several at once. This is the overlap case in full: not merely the region where
two zones intersect, but a Person standing in Zone B who remains bound by Zone A
through belonging or obligation — they carry A's jurisdiction with them.

```cpp
// Which zones' laws reach a given being, and WHY. A being under two reaches is
// under two jurisdictions; that is normal, and it is where contention lives.
enum class Binding {
    Presence,     // the being is within the zone's region
    Belonging,    // a member of the zone's Formation (Zone::getFormation)
    Obligation,   // a Relation binds it ("sworn-to", "owes", "employed-by")
    Ownership     // the zone owner's laws follow what they own
};
struct Reach { std::string zoneId; Binding binding; };
std::vector<Reach> reachOver(const Singular& being);
```

Three consequences worth stating explicitly:

1. **Contention is per (responsibility, subject).** Two zone laws may conflict only
   for the one Person standing in the overlap and be perfectly compatible for everyone
   else. A global "these two laws conflict" verdict would be false most of the time.
2. **`Belonging` and `Obligation` need no new machinery.** Zone already owns a
   Formation (`Zone.hpp:55`), and `ConditionNode::Related` already resolves against
   the Universe's relation provider. Affiliation-based jurisdiction is reachable today.
3. **`Presence` does not have a spatial test yet.** `Zone::buildProperties`
   (`Zone.cpp:27-41`) deliberately registers no position or shape — "a zone is
   extra-spatial, so position/shape/mass would be fictions." Spatial overlap therefore
   requires first giving zones a **region**: a `geom::SdfNode`, which slots directly
   into the existing `ConditionNode::InRegion` test (`evalSdf(region, probe) < 0`) and
   makes zone intersection an ordinary SDF boolean. Until then `Presence` means
   membership, not geometry. This is a real prerequisite, not a detail — see §14.

#### 5.3b Contention — the conflict as a being

```cpp
// A named, addressable conflict. Minted when a second claim arrives over the
// same (responsibility, subject); it is a Singular, so laws can quantify over
// unresolved contentions, folds can count them, and the Law Author can render
// the governance state of the world.
struct Contention {
    std::string id;                        // "contention-<n>"
    std::string responsibility;
    std::vector<std::string> claimantIds;  // laws / relations / formations
    std::vector<std::string> subjectIds;   // empty = everywhere
    std::vector<Reach> reaches;            // which zones reach, and how
    std::string concordId;                 // "" = unresolved
    std::time_t raised{0};
};
```

Raising one publishes `jurisdiction-contended` (subject: the Contention). Persons
learn about conflicts the same way they learn about anything else — by law.

**While unresolved, the first mover holds.** This is the whole safety story, and it
costs nothing to build: contention degrades to the pre-migration behavior, which is by
construction the one behavior already known to work. The world does not stall, freeze,
or pick arbitrarily; it simply keeps doing what it did before anyone claimed anything,
and says so out loud.

#### 5.3c Concord — one source of truth, of any shape

```cpp
// The resolving being for one Contention. ONE source of truth — but not
// necessarily one law:
//
//   Law        an arbiter metalaw, or a synthesized higher law, decides
//   Relation   the pairwise settlement: "A yields-to B", directed and weighted,
//              and its event timeline IS the negotiation record
//   Formation  the n-ary case: the laws that jointly hold, with the Formation's
//              RelationManager carrying the structure among them and
//              relationTypeTag naming the discipline. The FORMATION holds the
//              responsibility — its identity is the source of truth, and its
//              members remain their own beings.
struct Concord {
    std::string id;
    std::string contentionId;
    std::string holderId;          // the Law / Relation / Formation that now holds
    Discipline discipline;
    Formation consenters;          // the Persons who agreed (mirrors Law::_authors)
    std::vector<std::string> requiredConsent;   // who must agree for this to hold
    std::time_t ratified{0};       // 0 = proposed, not yet in force
};
```

#### 5.3d Disciplines — the engine ships the vocabulary, Persons choose

| Discipline | Concord being | What it does |
|---|---|---|
| `Partition` | Formation | the responsibility was too coarse — split it into finer ones so both claimants hold, disjointly. **Try this first:** a large share of contentions are naming artifacts, not disagreements. |
| `Synthesis` | Law | `LawSynthesis::compose(name, a, b, authors, allConditions, sequentialActions)` — **already implemented** (`LawSynthesis.hpp:19`). Conditions join under All/Any, actions under Sequence/Parallel, with `synthesized-from` provenance to each constituent. The synthesized law becomes the holder. This is the "law synthesis process" in code. |
| `Precedence` | Relation (`yields-to`, directed) | an authored ordering. The winner holds; the yielding law stays enabled for its other responsibilities. Persons authored the order — the program only executes it. |
| `Arbiter` | Law | a metalaw whose action names the holder as conditions change: inside Zone A's region A's law holds, outside it B's. Dynamic resolution, still authored. |
| `Concession` | Formation (empty claim) | every claimant releases; the first mover resumes. The honest "we did not agree," recorded rather than hidden. |

`LawSynthesis` also offers the **native** path — `synthesizeByDemonstration`, which
runs both laws on a referent while the `ChangeRecorder` watches and fits one fused
model (`LawSynthesis.hpp:31`). For genuine conflict resolution prefer the interpretive
path: the composition structure stays visible in the higher law's text, and Persons
consenting to a settlement need to be able to *read* it. Native synthesis is for
performance, after agreement.

#### 5.3e Consent is recorded, not assumed

A Concord takes effect only when every **affected author** has consented:

- the authors of every claimant law (`Law::authors()`), and
- the owners of every zone in the contention's reaches (`Zone::owner()`).

The precedent is already in the type system: an unauthored law returns
`ApplicationResult::Unauthored` and cannot fire — authorship is a covenant, not a
formality. An unratified Concord holds nothing, and the responsibility stays with the
first mover until it does.

The negotiation itself has a natural ledger. `Relation::events` (`Relation.hpp:105`)
is a timestamped timeline of `RelationEvent{timestamp, description, deltaWeight}` —
proposals, objections, and assent recorded on the relation between the contending
laws, with weight tracking how far apart the parties are. The conflict, the process,
and the settlement are all beings with histories.

#### 5.3f Telos — what makes agreement informed rather than merely counted

Friction between laws is not arbitrary: it undermines *the telos of each respective
law*. Give a Law its purpose, in words and — where possible — measurably:

```cpp
// Law
const std::string& telosStatement() const;    // what this law is FOR
const ConditionModel* telosTest() const;      // ...and when that purpose is served
```

The telos test is an ordinary `ConditionNode`, most naturally `Kind::Zone`, which is
*already* defined as "satisfied when f(bindings) lies within an authored [lo, hi]"
(`ConditionModel.hpp:78-85`). With it, a proposed Concord can be **evaluated** instead
of merely argued:

> A resolution is **telos-preserving** for law L when L's telos test still holds
> under the proposed Concord, over the contention's subjects, across the
> contention's scenario.

Run the proposal in shadow mode — the parity probe (§5.5) is exactly the instrument,
pointed at telos tests instead of at legacy values — and report per-law telos
preservation to every affected author *before* they consent. That turns "Persons must
agree" from a vote into an informed one, and it needs no new machinery: telos tests
are conditions, and evaluation is the probe.

A law whose telos test cannot be written is itself informative. It usually means its
purpose is not yet understood well enough to be traded away, which is a good reason to
decline a settlement rather than a gap to paper over.

#### 5.3g Defaults, and the Kernel exception

`declare(responsibility, tier, description, defaultDiscipline)`. A first mover may set
a default at declaration; a metalaw may override it by writing
`@jurisdiction.<slug>.defaultDiscipline`. Kernel-tier responsibilities **must** carry a
first-mover default — they cannot be left hanging on a negotiation that may never
conclude. Everything else defaults to `None`, which means exactly what it says: stay
with the first mover, and wait for the Persons.

That asymmetry is the framework's political claim in one line: **the substrate
guarantees only what must never fail; everything above it waits for agreement.**

### 5.4 The intent seam — *what* a law writes

A displaced decision needs somewhere to put its answer. Three kinds of quantity,
with different lifetimes:

| Kind | Example | Lifetime | Written by | Read by |
|---|---|---|---|---|
| **State** | `position`, `velocity` | persistent | holder of the responsibility | engine + laws |
| **Intent** | `moveIntent`, `lookIntent` | **one frame** — zeroed at `Sense` | holder | engine at `Act` |
| **Sense** | `grounded`, `supportY`, `key.W` | one frame, engine-written | engine only (read-only property) | laws |

**Accumulator discipline** (the rule that makes intent safe): *the engine zeroes
every intent quantity at the start of the frame's `Sense` phase, before any law
runs.* A law that stops writing therefore stops driving — no stuck keys, no
runaway motion when a law is disabled mid-frame, no need for laws to "turn
themselves off." The precedent is already in the tree: `Physics::RigidBody::accumulatedForce`
is documented as "Newtons, reset each step" (`Physics.hpp:52`).

Intent quantities are registered like any other property (R1) and should be **the
only new mutable surface** a migration adds. If a migration wants to add a mutable
`_someInternalFlag` to a being for laws to write, that is a design smell — either it
is real state (register it honestly) or it is intent (register it and zero it).

For law-private bookkeeping there is already a mechanism that needs no C++ change:
`Singular::setDynamicProperty` / `getDynamicProperty` (`Form/Singular/Singular.hpp:60`).

### 5.5 The parity probe — proving the law does what the code did

**Mandatory gate for every R4 migration.** Before flipping the claim, run both
paths and compare.

```cpp
// src/ZonesOfEarth/AuthorsOfLaw/LawParityProbe.hpp
//
// Shadow mode: the seed law computes for real while the legacy body computes
// into a scratch copy (or vice versa), and every frame the two answers are
// compared. A migration ships when the probe is silent across its scenario.
class LawParityProbe {
public:
    static LawParityProbe& instance();

    void enable(const std::string& responsibility, double tolerance);
    void observe(const std::string& responsibility,
                 const std::string& quantity,
                 double lawValue, double legacyValue);

    struct Divergence {
        std::string responsibility, quantity;
        double lawValue, legacyValue, delta;
        double worldTime;
        int frame;
    };
    const std::vector<Divergence>& divergences() const;
    nlohmann::json report() const;     // written beside the save; read by tests
};
```

Each ledger row names its **parity scenario**: a deterministic, scripted sequence
run headless in a test (fixed dt, fixed inputs, fixed world). Example for gravity:
*spawn player at y = 10 over flat ground, no input, 300 frames at dt = 1/60; compare
`position.y` each frame, tolerance 1e-4.*

Two honest caveats to record in the ledger rather than paper over:

- Some migrations are *intentionally* not bit-identical (a law expressed as an exact
  `OntoMath` antiderivative will differ from Euler integration by O(dt²)). State the
  intended difference and set the tolerance to it — do not silently widen tolerance
  until the probe goes quiet.
- A silent probe over one scenario is not a proof. Scenarios must include the edge
  cases the original code special-cased; read the legacy body for its `if`s and write
  one scenario per branch.

### 5.6 Seed laws — engine-authored law that a Person can edit

The behavior that replaces the C++ has to come from somewhere at first boot, and it
must not be a closure.

```cpp
// src/ZonesOfEarth/AuthorsOfLaw/SeedLaws.hpp
//
// The laws the engine ships with. They are AUTHORED, not hard-coded: built as
// ConditionModel/ActionModel trees, so they serialize, appear in the Law Author,
// and can be edited, disabled, or deleted by a Person like any other law.
// Installed at boot only when absent from the loaded world, and versioned so an
// engine update can add new seeds without clobbering a Person's edits.
namespace SeedLaws {
    struct Seed {
        std::string slug;        // stable identity: "seed.player.gravity"
        int version = 1;
        std::function<std::shared_ptr<Law>()> build;   // builds MODELS, never closures
    };
    void registerSeed(Seed seed);
    void installMissing(LawManager& laws, Singular& worldAuthor);
    nlohmann::json manifest();   // slug → version, saved with the world
}
```

**The non-negotiable rule: a seed law is built from `setConditionModel` /
`setActionModel`, never from `addCondition(desc, closure)` / `addAction(desc, closure)`.**
Those first-mover entry points (`Law.hpp:203`, `:209`) exist for engine bridges, and a
seed law built from closures is hard-code wearing a law's clothes: it cannot be
serialized, edited, synthesized, or understood by the Law Author. If a seed cannot be
expressed as models, that is a signal the *language* needs the missing piece — write
it down as a gap (§13) rather than reaching for a closure.

Seed authorship: the world/engine is the author. A law with no author is `Unauthored`
and cannot fire (`ApplicationResult::Unauthored`) — authorship is a covenant, and
seeds need a real one. Give the seeds the World being as author, and record
`authored-by` provenance so a Person can see where the law came from.

Versioning: the manifest saved with the world lets `installMissing` distinguish
"this Person deleted the gravity law on purpose" (slug present in manifest, law absent
→ leave it deleted) from "this world predates the gravity law" (slug absent from
manifest → install it). Without this, an engine update silently resurrects laws
Persons chose to remove, which is exactly the kind of thing the whole project is
against.

---

## 6. The Kernel floor — what never migrates

Some functionality must stay first-mover permanently. This is not a limitation to be
whittled away; it is the anti-tyranny and anti-Babel guarantee that makes authored
law safe to grant. It mirrors the `TransferPolicy` Kernel tier
(`Singularity/TransferPolicy.hpp`) and the authority ceiling already in `Law.hpp:160-170`.

**Kernel tier — declare, but never claimable:**

| Domain | Specifically | Why |
|---|---|---|
| Frame & lifecycle | the update loop, GL/GPU submission, buffer uploads | a law cannot author the machinery that runs it (it may freely author its own condition's *domain* — see §6.1) |
| Persistence | save, load, world serialization | you cannot author your way out of a corrupted save |
| The Universe wiring | providers, registrars, the clock | the domain laws range over cannot be law-defined without circularity |
| Authority | `Law::authorityLevel`, the metalaw ceiling | deliberately not a registered property today — keep it that way |
| Personhood | a Person's identity, ownership of their Home, the exit guarantee | the manifesto's floor: *nobody can be forced to stay* |
| Escape hatches | cursor release (ESC), quit, the menu key | a law must never be able to trap a Person in the world |
| Ceilings | `kMaxChainRounds`, `kMaxCallDepth`, spawn-rate caps | these bound Babel; a law that could raise them bounds nothing |
| Sense & Act | raw device reads, collision math, rasterization | first movement (§1) |

**Gated tier — claimable only after a metalaw opens the gate:** anything whose
failure mode is unrecoverable-in-world. The player's floor constraint is the
canonical example: gravity is `Governable` (float away, then turn the law off), but
"feet never sink below the support surface" is `Gated` (fall through the world and
there is no in-world remedy).

**Everything else is `Governable`.** The default should be generous — the point of
the project is that Persons author their world.

**Kernel tier carries one extra obligation:** a Kernel responsibility must declare a
default resolution discipline (§5.3g). Governable and Gated responsibilities may sit
contested indefinitely — the first mover keeps deciding and nothing is harmed by
waiting. A Kernel responsibility cannot wait on a negotiation that may never conclude,
so the first mover states in advance what happens if one is ever raised. That is the
whole difference between the floor and everything standing on it.

**Test for the tier of a new responsibility:** *if an authored law claims this and
is wrong, can a Person fix it from inside the world?* Yes → Governable. Only with
help → Gated. No → Kernel.

---

### 6.1 Reflexive authorship — a law over its own domain

**A law MAY author the variables its own condition reads. This is allowed, ordinary,
and necessary.**

The tempting prohibition — "a law may not author the conditions of its own execution" —
is wrong, and forbidding it would make simple numeric law unauthorable. Consider the
smallest useful law in the system:

```
WhileTrue:  y < 10   →   y := y + 1
```

Its action writes the very number its condition tests. There is no way to author "rise
until you reach ten" that does not do this. Any law that *changes what it checked* is
authoring its own conditions of execution, which means the prohibition would outlaw
nearly every law worth writing — every accumulator, every approach-and-stop, every
"heal until full," every `Flow` whose rate depends on the quantity it integrates.

Worse, forbidding it would not even buy safety, because the loop is what makes such
laws **terminate**. `y < 10 → y += 1` stops on its own. A law that cannot touch its
own domain has no way to satisfy itself and must be stopped from outside.

So state the line precisely:

> **A law may write the variables its condition reads. It may not write the terms by
> which it is judged.**
>
> **Domain** — the world-state the condition ranges over — is the law's to author.
> **Docket** — its own `enabled`, `authorityLevel`, its claims, its jurisdiction, its
> tier, its budget, which metalaws reach it — is not.

Domain is what the law is *about*. Docket is what the law is *accountable to*. A law
raising its own ceiling is not feedback, it is a law placing itself beyond the reach of
Persons — which is precisely what the authority ceiling in `Law.hpp:160-170` already
refuses, and why `authorityLevel` is deliberately not a registered property.

Two idioms worth knowing, because they make reflexive laws terminate cleanly:

- **`OnBecomeTrue` + a reflexive action is a natural one-shot.** The action makes the
  condition false, which re-arms the edge detector. It fires once per crossing, never
  in a spin.
- **The authored bounds ARE the duration.** For `Map` / `Flow`, `ActionNode::definedFor`
  ends a drive session the moment the function becomes undefined at the current values
  of its bound variables — and any bound variable may cut it, *including the one the
  action is writing*. A reflexive law that drives itself out of its own authored piece
  bounds stops itself, and publishes `law-drive-finished`. This mechanism already
  exists; reflexive authorship is what makes it expressive.

### 6.2 The accountability stack

Reflexive authorship is permitted **and never unbounded**. Every law — reflexive or
not — sits under six independent bounds. They are independent on purpose: each catches
a different failure, and no single one is load-bearing alone.

| # | Bound | What it catches | Machinery |
|---|---|---|---|
| 1 | **Metalaws** | a law behaving wrongly in ways a Person can name | `Law` is a legible Singular (`enabled`, `conditionMode`, `drives`, `name` registered by `Law::buildProperties`); a law whose target is a law IS a metalaw. **Exists.** |
| 2 | **Law conflict resolution** | two laws deciding one quantity | Contention → first-mover fallback → Concord (§5.3). A reflexive law claiming a responsibility contends like any other. **Specified, §13 commits 2–3, 11.** |
| 3 | **Zone jurisdiction** | a law reaching beings it has no standing over | `reachOver(being)` and the four bindings (§5.3a); a law's reach is its zone's reach. **Specified, §13 commits 13–14.** |
| 4 | **Singularity rate limiting + the event queue** | the failure unique to reflexive laws: a loop that is individually legal and collectively ruinous | `kMaxChainRounds = 8` (`Law.hpp:475`) and `kMaxCallDepth = 32` (`ScalarForm.hpp:329`) exist; the per-tick **budget** does not — §6.3. |
| 5 | **Fundamental Person rights** | a law reaching what no law may reach | Kernel tier: identity, ownership of Home, the exit guarantee, the escape hatches. **Never claimable, by construction.** |
| 6 | **Kernel–Singularity hierarchy** | everything above being renegotiated from below | Tiers (Kernel / Gated / Governable) granted at the Singularity level, never law-modifiable; the authority ceiling refuses lower governing higher. **Exists in part.** |

Bound 4 is the one reflexive authorship actually stresses, and it is the one still
missing. The others already refuse a law that oversteps; none of them notice a law that
stays perfectly within its rights ten million times per second.

### 6.3 `SingularityBudget` — the rate ceiling reflexive laws require

`kMaxChainRounds` bounds how far a cascade may propagate *within* one tick. It says
nothing about how much work one law may do, how many events it may mint, or how a
reflexive loop behaves *across* ticks. That is the gap.

```cpp
// src/Singularity/SingularityBudget.hpp
//
// What any law may spend in one tick. Granted at the Singularity level, never
// law-modifiable (a law that could raise its own budget has no budget) — the
// same anti-tyranny footing as TransferPolicy's Kernel gates and the authority
// ceiling. Legible READ-ONLY as @budget.*, so Persons and metalaws can SEE the
// spend without being able to alter the ceiling.
class SingularityBudget : public Singular {
public:
    static SingularityBudget& instance();
    std::string getIdentifier() const override { return "budget"; }

    struct Ceilings {
        int applicationsPerLawPerTick = 64;    // one law, one tick
        int applicationsPerAuthorPerTick = 512;// all laws of one author
        int eventsPublishedPerLawPerTick = 32; // ActionNode::Publish
        int spawnsPerLawPerTick = 16;          // Create / Spawn — Babel's rate knob
        int destroysPerLawPerTick = 16;        // ...and its counterpart
    };

    // Asked before each application/publish/spawn. Refusing is not an error:
    // it is the ceiling doing its job, and it is RECORDED, never silent.
    bool admit(const std::string& lawId, Kind kind);

    // What overflowed this tick, for the audit log and the Law Author's face.
    struct Overflow { std::string lawId, authorId; Kind kind; int attempted; };
    const std::vector<Overflow>& overflows() const;
};
```

Three rules that make this a ceiling rather than a silent failure:

1. **Throttle, never drop silently.** An admission refusal is published
   (`budget-exceeded`, subject: the law) and written to the `ApplicationRecord` as a
   new `ApplicationResult::BudgetExceeded`. A Person must be able to discover that
   their law is being held back, or debugging becomes archaeology.
2. **The event queue is a queue, not a stack.** Events minted by a law's action are
   *enqueued* for the next chain round — never dispatched re-entrantly from inside the
   handler. `LawManager::tick` already works this way (facts asserted during a round
   survive into the next, `Law.cpp:684-688`); the rule is that `ActionNode::Publish`
   must never grow a synchronous path around it. Re-entrant dispatch turns a reflexive
   law from a bounded loop into an unbounded stack, and the deadlock defect noted in
   `LAW_AND_CREATION_SYSTEM.md` §9 is the same hazard wearing a different hat.
3. **Oscillation is legal; invisible oscillation is not.** A reflexive law that flips a
   value every tick forever is a legitimate authored thing — an oscillator. It stays
   under the per-tick budget by construction (one application per tick), so nothing
   should stop it. What the Law Author owes a Person is *visibility*: a law applying
   every tick indefinitely should be legible as such, not silently churning.

**Budget is Kernel tier and read-only to law-text.** `@budget.applicationsPerLawPerTick`
is inspectable so a metalaw can *reason* about the ceiling, and unwritable so no law can
raise it. This is bound 6 protecting bound 4 — the hierarchy exists exactly so that the
rate limiter cannot be renegotiated by the thing it limits.

---

## 7. The Migration Ledger

One row per named responsibility. Lives at `laws/migration_ledger.json`, mirrored as
a table in this document. **An agent picks work by reading this file, not by reading
the codebase.** That is the point: it is the state that makes migration resumable
across cold starts.

```json
{
  "responsibility": "player.vertical-motion",
  "subsystem": "movement",
  "seam": "src/Singularity/Core/GameUpdate.cpp:786-791",
  "tier": "Governable",
  "rung": 3,
  "target_rung": 5,
  "owner_being": "player",
  "properties": ["velocity", "grounded", "flying"],
  "events": ["jump-started", "landed", "left-ground"],
  "phase": "Integrate",
  "seed_law": "seed.player.gravity",
  "parity_scenario": "tests/parity/fall_from_10.json",
  "test_target": "make test-migration-gravity",
  "blocked_by": ["framework.phases", "framework.jurisdiction"],
  "default_discipline": "None",
  "zone_scoped": false,
  "open_contentions": [],
  "notes": "binds g to @physics-gravity.strength so the existing slider still governs"
}
```

`default_discipline` records what happens if this responsibility is ever contested and
nobody has resolved it (§5.3g) — `None` for everything but Kernel rows. `zone_scoped`
says whether claims over it carry a `Reach`, which is what makes contention
per-subject rather than global. `open_contentions` is written by the running world, not
by hand: it is how a cold-starting agent learns that a migration is *waiting on Persons*
rather than waiting on code, which is not a state it should try to fix.

Seed the ledger from §9–11 plus a sweep for the obvious remaining candidates:
tool behavior (`GameToolbar`), brush placement (`BrushPlacementMode` in
`GameUpdate.cpp:620-637`), UI gating, zone switching, avatar posing, automation
playback, spawn rules.

**Ledger hygiene:** a row's `rung` only ever increases, and only after its exit test
passes. A row that regresses is a bug report, not an edit.

---

## 8. The procedure

Twelve steps. Written to be executed without re-deriving anything. One
responsibility at a time — **never batch**, because the parity gate is per-responsibility
and a failing batch cannot be bisected.

1. **Name it.** `<subsystem>.<decision>`, lowercase-dotted, verb-ish
   (`player.vertical-motion`, `camera.horizontal-motion`, `input.flight-toggle`).
   Add the ledger row with `rung: 0` and everything else best-effort.
2. **Find the seam.** The smallest contiguous block that is pure *Decide* (§1). Record
   the exact `file:line-line` in the ledger. If the block interleaves Sense and Act,
   your first commit is a pure refactor that separates them — no law involved, no
   behavior change.
3. **Find or mint the being.** Which Singular does this decision concern? If none
   exists, mint an extra-spatial one with a **stable identifier** and add it to the
   Universe provider (`GameInit.cpp:46`).
4. **R1 — legibility.** Register every value the seam reads and writes. Read-only for
   sensed/derived values. Test: resolve and read each path in a running frame.
   → ledger `rung: 1`.
5. **R2 — audibility.** Publish an event at each discrete decision the seam makes.
   Test: a bound law fires exactly once per occurrence. → `rung: 2`.
6. **R3 — governance.** Lift constants out of the body: either onto the owning being
   or into a first-mover bridge Law with a stable identifier. Test: a metalaw changes
   behavior through the property, and the change survives save/load. → `rung: 3`.
7. **Declare the responsibility.** `LawJurisdiction::declare(name, tier, description)`
   at boot, beside the first mover. Wrap the seam in `if (heldByFirstMover(name))`.
   Test: nothing changes (nobody has claimed it yet). This commit is behavior-preserving
   by construction.
8. **Choose the phase.** From §5.1. Write it in the ledger. Getting this wrong is the
   most common cause of a law that "does nothing" or "does half as much."
9. **Author the seed law** — models only (§5.6). Unit-test it in isolation first: a
   hand-built Universe, a hand-set clock, one subject, N ticks, assert the property.
   This is where you find out the language is missing something, and it is much
   cheaper to find out here than in the engine.
10. **Parity.** Enable the probe, run the scenario, iterate until silent within the
    stated tolerance. Record the scenario file in the ledger.
11. **Flip the claim.** Add the responsibility to the seed law's `claims`. If the claim
    comes back `Contended`, **stop** — a contention is a question for Persons, not a bug
    to work around, and the world is safely on the first mover until they answer (§5.3).
    Record the contention id in the ledger row and move to another row. Play-test
    the real game — the probe cannot see feel. Delete nothing. → `rung: 4`.
12. **Go native.** After a play-test cycle, delete the legacy Decide body and its
    jurisdiction guard (§4). → `rung: 5`. Then write the one-line note: what was
    non-obvious about this migration that the next agent would otherwise re-learn.

**Stop conditions — do not proceed, file a gap instead:**

- The seed law cannot be expressed in models without a closure → the *language* needs
  work; write down exactly which node kind is missing.
- The parity probe cannot be made silent and you cannot articulate why the difference
  is intended.
- The seam is in a per-vertex, per-pixel, or per-object-per-frame inner loop where
  law evaluation cost is not affordable (§12).
- The responsibility turns out to be Kernel tier once you ask the §6 test question.

---

## 9. Worked migration A — gravity, jump, and the ground

**Seam:** `Game::stepMovement`, `src/Singularity/Core/GameUpdate.cpp:695-829`.
Four separable responsibilities live in one function:

| Responsibility | Lines | Tier | Phase | Target |
|---|---|---|---|---|
| `player.vertical-motion` (gravity) | `:786`, `:788` | Governable | Integrate | Native |
| `player.jump` | `:785-791` | Governable | Intend | Native |
| `player.ground-constraint` | `:757-772`, `:798-806` | **Gated** | Constrain | R4, stays |
| `player.flight` | `:775-782` | Governable | Integrate | Native |

Object gravity is already at R3 — `Physics::updateBodies` is governed through
`PhysicsLawBridge`. This migration brings the *player* up to the same footing and
then past it.

### R1 — what must become legible

On `Person` (`src/Person/Person.cpp:31`):

| Path | Kind | Notes |
|---|---|---|
| `velocity` | rw vec3 | today `Game::_playerVelY` (`Game.hpp:424`), a bare float on the Game. Promote to a `glm::vec3` on the Person; the Y component is what exists now. |
| `grounded` | **ro** bool | today `Game::_playerGrounded` (`Game.hpp:425`) |
| `supportY` | **ro** float | today the local `supportY` at `:757`; a sensed value |
| `eyeHeight` | ro float | already exists via `_player.getBody().getEyeHeight()` |
| `flying` | rw bool | bridges `Physics::getFlying()` / `setFlying()` |
| `jumpSpeed` | rw float | today `constexpr JUMP_SPEED = 5.0f` at `:787` |

Promoting `_playerVelY` from a `float` on `Game` to a `velocity` vec3 on the Person
is the largest single edit in this migration, and it is worth doing on its own commit:
it is the difference between "the player's motion" being engine-private and being a
property of the being that is moving.

### R2 — events

`jump-started` (`:790`) and `landed` (`:802`) already exist. Add `left-ground` on the
grounded→airborne edge, so a law can respond to falling as well as landing.

### R3 — constants

`GRAVITY` at `:786` does **not** become a new property. Bind it to the existing
physics gravity law through its bridge, so the gravity a Person already tunes in the
physics UI is the same gravity the player feels. This requires the stable-identifier
fix from §3 so the bridge is addressable as `@physics-gravity`.

### R4 — the seed laws

**`seed.player.gravity`** — phase `Integrate`, activation `WhileTrue`, subject: the
player (targets Formation = {player}), claims `player.vertical-motion`.

```
condition:  All( Compare(flying, Eq, false) )
action:     Flow( path  = velocity.y,
                  f     = -g,                       // OntoMath::Expression, one term
                  binds = { g -> @physics-gravity.strength } )
```

`Flow` is exactly right here: it writes `path := path + f·dt` (`ActionModel.cpp:318-341`),
which *is* Euler integration of acceleration into velocity — the same arithmetic
line `:788` performs, expressed as authored mathematics. Parity should be exact to
floating-point rounding, so tolerance can be tight.

**`seed.player.jump`** — phase `Intend`, activation `OnEvent`, trigger `jump-pressed`
(minted by the keyboard migration, §11), claims `player.jump`.

```
condition:  All( Compare(grounded, Eq, true), Compare(flying, Eq, false) )
action:     Sequence[ Map( velocity.y, f = j, binds = { j -> jumpSpeed } ),
                      Publish( "jump-started", subject = "" ) ]
```

Note what just happened: **the event that R2 added is now published by the law**, and
the C++ publisher at `:790` is deleted along with the jump body at R5. Laws minting
the event vocabulary rather than only consuming it is the intended end state
(`ActionNode::Kind::Publish`).

**`seed.player.flight`** — phase `Integrate`, `WhileTrue`, condition `flying == true`,
action `Set(velocity.y, 0)` plus the intent-driven vertical motion. Replaces `:775-782`.

**Ground constraint stays first-mover** — Gated tier. Lines `:798-806` keep running,
now reading `@player.supportY`. A Person who wants to author their own floor must open
the gate through a metalaw, which is a deliberate speed bump on a decision that can
strand them.

### Engine after the migration

`stepMovement` keeps: the support-height query (`Sense`), the horizontal collision
resolve (`Act`), `_camera.pos.y += velocity.y * dt` (`Act`), the floor clamp
(`Constrain`, first-mover), the locomotion events, and the pose. It loses every
`constexpr` and every `if` that decided *how much*.

### Parity scenario

`tests/parity/fall_from_10.json` — player at y = 10 over flat ground, no input, 300
frames at dt = 1/60, compare `position.y` and `velocity.y` each frame, tol 1e-5.
Second scenario `jump_and_land.json` — grounded, jump on frame 10, run 120 frames,
assert `jump-started` and `landed` fire exactly once each and apex matches within 1e-4.

---

## 10. Worked migration B — camera movement

**Seam:** `stepMovement` step 1, `GameUpdate.cpp:714-731` (speed modifiers and WASD),
plus `MouseHandler` for orientation.

### R1 — mint the camera being

`Core::CameraState` (`src/Singularity/Core/CameraState.hpp:8`) is a plain struct with
no identity. But `Perspective : public Singular` already exists with an **empty**
`buildProperties()` (`src/Perspective/BasicPerspective/Perspective.hpp:19`) — the hook
is sitting there waiting. Give it a stable identifier (`camera`) and bridge the
`CameraState` fields into it (Pattern B, `RigidBodyBridge`-style):

| Path | Kind | Source |
|---|---|---|
| `position` | rw vec3 | `CameraState::pos` |
| `front`, `up` | ro vec3 | `CameraState::front/up` — derived from yaw/pitch |
| `yaw`, `pitch` | rw float | `MouseHandler` |
| `speed` | rw float | `CameraState::speed` (`:12`) |
| `fov`, `nearPlane`, `farPlane`, `sensitivity` | rw float | `PersonPerspective::CameraSettings` |
| `moveIntent` | rw vec3 | **new intent quantity**, zeroed each `Sense` |
| `speedScale` | rw float | **new intent quantity**, reset to 1.0 each `Sense` |

Register the camera being in the Universe provider so `@camera.fov` is addressable
world-wide.

### R2 — events

`camera-mode-changed` (perspective switch), `camera-teleported`. Movement itself is
level, not edge — `WhileTrue` covers it, no per-frame event.

### R3 — constants

`actualSpeed *= 2.5f` (sprint) and `*= 0.3f` (slow) at `:715-716` become
`@camera.sprintScale` / `@camera.slowScale`, or — better — disappear entirely into the
seed laws below, since a multiplier applied under a condition *is* a law.

### R4 — the seed laws

**`seed.camera.walk`** — phase `Intend`, `WhileTrue`, subject: camera, claims
`camera.horizontal-motion`.

The decision at `:720-731` is: intent = normalize(forwardXZ·(W−S) + rightXZ·(D−A)).
As models, with the input beings from §11 bound as numbers (`bool` satisfies
`std::is_arithmetic_v`, so `propertyValueToNumber` reads `pressed` as 1.0 / 0.0 —
verified at `Form/Singular/Property/PropertyValue.hpp:51`):

```
condition:  All( Compare(@ui.inputCaptured, Eq, false) )
action:     Parallel[
  Map( moveIntent.x, f = (w - s)·fx + (d - a)·rx,
       binds = { w -> @input.key.W.pressed,  s -> @input.key.S.pressed,
                 d -> @input.key.D.pressed,  a -> @input.key.A.pressed,
                 fx -> @camera.forwardXZ.x,  rx -> @camera.rightXZ.x } ),
  Map( moveIntent.z, f = (w - s)·fz + (d - a)·rz,  binds = { … .z } ) ]
```

`forwardXZ` / `rightXZ` are registered as **read-only derived vec3 properties** on
the camera — the pitch-flattening at `:721-723` is *Sense*, and belongs in C++.

**`seed.camera.sprint`** — phase `Intend`, `WhileTrue`, claims `camera.speed-modifier`:
condition `@input.key.V.pressed == true` → `Set(speedScale, 2.5)`. Slow is the twin.
Because `speedScale` is an intent quantity reset to 1.0 each frame, "not sprinting"
needs no law at all — that is accumulator discipline earning its keep.

The engine's `Act`: `pos += normalize(moveIntent) * speed * speedScale` — one line,
after `tick(Intend)`.

**Orientation (`camera.orientation`) is a separate row, and a harder one**: mouse look
runs on GLFW callbacks, not the frame loop. Migrate it *after* movement, by having the
callback write `lookIntent` (an intent quantity) and letting a `Sense`-phase law
convert `lookIntent` → `yaw`/`pitch` with the sensitivity curve. Keep the pitch clamp
first-mover (Gated): an unclamped pitch inverts the world and is hard to escape from
inside it.

### Parity

`orbit_and_strafe.json` — scripted key sequence over 240 frames, compare
`@camera.position` per frame, tol 1e-5.

---

## 11. Worked migration C — keyboard

The most valuable migration in the list, because **a keybinding is a law** — and once
that is true, rebinding, context-sensitive controls, per-Person control schemes, and
in-world macros all become authoring instead of engineering.

**Seam:** `KeyboardHandler::updateGameInput`, `src/Perspective/KeyboardHandler.cpp:290-433`.
That function is 143 lines of the same block repeated eleven times: poll a key, compare
to a `…PressedLast` flag, check `!menuOpen && !anyTextInputActive`, fire a callback.
The `GameKeyStates` struct (`KeyboardHandler.hpp:28-36`) is seven hand-maintained edge
detectors, with three more as function-local `static bool`s (`:349`, `:392`, `:403`) —
which are process-global and would break with two windows.

### R1 — keys are beings

Do **not** add a payload field to `ECA::Event` (§3, rule 4). Mint the beings:

```cpp
// One extra-spatial Singular per bound key, identifier "key-W", "key-SPACE", …
class Key : public Singular {
    std::string getIdentifier() const override { return "key-" + _name; }
    // properties: name (ro string), pressed (ro bool), justPressed (ro bool),
    //             heldSeconds (ro double)
};

// The device that owns them, identifier "input".
class InputDevice : public Singular {
    // properties (flat dotted, like shape.r): key.W, key.SPACE, key.LEFT_SHIFT …
    //   mouse.x, mouse.y, mouse.left, mouse.right, mouse.scroll
    // Poll once per frame in the Sense phase; edge detection lives HERE, once,
    // instead of eleven times.
};
```

Alongside it, mint `@ui` (`UIState`, identifier `ui`) with `menuOpen`,
`textInputActive`, `cursorLocked`, and the derived `inputCaptured` — because
`!menuOpen && !anyTextInputActive` appears in nearly every branch of the legacy
function and is exactly the kind of repeated condition that should be authored once.

### R2 — two events, not eleven

```
key-pressed    subject = the Key being,  object = the InputDevice
key-released   subject = the Key being,  object = the InputDevice
```

A law binds the single trigger `key-pressed` and selects its key with
`ConditionNode::identity("key-F")` — an existing node kind, zero new machinery. This
is why keys are beings: the identity of the key rides in the subject slot the event
vocabulary already has.

### R3 — the binding table becomes law

Every `bindKey(GLFW_KEY_X, "action", callback)` (`KeyboardHandler.cpp:160-285`) becomes
a seed law. The flight toggle at `:220` is the template:

**`seed.input.flight-toggle`** — phase `Sense`, `OnEvent`, trigger `key-pressed`,
claims `input.flight-toggle`:

```
condition:  All( Identity("key-F"),
                 Compare(@ui.inputCaptured, Eq, false) )
action:     Map( @player.flying, f = 1 - x, binds = { x -> @player.flying } )
```

`1 − x` is how a boolean toggle is expressed with the action vocabulary as it stands
(there is no `Not` action node; `Map` over an arithmetic expression covers it, and
`bool` round-trips through `PropertyValue` as 1/0). If this reads as a hack rather
than as arithmetic, that is a legitimate signal to add a `Toggle` action kind — but
file it as a language gap, do not special-case it in C++.

`jump-pressed` from §9 is likewise a seed law: `key-pressed` + `Identity("key-SPACE")`
+ `Publish("jump-pressed")`.

### The Kernel floor for input — read this before starting

Three things stay first-mover **permanently**, declared `Kernel`:

1. **The raw poll.** `glfwGetKey` is Sense.
2. **Cursor release (ESC) and quit.** A law that captured the escape hatch could trap a
   Person inside the world. This is the same guarantee as the manifesto's *"nobody can
   be forced to stay"*, at the input layer.
3. **The menu key.** For the same reason — it is how you reach the controls that would
   fix a broken law.

Everything else — every tool key, every perspective key, undo/redo, the chat toggle —
is `Governable`, and after R5 the entire `updateGameInput` function and the
`GameKeyStates` struct are deleted.

### Parity

`keys_all_bindings.json` — press and release every bound key with the menu open, the
menu closed, and a text field focused; assert the same set of actions fires in the
same order as the legacy path. This scenario is the one that must include a branch
per legacy `if` (§5.5).

---

## 12. Anti-patterns

**Closure seeds.** A "law" built from `addCondition(desc, lambda)` is hard-code with
ceremony: it cannot be saved, edited, synthesized, or read. Those entry points are
for engine bridges only.

**Migrating Sense or Act.** Reading a key or uploading a matrix is first movement.
A law that does it is slower C++ with worse ergonomics. Cut at the Decide seam (§1).

**Two systems, one quantity.** Never enable a seed law without declaring and claiming
the responsibility. Double gravity is silent and its symptom looks nothing like its
cause.

**The program picking a winner.** Resolving contention by authority level, claim order,
zone scope, recency, or any other rule the engine applies on its own is legislation
wearing the costume of a tiebreak. Detect, fall back to the first mover, publish
`jurisdiction-contended`, and wait (§5.3). The *only* automatic resolution is one a
Person or a first mover authored in advance as a default discipline.

**Conflating `authorityLevel` with jurisdiction.** `authorityLevel` bounds who may
*govern* a law; jurisdiction names who *decides* a quantity. Merging them lets a
high-authority metalaw win contests it was never granted, which is precisely the
tyranny the ceiling exists to prevent.

**Forbidding feedback.** Refusing to let a law write what its own condition reads
outlaws every accumulator, every approach-and-stop, every "until full" — and buys no
safety, because the loop is what makes those laws terminate. Bound the *rate* and
protect the *docket* (§6.1–6.3); never bound the domain.

**Confusing the domain with the docket.** The mirror error: permitting reflexive
authorship so enthusiastically that a law may write its own `enabled`,
`authorityLevel`, claims, tier, or budget. A law editing the terms by which it is
judged is not feedback — it is a law stepping outside the reach of Persons.

**Resolving by Synthesis when Partition was the answer.** Composing two laws that were
never really in conflict — they were claiming one coarsely-named responsibility —
produces a fused law nobody wanted and destroys two laws that were each fine. Ask
whether the responsibility can be split before asking whose law survives.

**Silent settlements.** A Concord with no recorded consenters, or one ratified without
the affected zone owners, is not a resolution; it is a coup with good intentions. An
unratified Concord holds nothing.

**Wrong phase.** A law that writes velocity in `Frame` acts one frame late, forever.
The symptom is "it works but feels wrong," which is the hardest kind of bug to
attribute. Write the phase in the ledger before writing the law.

**Level events.** Publishing `still-falling` every frame floods the Rete with facts
and costs real time. Levels are what `WhileTrue` is for.

**Hot loops.** Law evaluation is per-subject closure invocation with property
resolution by string. It is fine per player per frame; it is fine per object per
frame at world scale; it is **not** fine per vertex, per pixel, per SDF sample, or
inside `tessellateSdf`. Pair quantifiers are O(n²) per evaluation (flagged in
`LAW_AND_CREATION_SYSTEM.md` §2a) — never put one in a per-frame movement law.
When the honest answer is "this is too hot," stop at R3: a governed constant read by
fast C++ is a genuinely good outcome, not a failure.

**Widening tolerance until the probe goes quiet.** State the intended difference or
fix the law.

**Batching.** One responsibility per commit. The parity gate is per-responsibility and
a failing batch cannot be bisected.

**Deleting the legacy path at R4.** The whole reason jurisdiction is runtime-reversible
is that a broken authored law must be recoverable by disabling it. Keep the code one
full play-test cycle.

**Unstable identifiers in law-text.** `@law-7.strength` breaks next launch. Every being
that law-text addresses by name needs a stable `getIdentifier()` (§3).

---

## 13. Build order for the framework itself

Each row compiles, each has a test, and each is behavior-preserving on its own.

| # | Commit | Test |
|---|---|---|
| 1 | **Stable identifiers for first movers**: `PhysicsLawBridge` and every future bridge override `getIdentifier()` with a slug; existing law-text referencing generated ids is migrated | `@physics-gravity.strength := 3` works, and still works after save → load → relaunch |
| 2 | **`LawJurisdiction` core**: declare/claim/release/holderOf, `Tier`, legible property surface, `Law::claims()` serialized, `LawManager` claim sync each frame — holder is a **being id**, so relations and formations are already legal holders | declare a responsibility, claim it by law, observe the first-mover guard flip; Kernel refuses; disable the law and the guard flips back |
| 3 | **Contention**: second claim raises a `Contention` being instead of picking a winner, `jurisdiction-contended` published, first mover keeps deciding, contentions join the Universe | two laws claim one responsibility → behavior is byte-identical to pre-migration, the contention is addressable, and a law can quantify over it |
| 4 | **Phases**: `Law::Phase` (append-only, `Frame = 0`), `LawManager::tick(Phase)` filtering both passes, engine calls at the five seams | every existing law and saved world behaves identically; a law set to `Integrate` fires before the integrator, exactly once per frame |
| 5 | **Intent quantities + accumulator discipline**: `moveIntent` / `speedScale` on the camera, `lookIntent`, engine zeroing at `Sense` | a law writing `moveIntent` moves the camera; disabling it stops motion within one frame |
| 6 | **`SeedLaws`**: registry, `installMissing`, version manifest saved with the world, World-as-author + provenance | a seed installs on a fresh world, does not reinstall after a Person deletes it, and appears editable in the Law Author |
| 7 | **`LawParityProbe`** + a headless scenario runner (`tests/parity/*.json`) | the gravity scenario reports zero divergences with the legacy path on both sides |
| 8 | **Migration A** (gravity, jump, flight) through R4, then R5 | `make test-migration-gravity`; play-test |
| 9 | **Migration C** (keyboard) through R5 — before camera, because camera movement depends on legible input | `make test-migration-input` |
| 10 | **Migration B** (camera movement) through R5 | `make test-migration-camera` |
| 11 | **`Concord` + disciplines**: `Partition` / `Precedence` / `Concession` first (no new evaluation needed), then `Synthesis` wired to the existing `LawSynthesis::compose`, then `Arbiter`; consent Formation + ratification gate; negotiation recorded in `Relation::events` | a two-law contention resolves by each discipline; an unratified Concord holds nothing; a Formation of laws appears as `holderOf()` |
| 11b | **`SingularityBudget`** (§6.3): per-tick ceilings, `ApplicationResult::BudgetExceeded`, `budget-exceeded` published, read-only `@budget.*` surface, overflow log | a reflexive law is admitted normally; one that applies past its ceiling is throttled, records why, and cannot raise its own budget |
| 12 | **Telos**: `Law::telosStatement` / `telosTest` serialized, probe repointed at telos tests, per-law preservation report | a proposed synthesis reports which constituent's telos it breaks, before anyone consents |
| 13 | **Zone reach**: `reachOver(being)` over `Belonging` / `Obligation` / `Ownership`; zone-scoped claims; per-subject jurisdiction | a law of Zone A reaches a Person standing in Zone B through affiliation, and contends only over that Person |
| 14 | **Zone regions** (prerequisite for `Presence`): zones acquire a `geom::SdfNode` region, overlap is an SDF intersection, `InRegion` answers containment | two overlapping zones contend only within the intersection |
| 15 | **Ledger sweep**: rows for every remaining candidate, at whatever rung they truly sit | the ledger accounts for every `constexpr` and every `if` in the decide-shaped parts of the update loop |

Commits 2–3 come before phases deliberately: **contention must exist before anything
can be claimed**, or the first migration to be contested will be resolved by whatever
accident the code does when two claims arrive.

Nothing after commit 7 requires new framework machinery for *migration* — every later
migration is the §8 procedure applied to a new row. Commits 11–14 extend what
*governance* can express, and they can proceed in parallel with migrations, because an
uncontested responsibility never needs a Concord.

---

## 14. Known language gaps this framework will surface

Recorded here so migrations report them instead of routing around them with closures:

- **No `Toggle` action kind** — booleans invert via `Map(1 − x)` (§11). Works; reads
  poorly in the Law Author.
- **No vector-valued expressions** — `Map` writes one scalar, so vec3 intent takes
  three parallel nodes (§10). `OntoMath`'s growth path already lists this.
- **No witness binding from quantifiers** — a quantifier proves existence but does not
  export the witness to the action (flagged in `LAW_AND_CREATION_SYSTEM.md` §2a).
  Any migration that needs "act on the thing you found" hits this. This is what
  retired the pair quantifiers (`ForAnyPair`/`ForAllPair`, kinds 12/13) rather than
  growing them: pairs are modelled as Relations now, which name both participants as
  law text. `ForAny`/`ForAll` over a single kind remain and have the same gap.
- **Rete is incremental for event facts, by sweep for property state** — event facts
  propagate as they are asserted (`ReteNetwork::assertFact` maintains the node
  memories and queues activations; `evaluate()` only hands back what is pending).
  `WhileTrue` conditions are still re-evaluated by full sweep each tick. Fine at
  current world sizes; the ceiling is real and worth measuring before migrating
  anything that ranges over all objects every frame.
  - Incremental propagation SKIPS any alpha node no law is bound to and no beta
    reads. That is only safe because binding is retroactive: `bindLawToAlpha` /
    `addBetaNode` backfill the node's memory from the live fact list first, so setup
    order does not matter. Do not add a path that makes a node read without
    backfilling it — see `design_review_remediation.md` §1 (in the repo-root
    `docs/`, not this tree).
- **No per-phase authority ordering** — two laws in the same phase claiming different
  responsibilities that write the same property is legal and unordered. Jurisdiction
  catches the common case (same responsibility); overlapping *properties* across
  responsibilities is a design smell the ledger should catch by review.
- **Zones are extra-spatial** — `Zone::buildProperties` (`Zone.cpp:27-41`) registers no
  position or shape, deliberately. So `Binding::Presence` has nothing geometric to test
  and spatial zone overlap cannot be computed at all today. Giving zones a
  `geom::SdfNode` region is the unlock (§13 commit 14); until then jurisdictional reach
  is affiliation-based only, which covers `Belonging`, `Obligation`, and `Ownership`
  but not "the space where two zones overlap."
- **No `telos` on Law** — the field and its test do not exist yet (§5.3f). Without it,
  consent to a synthesis is a judgment call with no instrument behind it.
- **`LawSynthesis::compose` is pairwise** — n-ary settlements fold it left, which is
  correct for `All`/`Sequence` but produces a lopsided tree that reads poorly in the
  Law Author. An n-ary compose is a small addition worth making before Concords become
  common.
- **No consent primitive anywhere in the codebase** — `Law::_authors` is the nearest
  precedent (a Formation of Persons, gating whether a law may fire). Concord
  ratification should copy its shape rather than invent a second notion of agreement.
- **Newborns are not addressable across laws** — `ActionNode::Create` (see
  `LAW_AND_CREATION_SYSTEM.md` §2b) lets a law mint a generic Object and shape it
  through the node's children, but `@`-paths resolve a *literal* identifier, so a
  later law cannot name "the object the previous law just made." Migrating anything
  whose decision is *build this, then govern it* hits this. A binding slot
  (`Create` writing the newborn's id into a named property, and `@`-paths resolving one
  level of indirection) is the smallest fix.
- **Only `Object` holds elements** — composition landed as an element `Formation` on
  `Object`. A Person, Zone, or Law cannot yet be composed of parts the same way, which
  matters for the set-to-set-over-non-Objects work already listed in §7b of the
  companion doc.
- **`Destroy` sweeps element formations, not every Formation** — a destroyed being is
  released from element containers, but a Law's `targets` Formation could still hold
  it. Any migration that lets laws destroy what other laws target should close this
  first.
