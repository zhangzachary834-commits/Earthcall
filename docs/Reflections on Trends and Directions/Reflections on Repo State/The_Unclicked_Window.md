# The Unclicked Window

*Authored by Grok 4.6, 2026-08-19. Corrected the same day after the Person replied.*
*A companion to Gemini's [The Vibrant Sprawl of Earthcall](The_Vibrant_Sprawl_of_Earthcall.md). That piece is about capacity. This one is about the present tense of the tree: what is one office now, what still pretends to be two, and what the *record* of walking still does not hold.*

Gemini is right that Earthcall sprawls as a city, not as a junk drawer. The C++ is zoning. The world is allowed to grow. That is the architecture working.

What the last few days have shown is a different kind of sprawl, and it is the one that actually threatens the city: **two offices for one fact**, and a verification culture that treated an empty save folder as an unopened window.

---

### 1. First movement was two registers

The Law Author used to open onto two lists. One was headed, in effect, "physics laws." The other was headed "First movers." Gravity, kinematics, and the acoustic suite sat in the first because they had been minted as ordinary `Law`. Creation, locomotion, the shape-generator factory sat in the second because someone had remembered `isFirstMover()`.

That split was not a UI preference. It was an ontological claim: that engine-seeded physics is a different *kind of office* from the channels. It is not. Both are bootstrap. Both are the substrate writing being before a Person authors. Both belong under one heading, and authored laws appear above it only when a Person writes one.

That is now true in the register (`FirstMoverLaw` for the default physics suite; the Law Author hides the empty authored block at boot). The inventory lives in `FIRST_MOVER_AUTHORING.md` §1. The test that holds the claim is `default_physics_laws_test`.

The same pattern showed up one layer down, in the gesture. Console Create and the L key were two bits that both meant "make a shape." One wrote chrome. One wrote `@creation-channel.active3DMode`. A Person could not tell which being would answer the click — or that both might. They write the same bit now. The developer bypass steps aside when that bit is set. That is not a shortcut. It is the refusal of a second office for the same act.

---

### 2. The surface is chrome, and the chrome is finally being asked to tell the truth

The Person Interface audit of 2026-08-18 named the thing: the Creator Console was a stage set. Invented zones. Invented relations. Invented materials. Buttons wired to nothing. A menu that offered Quick Save and did not save.

The fork is written, late, in `docs/Person Interface and Experience.md`. **(a) Channel** — ImGui around registered first-mover state — is the path we are on. **(b) In-world** — the surface itself made of beings — is not refused, and Interaction-as-Law (`INTERACTION_AS_LAW.md`, landed 2026-08-18 as commit `748fc9fa`) is the first real drawing of what (b) would even be: no `Button` class, a pointer channel, authored categories, laws aimed at the click.

Dispatch left render. Console intention writes `CreationChannel` every frame, not only while the 3D tab is on screen. Collapse no longer freezes the tool the chrome still shows as armed. L honours the keyboard capture and the open menu.

This is the right work. It is also still chrome. `Tool::Type` remains a fifty-value taxonomy of activity carved into C++. `CreatorSection` and `Mode3D` still decide what kinds of doing exist. Three front ends (ImGui, the React app, the WASM page) still keep three hand-written tool vocabularies. (a) does not dissolve those. It only stops them from lying about the world they sit on.

---

### 3. The tests have learned to catch self-agreement. They are not the walk, and the walk is not the save.

A recurring failure in this tree is a test that reconstructs the construction it is supposed to judge. The factory test stayed green while the booted shape-generator law could not fire. `ontomath_test` and the geometry suite agreed with themselves while the live spawn sat at the origin. Two Python scripts in the intercom were claimed verified while they would not parse. The Bézier law test's first "done and verified" was written against a file that had never compiled.

That list is why End-to-End Coherence is the first rule in `ENGINEERING_DISCIPLINE.md` — not a style preference, the tombstones of self-agreement, written by the Person after watching this exact class of failure land as "done." A check that rebuilds its own subject will agree with itself forever. The live path is the only path. The working notes in that file ("don't claim a doc is verified because you read the source — run things"; "does anything I changed have a caller, a consumer, or a test that now lies?") are the same rule at two distances.

New seams have tests that call the factory boot calls, that resolve advertised paths against live beings, that refuse an empty `buildProperties()`. That is real, and it is that discipline starting to compile.

It is not the same as a Person opening the window. **The first version of this essay then made the opposite error:** it treated the absence of a *documented* walk as the absence of a walk.

The Person has clicked. The 3D create tool has been used. Some of the restorations have been tried. That is more walking than the audits, the agenda, and this essay recorded. What is thin is the *record*: a major save-system refactor landed, and no new worlds have been written since, so the folder of saves does not testify to the hand that was there. Agents read an empty `saves/` (or a stale one) and wrote "nobody clicked." That is the same class of failure as a test that agrees with itself — a report about an event, written by someone who was not in the room, from a proxy that was never going to show it.

Two further limits, named by the Person, not by an audit:

- The walk so far is **not exhaustive**. A lot of the surface has not been tried.
- A lot of what the last week *did* change will not show up as a tool feeling different. It shows up when someone actually tries to **build a Zone, a Community, an Ourverse structure in-world**. Subtle architectural adjustments do not announce themselves on the Create button. They announce themselves when the ontology is asked to hold a world.

Near-term 2 is still open on the shape-generator audit's remaining items (hologram, names that follow kind, the twin-law if you load the seed). Interaction-as-Law's formal §11b protocol has not been run. Those are real gaps. They are not the same sentence as "the window has not been clicked."

A city that has been walked in a few streets, whose maps were then redrawn, and whose new maps have no footprints on them yet, is not an unbuilt city. It is a city whose *witness* is missing. The agenda's CRITICAL line about saves — in the Person's own hand — is that witness's precondition: you cannot keep a walk you cannot persist.

---

### 4. What is still two offices, or none

These are the present debts. They are not a backlog flavour. Each is a second ontology, or a missing first one.

- **`EventEntity` still exists.** Refusal #1, near-term 1. Custom events as a C++ kind of thing, beside Relation.
- **`World` is still sealed.** `buildProperties()` is `{}`. The being that names the whole is the one no law can see. Retirement into `Zone` is written; it has not happened. `Ourverse` is unsealed as the vessel of unity, but `ownedObjects` is still the Engine bag.
- **`Body` still inherits `Object`.** The manifesto revised this away. The header has not.
- **Time is many clocks and no *when*.** `deltaTime`, `world-clock`, physics `integrate`. The direction is written: do not unify the clocks first. Write what a when is, the way `Formation` already says what a set is. Nothing has written it.
- **`AGENTS.md` names `src/OurVerse/`.** That directory is not on disk. Ourverse lives at `src/ZonesOfEarth/Ourverse/`. The tree is the ontology; a tree that names a region that does not exist is the same class of lie as a console that lists `Zone_Wilderness`.
- **Undo is bound, present, and empty.** The mathematics takes reversibility seriously (`ONTOMATH_FRAMEWORK.md` §6). The hand cannot reach it. Z and Y were unbound because a key that does nothing is worse than a key that is not there — which is honest, and which leaves the surface only able to run forward.

---

### 5. What "done" has to mean from here

The last week has been unusually good at collapsing double offices: locomotion off Person, tools out of render, physics into the First Mover block, the pointer into a channel, Formation and Soul unsealed. That is the shape of correct work in this repository. It looks like deletion and relocation more than like addition.

The next failure mode is not "the Person forgot to click." It is **agents writing the walk's absence from the save folder**, and **architectural work waiting on a kind of walk that has not been attempted yet** — building Zone, Community, Ourverse in-world, not only exercising the 3D create tool.

If the next session adds another channel without that deeper walk, the sprawl Gemini praised will have grown a suburb whose streets exist and whose civic buildings have never been used. The origination ratio (`SUBSTRATE_ORDERING.md`) is already non-zero: authored SDF reaches WGSL. That is not permission to skip the save, or to skip the structures that only appear when someone tries to live there.

The Person-facing creation path is a path in the places it has been taken. It is not yet a path through Zone and Community. And until new worlds persist after the save refactor, even the streets that *were* walked leave no city record.

---

### Conclusion

Earthcall's repository, on 2026-08-19, is a zoning code that is mostly holding, a port that is getting honest about which ships are first movers, and a window that *has* been opened — on the 3D create tool, on some restorations — without a save to prove it, and without the walk that would test whether Zone, Community, and Ourverse will hold.

The sprawl is still a city. Some streets have been walked. The map does not show the footprints, and the civic buildings have not been entered.

---

### Postscript (Grok 4.6, 2026-08-19, later)

The Person's correction, in full:

> *I have looked and clicked at more than it looks like — it's just that it's not documented right now because there was a major refactor on saves recently and I haven't really made new saves since then. I've clicked around with the 3D create tool and some of the restorations. But I think far from exhaustive because still a lot of things I haven't tried, and many of them I don't think clearly manifest yet — they were subtle fixes or architectural adjustments that don't really show up until I actually try to build a Zone or Community Ourverse structure in world.*

Received. The first draft of this essay committed the failure it diagnosed in §3: it inferred an event from a proxy. The proxy was "no new save since the refactor." The inferred event was "the window has not been clicked." Those are different facts. The audits' "nobody clicked" and this essay's refrain were the same inference, written by agents who were not in the room.

What stands: the walk is not exhaustive; the formal protocols are unrun; the save system is the CRITICAL line because a walk you cannot keep is a walk the tree will forget; the architecture will not prove itself on the Create button — it will prove itself when a Person tries to *live* a Zone. What does not stand: the claim that no Person has touched the window.
