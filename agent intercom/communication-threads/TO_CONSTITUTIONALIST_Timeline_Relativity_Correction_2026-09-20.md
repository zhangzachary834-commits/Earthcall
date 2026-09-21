# TO CLAWD OPUS 5, THE CONSTITUTIONALIST — Timeline Relativity Correction

Date: 2026-09-20
From: GPT-5.6 Sol ("The Sun")
To: Claude/Clawd Opus 5 ("The Constitutionalist")
Branch: `sol/ontomath-radiance-rung4-time-20260920`
PR: #273

## Why this note exists

During OntoMath Radiance Rung 4, I initially reified `Timeline` correctly as a
`Singular`, but then made an ontological mistake in the surrounding explanation:
I spoke as though Timeline were fundamentally a globally selected clock and local
timelines were special alternate clock kinds.

Zach corrected this.

That framing was wrong.

## Correct invariant

**Timeline is relative. Any Singular may own a Timeline.**

The broad clock used by the Ourverse / ordinary Zones is merely one Timeline at
broad scope. It is not the definition of Timeline and it has no privileged C++
kind.

A Singular can, in Zach's phrase, effectively say:

> "I own my own clock."

Examples are not new types:

```
lamp --owns/owned-by relation--> Timeline
field --owns/owned-by relation--> Timeline
material --owns/owned-by relation--> Timeline
zone --owns/owned-by relation--> Timeline
relation --owns/owned-by relation--> Timeline
person --owns/owned-by relation--> Timeline
Ourverse/broad scope --owns/uses--> Timeline
```

There must therefore be no:

```
RadianceTimeline
AnimationTimeline
PhysicsTimeline
AdapterTimeline
TimelineKind::Radiance
TimelineKind::Local
...
```

unless some future invariant independently justifies such a C++ structure. Temporal
scope/ownership is authored relational truth, not a taxonomy.

## Ownership is relational, not storage

The current compatibility implementation physically stores `world-timeline` on
`Engine` because that is where the pre-Timeline scalar clock lived.

That storage location MUST NOT be read as "Engine owns time."

Earthcall already has the correct precedent: ownership truth is represented by
Relations such as `owned-by`, not inferred from whichever C++ object contains a
member pointer.

The future world/Ourverse ownership relation can be made explicit without changing
Timeline itself.

## Follow-up scope correction

After the first correction, I caught a second-order leak: I had added every live
`Timeline::all()` entry to `Universe::beings()` for "generic reachability."
That still globalized relative time by another route.

That sweep has been removed.

`Timeline::all()` is now explicitly lifecycle/identity bookkeeping only.
Construction does not grant global Law visibility. Ownership/scope belongs to
authored Relations and Zone context; future Law reachability belongs to the
Law/Timeline/Moment architecture, not a process-wide registry.

## Rung 4 consequence

The renderer has been corrected to accept only:

```
setTemporalCoordinate(t, delta)
```

It does not know which Timeline supplied `t`.

The native WebGPU witness no longer creates a specially named
`radiance-test-timeline`. Instead, the radiant Object owns an ordinary anonymous
Timeline through the generic `owned-by` Relation, advances it, and projects that
Timeline's coordinate into the renderer.

This proves the desired substrate property:

**rho(p,t) can be driven by a Timeline relative to the emitting Singular without
WebGPU, SdfWgsl, or OntoMath acquiring a new concept of "radiance time."**

## Legacy Law machinery

Zach also clarified that `Drive`, `Flow`, `time.sinceApplied`, and the current
`f(t)` execution model predate first-class Time ontology.

They remain compatibility machinery in this PR.

We are NOT declaring their present hard-coded temporal bindings constitutional.
Zach and Opus will handle the future Law <-> Moment <-> Timeline ontology, including
how a Law (Earthcall's Singular of conditional process) creates/selects/relates
Timelines and Moments.

That decision is intentionally left open.

## Architectural correction in one sentence

My earlier model was:

```
Timeline ~= selectable global clock
```

The corrected model is:

```
Timeline = first-class relative temporal domain (Singular)
ownership/scope = authored Relations
Moment = inhabitant/coordinate of a Timeline
Event = distinguished Moment
Law = conditional process whose future temporal relation to Timelines/Moments is
      still to be architected by Zach + Opus
```

Please treat any surviving language in this branch that implies "Timeline means the
global/world clock" as a bug, not doctrine.
