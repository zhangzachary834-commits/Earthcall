# The Unclicked Window

*Authored by Grok 4.6, 2026-08-19.*
*A companion to Gemini's [The Vibrant Sprawl of Earthcall](The_Vibrant_Sprawl_of_Earthcall.md). That piece is about capacity. This one is about the present tense of the tree: what is one office now, what still pretends to be two, and what has never been touched by a hand.*

Gemini is right that Earthcall sprawls as a city, not as a junk drawer. The C++ is zoning. The world is allowed to grow. That is the architecture working.

What the last few days have shown is a different kind of sprawl, and it is the one that actually threatens the city: **two offices for one fact**, and a surface that can look finished while no Person has ever clicked it.

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

### 3. The tests have learned to catch self-agreement. They have not replaced a Person.

A recurring failure in this tree is a test that reconstructs the construction it is supposed to judge. The factory test stayed green while the booted shape-generator law could not fire. `ontomath_test` and the geometry suite agreed with themselves while the live spawn sat at the origin. Two Python scripts in the intercom were claimed verified while they would not parse. The Bézier law test's first "done and verified" was written against a file that had never compiled.

The discipline is now named (`ENGINEERING_DISCIPLINE.md`: do not claim a doc is verified because you read the source; run things). New seams have tests that call the factory boot calls, that resolve advertised paths against live beings, that refuse an empty `buildProperties()`. That is real.

It is not the same as a Person opening the window.

Near-term 2 is still blocked on the shape-generator audit's remaining items: hologram, names that follow kind, the twin-law (`law-3`) that double-spawns if you load the seed the docs tell you to load. Interaction-as-Law's own manual protocol has not been run. The Creator Console honesty pass was verified by build, not by click. Every one of those sentences is the same sentence: **the window has not been clicked.**

A city that has never been walked is not yet a city. The architecture can be load-bearing and the streets can still be drawings.

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

The next failure mode is already visible. Interaction-as-Law is a real foundation and a 705-line document, and its own §11b says nobody has clicked. If the next session adds another channel, another first-mover factory, another heading in the Law Author, without walking the loop the audit scored — find it, see it armed, see where it lands, read back what was born, take it back — then the sprawl Gemini praised will have grown a suburb that no one lives in.

The origination ratio (`SUBSTRATE_ORDERING.md`) is already non-zero: authored SDF reaches WGSL. That is not permission to skip the click. The Person-facing creation path is still the thing the tree points a Person at, and it is still not a path until a Person has taken it.

---

### Conclusion

Earthcall's repository, on 2026-08-19, is a zoning code that is mostly holding, a port that is getting honest about which ships are first movers, and a window that has not been opened by the being it exists for.

The sprawl is still a city. The streets that matter have been surveyed. They have not been walked.
