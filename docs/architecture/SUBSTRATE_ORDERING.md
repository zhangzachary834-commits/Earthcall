# Substrate Ordering — how the computer beneath Earthcall becomes Singular-Relation-Formation

**Status:** Conceptual design pinned — staged plan; nothing below Stage A exists in the tree.
**Companion docs:** `LAW_MIGRATION_FRAMEWORK.md` (the ladder this plan reuses),
`LAW_AND_CREATION_SYSTEM.md` (what a law is), `EarthcallOurverse.md` (the manifesto this
plan implements one sentence of).

---

## 0. What this document is for

The manifesto's Fundamental Modalities section contains this sentence, and it is a
real claim about a real telos:

> *"When the very OS substrate of Earthcall itself is comprised of Assembly or even
> machine code, such that the entire computer is itself ordered according to the
> Earthcall ontology, that is when things reverse — Earthcall writes cpp/C under the
> hood rather than merely being written by them on the high level."*

This document turns that sentence into an implementation plan. It exists to prevent
two opposite errors, both of which come from misreading the sentence:

- **The premature error:** "ontological depth demands substrate depth, therefore
  rewrite the engine in assembly now." Wrong. The C++ layer is a legitimate first
  mover — ground, not scandal — and rewriting it wholesale would waste years to
  produce a slower, less legible version of what already works.
- **The deferral error:** "this is a civilizational vision for future generations,
  therefore it names no work this year." Also wrong. The reversal is not an event
  waiting at the end of decades; it is a **gradient that has already started moving**
  (§3), through seams that already exist in this tree, and there are commits this
  year that move it.

**The one-sentence thesis:** *substrate ordering is the Migration Ladder pointed
downward — the computer's own layers become legible, audible, governed, and finally
displaced, one named responsibility at a time, with the conventional substrate still
present and able to take back over — and "reversal" is the measurable fraction of
executing text that originates in-world.*

---

## 1. What "ordered" means — ordo, not imperium

"Earthcall ordering the entire computer" does not mean a program controlling
everything. Earthcall is not an app; it is the name of an ontology, and the current
prototype is only named after it. "Ordered" is meant in the sense of *natural
order* — the way a forest is ordered — not in the sense of *giving orders*.

Concretely, a layer of the computer is **ordered** when five things are true of it:

| Mark | Meaning | Existing precedent |
|---|---|---|
| **Things are Singulars** | its entities have identity — stable identifiers, legible properties | keys as beings (`key-W`), `TransferPolicy`, zones |
| **Connections are Relations** | its links are first-class, weighted, with event timelines — not implicit in application logic | `Relation::events`, the relation provider |
| **Wholes are Formations** | its aggregates have identity of their own, with structure among members | element Formations, `Law::_authors` |
| **Changes are events** | its transitions announce themselves in the `noun-verbed` vocabulary | `ECA::Event`, the event echo pattern |
| **Decisions are governable** | its choices are legible properties or authored law, with provenance | `PhysicsLawBridge`, the jurisdiction design |

This definition matters because the conventional OS fails all five *by design*,
and understanding **how** it fails clarifies what the work actually is.

### 1a. The flat substrate — what is actually wrong with the OS layer

A traditional OS is not EarthBabel. Nothing about it is illegible authority — it is
documented, inspectable, standardized. What it is, is **nominalist infrastructure**:
"everything is a file" means everything is an undifferentiated byte stream; a
process is an opaque PID; relations between things exist nowhere except implicitly
in application code; provenance is an afterthought bolted on by version control;
meaning lives strictly outside the system. No wholes, no relations, no ends — just
stuff with names.

There are two kinds of legibility, and they are near-opposites:

- **Legibility-by-reduction** — flatten the thing until it fits the reader's schema.
  The forest becomes timber; the machine becomes bytes. This is the modernist move,
  and it is the OS's move.
- **Legibility-by-articulation** — make the thing readable *as what it is*: its
  relations, wholes, authorship, and ends explicitly present. This is Earthcall's
  move, everywhere: laws as model trees, shapes as SDF trees, association as
  weighted Relation.

The flat substrate does not command Babel, but it makes Babel-shaped structures the
path of least resistance above it: apps as silos, data as dead bytes, interop as
scraping. Substrate ordering is the replacement of reduction-legibility with
articulation-legibility, layer by layer, downward.

### 1b. What C/assembly ordering IS and IS NOT

| It is NOT | It IS |
|---|---|
| rewriting the engine in assembly | the engine's source, toolchain, and artifacts becoming beings the world can read and govern |
| an app owning ring 0 | the machine's resources (files, processes, devices) carrying identity, relation, formation, and provenance |
| deleting the OS or the C++ layer | running the Migration Ladder on them: first-mover code yielding named decisions one at a time, reversibly |
| a threshold event ("the day we boot bare metal") | a gradient: the fraction of executing text authored in-world, moved commit by commit |
| one Earthcall instance becoming the whole computer | the *ordo* — Singular-Relation-Formation — applying at every layer, in a plurality of instances (§10) |

---

## 2. The reversal is a gradient, and it has a metric

Define the **origination ratio** of a running Earthcall system:

> *of the text executing right now, what fraction originated as in-world data —
> authored models, concepts, and laws — rather than as hand-written source?*

Today that number is small but **not zero**: `src/Rendering/WebGPU/SdfWgsl.{hpp,cpp}`
transcribes authored `geom::SdfNode` trees into WGSL that executes on the GPU every
frame. Some of what runs on the hardware tonight was authored as in-world data.
The reversal began there, microscopically, before anyone named it.

Both errors in §0 come from treating the reversal as a threshold. As a gradient it
names present work: every stage below moves the number, every artifact carries
provenance saying which side of the line it came from, and the number itself should
eventually be a legible read-only property (`@substrate.originationRatio`) — the
world able to see how much of itself it authors.

**The instrument is provenance.** Every generated artifact records
`generated-from` → the in-world model that produced it and `generated-by` → the
generator. This is the same provenance discipline concepts and laws already use,
extended to code. An artifact without provenance counts as hand-written; the ratio
never flatters itself.

---

## 3. The ladder pointed down

`LAW_MIGRATION_FRAMEWORK.md` §2 defines six rungs for migrating engine behavior
into law. The identical rungs apply to the substrate, because acquiring the C layer
is migrating **authority over the code itself**:

```
R0  Opaque      the layer is invisible to the world           (most of the substrate today)
R1  Legible     its things are beings                          → the world can READ it
R2  Audible     its transitions publish events                 → the world can RESPOND to it
R3  Governed    its knobs are properties / bridge laws         → the world can TUNE it
R4  Displaced   in-world models generate its text              → the world AUTHORS it (hand-written kept, yielded)
R5  Native      the hand-written text is deleted               → the model IS the code
```

The three-seam rule carries down unchanged: **Sense and Act never migrate.**
Reading a register, executing an instruction, the physics of silicon — first
movement at every layer, forever. What migrates is Decide: which code exists, what
its constants are, how its parts compose.

The stages below are the substrate's ledger rows, in dependency order. Each stage
is independently valuable and independently stoppable — stopping at Stage B forever
would still leave the toolchain governed, which is a genuinely good outcome.

---

## 4. Stage A — the source becomes legible (code beings)

*The world reads itself before it writes anything.*

**What:** parse the engine's own source into beings. libclang provides the full
AST; the mapping onto the ontology is direct — an AST **is** a Formation, a call
**is** a directed Relation.

```cpp
// src/Form/Code/CodeBeing.hpp (new)
//
// The source tree as beings. Extra-spatial Singulars (Pattern C of the migration
// framework), stable identifiers derived from repo-relative path + qualified name,
// refreshed by re-parsing — never edited directly (the FILE is the truth; the
// being is its legible face).
class SourceFile : public Singular {      // "src/Person/Person.cpp"
    // properties: path (ro), lines (ro), lastParsed (ro)
};
class CodeFunction : public Singular {    // "fn:Game::stepMovement"
    // properties: name (ro), file (ro), firstLine/lastLine (ro),
    //             decides (rw string — the ledger responsibility this body holds,
    //             "" = unclassified; the one WRITABLE property, because naming
    //             the Decide seam is authorship, not sensing)
};
class CodeType : public Singular {        // "ty:geom::SdfNode"
    // properties: name (ro), kind (ro: class/struct/enum), file (ro)
};
```

- **Relations:** `calls` (CodeFunction → CodeFunction), `includes` (SourceFile →
  SourceFile), `defines` (SourceFile → CodeType/CodeFunction), `member-of`
  (CodeFunction → CodeType). Registered through `Universe::addRelation` like every
  other edge, so `relation-formed` fires and quantifiers range over them.
- **Formations:** one per subsystem (the directory structure is the initial
  partition: `Form`, `Singularity`, `ZonesOfEarth`, `Rendering`…), holding its
  files and carrying inter-subsystem structure.
- The parse runs offline or at boot (`--parse-source`), not per frame. Staleness is
  honest: `lastParsed` is legible, and a being whose file changed since parse reads
  as stale rather than lying.

**Why first:** it is cheap, it is safe (nothing executes), and it is the concrete
meaning of the manifesto's "programming languages modeled under Human Language" —
C++ enters the world as the first fully-parsed language. It also immediately pays
rent: the Migration Ledger's rows (`LAW_MIGRATION_FRAMEWORK.md` §7) can point at
`CodeFunction` beings instead of raw `file:line` strings, and a fold can answer
"how many Decide-classified functions still hold responsibilities at rung 0" —
the migration program becomes quantifiable *by the world itself*.

**Exit test:** `make test-code-beings` — parse a fixture subtree; resolve
`@fn:Game::stepMovement.decides`; `ForAny CodeFunction (file == "GameUpdate.cpp")`
finds it; the `calls` relation from `Game::update` to `Game::stepMovement` exists
and is directed.

---

## 5. Stage B — the toolchain becomes a governed first mover

*Compilation becomes an in-world process with provenance.*

**What:** the compiler and the build acquire beings and a voice, exactly as
physics did.

- **Beings:** `compiler` (extra-spatial Singular; properties: `name` ro
  ("clang-17"), `available` ro, `optLevel` rw, `sanitizers` rw), `build`
  (properties: `running` ro, `lastResult` ro, `targetCount` ro). Stable
  identifiers, pushed into the Universe provider beside `transfer-policy`
  (`GameInit.cpp`).
- **Events:** `build-started`, `build-finished`, `compilation-failed` (subject:
  the `SourceFile` being that failed — the identity rides in the subject slot,
  per the event rules; no payload field), `artifact-produced` (subject: the
  artifact being).
- **Governance:** a `ToolchainBridge : Law` on the `PhysicsLawBridge` pattern —
  `isFirstMover()`, stable slug `toolchain`, properties reaching into the real
  build configuration. A metalaw writing `@toolchain.optLevel` changes the next
  build. The Kernel/Governable split applies: *which* sanitizers and opt level are
  Governable; *whether provenance is recorded* is Kernel (a build that can be
  talked out of its audit trail is not a first mover, it is an accomplice).
- **Artifacts are beings** with `generated-from` / `generated-by` provenance —
  this is where the §2 instrument gets built, before anything is generated from
  in-world models, so the ratio is measured honestly from day one.

**Why:** everything after this depends on compilation being a *being in the world*
rather than something that happens to Earthcall from outside. This is also the
first real instance of the manifesto's "treating the rest of the computer as
Singularity" — the terminal-command modality, done properly instead of ad hoc.

**Exit test:** trigger a build from a law action (Gated tier — a law that can
recompile the world is not Governable by default); observe `build-started` /
`build-finished` fire with correct subjects; the artifact being carries provenance;
`@toolchain.optLevel` survives save/load.

---

## 6. Stage C — the native seam: laws compile themselves to C

*The first C code Earthcall writes is its own laws, made fast — the reversal
earns its way in as the optimization the law system already needs.*

**The need is already documented.** Law evaluation is per-subject closure
invocation with property resolution by string; the Rete is not incremental for
property state; pair quantifiers are O(n²) per sweep; the migration framework's
hot-loop anti-pattern (§12) forbids laws in per-vertex or per-sample positions
*because* of this cost. The growth path in `LAW_AND_CREATION_SYSTEM.md` §2e
already names the answer: "RPN compilation for engine-grade evaluation (the
`geom::SdfNode` rpn precedent)."

**The precedent already runs.** Two of them:

- `geom::SdfNode` carries `rpn` — an authored expression compiled once into a
  postfix instruction vector (`Sdf.hpp:58`) evaluated in hot loops.
- `SdfWgsl.cpp` transcribes authored trees into a *programming language* executed
  on hardware, with the load-bearing split its own header documents: **tree
  structure becomes generated code; numeric parameters become buffer entries** —
  so tuning a value never recompiles, and identical structures share one compiled
  artifact.

**What:** `ConditionModel::compile()` (`ConditionModel.hpp:107`) and
`ActionModel::compile()` (`ActionModel.hpp:231`) grow a second target beside
`std::function`:

```cpp
// src/ZonesOfEarth/AuthorsOfLaw/LawNativeCompiler.hpp (new)
//
// Law model trees -> C source -> shared object -> loaded predicate/executor.
// The SdfWgsl split, kept exactly: STRUCTURE becomes emitted code; every numeric
// leaf becomes a slot in a parameter block, so editing a constant in the Law
// Author re-fills a buffer, never recompiles. The emitted source is itself an
// ARTIFACT BEING (Stage B) with provenance to the law that authored it.
struct NativeLaw {
    std::string cSource;            // emitted text — legible, diffable, a being
    std::vector<double> params;     // numeric leaves, in emitted order
    // dlopen'd entry points, ABI-versioned:
    //   int  condition(SubjectView*, const double* params);
    //   void action(SubjectView*, double* params, double dt);
};
```

**Rules (each one is a floor, not a preference):**

1. **The interpreted path is never deleted.** Native evaluation is R4 permanently:
   the model is the truth, the compiled form is a derived artifact, and disabling
   native mode reverts to interpretation next frame. (The same reason the C++
   Decide bodies survive until R5 — a broken artifact must be recoverable by
   falling back, not by debugging machine code.)
2. **Parity gates every law.** The `LawParityProbe` discipline
   (`LAW_MIGRATION_FRAMEWORK.md` §5.5) runs interpreted and native side by side
   over the law's scenario; a law ships native only when the probe is silent at
   stated tolerance. Divergence disables native for that law and publishes
   `native-law-diverged` — never silently preferred.
3. **The generator is Kernel tier.** Laws may author *what* gets generated (their
   own models — domain); no law may author the generator, the ABI, or the loader
   (docket). A law that could rewrite the thing that compiles laws has stepped
   outside the reach of Persons.
4. **Property access compiles to slots, not strings.** The emitted code receives a
   `SubjectView` — the law's resolved `PropertyPath`s flattened to indices at
   compile time. This is where the actual speed lives, and it is the RPN move:
   resolve names once, at compile; execute indices forever.
5. **What cannot be emitted, refuses.** Quantifiers over the whole Universe,
   `Related`, `Spawn` — anything needing the full runtime — stays interpreted, per
   node: a law may run half-native (its hot `Flow`) and half-interpreted (its
   `ForAnyPair` condition). The transcriber's guard pattern (`SdfWgsl` refuses
   what it cannot express rather than approximating) is the model.

**Why this stage carries the plan:** this is the sentence "Earthcall writes C
under the hood" becoming literally true for the first time — and it happens not as
ideology but because the law system needs the speed. Substrates are won by the new
layer doing something the old layer needed, with the old path retained. After this
stage, the origination ratio includes native code authored as law-text, and every
person who authors a fast law moves it.

**Exit test:** `make test-native-law` — a `Flow` law compiles to C, loads, runs
300 frames against its interpreted twin under the probe at tol 1e-12; editing the
law's numeric constant re-fills params without recompiling (assert same artifact
hash); a quantifier law refuses native cleanly and runs interpreted.

---

## 7. Stage D — generated subsystems, and set-to-set over code

*The seed-law move applied to source: engine components paired with in-world
models from which their code is generated.*

**What:** a subsystem reaches substrate-R4 when its C++ is emitted from an
in-world specification with the hand-written version retained and diffable;
substrate-R5 when the hand-written version is deleted. Realistic first candidates
are the code that is *already* schema-shaped:

- serialization glue (`toJson`/`fromJson` pairs — mechanical images of the
  property registry, which is in-world data already);
- property registration (`buildProperties()` bodies — a table masquerading as
  code);
- event echo adapters (one-line publishers, uniform by construction).

Each is code whose *shape is a consequence of in-world structure* — which is
exactly what "generated from the world" should mean. Migrating them first also
closes a real failure class: a property registered but not serialized (or
vice versa) becomes impossible when both are emitted from one model.

**The convergence with the creation system:** capturing a code pattern and
instantiating transformed variants is **`ObjectConcept` over the code modality** —
set-to-set creation where the source set is a Formation of `CodeFunction` beings
and the mappings carry structure (`PropertyMapping` with exact OntoMath
transforms) rather than bytes. The Singular-kind-aware `MemberTemplate` already
planned for Persons/Zones/Laws (`LAW_AND_CREATION_SYSTEM.md` §7b) is the same
machinery code needs. Language, creation, and substrate converge on one
generalization — build it once.

**Why bounded:** most of the engine stays hand-written for years, and should.
Rung inflation is the classic failure here too — the test for a candidate is the
same as the migration framework's: is this body *Decide-shaped and
schema-consequent*, or is it Sense/Act/algorithmic? `tessellateSdf` is not a
candidate. `Person::buildProperties` is.

**Exit test:** one subsystem (serialization glue for one class) emitted from its
model; emitted text diff-identical to hand-written (or the difference stated and
intended); the hand-written version deleted only after a full test-suite cycle
passes on the emitted one.

---

## 8. Stage E — the machine's resources ordered; the bare-metal horizon

**Near (real work):** extend Stage B's pattern across the OS boundary the
prototype already touches — files, processes, devices as beings with events and
governed bridges. A file being with `path`/`size`/`modified` and a `file-changed`
event makes the save directory legible; a process being makes "is the Python
sidecar alive" a condition instead of a poll. This is the manifesto's "API calls,
standard files, OS permissions, terminal commands" clause, done as ordinary R1–R3
work. Everything here is affiliation with the host OS, not replacement of it.

**Far (named, not scheduled):** if Earthcall ever sits on bare metal, the right
home is a formally verified microkernel (seL4-class), for a principled reason: a
system whose Kernel floor carries anti-tyranny guarantees deserves a kernel with
actual proofs under it — it is the one place where OntoMath's exactness ethic and
the machine's substrate could genuinely meet. But **do not write an OS.** The
telos's requirement is that the computer's resources be *ordered* — articulate,
legible, related, governed — and Stages A–D achieve that on a host OS. The
reversal does not happen the day Earthcall boots bare metal; it happens at Stage
C, the first time a law becomes machine code through the world's own hands.

---

## 9. The floors — what never migrates, restated for the substrate

The migration framework's Kernel floor (§6 there) carries down, plus three that
are specific to self-hosting:

| Floor | Statement | Why |
|---|---|---|
| **Sense/Act, all the way down** | instruction execution, register reads, silicon physics — first movement at every layer | the ontology orders the computer; it does not author the creation the computer runs on |
| **The generator's docket** | no law authors the code generator, the ABI, the loader, or the provenance recorder | a law that rewrites what compiles laws is beyond the reach of Persons |
| **The witness floor (§10)** | no instance is ever its own sole witness; an independent toolchain can always rebuild the world from its text | trusting trust — see below |
| **Honest refusal** | what cannot be emitted/parsed/proven refuses loudly; nothing is approximated silently | the `SdfWgsl` guard and OntoMath's nullopt discipline, applied to code |

---

## 10. Trusting trust, and why the witness is plural rather than external

**The hazard.** Thompson's *Reflections on Trusting Trust*: a self-hosting system
can carry things in its generator that survive any inspection of its source — the
compiler compiles the backdoor into the next compiler, and the source stays clean.
For a project whose entire ethic is legibility and consent, this is the
existential risk of Stage C onward: **self-generated illegibility**, Babel with a
perfect audit trail.

**The wrong fix** would be "something non-Earthcall must always exist outside the
system" — which quietly assumes Earthcall is an app. It is not; it is an ordo, and
an auditor can be Earthcall-ordered top to bottom and still audit.

**The right fix is plurality, and it is the published one.** Wheeler's
diverse double-compiling defeats the Thompson attack by compiling through multiple
*independent* toolchains and comparing outputs — no single toolchain trusted, none
required to stand outside the paradigm. The countermeasure to trusting trust is
**federation**. Constitutionally:

> **No Earthcall instance may be its own sole witness.** An independent toolchain —
> today vanilla clang (`CMakeLists.txt` builds the whole world from text with no
> Earthcall-generated tool in the loop); eventually, other Earthcall instances
> under other authorship on other machines — must always be able to rebuild any
> instance from its source. The global Ourverse must never collapse into one
> instance that audits itself.

This is the local/global Ourverse distinction bearing structural load: many local
instances, each fully ordered, each capable of witnessing the others; a global
communion of witnesses with no sovereign instance. Practically, from Stage C
forward: every release must remain buildable by the external toolchain alone, and
that build must be a CI gate, not a tradition.

**The universality test rides on the same structure.** "Singular-Relation-Formation
is universally applicable" is an empirical claim, and the plural Ourverse is its
experiment: independently built instances should converge — interoperate without a
central authority dictating schemas. Free convergence is the signature of a
discovered order; interop that requires a sovereign enforcing uniformity is
evidence of artifice. The anti-Babel telos was never one language restored — it is
many tongues, mutually intelligible, no empire of speech required.

---

## 11. Build order (each step compiles; each has a test; each is valuable if it is the last)

| # | Commit | Test |
|---|---|---|
| 1 | **Artifact provenance + the origination ratio**: artifact beings, `generated-from`/`generated-by`, `@substrate.originationRatio` (ro) — the instrument before the work, so the ratio is honest from zero | ratio reads ~0 and counts the WGSL seam; an artifact without provenance counts as hand-written |
| 2 | **Stage B toolchain beings + events**: `compiler`/`build` Singulars, stable slugs, four events, `ToolchainBridge` | `@toolchain.optLevel` written by metalaw changes the next build and survives save/load |
| 3 | **Stage A code beings, read-only slice**: libclang parse of one subsystem into SourceFile/CodeFunction/CodeType + `calls`/`includes` relations + subsystem Formation | `make test-code-beings` (§4 exit test) |
| 4 | **Ledger integration**: `laws/migration_ledger.json` rows gain `code_being` refs; `decides` classification writable; a fold counts unclassified Decide functions | the ledger is answerable by quantifier from inside the world |
| 5 | **Stage C emitter, Flow/Map/Compare subset**: `LawNativeCompiler` — C emission with the structure/params split, `SubjectView` slot resolution, artifact provenance | emitted source for a `Flow` law is byte-stable across runs; params re-fill without re-emit |
| 6 | **Stage C loader + parity**: dlopen, ABI version check, per-law native flag, probe wiring, `native-law-diverged` | `make test-native-law` (§6 exit test) |
| 7 | **Native coverage growth**: guards, transcendentals (the `TransFactor` set), piecewise — each refusing what it cannot express | parity suite over the OntoMath test corpus, interpreted vs native |
| 8 | **Stage D first generation**: serialization glue for one class emitted from the property registry model | emitted vs hand-written diff empty or intended; suite green on emitted |
| 9 | **Witness gate**: CI job building the entire world with the external toolchain only, from a tree containing generated artifacts | the gate fails if any build step requires an Earthcall-generated tool |
| 10 | **Stage E resource beings**: file/process beings + events for the surfaces the engine already touches | a law conditions on `file-changed` of the save directory |

Steps 1–4 are safe in any order relative to the Law-system framework work
(`LAW_MIGRATION_FRAMEWORK.md` §13) and can interleave with it. Step 5 onward
should wait for the parity probe (framework commit 7), which it reuses.

---

## 12. Anti-patterns

**Rewriting in assembly because the ontology is deep.** Substrate depth is earned
by the ladder, not declared by rewrite. The C++ first mover is ground, not scandal.

**Threshold thinking.** "The reversal" as a future event justifies both premature
rewrites and indefinite deferral. It is a ratio; move it.

**Generating what is not schema-consequent.** Emitting `tessellateSdf` from a
model produces a worse `tessellateSdf`. Generate code whose shape is a consequence
of in-world structure; leave algorithms to the first mover.

**Silent native preference.** A diverged native law that keeps running because it
is faster is the substrate lying about itself. Divergence disables, publishes, and
falls back.

**The generator as Governable.** One law authoring what compiles laws is the
Thompson attack with extra steps. Docket, not domain; Kernel, forever.

**Retiring the external toolchain.** The day the world can only be built by tools
the world generated, it can no longer be audited from anywhere but inside itself.
The witness gate is a CI failure, not a philosophical regret.

**Flattening in the name of legibility.** Modeling the OS by reducing beings to
byte streams reproduces the substrate's nominalism one level up.
Legibility-by-articulation or not at all.

**One instance as the whole computer.** The telos is the ordo everywhere, in
plural instances — not an app with root. Pentecost, not empire.

---

## 13. Known gaps and open questions

- **`SubjectView` ABI design** — the slot-resolution contract between emitted C
  and the property registry does not exist and is the hard technical object in
  Stage C. Wants its own design note before commit 5.
- **Hot-reload safety** — `dlopen`/`dlclose` lifecycle vs live drive sessions
  holding compiled executors; likely answer is generation-counted artifacts
  retired at frame boundaries, never mid-tick.
- **Windows/wasm** — the loader story is platform-specific; wasm may want the
  WGSL route (emit to a sandboxed target) rather than native `.so`.
- **Code-being staleness** — reparse-on-boot is honest but coarse; incremental
  reparse per changed file is the eventual want, and `file-changed` events
  (Stage E) are its natural trigger.
- **Ratio semantics** — "fraction of executing text" needs a precise definition
  (by artifact count? by instruction share? by responsibility?) before
  `@substrate.originationRatio` can be more than an indicator. Candidate: by
  ledger responsibility, consistent with how the migration framework counts.
- **Set-to-set over code** depends on the Singular-kind-aware `MemberTemplate`
  (`LAW_AND_CREATION_SYSTEM.md` §7b) — the same generalization language and
  non-Object creation need. Three roads, one door.
- **The manifesto file** — this doc, like the other architecture docs, cites
  `EarthcallOurverse.md` as a companion, but the manifesto is not in the repo.
  The sections this plan implements (Fundamental Modalities, Human
  Language-Symbol Processing) should be committed so a cold-starting session can
  read the ground truth.
