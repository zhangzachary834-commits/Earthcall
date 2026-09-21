# Time, Timeline, and Moment

**Status:** Timeline first rung implemented on the OntoMath Radiance Rung 4 branch.
The future Law <-> Timeline <-> Moment ontology is intentionally not decided by
this implementation pass.

## 0. The distinction

Earthcall now has three different temporal ideas that must not collapse into one:

1. **Timeline** — a first-class temporal domain. It is a `Singular`.
2. **Moment** — an instant or closed interval that may inhabit a Timeline. It is
   also a `Singular`.
3. **Event** — a distinguished Moment carrying a transition edge, participants,
   and authorship. `Event : public Moment`.

`Universe` is none of those. It remains kernel working context. It may borrow a
Timeline as the currently authoritative temporal domain for legacy Law execution,
but it is not itself Time and it is not a temporal being.

There is no `class Time` anymore. The old empty `src/Time/Time.h/.cpp` placeholder
was replaced by `src/Time/timeline.hpp/.cpp`.

There is still no `class Duration`: a duration is represented by
`Moment::interval(start,end)` or by a difference between coordinates when only
the scalar measure is needed.

## 1. Timeline — a relative temporal domain

`Timeline` inherits `Singular`.

A Timeline is not intrinsically global. **Any Singular may own its own Timeline.**
Ownership is expressed with ordinary Relation truth (for example
`Timeline --owned-by--> Singular`), not by inventing subclasses such as
`ObjectTimeline`, by adding a timeline field to every kind of being, or by
inferring ownership from whichever C++ object stores the pointer.

A lamp may own a Timeline. A Field may own one. A Material, Relation, Person,
Zone, or any other Singular may own one. The broad clock shared across the
Ourverse/Zones is merely a Timeline at the broadest scope; it is not the
definition of Timeline.

A Timeline contains Moments and may also carry a currently advancing temporal
coordinate:

```
Timeline
  identity
  moments[]
  now
  delta
  hasClock
```

Those are ordinary legible properties. A Law or First Mover can inspect Timeline
state through the same property system used by every other Singular.

Timeline deliberately has **no enum of kinds**. Earthcall must never need:

```
WorldTimeline
RenderTimeline
PhysicsTimeline
AnimationTimeline
AdapterTimeline
GlowTimeline
ColorTimeline
...
```

as C++ types or enum members.

Instead, those are ordinary Timeline beings distinguished by identity, Relations,
authorship, Laws, and whatever future temporal ontology Zach and Opus establish.

Creating another temporal domain therefore means creating another Timeline being,
not editing a switch statement.

The implementation keeps a process-local live registry
(`Timeline::all()`) for lifecycle/identity bookkeeping. **That registry is not
temporal scope, ownership, authority, or Law reachability.** In particular, the
Universe provider does not sweep every Timeline into its global working set:
doing so would make a local Singular-owned clock globally visible merely because
it exists. Future Law/Timeline architecture must derive lawful reachability from
authored ownership, Zone context, and the temporal relations Zach + Opus define.

## 2. A Timeline contains Moments

A Timeline owns `std::shared_ptr<Moment>` members.

This is intentional because `Event : Moment`: an Event can inhabit a Timeline
without a second event-log ontology and without slicing away its distinction.

Insertion order is not temporal truth. `orderedMoments()` derives chronological
order from each Moment's own coordinates.

The current reflected Timeline vocabulary is:

- `hasClock`
- `now`
- `delta`
- `momentCount`
- `moments`
- `latestMoment`

Future work may enrich the Relations among Timeline and Moment beings. That is
not part of this rung.

## 3. Independent Timelines

Two Timeline beings are temporally independent unless authored structure relates
them.

An occurrence in Timeline A does not, merely because it occurred, constitute an
occurrence in Timeline B.

That distinction is what the Slow Adapter analysis was reaching for when it
described independent clocks. A display frame, maintenance opportunity, audio
sample domain, authored animation, or Zone-local temporal process need not all be
reducible to one universal frame counter.

The current implementation proves the substrate can hold multiple independently
advancing Timeline beings. It does **not** define the final Laws that relate,
synchronize, fork, pause, scale, or derive one Timeline from another.

## 4. The broad world Timeline is one relative Timeline

The current compatibility implementation stores one broad Timeline instance:

```
world-timeline
```

It replaces Engine's former raw `_worldTime` scalar as the primary live world
clock state, but **its C++ storage location is not ontological ownership**.
Timeline ownership belongs in Relations among Singulars.

Conceptually, this Timeline is the broad temporal domain shared by the
Ourverse/ordinary Zones. That does not prevent any Singular inside them from
owning an independent Timeline of its own.

`Universe` borrows that Timeline during normal execution:

```
world-timeline : Timeline : Singular
        |
        v
Universe temporal context
        |
        +--> legacy Law paths: time / time.delta / time.sinceApplied
        |
        +--> current default Screen temporal binding
```

`Universe::now()`, `Universe::dt()`, and `Universe::setClock()` remain as
compatibility projections so the existing Law machinery and isolated tests do
not all need to migrate in the same commit.

When no Timeline is bound, Universe retains a scalar fallback for isolated tests
and tools. That fallback is compatibility state, not a second temporal ontology.

## 5. Moment

`Moment` (`src/Time/Moment/Moment.hpp`) is a Singular whose temporal substance
is an instant or a closed interval.

Its serialized kind is append-only:

```
Kind::Instant
Kind::Interval
```

Its bounds are authored OntoMath `ScalarForm` values with cached doubles for
cheap ordering and comparison.

Its registered properties are:

- `kind`
- `start`
- `end`

`Moment::now()` currently stamps wall-clock seconds using `std::time`. That is
a convenience constructor for records; it is not the same thing as advancing a
Timeline and should not be mistaken for the future answer to Timeline-relative
authorship.

## 6. Event is a distinguished Moment

Zach's definition remains:

> **Event is a distinguished Moment.**

`Event : public Moment`.

An Event does not wrap a hidden timestamp object. It is a Moment elevated by:

- a transition verb;
- subject/object participants;
- an author.

Its temporal properties remain the Moment properties, while `verb`, `type`,
`subject`, `object`, and `author` add the distinction.

This preserves the no-black-box rule and keeps Event inside the Time ontology
rather than recreating private callback timestamp structs.

## 7. Legacy Law temporal execution is pre-Timeline machinery

This is the most important compatibility note for future architects.

The current Law action framework — including `Drive`, `Flow`,
`time.sinceApplied`, and the existing authored `f(t)` patterns — was created
before Timeline became first-class.

Today it still effectively treats temporal execution through hard-coded reserved
paths and Universe application onset:

```
t <- time.sinceApplied
dt <- Universe::dt()
```

and the action engine performs the corresponding continuous/drive behavior.

That machinery remains operational and is deliberately **not redesigned in this
pass**.

Its present implementation must therefore not be interpreted as the final
ontology of temporal Law.

The future questions belong to the Law/Timeline architecture pass, including:

- how a Law names or selects a Timeline;
- whether a Law creates a Timeline for a conditional process;
- how a Law's conditional process relates its start/end Moments;
- what `WhileTrue`, Drive, Flow, and OnEvent mean once temporal domains are
  explicitly first-class;
- how one Timeline lawfully depends on, scales, pauses, or derives from another;
- how authored processes such as animation, changing glow, changing field,
  changing color, maintenance, and similar continuous changes create/use their
  own Timelines rather than borrowing a hard-coded global clock.

Zach and Opus own those ontological decisions. Rendering work must leave that
space open.

## 8. Rung 4 rendering boundary

OntoMath names the temporal coordinate `t` through
`OntoMath::kTimeVar == "t"`.

OntoMath does not choose a Timeline.

WebGPU does not choose a Timeline.

The Renderer boundary accepts only an admitted temporal coordinate:

```
Renderer::setTemporalCoordinate(t, delta)
```

The current production first mover supplies that coordinate from the
Universe-selected world Timeline as a compatibility/default binding.

The native Rung 4 GPU witness intentionally does something stronger: the radiant
Object owns an ordinary, anonymously generated Timeline through the existing
`owned-by` Relation. The witness advances that Object-owned Timeline, projects
its coordinate through the Renderer boundary, and proves `rho(p,t)` changes
pixels without changing the authored AST, parameter buffer, or WGSL program.

There is no special "radiance Timeline" class, enum, identifier, or framework.
The same substrate proves "I own my own clock" for any Singular.

## 9. Author direction for temporal change

The intended direction is now explicit:

A changing glow, changing field, changing color, animation, or other process over
time should ultimately be represented through a Timeline and Laws, because a Law
is Earthcall's Singular of conditional process.

This branch does not decide the exact Law-to-Timeline semantics. It only removes
the rendering and clock assumptions that would prevent that architecture from
being authored later.

## 10. Invariants this rung establishes

1. Timeline is a Singular.
2. Timeline is relative: any Singular may own one through ordinary Relations.
3. Timeline contains Moments.
4. Event remains a distinguished Moment.
5. There is no enum/class proliferation for timeline kinds or owners.
6. Multiple Singular-owned Timelines can advance independently.
7. The broad world clock is one Timeline at broad scope, not the definition of Time.
8. Universe may borrow a Timeline but is not itself temporal ontology.
9. Renderer/OntoMath accept a temporal coordinate without knowing which Timeline
   or owner supplied it.
10. Legacy Drive/Flow/`f(t)` semantics remain compatibility machinery pending
    the future Law/Timeline ontology.
