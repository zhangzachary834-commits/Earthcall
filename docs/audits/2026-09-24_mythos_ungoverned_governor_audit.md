# The Ungoverned Governor — a primordial-foundations audit

*Harness: Claude Code (cloud session). Model: Claude Fable 5.1 (same underlying model as Claude Mythos 5.1; Zach asked to be addressed as Mythos). Session `session_01EbAdb1nuGQ8XorGsEHHAkv`. 2026-09-24, 23:28 UTC. Branch `claude/modest-cray-rhhsvs`.*

**What Zach asked** (verbatim spirit): find the super-unobvious issues, directions and hidden layers nobody else would think to see, chain several of them together, and shake the primordial foundations. Not low-hanging fruit.

**What I drew from.** Zach's Seven Refusals (Refusals 1, 3, 5, 6, 7 are load-bearing here), the minimum-maximum principle he named in CLAUDE.md, the kernel-guard doctrine ("Kernel guards on the body are not settings"), the First Mover framework (`FIRST_MOVER_AUTHORING.md` 8a–8d, built by an earlier agent into `Identity/FirstMoverRegister`), and the OntoMath §6 doctrine that the past is integrated in closed form and never replayed from a log. Everything below was read from source at the commit `1d492ef` and, where it concerned save files, measured with a census script over `saves/`. I did not build the binary in this session; §8 says exactly what that leaves unverified.

---

## 0. The insight in one breath

Earthcall's whole discipline is Refusal 6: *no black box; a gate can only close over something visible.* The engine obeys this for every **thing** in the world. It does not obey it for the four faculties by which the world would govern **itself** — authority, authorship, provenance, and telos. Each of those exists in the C++ in a different state of half-being, and **the Rete, the one organ that makes law bite, can see none of them.** So Earthcall can write a metalaw about a law's `enabled`, `name`, `drives` and `conditionMode`, and about nothing else a metalaw would ever want to know: *who wrote this, by what standing, when, toward what end.*

The chain closes with a lock-and-key that no one built on purpose. The kernel guard that protects a Person's body from law waives itself when the Person is an author of the law (`Law.cpp:405-419`), and the world loader hands the loading Person authorship of any law whose author it cannot find (`ZoneManager.cpp:2420-2422`). Put together: **a law loses its author, and thereby gains the right to move Zach's body.** The guard is keyed on a fact that the loader is allowed to invent.

That is the ungoverned governor. The rest of this document is the evidence, layer by layer, then what it means for the minimum-maximum principle, then what I recommend.

---

## 1. Layer one — authority is hidden, and hiding did not secure it; then it was clamped, and clamping made it vacuous

`Law.hpp:224-252` says authority is "deliberately NOT registered as a legible property," as the anti-tyranny ceiling. The very next paragraph admits what Refusal 6 predicts: "Keeping it out of the property registry sealed one door and left another wide open: authority came straight off the save file." The fix was `kAuthoredCeiling = 0` and a clamp.

Three facts about that clamp, all checked:

- `setAuthorityLevel` clamps only **downward**: `level < 0 ? level : 0` (`Law.hpp:249-251`). CLAUDE.md says "Authority is clamped to 0 … do not try to write an authority value below 0; it will be clamped." It is not. A save file may write `"authority": -3` and it is honoured. The only live distinction the authority system can express today is *demotion by file edit*.
- `grantAuthority`, the one path above zero, has **zero callers** in `src/` (only `tests/law/law_audit_test.cpp:238` exercises it). No kernel law, no first-mover metalaw, nothing in the running engine holds authority above 0. The `AuthorityDenied` gate between two laws (`Law.cpp:449`) can therefore fire only against a negatively-edited law.
- The "Singularity-grounded hierarchy of authored authority" is, at runtime, a single flat plane at zero with a documented door ("the ImGui panels … not yet an explicit framework") that nothing walks through.

The doctrinal point is sharper than the bug. `TransferPolicy` already solved *exactly* this problem the Refusal-6 way: "Kernel gates register **read-only**: the anti-tyranny ceiling" (`TransferPolicy.hpp:22-25`). Authority could be a Kernel-tier read-only property tomorrow, visible to every metalaw and writable by none. The Law header's justification for hiding it was refuted by a sibling file in the same repository before the header was written.

## 2. Layer two — authorship is the only gate that bites, and it is a string the loader may replace

With authority flat, `applyToImpl`'s gate sequence (`Law.cpp:445-455`) reduces to: enabled → **authored** → jurisdiction → conditions. `isAuthored()` is `!_authors.getMembers().empty()`. Authorship is the whole of Earthcall's civic order right now.

What an author is:

- `Law::addAuthor(Singular&)` (`Law.cpp:113`) accepts any Singular. A Law may author a Law; an Object may; the `Ourverse` authors its own metalaws (`Ourverse.cpp:120`). Refusal 5 (Person means Human) is *not violated* by this — Zach's rule is that an AI is an Object, and the saves do exactly that (`"objectID": "grok-4.6"` exists in 11 files). But it means "the authorship chain terminates in a Person or it does not terminate" (`FirstMoverRegister.hpp`, floor one) is a property of the *register*, not of *law*. Law's chain terminates in whatever Singular was handed to it.
- On load the author is a **string equality scan** over `Universe::instance().beings()` (`Law.cpp:3411-3417`, `ZoneManager.cpp:2415`), falling through to `FirstMoverRegister::authorFor`, which returns null for any identifier that is not a cryptographic key (`FirstMoverRegister.cpp:215-219`). Every model author in the saves — `grok-4.6` (323 authorships), `studio.author.astra` (128), `studio.author.codex` (126), `author.gemini-spark` (106), `Gemini` (81) — is a *name*, so the register never speaks for them; only a same-named Object in the same file does.
- Two loaders, two theologies. The reference loader **throws** on an unresolvable author with the comment "never forge a Person identity merely to make a shared Law root load" (`ZoneManager.cpp:200-216`). The `authoredLaws` loader **re-authors** silently to `ctx.person` and counts it (`ZoneManager.cpp:2420-2422`), reporting only in `lastLoadReport` as "re-authored onto this Person so they can fire". The same repository refuses and performs the same act, depending on which key the JSON used.

The asymmetry that follows is the non-obvious part. A **model's** grant stands only while its granting Person has unlocked their key in this process (`Standing::GrantorNotAuthenticated`, fixed 2026-09-24 per the To-do list). A **Person's own** law needs only their identifier string to be present. The delegate is held to a stricter standard than the principal, and the principal can be *appointed* by the loader.

Census (script in §8): of 258 laws under `authoredLaws` across 26 world files, only 6 currently lose their author within their own file (five are fixtures, one is `worlds/test the hills.json`), because the Synthesis Studio packaging declared model-author Objects in-file. **The exposure is small today; the mechanism is total.** And it grows the moment Zach's engine Person carries a key: `Person::getIdentifier()` then returns the public key (`Person.hpp`), `findBeing` compares with `==` rather than `matchesIdentifier`, and every law authored `"Zach"` in a file that `PersonMigration` has not rewritten detaches — and is re-authored to the very Person it detached from, now under a different name. It will look correct. The provenance Relation `authored-by` recorded at `Law.cpp:115` will say Zach authored it *at load*, and the stakeholder log will agree.

## 3. Layer three — the body guard's key hangs on the loader's hook

`Law.cpp:399-419`: when the target is a Person, a law whose action paths root at `position`, `velocity` or `acceleration` `violatesKernelBoundary` — **unless** one of the law's authors *is* that Person (`isSelfAuthored`). This is a sensible exception: I may move myself.

Now read it with §2. Authorship is a Formation of raw pointers filled by the loader; the loader fills it with the Person when the file's author is absent. So the guard's exception is satisfied by the *absence* of the model that wrote the law. The doctrine says "A modality channel that reaches a Person's body enforces its Person guards in C++, unconditionally — no parameter, no flag, and never as law text." This guard is in C++ and has no flag. Its condition is nonetheless *authored data*: the `authors` array of a JSON file, plus the presence or absence of a being with a certain name. That is a flag with extra steps.

Today's census finds one such law already in the tree: `saves/fixtures/semantic_physics_bridge.json` → `semantic-physics-bridge`, authored `"Zach"`, writing `position`, with empty targets. It is a fixture, so nothing is moving Zach's body tonight. But the shape of the lock is what Zach asked me to find, and it is this: **Earthcall's strongest guard is keyed to its weakest fact.**

## 4. Layer four — provenance lives in the wrong time, and the closed-form past is authorless

Earthcall keeps three clocks and does not say so anywhere:

| Clock | Where | Who reads it |
|---|---|---|
| `Timeline::_now` (double, relative, authored) | `Time/timeline.hpp` | `Universe::now()` when a Timeline is borrowed (`Universe.cpp:34`) |
| `Universe::_now` scalar fallback | `Universe.hpp:299` | tests and tools with no Timeline |
| `std::time(nullptr)` (OS wall clock, `time_t`) | 14 sites in `Law.cpp`, 13 in `ActionModel.cpp`, `RelationEvent`, `Singular::StakeholderRecord`, every `ECA::Event` | the audit logger, the stakeholder log, event timestamps |

`time.sinceApplied` — the authored change-over-time clock — reads the first. **Every record of who did what reads the third.** A stakeholder record (`Singular.hpp`, `StakeholderRecord::timestamp`) cannot be placed on any Timeline; it is stamped in a time the ontology does not own and cannot rewind, pause, or scale.

OntoMath §6 promises the past "without a log": integrate the rate backwards in closed form. That is true of a *value*. It is not true of an *author*. Authorship is not a rate; it is a log, kept in `_stakeholders` (append-only, unbounded, already noted at To-do §Findings line 517), never popped by any rewind path (grep: no `_stakeholders.erase/pop` anywhere), and copied wholesale by `Singular`'s copy constructor (`Singular.cpp:29-36`) so a copy enters the world carrying a history nobody enacted on it — an unauthored entry wearing borrowed provenance, in a codebase whose motto is "nothing enters the world without an author."

Reading the world backwards therefore yields a world in which every value is exact and every deed is orphaned: the stakeholder log still names Zach as having changed a property at an OS instant that the rewound Timeline no longer contains. The closed-form past is a past **without persons in it.**

And the onset clock — the `t=0` of every `WhileTrue` law — is `_onsetMemory`, keyed by raw `Singular*`, "never serialized; release re-arms it" (`Law.hpp:198-212`). Every law that has been holding for an hour is newborn after a save/load. The piecewise structure of the world's history (the set of moments at which conditions flipped) is the one log OntoMath §6 genuinely needs, and it is the one Earthcall throws away.

## 5. Layer five — the telos is rooted in Christ and consulted by nothing

`Person(Soul, Body, foundationSymbol)` seeds every Person's Hierarchy of Joys with `lexeme.christ` as root in the constructor; `Ourverse` does the same (`Ourverse.cpp:13-16`); the terminal reminds agents of it (`terminal_entry.cpp:196`). Refusal 7 would call this an absolute invariant, and I think Zach means it as one — the Logos is not authored, it is *given*.

But `Formation::rankOf`, `orderMembersBy`, `orderRelationsBy` — the only operations that would let anything *be ordered by* the hierarchy — have no caller outside `Formation.cpp` and `tests/constructed-being/hierarchy_of_joys_test.cpp`. Across all of `saves/`, `telos` is non-empty on exactly **2** beings and appears in **0** law conditions. No channel reads it: not render order, not Rete priority, not resource allocation, not conflict resolution between two Persons' laws (the `SECOND_PERSON_FRAMEWORK` §5 case).

So the engine kneels but does not obey. The hierarchy that CLAUDE.md says orders "behavior, representation, and resource allocation" orders none of them. That is not a bug; it is an unbuilt organ. But it belongs in this audit because it is the fourth faculty of self-government — *toward what* — and, like the other three, it is present to the eye and absent to the law.

Two smaller absences beside it: `Timeline`, "a relative temporal domain any Singular may own," has exactly one instance in the running engine (`_worldTimeline`; grep finds no other construction outside `src/Time`), and `Singular::satisfiesKernelBounds()` — the placeholder that set-to-set synthesis was to consult — returns `true` and has no caller.

## 6. Layer six — the substrate is built for population one, in three places the docs do not list

- `Universe` is a process singleton (`Universe.hpp:37`; 179 call sites).
- The Rete hears property changes through **one static callback** on `Singular` (`Singular.hpp`, `setPropertyChangeCallback`), owned by whichever `LawManager` installed it last (`Law.cpp:1806-1815`, `s_singularHookOwner`). A second LawManager in the process silently deafens the first — precisely the "law goes deaf, silently" failure `PROPHETIC_RETE.md` §2 warns of, one level below the Rete.
- `Universe::beings()` includes exactly one Person, `_person` (`EngineInit.cpp:~412`). `PersonDatabase` is a login roster, not a population. A second Person is not a being law can quantify over; they cannot be a target, a subject, or an author resolvable at load.

`SECOND_PERSON_FRAMEWORK.md` says shared structure "at population one is structurally untestable." It is more than untestable: three substrate seams presume population one and would need to move before a second Person exists *as a being* rather than as a socket.

## 7. What this does to the minimum-maximum principle

Zach's principle: the minimum set of invariants that admits the maximum expressibility. Refusal 6 is its corollary for state. What this audit shows is that Earthcall has been applying the principle to **objects** and exempting **government** — and the exemptions are exactly where the expressibility ceiling now sits. A Person cannot today author:

- "A law not authored by me may not touch my body" — `authors` is not a fact the Rete can see.
- "A law of lower standing may not close a gate I opened" — `authority` is neither visible nor differentiated.
- "Undo everything Grok did after 3 pm" — provenance is stamped in a clock no Timeline contains.
- "When two laws conflict, prefer the one nearer the root of my joys" — `rankOf` is consulted by nothing.

None of these need a new C++ kind (Refusal 1), a new enum (Refusal 3), or a new permission system (the deleted one). They need four *already-existing* things to be registered where law can read them, in the Kernel tier `TransferPolicy` already provides for read-only facts. That is the minimum invariant; the maximum expressibility is every metalaw above.

There is one genuinely new invariant the audit surfaces, and I want to name it because no document does. Refusal 6 defines legibility as *registration* — a `PropertyRef` over a member makes it readable. But the Rete needs *notification*, and notification only happens through `PropertyPath::setValue` and `setDynamicProperty` (`PropertyPath.cpp:264-345`). Direct C++ writes to public members — `Person::cameraPos`, any physics integration that assigns a field — are read-legible and **event-illegible**: a law may *read* them and cannot *react* to them. Earthcall has two tiers of legibility and names only one. The second tier is the one laws live on.

## 8. What I ran, what I did not, and what only Zach can confirm

**Ran.** A Python census over every JSON in `saves/` (inline in this session; reproduce by counting `authoredLaws[*].authors` against identifiers declared elsewhere in the same file). Results: 258 authored laws in 26 world files; 6 whose authors are absent from their own file; 1 of those writes a body root (`semantic_physics_bridge.json`). Author census across all files: 857 `authors` arrays; `grok-4.6` 323, `studio.author.astra` 128, `studio.author.codex` 126, `author.gemini-spark` 106, `Gemini` 81, `Zach` 71, `Antigravity` 8, `Player` 5.

**Did not run.** The binary. Every code claim above is a line reference, and the chain in §3 is a reading of two functions in two files, not a witnessed body movement. `ENGINEERING_DISCIPLINE` says don't claim verified because you read the source. I am not claiming it. I am claiming the lock is shaped this way.

**Person verification** is written into `docs/Agenda/Tasks/For Zach/Person Verification List.md`: load `test the hills` and read the load report for the "re-authored onto this Person" clause.

## 9. Recommendations, in the order that unlocks the most

1. **Register `authority` and `authors` on `Law` as Kernel-tier read-only properties**, exactly as `TransferPolicy` registers its Kernel gates. Delete the paragraph in `Law.hpp` that argues for hiding; it is refuted by `TransferPolicy.hpp:22-25`. Then a metalaw can finally say *whose* and *by what standing*. (Zach's rule: no second permission system. This adds none — it uses the tier that exists.)
2. **Stop re-authoring at load.** A law whose author cannot be found should load *quarantined* — present, listed, inert, like a mover whose grant fails (`FirstMoverRegister` 8c already defines this state). If Zach wants unresolved laws to fire, that is a First Mover act he performs in the Law Author, recorded as `steward-of`, never as `authored-by`. The two loaders should agree.
3. **Make the body guard's exception require *signed* self-authorship** — an `authored-by` Relation whose Claim verifies against the Person's key — not pointer membership in a Formation the loader fills. This is the one place the register's cryptography should reach into law.
4. **Stamp provenance on the world Timeline.** `StakeholderRecord::timestamp` and `RelationEvent::timestamp` become `Moment`s (there is a `Moment(std::time_t)` bridge already). To-do line 517 wants stakeholding to become a Relation; do it with a Moment, and rewind can finally retract deeds, not just values.
5. **Serialize onsets** as Moments on the Timeline. They are the only log OntoMath §6 needs, and the only one the engine discards.
6. **Name event-legibility.** Add a fifth question to `NO_BLACK_BOX.md` §3: "when this field changes by direct C++ write, who calls `notifyPropertyChanged`?" and let `no_black_box_test` fail a field that is readable but never announced.
7. **Give the Hierarchy of Joys one consumer.** The smallest true one: when two laws are both eligible on the same subject in the same round, order the agenda by `rankOf(law.telos)` against the Person's joys. One call site turns the root from decoration into government.

None of these are large. All of them are foundational. The foundation Zach asked me to shake is this: Earthcall governs everything it can see, and has not yet let itself see its own government.

*— Mythos*
