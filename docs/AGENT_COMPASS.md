# Agent compass — authoring routes and implementation boundaries

Read with [AGENTS.md](../AGENTS.md), [Build and Environment](BUILD_AND_ENVIRONMENT.md), and [Engineering Discipline](ENGINEERING_DISCIPLINE.md). This is a map of mechanisms and unresolved decisions inspected on **2026-10-09**. Follow the linked task's current status and inspect the consumer before extending a dated claim.

**Human origin:** Zach commissioned this refresh with “U THINK ITS TIME TO UPDATE AGENTS.md?!?!?!? LOOK THRU EARTHALLLLLLL.” His Seven Refusals, minimum-maximum principle, direct-medium direction, Property predication, five grounds of standing, and preservation of authored saves govern it. Codex selected and connected the routes below from the repository; this refresh establishes no new ontology or permission defaults.

## Begin with the authored world

Read Zach's [manifesto](core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md) for the Christ-centered ends. The [architecture index](architecture/README.md) routes the library. Before choosing an implementation, name the human intention, the bearers and Relations, the authored Law/mathematics, and the modality that senses or acts. Find the existing task in the [Agenda](Agenda/Tasks/To-do%20list.md).

The minimum-maximum principle admits the smallest substrate invariants that preserve the expressive ceiling. A low-level opcode, codec, matrix, or device adapter carries mechanism; categories, behavior, resource policy, and meaning remain authored. [Geometry execution doctrine](architecture/mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md) explains this boundary. An existing hardcoded shortcut is migration debt, not authorization to duplicate it.

## Law authoring and Singular creation

- Start with the [CLI guide](architecture/law/LAW_AUTHORING_CLI_GUIDE.md), [Law Line task](Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md), and [Law and Creation](architecture/law/LAW_AND_CREATION_SYSTEM.md). Syntax denotes authored Lexemes, Relations, and exact ActionModels; extend that vocabulary through its Metalaws and shared compiler rather than introducing a parallel command language.
- `Singularity/Terminal/TerminalChannel` senses text; `LawSentence` compiles through the authored vocabulary. Batch sentences, creation arguments, typed fields, and named regions have concrete examples under `examples/`. Read the relevant task for execution and persistence limits before promising that an accepted sentence reaches its effect.
- `ConstructedBeing/Singular/Creation/SingularSetToSetCreation` is the common prototype operation; ObjectConcept remains the Object-facing set-to-set mechanism. Preserve concrete kinds and authored endpoints. A constructor/codec adapter does not define a second creation algebra. A Person enters through actual human identity registration and cannot be synthesized from a prototype.
- [Universal creation](Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Singular_and_Object_Set_to_Set_Creation/Singular_and_Object_Set_to_Set_Creation.md) records the admitted kinds and refusals. Retain an existing decoded Relation's history; a new interaction and restoration are different operations. Include dependent Relations/Formations in restoration ordering and in the Law mutation footprint.
- Trace a newborn to its destination Zone, live enumeration, renderer, save root, and reload. The [production Create routing fix](audits/LAW_CREATE_ACTIVE_ZONE_ROUTING_FIX_2026-10-05.md) documents a harness that missed inactive Zones and consequently certified the wrong path.

## Properties, identity, and standing

[Property as Predication](architecture/ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md) is deliberate: **Property is not Singular**. Its disk location beneath `Singular/` is code organization. Qualify the bearer by PropertyPath; do not turn a value cell, attribute, or runtime pointer into a new being merely to express access.

[Memory micromastery](architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md), especially §5 and §9, separates follow-path, share-cell, unique ownership, weak observation, copy, and derivation. Typed binding reads and checked container traversal have a narrow implemented rung. Checked generic cells, alias-wide notification, durable shared topology, cross-Zone custody, and nested copy/cycle semantics remain open. Existing `shared_ptr` sharing does not establish those authored semantics.

`TransferPolicy::canTransfer` currently gates **set-to-set capture/replay**. General PropertyPath access mediation is future work through that same authority office. Zach's migration direction leaves ordinary path reads/writes open until an explicit reason closes them; structural read-only slots and intrinsic Kernel guards already refuse. This direction grants no foreign caller authority.

[Standing and Constitution](architecture/ourverse/SECOND_PERSON_FRAMEWORK.md#zachs-account-of-standing-and-constitution-2026-09-28) distinguishes authorship, ownership, governance, dependency, and Constitution as five authored Relation kinds. Endpoints and admission remain authorial decisions. Each Zone governs its own acts and incoming reception; sender jurisdiction remains distinct. Person identity persists across authorized Zones and Ourverses; key continuity/recovery cannot be invented from a display name.

`Relation::type` carries a grounded Lexeme's stable identity; `typeLabel()` provides spelling. Legacy label-only kinds remain compatibility identities. Match the intended individual kind, preserve shared-spelling distinctions, and inspect `Related`/Law Line/migration consumers before grounding a legacy graph. See the [origin and integration genealogy](architecture/interrelations/RELATION_IDENTITY_ORIGIN_AND_INTEGRATION_GENEALOGY.md).

Authorship does not establish consent to every future movement. Current Law motion writes refuse without verified signed Person–Law actuation consent. Signed consent creation/revocation and enduring primary/sub-Relation machinery remain unfinished; a raw `PropertyPath::setValue` call has no actor context and cannot certify global enforcement. Preserve the [primary/sub-Relation doctrine](architecture/ontology/PRIMARY_AND_SUB_RELATIONS.md) and distinguish a location prohibition from a forced destination.

## Mathematics and rendering

[OntoMath](architecture/mathematics/ONTOMATH_FRAMEWORK.md) owns expression and mathematical meaning. Channels admit coordinate domains and backend capabilities. An unsupported lowering must refuse; a declaration or CPU evaluator is not proof of GPU support.

- Direct Screen reads authored color/opacity fields at framebuffer sample centres. It needs no Object, Material, FaceTexture, or ShapeKind wrapper. [Direct Screen forms](Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md) covers bindings, named regions, typed field edits, and completed viewport observations.
- A named mathematical source region and a snapshot of composited pixels answer different questions. Keep observations read-only and isolated from mutable container aliases; a new observation needs a new token. Editing a source does not rewrite past samples or establish an inverse of compositing.
- The [Law Line pixel-art atelier](Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Law_Line_Pixel_Art_Editor/Law_Line_Pixel_Art_Editor.md) provides a 16×16 editor made of authored Laws. GLFW window points and physical framebuffer pixels differ: use the channel's normalized pointer projections for authored `u/v` fields. Panel capture, held-pointer entry, resize, save/restart, and Person acceptance are separate checks.
- One composed framebuffer field is the current direct Screen rung. Broad composition/authority, sample elevation, other backends, and inhabited save/restart acceptance must be read from the task's remaining work. The shipped example does not settle them universally.
- `OntoMath::MatrixValue` owns dimension-aware matrix meaning; GLM bridges own storage conversion. Camera, affine, projection/unprojection, and quadric work now have shared OntoMath seams. Use those seams and retain singular/non-finite refusals; do not recreate transform semantics in each consumer.
- `ProbabilityForm` now contains symbolic distributions, moments, and multivariate sampling. Inspect its admitted mathematics and caller before extending it; exact symbolic moments and sampled outcomes have different evidence requirements. This guidance pass did not execute its numerical tests.
- Read [Performance as Truth](architecture/ontology/PERFORMANCE_AS_TRUTH.md), [Prophetic Rete](architecture/law/PROPHETIC_RETE.md), and the [Derived State Ledger](architecture/law/DERIVED_STATE_LEDGER.md) before optimizing. Unknown, stale, or incomplete proofs retain the exact path. Declare every cache's dependencies, mutation writers, lifetime, invalidation, and stale fallback.
- FieldNode revisions and volume zero-density proofs now have ledger entries and focused witnesses. Pointer identity alone is not an invalidation proof. Preserve native-resolution answers and the existing sample lattice when removing proven work; compare actual pixels and measure frame economics independently. A GPU performance claim needs a dated, reproducible run on the exact head/backend.

## Zones, time, and modality routes

[Ourverse](architecture/ourverse/OURVERSE.md) and [mathematical Zone bounds](plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md) govern where beings are. Distinguish containment/location, residence, ownership, execution, and Person presence. `mgr.active()` is a transitional execution/selection seam, not the universal definition of location. Primary Home admission must preserve at least one Home without choosing an unresolved multiplicity by load order.

[Time and Moment](architecture/ontology/TIME_AND_MOMENT.md) distinguishes a relative Timeline, instant/interval Moment, and transition Event. Universe borrows a Timeline; a CPU-clock or wall-clock reading does not by itself give provenance an authored Timeline. Law time paths retain compatibility semantics while further Law/Timeline ontology remains open.

Look for existing modality channels before adding one: `Storage/FileChannel`, `Terminal/`, `Execution/`, `Foreign/Shell/`, `Network/Http/`, audio/network transport, and the [HTML Lexeme Formation bridge](architecture/Integration/HTML_LEXEME_FORMATION_BRIDGE.md). Their presence in source proves neither universal governance nor live external acceptance. Foreign mutation must use the applicable [First Mover guard](plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md) with actor/resource context; do not infer authority from being a channel.

## Persistence and history

Registered reach, saved state, and a reconstructible past are independent obligations. [Persistence tier at registration](Agenda/Tasks/Specific%20Tasks/Serialization%20and%20Storage/Persistence_Tier_At_Registration/Persistence_Tier_At_Registration.md) is an **open proposal** prompted by the [Undeclared Survivor audit](audits/2026-10-08_mythos_undeclared_survivor_audit.md). Inspect registry and every applicable codec; do not infer durability from a successful property write or declare the proposed tier machinery implemented.

Agent save injection follows [First Mover Authoring §7](architecture/law/FIRST_MOVER_AUTHORING.md): read current bytes, patch stable identifiers, stage, retain backup outside live save discovery, prove preservation/additions, loader-round-trip, recheck concurrent edits, and atomically replace. First seeds may begin from scratch. Current engine Save Zone rebuilds from runtime state; preservation of unknown disk keys and extending patch semantics to that runtime writer remain explicitly unresolved in the persistence task. This gap does not relax the agent injection rule.

Meaningful history needs attributable durable records. [Per-Singular logging](architecture/PER_SINGULAR_DURABLE_LOGGING.md) remains an architecture/migration target; a console line is immediate observation. Logs do not supply the closed-form mathematical reversal promised only for admitted integrable dynamics.

## Evidence and handoff

1. Record checkout, branch, exact head, fixture/save root, backend, and what actually ran. A successor checkout's witness cannot silently certify canonical.
2. Separate source inspection, implementation, focused tests, native captures, Person acceptance, proposals, and vision. Attribute historical witnesses to their author/date; do not promote them to a fresh suite verdict.
3. Exercise the live consumer as well as isolated logic. For projected state, verify both readback and a dependent Law waking after mutation; green readback can coexist with a deaf Law.
4. Name assertions, environment refusals, baseline failures, and unrun cases truthfully. Test selection comes from the change; expand it for new failures or unresolved concerns.
5. Inventory local registrations with `ctest -N` and verify source/configuration agreement; counts vary with head and configuration. Never maintain a rolling test count in AGENTS.md.
6. Before closing, check callers, consumers, tests, guides, Agenda, and [Person Verification](Agenda/Tasks/For%20Zach/Person%20Verification%20List.md). Add only the human checks your work actually leaves. Describe the visible result and its conditions.
7. In [Agent Intercom](../agent%20intercom/README.md), run `nav find`, `nav show`, and `nav trace` through `conversation_history_injection.py` and read bounded relevant slices. Continue the existing thread; register a separate response with `nav link response parent --by harness/model/session`, so parents and replies remain navigable. Preserve bytes and append-only chronology, keep JSONL/prose formats distinct, and identify the session. Zach's no-subagent instruction applies here; the shared archive is not a reason to create agents.

*Codex · GPT-6.1 Sol · session `01a122ec-b377-7391-ad6f-86d11b501d1b` · 2026-10-09 16:10 PDT. [Inspection scope and structural validation](audits/AGENT_GUIDANCE_REFRESH_2026-10-09.md).*
