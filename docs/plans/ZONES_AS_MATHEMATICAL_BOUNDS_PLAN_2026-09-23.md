# Zones as Mathematical Bounds

*Claude Opus 5.5 · session `b0dcb70f-a02a-4081-8589-0aae3ab30551` · 2026-09-23.*

## 0. Origin — what Zach said, what is mine

Zach, 2026-09-23, while reviewing the interaction work:

> "The paradigm of 'Zones being purely discrete 3D/2D instances' is outdated … 'Currently
> located Zone' assumes a game-like paradigm where the main unit/semantics are in whole 3D
> worlds or 2D files, and the Person just happens to switch between them … EArthcall is a
> channel for reality, and that means *Person's currently located zone* must be decoupled from
> *whether a Zone is active* — you may want to have the states of another Zone changing and
> running even when you aren't present in it. I plan to move to a richer Ontomath based model
> of continuous bounds and also discrete dimensions (if needed)."

Then: *"let's pivot and work on implementing the full vision of zones-as-mathematical-bounds
instead of 'one 3d world/object collection here, another there, another there'."*

His three decisions (same day):

1. **Dimensional Zone.** The shared space is a Zone, *"a Dimensional Zone which basically
   represents an entire continuum of a dimension upon which values could be had (like 1D, 2D,
   3D space)."*
2. **Ownership is a Relation.** *"Ownership is already a Relation, not merely an arbitrary
   position in a bound."* Where a being is and who holds it must never be conflated.
3. **`running` is authored**, and today's behavior is the default.

The manifesto already says this (`EarthcallOurverse.md` §Zone, ll. 277–293):
- *"fields that host shared existence"*;
- *"Any field or spectrum can be modeled into a Zone if needed, even Condition bounds"*;
- *"Zones can exist inside other zones. Zones can also overlap … Multiple zones that share a
  field can have the field itself be a Zone."*

The mechanisms below — axes bound to property paths, extents reusing the `Zone` condition's
satisfaction semantics, the rung order — are mine.

## 1. The model

### 1.1 Four questions about a being and a Zone, never collapsed

| Question | Answered by | Exists today |
|---|---|---|
| Who **owns** it? | Relation (`owned-by`) | yes |
| Which Zone's **jurisdiction** is it affiliated with? | `Singular::designatedZones` | yes |
| Which Zone **holds** it (residence: the store it lives in, the frame its coordinates are written in)? | the Zone's object store | yes (implicit) |
| Where **is** it? Which Zones contain it? | **derived** from its coordinates and the Zones' extents | **new** |

A being can reside in the Cathedral's store and stand outside the Cathedral's extent. Then it
is *located* outside the Cathedral, while the Cathedral still holds it, may own it, and may
still govern it by affiliation.

### 1.2 Dimensional Zone

A Zone that carries **axes**. Each axis is a named coordinate bound to a property path on
beings:

```
dimension.x = "position.x"
dimension.y = "position.y"
dimension.z = "position.z"
```

These are dynamic properties, so they are authorable (`AddProperty`), law-readable, and
persisted. A Zone with at least one `dimension.*` is Dimensional. There is **no enum of
dimension kinds** and no `DimensionalZone` class: 1D, 2D, 3D, or a non-spatial continuum
(`dimension.pitch = "audio.frequency"`, `dimension.t = "time.now"`) are the same thing with
different axes. This is the Timeline pattern (`TIME_AND_MOMENT.md`: a relative domain,
identity by being, no subclass per kind) carried from time to every continuum.

A Dimensional Zone with no `within` is **its own discrete dimension**. Reaching it means moving
the Person there discretely, which is what `switchTo` does today. So every existing save,
where each Zone is parentless, keeps exactly its current behavior.

### 1.3 Bounded Zone

A Zone that is `within` another (the existing `parentZone` field, now a registered property)
and has:

- **`placement.<axis>`**: where its own frame's origin sits in its parent's frame
  (translation; rotation and scale in a later rung);
- **`extent`**: an OntoMath `ScalarField` whose AST is a `Piecewise` over the axis names,
  plus optional **`extent.lo` / `extent.hi`**.

A point is inside when f is defined **and** `lo ≤ f ≤ hi` (an absent side is open). These are
exactly `ConditionNode::Kind::Zone` semantics ("the authored satisfaction zone of a
mathematical function"), so there is one meaning of "zone of a function" in the whole tree.
Signed-distance regions set `extent.hi = 0`. Defined-set regions (the `ElevatePixels`
precedent) omit both bounds. A bounded Zone with no extent is unbounded within its parent.

Containment nests. A being is in Z iff Z's extent holds at the being's coordinates in **Z's
own frame**, and Z's parent contains it. Moving a Zone is a write to its placement; its
shape is untouched.

### 1.4 Coordinates

A being residing in R has local coordinates (its axis properties). Its coordinates in the
Dimensional root's frame are local plus every placement from R up to the root. Its coordinates
in any Zone Z's frame are root coordinates minus Z's placement chain. A Person resides in the
Zone they are present in.

## 2. Rungs

| Rung | What | Behavior change for existing saves |
|---|---|---|
| **1 (this pass)** | Ontology surface: `within`, `dimension.*`, `placement.*`, `extent`, persistence; the kernel locator; law readings `@world.zoneId`, `@world.zonePath`, `@world.dimensionalZoneId`. | none |
| 2 | **Running decoupled from presence.** Law-writable `running`, derived `present` (a Person is located in it). The law closure becomes the union of present and running Zones, not "the active one". Physics and the Zone's Timeline step for every running Zone. `switchTo` becomes "move the Person"; it no longer unloads laws of Zones that are still running. | none by default |
| 3 | **One continuum on screen.** Render, pick, locomotion, audio and interaction `reachable` gather every Zone sharing the Person's Dimensional root, each placed by its placement chain. `mgr.active()` call sites (~60) migrate to `presentZones()` / `zonesIn(root)`. | none unless Zones are placed |
| 4 | **Edges.** `zone-entered` / `zone-exited` published for any being on a location transition (edges, never levels), replacing today's switch-only `zone-entered`. Relation and beings providers range over running Zones. | `zone-entered` gains subjects |
| 5 | **Beyond space.** Non-spatial axes in practice; overlap and jurisdiction friction surfaced to MetaLaw (manifesto l. 293); rotation/scale placement; a spatial index (BVH over extent interval bounds via `MathNode::evalRange`, per `PROPHETIC_RETE.md`: bounds may only prove IMPOSSIBLE). | none |
| 6 | **Placing the existing worlds.** Authoring tools and a patch-only save injector to place existing Zones within a shared Dimensional Zone. **Only with each save owner's authorization** (save files are sacred). | opt-in per save |

## 3. Refusals held

- No `DimensionalZone`, `Space`, `Region` or `Boundary` class (Refusal 1); no enum of zone or
  dimension kinds (Refusal 3). `Zone::Scope` (Global/World/Regional/Local/UI) predates this and
  is now redundant with authored extents. Retiring it is a later rung, and its ints stay
  serialized.
- Every new field is registered (Refusal 6). The derived location is read-only, because a
  law that could write "where it is" would be lying to every other law.
- Location is a **reading**, not a stored field, so it cannot drift from positions. Its one
  cache (§4) declares its invalidation.

## 4. Derived state declared (`law/DERIVED_STATE_LEDGER.md`)

`ZoneManager` keeps a residence index (being → the Zone whose store holds it), rebuilt at
most once per Universe tick and on any miss. Invalidation: the Universe clock advancing.
Staleness bound: a being moved between stores mid-tick answers with its old residence
until the next tick. Guarded by `dimensional_zone_test`.
