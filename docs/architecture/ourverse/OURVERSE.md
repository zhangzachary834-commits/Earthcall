# Ourverse

**The vessel of unity in Christ — an ordering principle, not a physics bag.**

**Status:** Liturgical surface implemented and tested. All legacy Engine object bag
members, physics/collision stubs, dead Game-split files (`OurverseUI`, `OurverseNodeGraph`,
`OurverseSaveLoad`), and `Engine::_world` naming debt have been decoupled and retired.
The ontological surface below is the sole definition of Ourverse.
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

## 2. Local, global, and ecumenical convening

Zach clarified on 2026-10-02: local Ourverses may be authored through
Create; global Ourverses may not. **Global means the entire continuous
machine-wide space**, not all humanity or every digital Earthcall Person.
Ecumenical unity remains an end of convening; it does not make a runtime
instance a representation of every human being. A local Ourverse may
name its convening destination via `convenesToward`; empty means
"not yet convened."

**Implementation boundary:** the current C++ Ourverse still returns the
literal identifier `Ourverse` and has no local/global extent representation.
The universal creation operation therefore refuses this kind until Zach
specifies the authored structure that identifies its extent. Do not infer
scope from that literal name, introduce a scope enum, or treat a newly
generated identifier as proof that an Ourverse is local. Distinct local
identities and persistence must accompany the eventual creation adapter.

No Person, Relationship, or Community may own either layer the way
they own a Home.

*Clarification recorded by Codex / GPT-6 / session
`01a0e64f-5853-7d30-8196-995b4fd16b89` / 2026-10-02 17:02 PDT;
authorial decision: Zach.*

---

## 3. Property paths (this rung)

| Path | Meaning |
|---|---|
| `gatheringZone` | identifier of the unowned gathering Zone |
| `joys` | identifier of the shared joy hierarchy |
| `filamentCount` | how many Zone-to-Zone filaments |
| `metalaws` | identifier of the metalaw Formation |
| `convenesToward` | identifier of the ecumenical Ourverse, or empty |

`ownedObjects` / `cameraPos` and dead Game physics loops were completely retired
from `Ourverse.hpp`/`.cpp`. The being carries no Engine-bag debt.

---

## 4. First rung, and what is not this rung

**This rung:** unseal the surface above; mint the gathering Zone;
refuse ownership of it; weave undirected filaments; seed shared Joys;
register first-mover metalaws; `convenesToward` empty by default;
retire all dead Game husks and Engine bag debt from the class.

**Not this rung:** populating the ecumenical Ourverse; a full
jurisprudence of "due weight"; making every Community automatically
receive a gathering Zone at birth (the ensure path exists; Community
authoring is still a stub).
