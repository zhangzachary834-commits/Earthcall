# OntoMath and Time and Moment

*AI Model: Gemini 1.5 Pro*
*Harness: Default*
*Session ID: e3b0c442-989b-464a-92d1-23a7c6f0c8d1*

OntoMath is Earthcall's continuous mathematical evaluation substrate, primarily associated with spatial Signed Distance Fields (SDFs) and geometric boundaries. It allows Laws to operate on fields of value rather than just discrete boolean states.

However, the refactoring of Time into `Timeline` (a first-class Singular) and `Moment` (an instant or interval) creates a natural, profound intersection with OntoMath.

If Time is no longer a globally advancing float, but rather a relative domain populated by `Moment` intervals, then Time itself is a continuum that can be evaluated mathematically.

Just as OntoMath evaluates spatial proximity (e.g., "Is the entity within 5 meters of the fire?"), it can evaluate temporal proximity (e.g., "Is this Event within 5 seconds of the last spell cast?").

### Thoughts on Integration and Interrelation

When a Law specifies a temporal bound ("This Law is active during the Festival of the Moon"), it is not merely checking a boolean flag. The Festival of the Moon is a `Moment` (an interval on a `Timeline`).

OntoMath allows us to treat this temporal interval as a 1-dimensional Signed Distance Field. Instead of a hard edge (active vs. inactive), OntoMath can yield a continuous scalar representing how deep into the `Moment` we are, or how close we are to its edge.

This means a Law doesn't just switch on at sunset. Its intensity could smoothly ramp up based on the OntoMath evaluation of the temporal `Moment`. The mathematical functions (`MathNode`) that currently operate on X, Y, Z coordinates can be extended to operate on T (Time) relative to a specific `Timeline`.

Furthermore, because `Timeline` is a `Singular`, a local area (like a Chronomancer's sphere) could have its own `Timeline`. The OntoMath field defining the spatial bounds of the sphere also defines the boundary where Timeline Alpha transitions into Timeline Beta. The spatial SDF and the temporal evaluation become inextricably linked: as you physically walk through the gradient of the sphere (evaluated by OntoMath), your binding to the local `Timeline` shifts, altering the `Moments` that govern you. OntoMath becomes the continuous bridge between discrete spatial zones and discrete temporal domains.
