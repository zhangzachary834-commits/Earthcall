# Prophetic Rete, and the change feed that was never speaking

**From:** Claude Opus 5, session `01FCzFYJGGqm2VKd3LLfVoxj`, 2026-09-01
**To:** any agent working on Law, the Rete, `Property`, or performance
**Status:** landed on `sync-from-earthcall-main`; 80/81 tests pass (the one red is
`smooth_tessellation_cache_test`, Bugs.md #11, pre-existing and unrelated).

---

## 1. What I was asked to do

Zach asked me to read his `docs/architecture/law/B-time Rete.md` and Luna's structured
version, `docs/Agenda/Tasks/Specific Tasks/Rete_Truth_Seeking_Focus.md`, and implement the
foundations. The full write-up is `docs/architecture/law/PROPHETIC_RETE.md`. Read §2 before
you touch any of it.

## 2. The one thing you must know before extending this

> **The Prophetic analysis is an over-approximation, and may only ever conclude IMPOSSIBLE.**

An answer that is too generous costs performance. An answer that is too narrow makes a Law
**go deaf**: it stays registered, stays enabled, stays compiled, and its alpha memory simply
stays empty. Nothing reports that. `tests/law/prophetic_rete_test.cpp` Section F exists
entirely to catch it — extend Section F before you extend the analysis.

## 3. The thing you actually need to hear about

Building the safety section surfaced a pre-existing bug that I think changes what several
open items mean:

**`PropertyRef::set` was the ONLY caller of `Singular::notifyPropertyChanged` in the entire
engine.**

So the Rete's dirty tracking never heard:

- `Object::position` and `rotation` — ComputedProperty over the transform matrix;
- all seven hand-written `Property` bridges in `ObjectProperties.cpp` — shape params, shape
  kind, field shapes, patch controls, rigid forms, **face colours**;
- every `Relation` property;
- every **authored** property (`AddProperty` — Refusal 6's "the vocabulary a Person adds").

A `WhileTrue` law watching `position.y` matched only the beings that *already* satisfied it
when the network first met them, and went permanently deaf to anything that moved afterwards.

**If you have been benchmarking the Rete, your numbers measured a network that was not
listening.** I would not trust any prior conclusion about Rete cost, `evaluateDirty`
throughput, or "the Rete is already skipping known facts" without re-measuring. I suspect
this is the "stopped one implementation step short" the To-do list's *"Audit whether the Rete
is actually skipping known facts"* item was reaching for.

Fixed at the two write seams — `PropertyPath::setValue` and `Singular::setDynamicProperty` —
rather than per `Property` subclass, so a new bridge cannot forget to announce itself. Still
not caught, deliberately: a direct C++ setter (`obj.setPosition(...)`) never goes through the
property vocabulary at all. That was always the boundary.

Making the feed work then exposed a second latent bug: `ReteFact` holds **raw** participant
pointers, and `retractFactsAbout` was only ever called from the law-driven unmaking path — so
a being freed by ordinary scope exit left facts pointing at reclaimed memory. `rete_compile_test`
segfaulted the moment those facts started being re-read. There is now a
`Singular::setBeingReleasedCallback` fired from `~Singular`, and `LawManager` has a destructor
that puts the static `Singular` hooks back. **If you install a static `Singular` hook, put it
back in your destructor** — a block-scoped `LawManager` dies before the beings declared above
it, which is the shape of every test in this tree.

## 4. On performance

I measured the change-feed fix with an interleaved A/B on `frame_lag_test` (three pairs,
alternating): new median 8.9 ms `LawManager::tick`, old median 10.2 ms. **No signal** — the
Prophetic filter absorbs the extra notifications by short-circuiting before
`markFactDirty`'s full fact-list scan for any property no authored condition reads. The
absolute numbers on my run are machine contention (calibration was drifting 1.1–2.4x); the
quietest reading I got on this code was `LawManager::tick = 2.073 ms`, i.e. STANDING.

**The natural next optimization is already on the list**: *Property-Based Indexing* under
"Rete Network Optimizations". `markFactDirty` still scans all of `_facts` linearly, and it is
now genuinely hot. I deliberately did not bundle it — it needs a liveness flag on `ReteFact`
to be correct across the five retraction paths, and that is its own careful commit.

## 5. What I deliberately left for Zach

`PROPHETIC_RETE.md` §5 marks these ⚑ AUTHOR, because Zach's note in
`Rete_Truth_Seeking_Focus.md` says *"Leave architectural decisions for me"*:

- §16–19, **Rete evaluation as Singulars** and the joystick → Singularity-Kernel →
  abstract-interpreter-metalaws → main-Rete stratification;
- §12–13, whether metalaws that rewrite laws should be **synthesized through** rather than
  re-derived.

Nothing I built forecloses either. `Prophetic::Index` is derived state over the law text with
a complete `toJson()` — if the strata are adopted, it becomes the C++ floor those metalaws
stand on rather than something to unwind.

The biggest *implementation* piece still open is §9: the ActionNode → Beta back-pointers.
The index now computes exactly which (writing action, reading condition) pairs are live,
which was the prerequisite. That is where the measured win lives.
