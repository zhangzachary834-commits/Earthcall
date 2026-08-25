# The frame lag probe — what a frame of Earthcall costs

**Author:** Claude (Opus 5), Claude Code session
**Session ID:** `4f964861-c6d6-405c-a03c-f59181c8a27c`
**Date:** 2026-08-24
**Timestamp:** 2026-08-24T16:21:14-07:00
**Tree:** `bb1b4737` + working changes, branch `sync-from-earthcall-main`
**Asked for by:** Zach — *"implement a lag test, a make test that systematically checks for lag."*
**Artifacts:** `tests/singularity/frame_lag_test.cpp`, `tests/singularity/frame_lag_baseline.txt`,
`cmake --build build --target lag`

---

## What was built, and why it is shaped this way

Zach's ask was one sentence with a hard word in it: *systematically*. A single
`assert(frameMs < 16.6)` is not systematic — it is one number, on one machine, on one
world, and it is either red forever or green by being generous. So the probe asks four
different questions, ordered by how much they depend on the clock:

| § | Question | Depends on the clock? |
|---|---|---|
| 1 SHAPE | does per-frame cost grow faster than the world does? | a *ratio* of times — survives a slow machine |
| 2 QUIESCENCE | on a world where nothing happens, is the frame still finding work? | no, not at all |
| 3 STEADY | does the authored world hold 60 Hz, without a hitch and without drift? | yes |
| 4 LOAD | how long before the Person is in the world? | yes |

The frame it steps is `Engine::update`'s world half (`EngineUpdate.cpp:163-165`) plus the
law drain that follows it (`Engine.cpp:318`) — `Zone::update`, `applyFormationRelations`,
`Universe::setClock`, `LawManager::tick`. The three windowed channels (locomotion,
creation tools, interaction) need a live GLFW window and an ImGui frame and are left out.
What remains is the part of a frame that scales with the size of the world, which is the
part that lags.

### Three verdicts, not two

This is the design decision worth arguing about. A perf test with only pass/fail must
choose between lying and being red forever. This one prints:

- **ok** — meets the aspiration (what a frame *ought* to cost).
- **STANDING** — misses the aspiration, matches the baseline in
  `tests/singularity/frame_lag_baseline.txt`. A cost already known and already written
  down as a task. Reprinted every run so nobody forgets it. **Not a failure.**
- **LAG** — worse than the baseline. *This* is the failure: something you just did made
  Earthcall slower.
- **IMPROVED** — comfortably better than the baseline; re-record it or the tripwire stays
  slack.

So the suite stays green (66/66) while the actual, current lag is read aloud on every
single run. Nothing is hidden and nothing is red for a reason nobody has time to fix.

### It knows when it cannot trust the clock

A machine under load measures every duration long. That is lag in the room, not lag in
Earthcall. The probe times a fixed reference workload before and after the measurements;
if the machine's speed moved by more than 1.4x while it was working, **every timing
verdict is reported and none of them may fail the run** — and `--rebaseline` is refused
outright, because a baseline recorded on a contended machine is a tripwire set at the
wrong height. §2's invariants never touch a clock and are enforced either way.

This was not a hypothetical, and it did not stay a design note — see §3, where it caught
two findings of mine that were contention wearing a lag costume. Every wall-clock budget is
also divided by that calibration factor, so the baseline file means roughly the same thing
on someone else's machine; `kReferenceCalibrationMs` is the idle-machine time for that
workload, and has to be taken from an idle machine or it scales every number in the report.

`ctest` gets `RUN_SERIAL TRUE` for this test, so it is not itself measured while three
other tests fight it for the box.

---

## What it found on day one

### 1. `Zone::update` is quadratic in the population — the one finding that survived every re-measurement

The clearest result in the whole run, and it reproduces on every machine state:

```
   64 objects -> frame    1.18 ms   (zone   1.08   relations 0.000   law 0.10)
  128 objects -> frame    3.46 ms   (zone   3.28   relations 0.000   law 0.18)
  256 objects -> frame   11.47 ms   (zone  11.10   relations 0.000   law 0.36)
  512 objects -> frame   41.38 ms   (zone  40.67   relations 0.000   law 0.71)
```

Zone cost per doubling: 3.0x, 3.4x, 3.7x — a doubling that costs nearly four times as much
is the signature. Fitted `n^1.75`, and it came out between 1.67 and 1.82 on every machine
state measured, contended or idle. `LawManager::tick` over the same populations fits
`n^0.97` — the Rete drain is honestly linear, and the law engine is **not** where the lag
is. Formation relations are too cheap to fit a curve to at 512 objects at all.

`macOS sample(1)` on the probe mid-run gives the exact path, 100% of samples:

```
Zone::update  (Zone.cpp:410)
  Physics::updateBodies  (Physics.cpp:368)
    Physics::dispatchCollision  (CollisionDispatcher.cpp:615)
      gjkEpaCollision  (CollisionDispatcher.cpp:578)
        epaPenetration  (CollisionDispatcher.cpp:263)
          addBorderEdge  (CollisionDispatcher.cpp:214)   <- linear scan of an edge vector
```

`Physics::updateBodies` (`Physics.cpp:135`, pair loop at `:327-343`) is an all-pairs
`for i / for j = i+1` with **no broadphase**. There is an AABB pre-filter at `:353`, but
B's AABB is recomputed inside the inner loop, so every pair costs a corner walk whether it
overlaps or not — the `n²` is paid before the filter can reject anything. A second
all-pairs loop sits at `:222`. On the chess board the filter does not reject much anyway:
32 pieces on adjacent squares genuinely overlap in AABB terms, so most pairs go on to
GJK/EPA.

**And it is on by default.** `g_legacyEngineEnabled = true` (`Physics.cpp:33`) and nothing
in the app turns it off — the only caller of `setLegacyEngineEnabled` in the tree is
`tests/zones/ground_plane_test.cpp:64`. The running app pays this every frame.

This is what `ce5c1cbe` ("Attempt to fix chess lag") was reaching for. That commit fixed a
`Singular` copy/move slicing bug — a real bug, already noted as mis-labelled in
`Week_Of_2026-08-24_Structural_Debt.md` — and left the quadratic untouched.

### 2. The chess frame is close to its budget — and the first two numbers I wrote here were wrong

Chess: 35 objects, 36 laws, 38 relations. Simulation-half medians, Debug `-O0`, on an
**idle** machine, calibrated:

| | measured | aspiration |
|---|---|---|
| median simulation frame | 6.0-7.1 ms | 5.5 ms (a third of a 60 Hz frame) |
| of that, `Zone::update` | 4.3-5.0 ms | 3.0 ms |
| of that, `LawManager::tick` | 1.8-2.1 ms | 2.0 ms |
| 95th-percentile frame | 6.2-7.4 ms | 16.6 ms |
| opening the world | 1.4-2.1 s | 4.0 s |

So: slightly over budget, no hitching, and the load is fine. `LawManager::tick` sits at its
aspiration. At 35 objects the quadratic in §1 has not yet had room to hurt — 32 pieces are
not 512 — which is exactly why §1 exists and why it is the finding that matters.

**An earlier draft of this document said the opposite,** and the correction is the most
useful thing in it. Written from runs taken while this laptop sat at load average 40-93
(a build, a browser, another agent), it reported a ~18 ms median frame, a ~29 ms p95 with
150 ms hitches, and a 4.5-6.5 s load, and concluded the chess world could not hold 60 Hz.
Re-measured on an idle machine, the same tree gives the table above. The lag was in the
room, not in Earthcall.

I caught it only because the probe's own calibration number moved — which is the mechanism
working, one layer up from where I designed it to work. Two things follow, and both are now
in the code:

- `kReferenceCalibrationMs` had been set from a contended machine (5.5 ms) and was silently
  scaling every reported duration by 2.5x. It is now 2.0 ms, the best of fifteen runs on an
  idle box, with a comment saying where that has to come from.
- `frame_lag_baseline.txt` was re-recorded idle, and its header now says in capitals that a
  baseline recorded on a busy machine is a tripwire set at the wrong height.

The honest summary of §2 and §3 is: **on a shared laptop the chess frame is dominated by
whatever else is running, and any lag report from such a machine — including one written by
an agent in a hurry — is worth exactly nothing.**

### 3. The machine measures back

Same tree, same binary, same world, four sessions of the same afternoon:

| load average | calibration workload | chess frame median (raw) | load time (raw) |
|---|---|---|---|
| ~7 (idle) | 2.0 ms — the reference | 6.0 ms | 1.4 s |
| ~40 | 6.1 ms (x3.0) | 18.8 ms | 6.8 s |
| ~93 | 75 ms (x37) | 26.0 ms | 28.8 s |

(The middle row is why the constant had to be re-derived: under the old 5.5 ms reference
that machine reported itself as x1.1 — *normal* — and its numbers went into the first draft
of §2 unchallenged.)

A 4x spread on the frame and a 20x spread on the load, with nothing in Earthcall changing.
This is why the probe times its reference workload before and after every run, refuses to
enforce a timing when those two disagree by more than 1.4x, refuses to `--rebaseline` on a
contended machine, and carries `RUN_SERIAL TRUE` so `ctest -j4` does not measure it while
three other tests fight it for cores.

It is also why §1's growth exponents are the load-bearing result and §2's absolute
milliseconds are not: the exponent moved from 1.67 to 1.75 across every one of those
machine states, because a ratio of two contended measurements divides most of the
contention out.

### 4. What is healthy, and worth saying

- **Nothing fires on an idle world.** 0 law firings across 106 idle frames. No
  level-triggered event is masquerading as an edge, which was the thing most worth ruling
  out.
- **The population is stable.** No law spawns an object per frame.
- **No being registers a property path twice.** The `buildProperties()`-from-a-constructor
  bug CLAUDE.md warns about is not present in the chess world.
- **No drift.** The last quarter of a 128-frame run costs 1.02x the first. Whatever the
  frame costs, it does not get dearer as it runs.
- **`LawManager::tick` is linear.** Worth naming because the Rete engine is the obvious
  suspect for "the world got slow" and it is innocent.

---

## How these numbers were set

`frame_lag_baseline.txt` holds the **dearest** value seen across four *idle* runs, padded
about 15%, not any single run's numbers. Idle spread was ~15% on times and ~0.04 on
exponents; a tripwire set at one run's best fires at the next run's contention and gets
switched off by the third. Tolerances on top of that: **1.5x** on a time, **+0.20** on an
exponent, and a hard wall at **`n^2.0`** that no baseline may excuse — a frame that scans
every pair is a frame that stops working the moment a world gets lived in.

Re-record with `./build/frame_lag_test --rebaseline`, on a quiet machine (the test refuses
otherwise), and only downward — or after a cost that was accepted deliberately, saying which
in the commit message. `./build/frame_lag_test --calibrate` prints this machine's speed
against the reference; if that number will not hold still, nothing else the test says will
either.

## What I did not do

- **Did not fix the quadratic.** Zach asked for a test. Putting a broadphase into
  `Physics::updateBodies`, or deciding whether the legacy engine should be on by default at
  all, is a change to how the world behaves and is his call. Both are now on the to-do list
  under **Performance** with the measurements attached.
- **Did not measure a Release build.** Every number here is `-O0`, which is what the suite
  builds and what `docs/BUILD_AND_ENVIRONMENT.md` tells a Person to build. How much of the
  quadratic's constant is `-O0` and how much is the algorithm is still unknown — though the
  *exponent* is a property of the algorithm and no optimiser will change it. On the list.
- **Did not measure the three windowed channels** (locomotion, creation tools,
  interaction). They need a live window; the interaction channel's per-frame pick over
  every object in the Zone is the most likely lag among them and is worth a probe of its
  own. On the list.
- **Did not click anything.** This is another headless test, and
  `What_The_Test_Suite_Can_See.md` §3 is right that the felt surface has failure modes
  `ctest` cannot see. On an idle machine the simulation half of a chess frame is 6-7 ms,
  which leaves room for a render inside 16.6 ms — but whether it *feels* that way under the
  hand is not something this or any headless probe can answer.
