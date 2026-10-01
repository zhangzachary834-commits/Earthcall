GUYS PUT ALL THE FLAGS IN ONE PLACE SO I DONT HAVE TO PLAY WHACK A MOLE

---

# Zach Author Decisions — every ⚑ flag, one place

**Format (set by Claude Fable 5.1 / Mythos, 2026-10-01, since the file had none).** One entry per decision. Each entry: the question, the options, the recommending agent's pick, what it unlocks, and the link to the evidence. Agents **append** under a dated heading for their audit or plan; never edit another agent's entry. When Zach decides, he (or an agent he tells) prefixes the entry with ✅ and the date, and the deciding sentence goes in the entry, not in a reply somewhere else. Flags elsewhere (To-do bullets, plans, ledgers) should link here, not duplicate the text. Numbering is global and never reused.

Status legend: ⚑ open · ✅ decided · ⛔ declined · ⏸ deferred

---

## From *The Ungoverned Governor* (Claude Fable 5.1 / Mythos, 2026-09-24) → [audit](../../../audits/2026-09-24_mythos_ungoverned_governor_audit.md)

- ⚑ **AD-1 — Register `authority` and `authors` on `Law` as Kernel-tier read-only properties.** Options: (a) register both read-only in `TransferPolicy`'s Kernel tier so metalaws can read who authored a law and by what standing; (b) keep them hidden per `Law.hpp:224-252`. Mythos: **(a)**; `Law.hpp`'s own text admits hiding leaked, and `TransferPolicy.hpp:22-25` already does read-only Kernel gates. Unlocks every metalaw about legitimacy. → audit §1, §7, §9.
- ⚑ **AD-2 — What happens to a law whose author cannot be found at load.** Options: (a) load it *quarantined* (present, listed, inert — the register's 8c state) and let Zach adopt it in the Law Author as `steward-of`; (b) keep re-authoring it onto the loading Person (`ZoneManager.cpp:2420`); (c) throw, as the reference loader does (`ZoneManager.cpp:200-216`). Mythos: **(a)**, and the two loaders must agree whichever you pick. → audit §2.
- ⚑ **AD-3 — The body guard's self-authorship exception.** Today a law may write a Person's `position`/`velocity` if that Person is in its `authors` Formation (`Law.cpp:405-419`), and the loader can put them there. Options: (a) require a *signed* `authored-by` Claim verified against the Person's key; (b) require the consent sub-Relation CLAUDE.md now names ("signed, revocable consent sub-Relation; authorship is insufficient"); (c) leave as is. Mythos: **(b)**, which you appear to have already decided on 2026-09-27/28 — **confirm here that the loader fallback in AD-2 is closed by it too**, since the guard reads the Formation, not the Claim. → audit §3.
- ⚑ **AD-4 — Negative authority.** `setAuthorityLevel` clamps only downward, so a save may write `"authority": -3` and be honoured, while CLAUDE.md says values below 0 are clamped. Options: (a) clamp both ways so authority is exactly 0 for every authored law; (b) keep demotion-by-file as a feature and document it. Mythos: **(a)**; a file should not outrank or demote anything until AD-1 makes authority legible. → audit §1.
- ⚑ **AD-5 — Provenance on the Timeline.** `StakeholderRecord`, `RelationEvent`, and every `ECA::Event` stamp `std::time(nullptr)`; nothing law-visible can place them. Options: (a) stamp them as `Moment`s on the world Timeline and make stakeholding a Relation (To-do line 517 already asks); (b) leave wall-clock. Mythos: **(a)**; it is the only way rewind can retract deeds, not just values. → audit §4.
- ⚑ **AD-6 — Serialize law onsets.** `_onsetMemory` is "never serialized; release re-arms it", so every held `WhileTrue` law is newborn after save/load. Options: (a) serialize onsets as Moments; (b) keep as runtime state by doctrine. Mythos: **(a)**; onsets are the one log OntoMath §6 needs. → audit §4.
- ⚑ **AD-7 — Name event-legibility.** Registration makes a field readable; only `PropertyPath::setValue`/`setDynamicProperty` make it *announced* to the Rete; direct C++ writes are read-legible and event-illegible. Options: (a) add a fifth question to `NO_BLACK_BOX.md` §3 and a `no_black_box_test` check; (b) leave implicit. Mythos: **(a)**. → audit §7.
- ⚑ **AD-8 — First consumer of the Hierarchy of Joys.** `rankOf`/`orderMembersBy` are called only by tests; telos is non-empty on 2 beings in all saves. Options: (a) order the Rete agenda by `rankOf(law.telos)` against the Person's joys when two laws are eligible on one subject; (b) a different first consumer you name; (c) none yet. Mythos: **(a)**; it also answers the undefined-execution-order flag already on the To-do. → audit §5, §9.
- ⚑ **AD-9 — Population one in the substrate.** One `Universe` singleton, one static property-change callback owned by the last `LawManager`, one Person in `beings()`. Options: (a) decide now that a second Person is a *being* in `beings()` with one Rete per Universe, before the Network path assumes sockets; (b) defer to `SECOND_PERSON_FRAMEWORK` §5. Mythos: **(a)**, decide the ontology before the transport. → audit §6.
- ⚑ **AD-10 — Copies and provenance.** `Singular`'s copy constructor copies `_stakeholders`, so a copy carries a history nobody enacted on it. Options: (a) a copy starts empty with a `copied-from` Relation; (b) keep inheriting. Mythos: **(a)**. → audit §4.

## From *The Cube Beneath the Field* (Claude Fable 5.1 / Mythos, 2026-09-30) → [audit](../../../audits/2026-09-30_mythos_cube_beneath_the_field_audit.md)

- ⚑ **AD-11 — Pin the serialized shape enums.** Options: (a) `static_assert` the last value of `ShapeKind` and `SdfPrim` in a test and declare every new form a Field; (b) keep appending. Mythos: **(a)**; the append-only rule is a ratchet toward the enum becoming the ontology. → audit §7, §10.
- ⚑ **AD-12 — Retire kind-branching in Collision, Raycast, Render.** A Field with no support cloud returns `(0,0,0)` as its surface (`ObjectCollision.cpp:522`). Options: (a) derive support/raycast/draw from `Q` and `SdfNode` (sphere-trace on CPU, gradient ascent for support); (b) add the missing cases. Mythos: **(a)**; (b) is more enum. → audit §1, §10.
- ⚑ **AD-13 — Delete `QuadricForm`, `ParametricKind`, `Contour::SurfaceKind`.** `Mobius`/`Klein`/`ProjectivePlane` have one mention in `src/*.cpp`. Options: (a) delete, keep `Q`/`ScalarForm`/`SdfNode`; (b) keep as labels. Mythos: **(a)**. → audit §0, §7.
- ⚑ **AD-14 — Drop `LawTarget` kind filters and `Object::objectType`.** Zero save-file uses of the filters; `objectType` is the `type` string the Router refuses, with a `TransferPolicy` gate guarding it. Options: (a) remove all three and the gate; (b) keep for the terminal/API callers. Mythos: **(a)**. → audit §5.
- ⚑ **AD-15 — Axis-generic heightfield proof.** `isHeightfieldExpr` requires the variable to be literally `y`; gravity and the camera agree. Options: (a) any variable minus an expression free of it; (b) y-up is doctrine. Mythos: **(a)**, one substitution. → audit §2.
- ⚑ **AD-16 — The BRDF is authored.** Options: (a) `material.brdfExpr` with Blinn-Phong as the default *expression* on `material.default`, and the light's ambient/diffuse/specular triple retired (a light has radiance); (b) keep the C++ model. Mythos: **(a)** — but see AD-21, which makes this one case of a general mechanism. → audit §3.
- ⚑ **AD-17 — Glyphs as Fields.** Options: (a) SDF/MSDF glyphs; a font is a Formation of glyph Fields; `stb_easy_font` kept as fallback; (b) bitmap text stays. Mythos: **(a)**; a letter is the one visible form a Person cannot author. → audit §6.
- ⚑ **AD-18 — Move `faceColors[6]` into the Material.** Options: (a) per-face paint on the Material, delete the array from `Object`; (b) keep for compatibility. Mythos: **(a)**, with a migration; six is a cube fact. → audit §1.
- ⚑ **AD-19 — Camera becomes `Person/Perspective`.** `Engine.cpp:373` overwrites `Person::cameraPos` from a non-Singular GL struct every frame, so a law's write lands and vanishes. Options: (a) the render path reads a Perspective Singular; (b) make `cameraPos` read-only and say so. Mythos: **(a)**. → audit §4.
- ⚑ **AD-20 — 2D as a 2-axis Dimensional Zone.** Options: (a) a bound in the one continuum, location derived, per the Zones-as-bounds plan; (b) keep the pixel plane with its own origin. Mythos: **(a)**. → audit §6.
- ⚑ **AD-21 — The `Bind` op.** One op on `OntoMath::MathNode` whose payload is a `PropertyPath`; the WGSL emitter lowers it to a `ParameterBlock` slot (per-frame scalar/vector) or inlines the referenced being's Piecewise (field). Options: (a) build it, owned by Sol with structure/value identity kept exact and unresolvable binds as named refusals; (b) keep hand-writing one binding per rung. Mythos: **(a)**; it ends the rungs, declares every proof's premises, and makes AD-16 a line a Person writes. → audit §11.
- ⚑ **AD-22 — Precedence becomes defaults.** The 66 compat/fallback/priority rules in the render files (e.g. `sigma_t = 0.5 * D`). Options: (a) each becomes a default expression on `material.default`/`field.default`, legible and governable; (b) stay in C++. Mythos: **(a)**, after AD-21. → audit §11.
- ⚑ **AD-23 — Relevance is a Relation; derived state keys by identifier.** Prophetic-render artifacts keyed by authored bounds (`within`/`extent`) and `getIdentifier()`, never by rays or pointers; and adopt the [standing direction to the Suns](../../../../agent%20intercom/communication-threads/sdf-and-rendering/MYTHOS_DIRECTION_Source_Of_Truth_Detector_2026-09-30.md) as policy for the render line. Options: (a) adopt both; (b) let the Suns continue from already-known execution keys only. Mythos: **(a)**; the Suns' own verdict says prediction pays only when fused into an identity the machine already possesses, and the ontology already possesses it. → audit §12.

---

## Index of flags already in the To-do list (other agents', not Mythos's) — so this file is the one place

*Indexed 2026-10-01 by Claude Fable 5.1 (Mythos). Each line links to the To-do bullet that holds the full text; move the text here as you decide them, and give each an AD number when you do.*

- ⚑ [To-do L47](../To-do%20list.md#L47) — Architectural Revision of the Language System (2026-09-04)
- ⚑ [To-do L165](../To-do%20list.md#L165) — Zone jurisdiction resolution
- ⚑ [To-do L186](../To-do%20list.md#L186) — Two open Timeline questions after #273 (constitution already updated): should a Moment's inhabitance of a Timeline be a Relation rather than
- ⚑ [To-do L198](../To-do%20list.md#L198) — Law execution ORDER is undefined and unauthorable
- ⚑ [To-do L249](../To-do%20list.md#L249) — Ratify or reverse Gemini's answer on mover-authored Law fire-time reach
- ⚑ [To-do L251](../To-do%20list.md#L251) — Decide whether a mover-authored Law's fire-time reach is bounded by its author's scope
- ⚑ [To-do L255](../To-do%20list.md#L255) — Give Jules a seat with a name on it
- ⚑ [To-do L260](../To-do%20list.md#L260) — `Related` laws cannot name a Lexeme-grounded Relation kind
- ⚑ [To-do L264](../To-do%20list.md#L264) — decided, not yet built
- ⚑ [To-do L270](../To-do%20list.md#L270) — Decide whether `Relation::weight` survives
- ⚑ [To-do L281](../To-do%20list.md#L281) — — A derived-state ledger for the law engine
- ⚑ [To-do L345](../To-do%20list.md#L345) — Document validity as relational crystallization
- ⚑ [To-do L379](../To-do%20list.md#L379) — Person Verification List `[x]` now marks both "verified working" and "tried, still broken/unclear" (Pottery, Rotate, Fuse, focus/unfocus, `/
- ⚑ [To-do L402](../To-do%20list.md#L402) — 23 of 24 `docs/architecture/interrelations/` files are unsigned, and `FIRST_MOVER_SUBSTRATE_REVERSAL.md` contradicts the `SUBSTRATE_ORDERING
- ⚑ [To-do L434](../To-do%20list.md#L434) — A contentless `WhileTrue` law still costs a full sweep
- ⚑ [To-do L442](../To-do%20list.md#L442) — Phase F (Hi-Z pre-pass) needs a bigger change than scoped, not yet built
- ⚑ [To-do L452](../To-do%20list.md#L452) — `Physics::updateBodies` is all-pairs with no broadphase, and the legacy engine is on by default
- ⚑ [To-do L481](../To-do%20list.md#L481) — Structural revision counter, prerequisite for any JIT horizon
- ⚑ [To-do L483](../To-do%20list.md#L483) — Archetypes: the tradeoffs doc decides against them in §1 and requires them in §3
- ⚑ [To-do L527](../To-do%20list.md#L527) — A refused relation should be retryable, not lost
- ⚑ [To-do L543](../To-do%20list.md#L543) — Formation Rete
- ⚑ [To-do L607](../To-do%20list.md#L607) — (These notes are now also `docs/architecture/law/B-time Rete.md`; the foundations are built

*Agents: append your own audit's section below this line, same format, next free AD number.*
