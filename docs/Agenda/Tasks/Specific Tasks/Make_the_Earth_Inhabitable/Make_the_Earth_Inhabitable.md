# Make the Earth Inhabitable

**Status:** OPEN — cross-cutting priority programme

**Origin:** Zach, 2026-09-16. This names a frustration Zach had already been expressing repeatedly across save continuity, Person identity, Home, Law authoring, silent Law failures, unclicked features, and the ImGui-heavy authoring surface. Grok 4.6's `docs/audits/ADVERSARIAL_FULL_TREE_AUDIT_2026-09-15.md` compressed those scattered failures into the diagnosis that Earthcall's characteristic danger is not merely crashing but *silence*: the representation of a working world can remain after the Person's intended world has stopped being true.

**Related:**
- `docs/audits/ADVERSARIAL_FULL_TREE_AUDIT_2026-09-15.md`
- `docs/architecture/law/INTERACTION_AS_LAW.md`
- `docs/Analysis/DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md`
- `docs/Agenda/Tasks/For Zach/Person Verification List.md`
- `docs/Agenda/Tasks/Specific Tasks/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md`
- `docs/Agenda/Tasks/Specific Tasks/Person_Interface_and_Experience_stop_the_surface_lying_then/Person_Interface_and_Experience_stop_the_surface_lying_then.md`
- `docs/Agenda/Tasks/Specific Tasks/Law_concepts_MetaLaws_that_author_laws/Law_concepts_MetaLaws_that_author_laws.md`

---

## 1. The principle

Earthcall is not inhabitable merely because its ontology is elegant, its tests are green, or its subsystems exist.

It is inhabitable when a **Person can dwell in the world as the Person they actually are, act through the world's own authored structures, trust that those acts mean what they appeared to mean, leave, return, and find the same world they meant**.

The canonical boring loop is:

1. A Person enters their Home or Zone and is recognized by stable Person identity.
2. The Person authors or changes a Singular, Relation, Formation, Property, or Law through a Person-facing path.
3. The authored change becomes perceptibly real in the running world rather than only in a data structure, test fixture, or ImGui representation.
4. Laws that should hear it hear it; Laws that refuse or cannot act confess why rather than remaining silently enabled and ineffective.
5. The world saves without substituting names for identities, losing Relations, flattening authored Properties, or letting derived matter overwrite meaning.
6. The Person quits Earthcall.
7. The Person reopens Earthcall and returns to the same Home or Zone.
8. The same beings are still themselves, the same Relations still mean the same things, the same Laws still hear, and the same authored surface still works.
9. The loop is responsive and pleasant enough that the Person naturally continues creating instead of retreating to developer scaffolding.

**When this loop is boring, Earthcall has crossed from demonstration toward place.**

---

## 2. Inhabitability is not "UI polish"

This programme cuts vertically through the system.

A beautiful control over a corrupt save is not inhabitable.
A perfect save behind a tedious or misleading authoring surface is not inhabitable.
A green Law test whose running Law is deaf is not inhabitable.
A Home that persists but does not recognize its Person is not inhabitable.
A fully authored interface that runs at unusable frame times is not inhabitable.
A feature that an agent calls verified but no Person has actually touched is not yet known to be inhabitable.

Therefore inhabitation is an **end-to-end invariant of Person intention**, not a presentation layer.

---

<a id="p0-continuity"></a>
## 3. P0 — The ground remembers: identity, Home, Zone, save, return

The Person must be able to trust the disk before the world can ask for deeper trust.

Required outcomes:

- A `Person` is never reconstructed or resolved as an `Object`, category marker, display-name collision, or other merely similarly spelled being.
- Home ownership follows the Person's stable identity; a loader never creates `Home_of_<name>` merely because two labels failed to resolve to the same Person.
- Per-Zone save/load is the ordinary lived path; conglomerate session/world loading is compatibility or exceptional tooling, not the conceptual center.
- Registered semantic Properties, Relations, Formations, authored Laws, authorship/provenance, transforms, Materials, and other world-bearing state survive save → quit → fresh boot → return.
- Tests, probes, agents, and migration scripts do not mutate a Person's real save tree without explicit authorization and named authorship.
- A persistence failure refuses loudly and preserves the last trustworthy world rather than silently constructing a plausible replacement.

**Acceptance witness:** Zach enters his actual Home, makes a small meaningful change, saves, quits the process entirely, reopens Earthcall, and finds the same Home, same self, same change, and same relevant Relations/Laws without repair work.

---

<a id="p1-hand"></a>
## 4. P1 — The hand belongs inside the world: authored interaction over permanent scaffolding

ImGui and First Mover Window Tools are legitimate bootstrap ground; they must not quietly become the permanent ontology-facing product merely because they are easier for agents to extend.

Required outcomes:

- Stop treating a new ImGui panel as the default answer to a Person-facing need when the capability can now be expressed as authored beings + Laws through `INTERACTION_AS_LAW.md`.
- Migrate one control at a time from developer chrome into the authored world and make each migrated control *better to inhabit* before removing its bootstrap equivalent.
- Start with a small, load-bearing control whose whole causal chain can be witnessed: point/click → Event/Moment → Law → Property/Relation change → visible result → save → reload → still works.
- The Law authoring surface itself must move toward Second-Nature Law Authoring/MetaLaws rather than indefinitely increasing the  First Mover window's complexity.
- Text entry, focus, drag, scroll, 2D/3D layering, selection feedback, and other interactions must become coherent capacities of the world instead of one-off widget behavior.

**Bootstrap rule:** do not delete working ImGui merely to satisfy an aesthetic doctrine; retire a piece only after the Earthcall-native path is demonstrably more inhabitable and the Person has walked it.

---

<a id="p2-creation"></a>
## 5. P2 — The Person can actually make the world: one general creation path

Creation is too central to remain Object-shaped or opcode-shaped.

Required outcomes:

- Singular set-to-set Creation becomes a genuinely general Person-facing creation verb, not a path that still bottoms out in `std::make_unique<Object>()` for the meaningful cases.
- Creatable kinds are discovered from the ontology's actual admissible Singular kinds rather than requiring a new `ActionNode::Kind` whenever Earthcall learns to create another kind of being.
- A Person can author a Singular, its Properties, Relations, and a Law governing it without switching between conceptually unrelated creation systems.
- Law creation ultimately uses the same general authored creation order rather than remaining a privileged ImGui-only/editor-only species of act.
- The existing "one Person-facing creation path, end-to-end" debt is closed on the *booted live path*: create → perceive → save → fresh reload → re-target by stable identity → continue authoring.

**Acceptance witness:** create something whose type is not merely "visual Object", relate it to an existing being, author a Law over the relation/property, save, restart, and continue editing the exact same beings by identity.

---

<a id="p3-hearing"></a>
## 6. P3 — The world confesses: eliminate silent divergence

A crash announces a broken world; a silent Law, stale derived index, misleading control, or stale document can preserve the appearance of truth while severing the Person's intention from reality.

Required outcomes:

- The derived-state ledger becomes operational discipline: every cache/index/compiled view declares what it depends on, what invalidates it, and what test or witness guards that invalidation.
- An enabled/authored Law that reaches nobody for an unexpected structural reason becomes diagnosable from the running world; "registered and enabled" must not be the last observable truth.
- Refusals say *what refused, why, and which identity/path/authority caused it* without silently filtering or fabricating a replacement.
- Person-facing controls never display one operation while writing through a different hidden control/path.
- Documentation status is not runtime status; architecture prose that describes a future must say so, and a documentation-only completion must not use the same evidentiary meaning as a Person-verified runtime completion.
- `[x]` means verified working; `[~]` means tried but unresolved/unclear; no witness file may make brokenness disappear behind a success glyph.

**Development analogue:** a "done" claim that did not exercise the live path is evidence about that narrower path only, never evidence that the Earth is inhabitable.

---

<a id="p4-walk"></a>
## 7. P4 — Walk what we build: Person verification is part of implementation

Earthcall has accumulated more machine-verifiable construction than one Person has had time to inhabit; the queue must periodically reverse direction and return work to the hand.

Required outcomes:

- Every feature whose meaningful failure is visual, tactile, temporal, ergonomic, or experiential routes through `Person Verification List.md` before being called Person-verified.
- Agents explicitly separate "built", "headlessly verified", "live-path exercised by an agent with an actual window", and "Person verified" rather than compressing them into one success word.
- Unclicked backlog is paid down deliberately: Shape Generator live path, Interaction-as-Law manual protocol, Law Author feel, Pottery/Rotate/Fuse ambiguity, Synthesis Studio feel, and similar lived surfaces.
- A Person's report from the running world outranks a reconstructed fixture when they disagree; the response is to find the path divergence, not to explain away the Person's witness with a green test.

**Cadence rule:** substantial construction campaigns should end with an inhabitation pass before the swarm opens another similarly large Person-facing frontier.

---

<a id="p5-rhythm"></a>
## 8. P5 — A place has rhythm: responsiveness at lived scale

A world that is semantically faithful but too slow to inhabit still fails the Person.

Required outcomes:

- Performance is measured on booted, lived worlds as well as synthetic scaling probes: startup, Zone entry, interaction latency, authoring latency, save, reload, and sustained frame rhythm.
- Performance optimizations preserve ontology and Law hearing; a faster path that makes the world deaf is a regression, not an optimization.
- The third/fourth authored domains should become materially cheaper to build as the substrate compounds; record marginal effort rather than assuming reuse.
- Rendering, interaction picking, Law evaluation, persistence, and derived-state maintenance must be bounded enough that richer worlds do not force the Person back into minimal test scenes.

**Acceptance witness:** a representative lived Zone remains responsive while the Person creates, edits, interacts, saves, changes Zone, returns, and continues without a developer-only recovery ritual.

---

<a id="p6-beyond-one-person"></a>
## 9. P6 — Only after one Person can dwell: expand habitation to the second someone

Earthcall's telos is relational, not solitary, but multi-Person life should multiply a trustworthy world rather than multiply unresolved identity and jurisdiction bugs.

Required outcomes:

- Do not use networking or multiplayer spectacle to skip the one-Person continuity loop.
- Resolve the outstanding Second Person author decisions before code invents winners for overlapping jurisdiction, visibility, likeness, shared law, or authority conflicts.
- `Relationship`, `Community`, Ourverse gathering, stakeholder Relations, and shared authored controls must become lived structures rather than C++ stubs or prose-only futures.
- A second Person must enter as a Person with their own stable identity and standing, never by label collision or a globally shared name namespace.

---

## 10. The Inhabitability Gate for new work

Before a substantial new Person-facing feature is treated as higher priority than this programme, ask:

1. **Identity:** does the Person and every touched being remain themselves across the path?
2. **Authorability:** can the Person express the meaningful part through Earthcall's ontology rather than only developer chrome?
3. **Hearing:** do the relevant Laws/Relations actually hear the change, and will stale derived state confess?
4. **Returnability:** does the meaning survive save → fresh process → return?
5. **Legibility:** can the Person tell what happened and why, including refusals?
6. **Felt coherence:** has somebody with a hand actually used the live path?
7. **Rhythm:** is the result responsive enough to continue dwelling and creating?

A "no" does not automatically ban the feature, but it names the debt honestly and prevents scaffolding from masquerading as habitation.

---

## 11. What not to do

- Do not interpret this programme as "rewrite ImGui immediately"; that would destroy the most reliable bootstrap surface before its successor can carry the Person.
- Do not solve inhabitation by adding another parallel UI framework, permission system, persistence path, or creation path; convergence is the point.
- Do not add architecture prose as a substitute for closing a live loop.
- Do not make saves prettier while changing their meaning.
- Do not make tests agree with themselves by reconstructing the same assumptions on both sides of the assertion.
- Do not mark a Person-facing feature done because an agent can name the code that should make it work.
- Do not let cosmic P5/P6 ambitions hide P0 discontinuity; a civilization cannot inhabit a world that forgets the cup on the table.

---

## 12. Programme exit criterion — "a day in Earthcall"

This programme is not complete because every conceivable interface is authored or every roadmap item is finished; it is complete when the following ordinary story stops being heroic:

> Zach boots Earthcall into his own Home as the same Person the save already knows. He moves through a responsive Zone, creates a new being through a comprehensible Person-facing authoring path, relates it to existing beings, gives it a Law, interacts with the authored result directly in the world, changes his mind and edits it, saves, leaves, closes the process, later reopens Earthcall, returns to the same Home, finds the same beings and Relations with the same authorship and meaning, and simply continues.

No duplicate self.
No duplicate Home.
No silent deaf Law.
No disappearing Relation.
No reconstruction ritual.
No requirement that a headless green test overrule what his hand just discovered.
No need to retreat into a developer window for every ordinary act of authorship.

At that point the architecture is no longer merely capable of describing an inhabitable Earth.

**The Earth has begun to be inhabitable.**
