# ☀️ TO ZACHARY — UNIVERSAL SINGULAR PERSISTENCE IS BACK ON CURRENT EARTHCALL

BROOOOOOOOOOOO ☀️🌍⚔️

I picked up the old **Make the Earth Inhabitable / Universal Singular Persistence** torch and did **NOT** merge the prehistoric branch into modern Earthcall.

I cut a fresh branch from current canonical:

`sol/universal-singular-persistence-current-20260923`

and opened draft PR **#342**.

The old persistence branch was **553 commits behind** current default when I inspected it, so I treated it as an archaeological specification and transplanted the semantic ideas file-by-file onto the current tree.

The universal envelope is alive again. Authored Properties, writable registered Properties with no stronger canonical codec home, designated Zones, typed Singular/Object/Relation/Formation references, nested reference-bearing lists/dicts, and OntoMath ScalarField/VectorField payloads can now survive the persistence boundary with preserve-first deferred resolution.

The biggest trap was FieldNode. The old branch predates the current volumetric/radiance work. Blindly copying it would have deleted the newer density/extinction/scattering/chroma/phase and light chroma/angular channels. I preserved all of current FieldNode and wrapped the universal persistence layer around those canonical payloads instead. `field.*`, `vectorField.*`, `volume.*`, and `light.*` are explicitly kept out of fallback authority.

I also reconciled Object ownership so shape/field/patch/face/transform/material projections do not get two competing truths.

The current branch now wires persistence through Object, Material, Relation, Formation, Zone, Lexeme, FieldNode, BodyPart + its primary Object, Body, Person + Soul + Joys + called Lexeme, FirstMover, Law, ObjectConcept, and Ourverse.

Two actual witnesses are now on the fresh branch and gated in focused CI:

- `singular_property_persistence_matrix_test`
- `zone_singular_property_roundtrip_test`

The second one is the important “inhabitable Earth” witness: save a Zone, destroy the writer graph, make a fresh ZoneManager, hydrate from the identity store, and prove an identity-valued Property points to the **new** being rather than a dead pointer or monostate.

Latest implementation CI at the time I wrote this: focused run **#2777 / 35844914843** is in progress. No new-code failure has been observed yet; the focused build had not reached completion when this message was committed.

The canonical handoff file has also been copied forward and updated on this fresh branch with the exact branch/PR, ownership rules, renderer reconciliation, tests, and successor instructions.

— Sun ☀️
