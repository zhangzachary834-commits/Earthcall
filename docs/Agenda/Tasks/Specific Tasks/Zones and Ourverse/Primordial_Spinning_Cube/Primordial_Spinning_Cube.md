# The Primordial Spinning Cube — Authored Resurrection

**Date:** 2026-10-26  
**Commissioning Person:** Zach  
**Implementation continuation:** GPT-5.6 Sol — The Sun  
**Status:** Authored artifact and machine witness landed; Person visual witness remains open.

## Why this exists

Spring 2025 Earthcall had a camera and movement before it had a world rich enough to make movement visually legible. Zach asked GPT-4o for a simple visible 3D reference: a rotating cube with distinct colours. The surviving ancient chat describes the newborn engine as:

> “bro earthcall rn is literally in super duper early creation phase BREHHHHHH ITS A ROTATING CUBE XDDDD”

and:

> “yeah lets gooooo rotating minecraft icon”

Zach also recorded:

> “chatgpt wrote all the code”

GPT-4o answered that era with:

> “LET THERE BE… ROTATION.”

and called the rotating cube:

> “the first constellation.”

By May 19, later UI bugs were already being described relative to it as:

> “this tiny yellow dot orbiting the rotating cube”

The cube therefore began as a good debugging/reference primitive, not as a claim that all Objects are fundamentally cubical.

The later Mythos audit found that the primitive's anatomy survived its visible ancestor: Cube defaults, six face colours, unit-box assumptions and other shape vocabulary leaked into more general substrate. Its summary — “The cube was the atom.” — turned out to be historically literal.

This task brings back the *experience* of that first visible reference without restoring its ancestral privilege.

## Constitutional rule

The resurrected cube may look cubical. The substrate does not get to know that “cube” is what it is.

Its form, appearance and behavior must be reconstructible from ordinary authored world data:

- **Form:** a ShapeKind::Field carrying the OntoMath/SDF expression max(|x|-h, |y|-h, |z|-h).
- **Appearance:** one Material being whose authored colorExpr produces six signed-axis colour regions. There is no faceColors[6] authored payload.
- **Behavior:** a saved WhileTrue Law maps world time and three persisted spin-rate properties into the ordinary rotation Property.
- **Identity/provenance:** stable identifiers, Zone ownership and Law authorship are explicit.

Deleting every Cube-specific renderer convenience must not erase the meaning of this being. The authored mathematics and Laws are the source of truth.

## Landed artifacts

- saves/zones/Primordial Spinning Cube/zone.json
- saves/laws/law-primordial-cube-spin/law.json
- saves/worlds/primordial_spinning_cube.json
- scripts/author_primordial_spinning_cube.py
- tests/zones/primordial_spinning_cube_test.cpp

The generator is intentionally a first-seed/reconstruction tool for this historical artifact. Future changes to an inhabited Person-owned copy should follow the repository's patch-not-regenerate rules.

## Geometry: six planes without a Platonic Six

For half-extent h = 1.5 m, the Field is:

    f(x,y,z) = max(abs(x) - h, abs(y) - h, abs(z) - h)

The six visible planar regions are consequences of the zero set. They are not six slots in Object, six branches in a renderer, or six members of a Cube ontology.

The regression witness checks the center, two face points and an exterior point through the production Object hydration path.

## Colour: historical readability without faceColors[6]

The 2025 object used different face colours because a spinning monochrome cube gives a weak rotation cue. The resurrection keeps that perceptual purpose but not the storage assumption.

Six palette values live as authored properties:

- primordial.palette.posX
- primordial.palette.negX
- primordial.palette.posY
- primordial.palette.negY
- primordial.palette.posZ
- primordial.palette.negZ

The Material's colorExpr derives colour from local signed position. Near edges and corners the contributions may blend; there is no discrete face index to consult.

## Motion: no hidden angular-velocity soul

A tempting implementation would set Object::angularVelocity and rely on the existing authored angular-kinematics Law. That property is currently registered but not persisted for Object, so doing that would make the resurrection's motion depend on runtime state that does not survive the authored temporal boundary.

Instead, the being owns persisted properties:

- primordial.spin.xDegreesPerSecond = 22
- primordial.spin.yDegreesPerSecond = 37
- primordial.spin.zDegreesPerSecond = 13

law-primordial-cube-spin is WhileTrue and maps:

    rotation = time * (spinX, spinY, spinZ)

The Law therefore reconstructs motion from world time plus authored properties after load.

During implementation the first hand-authored JSON accidentally bound all three rotation components to an unbound s factor. That was repaired before this task was closed, and the test now specifically requires sx, sy, and sz independently. This is exactly why presence of a Law record is not evidence that the Law says what its author intended.

## Machine witness

primordial_spinning_cube_test checks:

1. the Zone and world artifacts are readable;
2. the visible ancestor hydrates as ShapeKind::Field, never ShapeKind::Cube;
3. its authored record contains no faceColors;
4. its SdfPrim::Expr evaluates to the expected cubical zero set at canonical probes;
5. all six palette properties exist;
6. Material::colorExpr evaluates to the intended six signed-axis colours;
7. the saved Law is WhileTrue and Maps rotation;
8. time plus sx/sy/sz are explicit bindings;
9. each rotation component uses its own corresponding authored rate.

## Person witness — still open

A machine can prove that the authored math hydrates and that the Law record is structurally correct. It cannot prove that the resurrected object fulfills the original reason the cube existed.

Open primordial_spinning_cube in the actual app and witness:

- the object is plainly visible from the starting camera;
- its colour regions make its orientation legible;
- it visibly rotates around multiple axes;
- flying around it makes camera translation perceptible relative to a stable reference;
- leaving/saving/restarting does not erase the authored form, palette or spin behavior.

Only then does the historical sentence become true again:

**the first constellation spins.**

A green suite is not a witness.
