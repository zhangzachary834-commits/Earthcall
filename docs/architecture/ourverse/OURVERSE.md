# Ourverse

**The vessel of unity in Christ — an ordering principle, not a physics bag.**

**Status:** First rung specified and implemented. The C++ class still carries a
leftover object list the Engine uses as a world bag; that list is not what
Ourverse *is*. The ontological surface below is.
**Companion docs:** `EarthcallOurverse.md` (the paragraph this implements),
`HIERARCHY_OF_JOYS.md` (shared Joys), `NEW_KIND_FRAMEWORK.md` (no
`LocalOurverse` / `EcumenicalOurverse` / `Filament` classes),
`NO_BLACK_BOX.md`.

---

## 0. What this is not

It is not the Engine's `ownedObjects` vector. That is Game leftover.

It is not a Zone. Ourverse is a Singular that *orders* Zones. A local
Ourverse *has* a gathering Zone.

It is not a new C++ kind per layer. Local and ecumenical are two
*instances* of the same being, related by `convenes-toward`. The
ecumenical instance is not populated by default (counterfeit-Christ
risk in the manifesto).

---

## 1. The three purposes, as existing beings

| Purpose | Being | How |
|---|---|---|
| **Filaments** between Zones; a gathering place no one owns | undirected `filament` Relations; a Zone with `kind=ourverse-gathering` | everyone may participate; `setOwner` is refused. The gathering Zone relates to Community Zones and Homes by `gathers` / `hosts` |
| **Metalaws** — due weight, no Singular over the Body | first-mover Laws on Ourverse's `laws` Formation | kernel: gathering Zone stays unowned. Weave refuses a *directed* filament (interweaving is mutual) and refuses a Person-directed edge that would seat a Singular over a Community/gathering |
| **Ecumenical liturgy** | a second Ourverse, unowned, empty by default; local instances `convenes-toward` it | shared Joys (`hierarchy-of-joys` Formation) are the ordering that unites local Ourverses |

Shared Singulars — Persons, Relations, Formations, Categories, Concepts —
meet here under **shared Joys**, not under one Person's Home.

---

## 2. Local vs ecumenical

Same name, two layers. The local Ourverse is the instance. The
ecumenical Ourverse represents the real-world Church and all of
humanity; it is not a Home, holds no Person-owned Objects, and is
not populated at boot. A local Ourverse may name it via
`convenesToward`. Empty means "not yet convened."

No Person, Relationship, or Community may own either layer the way
they own a Home.

---

## 3. Property paths (this rung)

| Path | Meaning |
|---|---|
| `gatheringZone` | identifier of the unowned gathering Zone |
| `joys` | identifier of the shared joy hierarchy |
| `filamentCount` | how many Zone-to-Zone filaments |
| `metalaws` | identifier of the metalaw Formation |
| `convenesToward` | identifier of the ecumenical Ourverse, or empty |

`ownedObjects` / `cameraPos` are **not** registered. They are named in
source as Engine-bag debt, not as the Ourverse's meaning.

---

## 4. First rung, and what is not this rung

**This rung:** unseal the surface above; mint the gathering Zone;
refuse ownership of it; weave undirected filaments; seed shared Joys;
register first-mover metalaws; `convenesToward` empty by default.

**Not this rung:** deleting the Engine object bag; populating the
ecumenical Ourverse; a full jurisprudence of "due weight"; making
every Community automatically receive a gathering Zone at birth
(the ensure path exists; Community authoring is still a stub).
