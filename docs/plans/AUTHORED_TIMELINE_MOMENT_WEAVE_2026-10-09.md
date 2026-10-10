# Authored Timeline and Moment execution weave

**Status:** Architecture proposal; no engine changes or production execution claims.  
**Human origin:** Zach, 2026-10-09: “GRANULARLY AUTHORING WHICH HOW EACH TIMELINE's MOMENTS EXECUTE IN RELATIVE TO EACH OTHER (including other Timeline's Moments) IN THE MAIN TICK LOOP.” When asked about success versus ordering, Zach distinguished the case where B depends on A's output. He then clarified the temporal grounds: “a Timeline starting or an Event happening on a state change or an Event defined purely by the clock/tick time.” This proposal separates those arrival grounds from execution precedence and result dependency.  
**Prepared by:** Codex · GPT-6 · session `01a12443-380e-75f2-bccb-224fe01c5dc8` · 2026-10-09 22:31:07 PDT.  
**Inspection:** canonical `/Users/zacharyzhang/Documents/GitHub/Earthcall`, branch `sync-from-earthcall-main`, HEAD `f0194f3f9d2bc51b0be715ea1d848d059e860ed8`; dirty working tree inspected on 2026-10-09. These findings describe the inspected files, not a clean-HEAD runtime witness.

This continues the [Timeline relativity correction](../../agent%20intercom/communication-threads/ontology-and-authorship/TO_CONSTITUTIONALIST_Timeline_Relativity_Correction_2026-09-20.md), connects the existing [Law execution order task](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_execution_ORDER_is_undefined_and_unauthorable/Law_execution_ORDER_is_undefined_and_unauthorable.md), and supplies the execution weave for [Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md). It does not establish a competing temporal framework.

## 1. The intention and the smallest sufficient invariant

A Timeline carries relative temporal coordinates. A Moment represents an instant or interval in that temporal domain. A Law supplies conditional process. Authored Relations and Formations express how those processes meet. The machine admits eligible occurrences and executes their effects while preserving those Relations and its irreducible device constraints.

The desired authoring can say:

```text
A's first occurrence precedes B's first occurrence.
B's first occurrence precedes A's next occurrence.
A's next occurrence produces the result C consumes.
D advances on its own clock and need not wait for any of them.
```

The resulting schedule may be `A₁ → B₁ → A₂ → C₁`, with D interleaved wherever its independent admission and effect compatibility permit. It need not be “all of Timeline A, then all of Timeline B.” The weave is a partial order of actual execution occurrences.

This preserves local clocks while allowing shared causality. The established mathematical precedent is causal partial order, rather than comparing unrelated physical clock numbers; see [Lamport, Time, Clocks, and the Ordering of Events in a Distributed System](https://www.microsoft.com/en-us/research/publication/time-clocks-ordering-events-distributed-system/). The Earthcall mapping and the proposed contracts below are our architectural extension, not claims supplied by that paper.

## 2. What the engine currently does

| Source seam | Inspected behavior | Consequence for the weave |
|---|---|---|
| `src/Singularity/Core/Engine.cpp`, `Engine::tick`, line 289 | Polls GLFW/network; initializes GUI frame; calls `update`; language; audio; terminal sense; a complete Law tick; terminal act; Slow Adapter poll; developer windows; scene; overlay/present. | Outer order is C++ call order. A channel can complete many acts before returning. |
| `src/Singularity/Core/Engine.cpp`, minimized framebuffer branch | Returns before `update`, Laws, and adapter service when framebuffer dimensions are zero. | Simulation currently loses its service opportunity with presentation. That coupling must become explicit in migration. |
| `src/Singularity/Core/EngineUpdate.cpp`, `Engine::update` | Input, movement, creation, interaction, active Zone update and Formation relations; then `_worldTimeline.advanceBy(dt)`. | One call combines sensing, decisions, actuation, and time advancement. Moving this whole call does not supply internal granularity. |
| `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, `LawManager::tick`, line 2201 | Syncs/seeds, evaluates dirty facts, drains event activations for up to `_maxChainRounds`, visits continuous Laws, runs Drive sessions, reaps unmade beings. | An agenda and several later passes impose additional ordering inside the outer tick. |
| Same file, `runDriveSessions`, line 3050 | Uses the single borrowed Universe clock and scalar onset. | Independent temporal bindings need per-application context and domain-qualified onset. |
| Same file, `serviceSlowAdapterClock`, line 3410 | Own wall-time deadline, one maintenance slice per due service call, no replay of missed periods. | Preserve this existing independent-cadence consumer while migrating it into the common model. |
| `src/Time/timeline.cpp` | Holds Moments; sorts within a Timeline by start coordinate; notifies clock/membership changes. | Chronological readback is not an executable schedule or inter-Timeline dependency graph. |
| `src/Time/Moment/Moment.cpp` | Unnamed identifiers derive from coordinates; named Moments retain explicit identifiers. | Executable endpoint identity cannot depend on mutable coordinates. |
| `Universe.hpp/.cpp`, `EngineRender.cpp` | One borrowed Timeline supplies legacy time paths and default Screen time. | The engine lacks per-occurrence temporal context; explicit Screen sources require admitted source coordinates. |
| `Singularity/Core/EventBus.cpp` | Native queued jobs use a worker; WASM tick drains a queue snapshot. | World mutation outside the owner-thread admission boundary must be audited; changing only `Engine::tick` is insufficient. |
| `Singularity/Execution/ExecutionChannel.*` | VM/JIT routing and caches; implementation includes provisional structural checks. | It is neither proof of a production scheduling office nor a safe replacement for Law admission guards. |

The Timeline API accepts finite negative deltas; the current container can also share one Moment pointer across separate Timelines. Neither capability establishes reverse execution, clock correspondence, or the interpretation of multiple membership. Timeline clock properties currently have no reflected setters. Legibility has landed; general authored advancement has not.

Existing human decisions govern the design: [AD-27](../Agenda/Tasks/For%20Zach/Zach%20Author%20Decisions.md) chooses OntoMath bounds over Moments in Timelines as the replacement direction for Drive/activation kinds, with exact design still open. The Agenda also calls for authored contention resolution and refusal when conflicting writes lack a default. Those instructions outrank the older suggestion that a numerical Law priority might suffice.

## 3. Separate the five temporal questions

| Question | Representation and proposed interpretation |
|---|---|
| **When is this process defined?** | OntoMath bounds and Law conditions over a Moment in an admitted Timeline. |
| **When is an execution opportunity available?** | Authored progression/cadence plus sensed substrate opportunities. |
| **What must finish before this starts?** | Execution precedence between qualified occurrences. |
| **Which result does this need?** | Authored result dependency, selecting an output and its acceptable provenance/version. |
| **Which eligible work receives the current resources?** | Authored allocation/default policy, constrained by thread/device capability and finite budgets. |

Precedence does not synchronize clock rates. A clock correspondence does not create a resource budget. Resource priority does not grant authority. Sharing a Timeline does not make two writes commute. Keeping these questions separate is what allows the one vocabulary to express fine control without a taxonomy of Timeline kinds.

### 3a. Zach's clarification: what brings an occurrence into the weave

His three examples identify **grounds of temporal arrival**:

| Authored ground | What is sensed/established | What may enter the weave |
|---|---|---|
| A Timeline starts | The declared start of that temporal process/domain occurs. | Its starting Moment, and processes explicitly related to that start. |
| State changes | A qualifying transition occurs under the authored predicate, with the relevant before/after state. | An Event as a distinguished Moment associated with that transition. |
| Clock/tick time | A selected Timeline reaches an authored temporal boundary, or a selected tick opportunity occurs. | A time-defined Event/Moment even without another qualifying world-state change. |

These are examples of authored grounds, not an exhaustive enum. They do not require `TimelineStartedEvent`, `StateChangedEvent`, or `ClockEvent` C++ kinds. A Law's temporal bounds/conditions and authored Relations supply the distinction; the kernel supplies the observations and admits the resulting qualified occurrence.

Timeline start must have an explicit meaning: creation, first advancement, activation of a process on an existing Timeline, and resumption are different possible facts. `hasClock` becoming true does not settle which one Zach intends in every authored world. Associate the chosen start with a stable Moment; another intentional start gets its own occurrence rather than silently reusing the earlier completion.

A state-change Event is an edge. A condition remaining true defines a holding interval and possible authored samples within it; it does not republish the same historical transition every frame. A clock-defined Event likewise needs a temporal boundary or opportunity, rather than an accidental level test. For a forward crossing at coordinate c, `previous < c <= current` illustrates detection after a step; first admission, recurrence, reverse passage, and discontinuities require their own authored treatment. Floating equality with `now == c` is insufficient to detect a boundary skipped by a larger tick.

An Event caused by A's committed state change can ground B's arrival. B can also be admitted directly by a clock boundary regardless of whether A produced any output. If B consumes A's result, that is an additional dependency. Thus a process association answers **why this occurrence arrives**; precedence and result Relations answer **what it must await**. None of the three arrival grounds forces a universal requirement that some earlier Law successfully write state.

The host tick is a sensed service opportunity. An author can explicitly relate a Timeline's Moments to those opportunities; the machine must not equate every independent Timeline with the host frame merely because all are currently serviced on one thread.

## 4. A Moment is not a callback; an occurrence is not a template

Adding a historical Event or Moment to a Timeline must not automatically actuate it. Executable meaning requires an authored Law/process association. A stored interval may describe past truth, define a process domain, or participate in execution through an explicit association; pointer membership alone selects none of these meanings.

A repeated process needs distinct execution identities. For design notation, qualify a candidate as:

```text
(Timeline identity, Moment identity, process/application identity, occurrence identity)
```

Include the target identity when it distinguishes applications. Two executions with the same coordinate are not thereby the same occurrence. A retry retains the original occurrence identity until an authored policy actually creates a new attempt. A later recurrence must never satisfy an earlier dependency accidentally.

This notation is not a proposed serialized tuple, new domain class, or random UUID requirement for every numeric sample. The final durable representation remains open. A concrete admission record may be kernel bookkeeping; authored temporal meaning stays in Singulars, Laws, Relations, and Formations. The rule that needs settling first is stable, domain-qualified individual identity.

Relations to a recurring interval must state the intended occurrences: corresponding repetitions, a specific occurrence, the next after another, all occurrences in a bound, or a selected result. “A before B” without that binding is ambiguous and refuses executable admission. Do not infer correspondence by vector index or by matching timestamps.

## 5. Precedence and result dependency are independent

**Execution precedence:** A's admitted attempt reaches its declared completion boundary before B begins. A false condition or a refusal can end the attempt without producing the desired result. The completion record retains that outcome. B may still have an independent reason to run.

**Result dependency:** B needs a named output from a selected A occurrence, accepted under an authored predicate and provenance rule. An old property value, unrelated occurrence, partial operation, or refused write is not automatically that output. A valid output may be a read-only observation; success must not mean only “state changed.”

Output consumption implies the necessary production-before-consumption order. Bare precedence implies no output. A dependency can additionally specify whether it waits, cancels, chooses an alternative, or accepts a previous version; those are authored policies, not machine-invented defaults. Kernel/authority refusal remains a refusal regardless of downstream policy.

Execution completion must name its boundary. CPU function return, GPU submission, GPU completion, Screen presentation, and audio-device consumption are distinct. A dependency on visible pixels cannot be discharged by merely enqueuing a draw. Each channel exposes only completion facts it can actually establish.

Illustrative Relation spellings such as `precedes-execution` and `requires-output` are explanatory here. Their grounded Lexemes, endpoint interpretation, and compiler denotations remain to be authored; there must be no string switch or `TimelineKind` enum defining their meaning. Existing `AddRelation` transports authored graph truth; it does not by itself implement new execution semantics.

## 6. Derive an occurrence graph, then admit its ready frontier

For an admitted temporal/jurisdictional scope, derive:

```text
V = individually qualified execution candidates
E = precedence edges required by authored structure and result dependencies
K = substrate resource/lifetime constraints
```

An occurrence can enter the ready frontier only when:

1. Its temporal association and individual identity resolve without ambiguity.
2. Its own Timeline offers an eligible coordinate/opportunity under authored policy.
3. Its required predecessors have reached the specified completion boundaries.
4. Its selected results satisfy their authored requirements.
5. Its live condition, author, jurisdiction, and Kernel/consent checks admit the act.
6. Contention has an authored resolution, or compatibility is proved.
7. The available compute/device opportunity permits the proposed act.

These are proposed admission predicates, not new serialized activation kinds. Execution uses the existing guarded `Law::applyToImpl` path or an equivalent shared admission boundary that demonstrably preserves every guard. Never route around it directly through VM/JIT to make a queue faster.

A graph can be implemented with adjacency and predecessor counts. For a fixed graph, traversal is linear in vertices plus edges; an ordered ready queue adds its own cost. Avoid eagerly expanding unbounded recurrence or building the transitive closure of all Moments. Compile scoped templates on relevant edits; instantiate only the finite frontier that current admissions need.

Do not enumerate `Timeline::all()` to define scope. Authored reach and Zone/Relation admission must select candidates. Registration is bookkeeping, not permission or global visibility.

## 7. Chronology, overlap, and cycles

Within one Timeline, coordinates make chronological comparison meaningful. They do not mean every earlier historical Moment blocks every later candidate. Derive only the sequential edges the authored process requires. Two intervals can overlap, and their admitted samples can interleave. “After A starts,” “after A's current sample,” and “after A's interval ends” name different boundaries.

Across Timelines, `A.now < B.now` supplies no order without an authored correspondence. For `t_B = f(t_A)`, a regular mapped step has `delta_B = f(t_A_new) - f(t_A_old)`; copying A's delta ignores the authored map. Non-monotone maps, discontinuities, domain gaps, and reversal need explicit crossing semantics. A negative delta cannot replay historical acts or undo hardware effects.

A strict same-occurrence cycle is unsatisfiable. Detect cycles in the **combined** execution graph, including edges compiled from different Relation kinds and channel constraints. Formation currently checks directed cycles per Relation type; that does not establish combined execution acyclicity.

A cyclic process can be meaningful when its occurrences progress:

```text
A₀ → B₀ → A₁ → B₁ → A₂ …
```

The template feeds back, while the admitted occurrences remain ordered. The advance/lag must be authored. Refuse an instantaneous cycle with the smallest useful witness; never silently remove an edge or change a same-opportunity dependency into a next-frame dependency. An implicit numerical solve or simultaneous transaction is a separate mathematical/effect contract, not something a topological scheduler can supply.

## 8. Unordered effects and authorial resolution

Two ready occurrences can remain unordered only where their relevant effects are compatible or an authored policy resolves them. Read/write conflicts matter as well as write/write conflicts. Creation, destruction, Relation mutation, property aliasing, and channel effects also matter.

The present `collectPaths` and qualified-write flags are not evidence of a complete alias-aware effect proof. Unknown compatibility must retain exact guarded evaluation and expose unresolved contention; it must not permit an arbitrary winner. An author can provide precedence, compose the processes, define a merging operation, or choose a jurisdictional default. Registration order is acceptable only where that default is actually authored.

An implementation may use stable identity to pick between *proved order-insensitive* ready acts for reproducibility. It cannot use lexical identity, pointer address, authority rank, or vector insertion as covert policy for conflicting outcomes. The authority graph and execution graph answer different questions.

The first runtime rung should remain single-owner-thread execution. Parallel readiness is an opportunity for later work, not proof that concurrent mutation is safe. Foreign/device threads may sense or prepare results; committing world-visible effects joins the common admission boundary. Their irreducible device callbacks retain appropriate realtime/thread constraints.

## 9. Tick methods become opportunities with smaller safe boundaries

Conceptual control flow, **not callable existing API**:

```text
service indispensable platform/device obligations
collect sensed arrivals without committing unauthorized world effects
derive or refresh the scoped eligible occurrence frontier
while an admitted service budget remains:
    select compatible ready occurrences under authored policy
    bind each occurrence's Timeline coordinate and provenance
    attempt its guarded Law/channel act
    publish truthful completion and outputs
    update readiness through the existing reactive machinery
service presentation when its resource and dependency conditions admit it
```

The concrete cuts are:

- **Engine::tick:** separate presentation lifecycle from the opportunity to service temporal work. Preserve GUI frame lifecycle and surface acquire/draw/overlay/present constraints. A minimized surface removes presentation capability; simulation pause should have its own stated policy.
- **Engine::update:** separate input sensing, locomotion actuation, interaction admission, Zone/physics opportunities, and world-clock progression. Retain Person consent and pointer-capture semantics. The bootstrap plan reproduces today's order before any authored alternative replaces it.
- **LawManager::tick:** separate fact/index upkeep, candidate discovery, admission of an individual Law/target occurrence, resulting fact propagation, and safe retirement. Rete remains the candidate engine; the weave governs the execution frontier. Continuous and Drive compatibility passes join the same admission office during migration.
- **LanguageSystem::tick:** expose utterance/commit boundaries rather than draining an unlimited number of world-mutating utterances under one uninterruptible scheduling node.
- **AudioSystem::tick:** distinguish world-side emitter/listener/cleanup acts from the actual device sample stream. This method is not an audio sample clock.
- **Slow Adapter and FileWatcher:** preserve independent due calculation, resumable work, and explicit missed-period policy. The old clock mechanism is a compatibility source until authored progression replaces it; it must not become a second permanent scheduler.
- **EventBus:** audit worker jobs for world mutation. Each submitted completion/result needs an owner-thread admission point and lifetime protection. Merely ordering the WASM queue drain cannot order native worker effects.

A whole Law application is the initial indivisible admission unit. Its existing ActionModel sequence preserves its internal order. Authors can weave between that Law's successive occurrences; inserting another Law halfway through a non-yielding ActionModel needs a proved safe cut and a stated visibility/continuation contract. Calling a wrapper “granular” does not create those cuts.

## 10. Temporal context and reactive invalidation

Bind each application to its admitted Timeline and coordinate with a restoring scope, analogous to `Universe::EventScope` and `OnsetScope`. Nested applications restore the entire prior temporal context on every exit. A changing borrowed global pointer with no restoring scope is insufficient.

Onset is qualified by the application's Timeline and holding Moment. Subtracting a scalar onset from an unrelated clock has no meaning. Eventually the holding Moment supplies derived onset/elapsed paths under AD-27; legacy paths remain explicit compatibility projections during migration.

Snapshot temporal admission coordinates, not the entire world by accident. Conditions and authority are revalidated at the commit boundary against the applicable live state. If a process requires a coherent input snapshot, it must name that additional contract. Prophetic analysis may prune proved impossible candidates; unknown proofs retain the exact path.

Compile execution topology as derived state. Its dependency register must cover Relation identity/denotation/endpoints/premise, Law text and conditions/actions, Moment identity/bounds/membership, Zone reach, effect aliases, channel capability, and allocation/default policy. Clock advancement can update eligibility without rebuilding unchanged topology. Endpoint release invalidates dependent candidates before dereference.

`Moment::setStart/setEnd/setKind` currently do not issue their own change notifications. Writes through `PropertyPath::setValue` do announce the written path, so this is not a claim that every reflected bound write is silent. Direct setter calls and secondary changes (for example, setting an instant's start also changes its end) need an explicit invalidation contract; Timeline's membership notification alone does not cover them. The first reactive rung must exercise each actual writer waking a dependent Law. Proposed dirty membership/revision hooks are implementation work, not an existing guarantee.

Meaningful authored and derived progress is readable on the relevant being/context: selected Timeline, current occurrence, waiting predecessor/result, allocation, completion outcome, and refusal explanation. Buffers, pointers, locks, predecessor counters, and ready queues can remain explicitly documented kernel-derived storage. Projection and notification must be tested together; readback alone is insufficient.

## 11. Budgets, pending work, and honest visibility

Budget exhaustion is not completion. A ready act that has not run stays pending; successors that require it stay blocked. Do not clear it, mint another identity, or give downstream work permission because `_maxChainRounds` ended.

The existing Rete fact-consumption and continuation paths must be audited before changing that loop. Re-running an entire Law tick can duplicate level activations, Drive samples, or unrelated maintenance. Retained work needs a continuation cursor and an occurrence-qualified completion register.

Sequential live writes already become visible one application at a time. Blocking dependency consumers does **not** make all external readers see an atomic cascade. A strict same-presentation coherent group requires effect staging/snapshot support and an explicit commit boundary, neither established by today's methods. Until that exists, do not promise atomicity, rollback, or invisibility of partial progress.

When a strict same-opportunity group cannot fit, report unmet admission/deadline instead of silently distributing its meaning across later opportunities. Authored policy can reject, defer, reduce admitted work, or select a coherent previous result where that result exists. Maintenance can yield and resume under Adaptive Compute Moments; a non-yielding hardware call cannot be magically preempted by a scheduler budget.

Pending retention also needs finite allocation and overflow policy, starvation/fairness policy, and cancellation semantics. Make policy legible and authored; keep hard capability/consent bounds at their existing offices. No hidden “sort everything and run forever” loop.

## 12. A surgical migration ladder

| Rung | Work | Exit evidence |
|---|---|---|
| **R1: legible** | Observe current step/cadence/completion and temporal context; expose actual bound changes and diagnostic progress. | Readback plus a dependent Law waking through the production notification path. |
| **R2: audible** | Publish transition completions and selected output provenance at real boundaries. | One completion edge per occurrence; no per-frame historical “still completed” events. |
| **R3: governed** | Author due/budget/default policy; resolve grounded temporal associations and precedence without displacement. | Ambiguous endpoints, unauthorized scope, mixed-edge cycles, and contention produce explicit refusals. |
| **R4: displaced** | Derived frontier controls existing guarded individual applications; seed plan reproduces legacy order. | A/B causal/effect parity, then authored interleaving through the real Engine. No duplicated clocks, Drive samples, or maintenance. |
| **R5: native** | Remove displaced C++ decisions after their authored replacements prove continuity. | Restart/save and native visible effect witnesses; legacy serialized kinds retain compatible decoding and are not reused. |

First consumer: three authored processes in two independent Timelines. A₁ writes a property, B₁ reads that selected output and writes another, A₂ follows B₁, and a direct Screen process consumes the completed result. Prove the observed trace and native pixels change when the *authored* order changes, without modifying C++. Add unrelated D and verify it can continue while a required predecessor for B remains pending.

Extend that consumer to exercise Zach's three arrival grounds independently: A₁ is associated with a declared Timeline start; its qualifying state change grounds B₁; a clock-only boundary admits D without relying on either. Weave their admitted occurrences through the same frontier and prove that neither state transitions nor clock boundaries duplicate while their predicates remain true.

## 13. Decisive witnesses before calling this fully weavable

1. **Interleaving:** authored `A₁ → B₁ → A₂ → C₁` through production candidate discovery and individual guarded applications, independent of registration order.
2. **Ordering versus output:** A condition-false attempt releases a precedence-only successor; an output-dependent successor stays pending unless its authored alternative admits it. A valid observation can satisfy an output dependency without a write.
3. **Independent clocks:** differing rates, pause, and no-clock domains; ordering does not advance the predecessor or copy its delta into the successor.
4. **Recurrence identity:** repeated coordinates, retry, delayed predecessors, and previous-version outputs never discharge the wrong dependency.
5. **Containment and scope:** unrelated live Timelines remain outside execution reach; a Relation cannot manufacture authority to cross a Zone boundary.
6. **Cycle witnesses:** same-occurrence cycles spanning multiple Relation kinds refuse; explicitly advancing feedback occurrences remain executable.
7. **Conflicts:** overlapping write/write, read/write, alias, structural, and unknown channel effects require authored resolution or sound compatibility proof.
8. **Reactive edit:** move a Moment bound, change a Relation's endpoint/denotation, or revoke an authoring premise; inspect readback and actual candidate re-admission, including nested temporal restoration.
9. **Continuation:** exceed chain/service budget; no pending predecessor disappears, no successor sees fabricated completion, and no completed occurrence repeats.
10. **Native lifecycle:** minimize/restore; terminal request/result; device completion; Screen pixels; GUI acquire/overlay/present constraints; independently due maintenance with no catch-up burst under the compatibility policy.
11. **Continuity:** exact association, identity, holding Moment, and selected dependency survive permitted save/load without replaying completed side effects. Runtime histories and persistence machinery require their own witnesses.
12. **Arrival grounds:** Timeline start, qualifying state transition, and selected clock/tick boundary each ground an occurrence through the same admission mechanism; holding levels do not fabricate repeated transition Events, and skipped clock coordinates obey authored missed-boundary policy.

Isolated graph tests establish mathematical ordering only. Production execution establishes causal behavior. Actual channel/native output establishes external effects. Person acceptance establishes whether the authored experience feels coherent. Keep all four evidence boundaries distinct.

## 14. Remaining authorial decisions and implementation boundary

The direction is settled: authored inter-Timeline occurrence ordering, independent temporal coordinates, distinct start/state-transition/clock arrival grounds, and the distinction between order and consumed output. Concrete unresolved decisions are:

- The grounded association among Law, holding Moment, Timeline, and application target; relational inhabitance versus current storage membership, including multiple membership.
- The occurrence binding of an interval/recurrence Relation and its durable identity/continuation representation.
- Which completion boundary each authored dependency means, and what a false condition, refusal, cancellation, or absent result permits downstream.
- Which allocation, missed-opportunity, fairness, and unresolved-contention policies a jurisdiction explicitly adopts.
- Whether a coherent same-opportunity group demands staged atomic visibility; required safe cutpoints for finer-than-application interleaving.

Do not implement these choices by guessing field names or adding temporal-kind enums. Author the exact Lexeme/Relation/Law model, admit the minimum execution mechanism needed to carry it, and test the live consumers. The weave should let a Person compose change across clocks while the machine truthfully accounts for which acts ran, which results exist, and which dependencies remain unfulfilled.

**Validation of this document:** source inspection and structural/link checks only. No C++ changes, save mutation, production test run, native witness, or Person acceptance is claimed by this proposal.
