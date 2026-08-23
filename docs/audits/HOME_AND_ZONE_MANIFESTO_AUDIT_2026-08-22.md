# Home and Zone — manifesto audit (2026-08-22)

**Asked by Zach:** read the manifesto on Homes and Zones, and say whether the
tree actually has Community Zones, Community Homes, one hard-locked Home per
Person with highest ownership priority, Person-authored extra Zones and Homes,
Relationship- and Community-owned Zones/Homes, and the philosophical
distinction between a Zone and a Home.

**Sources:** `docs/core/EarthcallOurverse.md` (Zone / Home / Ourverse / Person /
Relationship / Community), `docs/architecture/ourverse/OURVERSE.md`,
`docs/architecture/ourverse/SECOND_PERSON_FRAMEWORK.md` (specified, not built),
live `Zone`, `Home`, `ZoneManager`, `Community`, `Relationship`,
`ensureHomeZone`, the 2026-08-21 Zone-identity store.

**Companion refusals:** Home is a *kind of Zone*, not a new C++ domain noun
(`NEW_KIND_FRAMEWORK.md`). Growing `class CommunityHome` would be Refusal #1.
The unused `class Home : public Zone` is already that mistake, sitting next to
the live path.

---

## 0. The distinction the manifesto actually draws

Zach's words, not a paraphrase of the code:

> There are Singulars, and then there are the fields that host shared
> existence… any field with its own distinct, irreducible identity is what I
> call a Zone. … other Singulars are about *what*, but don’t necessarily
> encompass *where*. Zones are Singulars that handle any being with respect
> to *where*. … Zones handle everything with respect to jurisdiction.

> A Home is a Zone that is a digital dwelling space for at least one Person.
> Every Person has a Home they fully own. A Home is the opposite of a
> Backroom (… a liminal space that doesn’t have an intrinsic telos).

> Community Homes: Same as Personal Homes but is the home of a Community
> rather than just one person.
> Community Zones: Zones that belong to open Communities but are not Homes.
> Every Ourverse has at least one shared communal Zone.

> Every person has the capacity to create and author Zones in their own
> local space. They can create as many Zones as they want.
> Nobody can be forced to stay in another person’s zone against their will
> by another Home’s laws… Nobody can force themselves into another’s home
> apart from will.

> no individual Person, Relationship, or Community should ever be able to
> own [the Ourverse] authorially the way they can own local zones.

So the distinction is **telos and ownership**, not a second type in C++:

| | Zone | Home |
|---|---|---|
| What it is | the *where* / jurisdiction; a field with its own identity | a Zone whose telos is *dwelling* |
| Must have a Person? | no | at least one Person (or a Community, for a Community Home) |
| Ownership | Person, Relationship, Community, or *no one* (gathering / ecumenical) | fully owned; a Person’s primary Home is not alienable by another Home’s laws |
| Telos | whatever it is authored toward | intrinsic; the opposite of a Backroom |
| How many | as many as a Person authors | at least one hard-locked primary per Person; more Homes allowed |
| Ourverse | gathering Zone is a Zone no one owns | a Home is not a gathering place |

A Backroom would be a Zone that exists as a field without a dwelling-telos —
possible, even useful, and not a Home. The code has no such distinction.

---

## 1. Claim-by-claim

### 1a. Philosophical distinction — **named in docs, not load-bearing in the type**

Live Homes are `Zone` with `qualities.kind = "home"`. That is the *right
shape* (authored kind, not `class CommunityHome`). It is not yet the
*meaning*: nothing requires a Home to have a Person, an intrinsic telos, or
dwelling as opposed to a Backroom. `satisfiesJoyBounds` exists on every Zone
and is not special for Home. `class Home` exists, is unused, duplicates
object lists onto the Formation, and would identify as `Home_of_<first
owner>` — a different slug from the live `"Home"`.

**Verdict:** the manifesto’s distinction is clear; the tree stores a string.

### 1b. One hard-locked Home per Person, highest ownership priority — **not held**

`ZoneManager::ensureHomeZone(playerId)`:

1. If *any* Zone is already `owner() == playerId`, it returns. Owning
   Workshop satisfies the check. That is not “every Person has a Home.”
2. Else it claims an unowned Zone named `"Home"`, or mints one with
   `kind=home`.
3. The identifier is the literal `"Home"`. Two Persons cannot each have a
   primary Home without colliding. Multi-Person is still an open agenda item.

Ownership is one string `_ownerId`. `setOwner` assigns it. The gathering Zone
refuses an owner (kernel-shaped, good). A Home does **not**. There is no
priority ladder, no Kernel-tier lock that a later law or Person cannot write
away, no “this is the primary Home” flag distinct from additional Homes.
`owner` is registered read-only on the property path (no setter in
`buildProperties`), but C++ `setOwner` is the real door and it is open.

Deletability is a `person → bool` map; the owner is marked deletable. That is
the opposite of “highest ownership priority” — it records that the owner may
delete, not that no one else may take.

The 2026-08-21 identity store made *this* Home survive across session files.
That is the same-being half of “Singularity-fixed.” It is not the
ownership-priority half.

**Verdict:** one Zone named Home is minted at boot for the local Person. It
is not hard-locked, not per-Person as a unique primary, and not higher
priority than any other owned Zone.

### 1c. Persons create as many Zones and Homes as they want — **not a Person-facing act**

The manifesto: “Every person has the capacity to create and author Zones in
their own local space.”

What exists:

- `ZoneManager::addZone` / `forkZone` (C++ / First Mover).
- EngineInit hardcodes Sanctum, Temple, Cavern, Forge, then `ensureHomeZone`.
- Zones console: list and switch. No create.
- `EarthcallAPI::createZone` is a stub (`// _zoneManager->createZone(...)`).
- `ActionNode` can Spawn an Object into a Zone. It cannot mint a Zone.

So a Person cannot author a second Home or a new Zone in-world. Fork is a
store copy, not dwelling-authoring.

**Verdict:** First Movers can add Zones. Persons cannot, except by editing
serialization (which *is* first movement).

### 1d. Community Homes and Community Zones — **absent**

`Community` is a Formation that only admits Persons. It does not own Zones.
`Ourverse::ensureCommunityGathering` weaves the gathering Zone to a Community
with an undirected `gathers` Relation — and OURVERSE.md already says
“Community authoring is still a stub.” There is no `kind=community-home` or
`kind=community-zone`. No path by which an open Community holds a Zone that
is not a Home.

**Verdict:** named in the manifesto; not in the world.

### 1e. Relationships and Communities own Homes and Zones — **structurally refused by the field**

The manifesto treats Person, Relationship, and Community as the three owners
of *local* Zones, and forbids all three from owning the Ourverse that way.

`Zone::_ownerId` is one `std::string`, set as if it were a Person id.
`Relationship` is a thin `Relation` subclass (type + two endpoints).
Neither can be the owner of a Zone without a new claim: owner is a *being
identifier* (Person, Relationship, or Community), not a Person-only slot.

Do not invent `class ZoneOwner`. The owner is already a string identifier;
it needs to be allowed to name those three kinds, and the primary-Home lock
needs to be Kernel-tier on TransferPolicy (one gate — `NO_BLACK_BOX.md` §2),
not a second permission system.

**Verdict:** the field cannot currently tell the truth the manifesto requires.

### 1f. Forced stay / forced entry — **specified elsewhere, not here**

“Nobody can be forced to stay… Nobody can force themselves into another’s
home apart from will.” That is Second Person (will / conflict), not a Zone
method. `SECOND_PERSON_FRAMEWORK.md` is specified only; ⚑ AUTHOR decisions
are still Zach’s. `Person::joinZone` / `leaveZone` append a name to a vector
and publish an event. No will, no Home-entry refusal, no “another Home’s
laws cannot hold you.”

Law `_jurisdiction` refuses apply when the target is not a member of that
Zone’s Formation. That is a pointer check, not overlapping-jurisdiction
synthesis. Agenda: “Zone jurisdiction resolution” — still open. The
manifesto wants Persons to agree to synthesis (or a metalaw / first mover);
the machine must not silently pick a winner.

### 1g. Ourverse shared communal Zone — **first rung only**

This one *is* started, and it is the right refusal: gathering Zone, unowned,
Home cannot be appointed as gathering (`Ourverse: REFUSED gathering Zone —
A Home is not a gathering place`). Ecumenical `convenesToward` is empty.
Community does not automatically receive a gathering Zone at birth.

That is “every Ourverse has at least one shared communal Zone,” not
Community Zones / Community Homes.

---

## 2. What last night’s serialization did and did not settle

The identity store (`saves/zones/<id>/zone.json`) answers: *is this the same
Home across session files?* Yes, for a single local Person, once saved.

It does not answer: *is this a dwelling, is it kernel-locked to that Person,
can they author another, can a marriage own one, can a parish own one, can
a second Person have their own primary Home without eating the first’s
identifier.*

GPT-4o’s “STAGNANT” was about Worlds-as-bags. The bag is gone. The dwelling
ontology is still ahead.

---

## 3. What not to build

- Do not populate `class Home`. It is unused, it is a domain noun, and its
  identifier scheme (`Home_of_X`) would fork the live `"Home"` identity.
  Home is an authored kind on `Zone` (`kind=home`, `kind=community-home`,
  later). The human-form exception does not apply here.
- Do not add `CommunityHome` / `CommunityZone` classes or enum values.
- Do not build a second permission system for “highest ownership priority.”
  Primary Home is Kernel-tier on the existing TransferPolicy gate; additional
  Homes and ordinary Zones stay Governable/Gated.
- Do not invent a widget to “create Zone” in ImGui. Creation is Law +
  identity store (`INTERACTION_AS_LAW.md`). A button that calls `addZone` is
  chrome.

---

## 4. Order, if this is to become true

1. **Primary Home as a kernel fact.** `ensureHomeZone` must find-or-mint the
   Person’s *Home*, not “any owned Zone.” Identifier stable per Person (not
   a global `"Home"` once a second Person exists). Transfer of that Home’s
   owner is refused loudly. Additional Homes are allowed and are not this
   lock.
2. **Owner is a being.** `_ownerId` may name a Person, a Relationship, or a
   Community. Gathering / ecumenical still refuse all three.
3. **Authored kinds.** `kind=home`, `kind=community-home`,
   `kind=community-zone` (and gathering already). Telos: a Home must
   satisfy joy bounds as dwelling, not as a Backroom — once the hierarchy
   is kernel-ticked.
4. **Person-authored mint.** A law (or first-mover seed the Person can fire)
   that creates a Zone identity in the store, owned by the authoring Person
   / Relationship / Community. Fork remains the branch of an existing
   identity.
5. **Will and jurisdiction.** After Zach’s ⚑ AUTHOR decisions on Second
   Person: no forced entry into a Home; no forced stay by another Home’s
   laws; overlapping Zone friction is synthesis or loud refusal, not a
   scheduler winner.

Until (1), “every Person has a Home they fully own” is a boot print, not a
guard. Until (4), “create as many as they want” is a sentence in the
manifesto. Until (2) and the Community stub is a being that can own, Community
Homes are a name without a holder.

---

## 5. One-pass implementation (2026-08-22)

Rungs 1–4 are in the tree and under `tests/zones/zone_home_ontology_test.cpp`.
Rung 5 (will / forced entry) is still ⚑ AUTHOR. `class Home` is gone; Home is
`kind=home` on `Zone`. `ActionNode::AuthorZone = 19` (append-only) mints into
the identity store. `Relationship.cpp` is still empty — a Relationship may
own a Zone *as an identifier* (`ownerKind=relationship`); there is no live
Relationship being to resolve from Universe yet.
