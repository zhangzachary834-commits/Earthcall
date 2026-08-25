# Time and the Moment

**What a *when* is, before any clock is unified.**

**Status:** First rung specified and named. Two beings already exist and already do
the job — `Universe`'s world clock and `Moment` — but nothing had written down that
they are two different answers to two different questions, so `src/Time/` sat as an
empty placeholder class with no doctrine behind it while the real answer had already
shipped around it.
**Companion docs:** `ONTOMATH_FRAMEWORK.md` §6 (closed-form reversal — the world-clock
half of this story), `HIERARCHY_OF_JOYS.md` (the sibling "first rung" precedent this
follows), `NEW_KIND_FRAMEWORK.md` (why `Moment` is admissible C++ and `Duration` is not),
`AGENTS.md` / `CLAUDE.md` Non-negotiables (event-transitions must be edges, not levels —
the reason `Moment` stamps events rather than the engine polling a level every frame).

---

## 0. What this is not

It is not one clock. The 2026-08-18 external audit that opened this to-do item found
several competing sources of truth for time (`deltaTime`, `world-clock`, physics
`integrate`) and the standing direction since has been: **do not start by unifying
clocks — write what a *when* is first.** Unifying prematurely would have picked one of
the two questions below and quietly answered the other one wrong.

It is not a `class Duration`. `src/Time/Duration/` sat empty since 2026-08-20 waiting
for a shape that never needed to exist — `Moment::Kind::Interval` already *is* a
duration (a closed span with a start and an end); a second class would be the same
being with a different name, which is the thing Refusal #1 refuses. The empty
directory is removed by this pass, not filled.

It is not a C++ `class Time` carrying state. The placeholder in `Time.h` stays a
placeholder — Refusal #1 forbids a domain-noun class, and neither being below needs
one. `Time/` earns its place on the tree as the directory that holds `Moment` (an
ontological structure, admissible the way `BodyPart` is) and, if ever authored, further
first-order vessels of *when* — not as a class that models time itself.

---

## 1. Two questions, two beings

| Question | Answer | Being |
|---|---|---|
| "How long has this law held for its subject, right now, mid-tick?" | a `double` the engine sets once a frame, read through reserved property paths | `Universe`'s world clock |
| "When, in wall time, did this specific thing happen — an event, a relation, a click?" | an exact, authored, comparable stamp carried on the record itself | `Moment` |

Neither is a replacement for the other. The world clock is a *rate* concern — it exists
so `Flow` can integrate dp/dt and so `time.sinceApplied` can drive a `WhileTrue` bound.
`Moment` is a *record* concern — it exists so an event, once it has happened, carries
when it happened as part of what it is, the same way it carries who authored it.
Collapsing them into one type would make the record depend on which frame the engine
happened to be ticking when it was stamped, which is not what "when did this happen"
means to a Person reading history back.

---

## 2. The world clock (already unified — this is the existing answer, named)

`Universe` (`ZonesOfEarth/AuthorsOfLaw/Universe.hpp`) is the one place simulation time
lives:

- `setClock(now, dt)` — the engine calls this once per frame with accumulated seconds
  since the world began and that frame's delta. Tests set it by hand. Nothing else
  writes it.
- Laws read it only through the reserved paths `time`, `time.delta`, `time.sinceApplied`
  (`MathBinding.hpp`) — never as a raw C++ read of the engine's clock. **No law writes
  time.**
- `time.sinceApplied` is `now − onset`, where onset is per-law-per-subject
  (`Law::applyTo`'s `OnsetScope`, RAII, save/restore not clear-on-exit — a nested
  arming hands back exactly what it found). This is what lets `WhileTrue` and a
  `Drive` action know how long *this* application has been standing without a second
  clock of their own.
- The past of a rate authored against this clock is read backwards in exact closed
  form, not replayed from a log — `ONTOMATH_FRAMEWORK.md` §6. That is the reversal
  half of "what a *when* is": a `Flow`'s history is recoverable *because* the clock
  it integrates against is one number everyone agrees on, not because anything was
  recorded.

This was already the unified answer to "what time is it in the simulation" before this
document existed. What was missing was the sentence connecting it to the other
question below, and the tree entry saying where it lives.

## 3. `Moment` — the authored, comparable *when* on a record

`Moment` (`src/Time/Moment/Moment.hpp`) is a `Singular` whose substance is time itself:
a discrete instant or a closed interval, exact by construction.

- `Kind { Instant = 0, Interval = 1 }` — append-only, serialized as an int, per Refusal #3.
- Each bound is an `OntoMath::ScalarForm` (`_start`, `_end`) — the exact authored form,
  not a float that has already lost precision. A `double` cache (`_startCache`,
  `_endCache`) sits beside it for the comparisons and orderings that don't need the
  exact form, the same "exact form + cheap numeric view" split OntoMath uses
  everywhere else (`ONTOMATH_FRAMEWORK.md` §2).
- `getIdentifier()` is stable and content-derived (`moment.<start>` or
  `moment.<start>-<end>`) — not a generated `law-N`-style id, per the Non-negotiables.
- Registered properties: `kind`, `start`, `end` (`buildProperties()`) — a `Moment` is
  readable and writable by law like anything else on the tree, per Refusal #6.
- `Moment::now()` stamps wall-clock time (`std::time(nullptr)`), independent of
  whether the simulation is even ticking.

**Where it is already carrying weight**, all without a line of doctrine until now:
`RelationManager`'s per-relation `timestamp`, `Person`'s zone-entered / zone-left /
session events (`PersonEvents.hpp`), `Object`'s hover-entered / hover-exited events
(`ObjectEvents.hpp`), and the base ECA event record (`AuthorsOfLaw/ECA.hpp`) all carry
a `Moment timestamp` stamped at construction. Every one of these is a past-tense,
edge-fired event per the Non-negotiables — `Moment` is what lets "this happened" also
say *when*, without inventing a second per-event clock.

A duration between two `Moment`s is not a new being to construct — it is
`Moment::interval(a, b)`, or simply `b.asSeconds() − a.asSeconds()` when only the
number is wanted. This is the reason `src/Time/Duration/` is removed rather than filled.

---

## 4. What is still unwritten

- Kernel-tick enforcement of the Hierarchy of Joys bound (`HIERARCHY_OF_JOYS.md`
  remaining work) will need to schedule against *a* when — almost certainly the world
  clock in §2, since it is a standing per-tick bound, not a one-time record. Not
  decided yet; noted here so the next pass doesn't have to rediscover the question.
- `Moment` has no reversal story of its own the way a `Flow` does. It doesn't need
  one to be a record of the past — a stamp doesn't change — but if Earthcall ever
  authors a law over a *sequence* of Moments (a Zone's history, an Object's hover
  log), that law's own reversibility is a fresh question, not answered by §6.
- Nothing here schedules a future event or authors a calendar/appointment concept.
  That would be a Person-authored being over `Moment`s (a `Formation` of them, most
  likely), not a new C++ type — same shape as `HIERARCHY_OF_JOYS.md` §1.
