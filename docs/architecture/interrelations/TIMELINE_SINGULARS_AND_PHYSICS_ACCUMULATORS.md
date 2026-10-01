# Timeline Singulars and Physics Accumulators

**How the authored, relative domain of Time reconciles with the rigorous stability of the Fixed-Timestep Physics loop.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/TIME_AND_MOMENT.md` (Timeline as a relative, authored Singular domain)
*   `../events/PHYSICS_AND_COLLISION.md` (The fixed-timestep physics accumulator)

---

## The Interrelation

In Earthcall, there is a deep tension between the nature of simulation stability and the nature of authored reality.

The physics engine (`PHYSICS_AND_COLLISION.md`) demands stability. To prevent temporal instability, step doubling, and jagged behavior under variable framerates, it operates on a deterministic fixed-timestep accumulator. It consumes raw, wall-clock `dt` from the execution environment to step forward exactly `FIXED_DT` seconds at a time. It assumes a monotonic, linear progression of time.

Conversely, the temporal ontology (`TIME_AND_MOMENT.md`) rejects universal, hardcoded time. A `Timeline` is an authored `Singular` being. This means a Person can author a Law that slows down a Timeline, reverses it, halts it, or creates a localized Timeline that only affects specific Relations within a Zone.

### Bridging the Substrates

If a Person authors a Law that says "Time moves at 0.5x speed for the next 10 seconds", how does the fixed-step physics loop respect this without shattering its numerical stability?

The interrelation requires that the physics accumulator must decouple its internal simulation step (`FIXED_DT`) from the semantic velocity of the active `Timeline`.

Instead of consuming the raw wall-clock `dt` provided by the CPU, the Zone's update loop must query the active `Timeline` Singular for its perceived `dt` (the delta between the current Moment and the previous Moment).

If a Timeline is paused, its perceived `dt` is 0. The accumulator receives 0, and no physics steps occur, flawlessly pausing physics without breaking the fixed-step logic. If a Timeline runs at 0.5x, the accumulator receives half the wall-clock time, causing it to trigger a `FIXED_DT` physics step half as often.

In this way, the mathematical rigor of the physics substrate remains completely intact, while remaining perfectly subordinate to the authored semantic truth of the `Timeline` being.
