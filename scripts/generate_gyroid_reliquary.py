#!/usr/bin/env python3
"""
The Gyroid Reliquary — a Zone authored entirely in OntoMath
=================================================================================
Zone: saves/zones/The Gyroid Reliquary/zone.json
Authored by: opencode (space-bunny-free) — session gyroid-reliquary-2026-09-29
Under the authority of: Zachary Zhang (the Person who asked for it)

WHAT THIS IS
------------
A cathedral with no walls. Its entire architecture is ONE triply-periodic
minimal surface — the gyroid

    g(x,y,z) = sin(kx)·cos(ky) + sin(ky)·cos(kz) + sin(kz)·cos(kx)

— solved, not modelled. A minimal surface is the Plateau problem's answer: the
soap film between two wire loops, the shape a surface takes when it is pulled
as tight as it can possibly be. The gyroid is the only such surface that fills
all of space, and it does it perfectly periodically, forever, at every scale.

Two properties of the gyroid this Zone is built to show:

  1. g > 0 and g < 0 are two INTERPENETRATING LABYRINTHS that are congruent
     mirror images of each other. There is no outside. Every "room" has a
     congruent twin across the membrane. One surface, two churches, and the
     membrane is the only thing between them.
  2. |g| = t is a single unbroken surface with no edge, no seam and no end. It
     cannot be built, welded, or 3D-printed. It can only be EVALUATED. Which is
     exactly what OntoMath is for.

WHY IT IS WRITTEN THIS WAY (the Refusals)
-----------------------------------------
Refusal 1 — no new C++ class for a domain noun. There is no `Gyroid` class
  here, and none is wanted: `SdfPrim::Expr` + an OntoMath `MathNode` is the
  substrate that already expresses this exactly. Nothing was added to the
  engine to make this Zone exist.
Refusal 3 — no new enum value for a kind of thing. Not one `ShapeKind`,
  `SdfPrim`, or `MathNode::Op` was added. Every geometric primitive below
  (sphere, box, cylinder, torus) is written out as OntoMath arithmetic using
  `Length`, `Abs`, `Sqrt`, `Intersection` (= max) and `Difference` (= max with
  the subtrahend negated), so the Zone's forms are Persons'-readable
  mathematics rather than parameter slots in a C++ enum.
Refusal 6 — no black box. Every value chosen here is carried as an authored
  property on the being that owns it (`reliquary.*`), registered and Law-
  readable, not buried in a header. Where the engine's actual behaviour is
  narrower than the effect it sounds like, the property says so and says why
  (see `reliquary.analyticGradient` on the floor).
Refusal 7 — no new methods defining variable behavior. The light breathes
  because an authored OntoMath `t` term says so; nothing in C++ decides it.

COORDINATE SPACES — READ THIS BEFORE CHANGING ANY NUMBER
--------------------------------------------------------
Everything authored in this file is in METRES, and that is not a coincidence:

  * An Object's `mathNode` is evaluated in OBJECT-LOCAL METRES. The raymarcher
    maps the world ray through `inst.invModel` (SdfWgsl.cpp:1622) and `extents`
    is used only to size the unit proxy cube and the AABB (SdfWgsl.cpp:1304) —
    it is never folded into the local coordinate. `transform` places the object.
  * A FieldNode's expressions are evaluated in WORLD METRES RELATIVE TO THE
    FIELD'S `origin`: the volume transport computes `p = worldP - origin`
    (SdfWgsl.cpp:3616) and the source field computes
    `curLocal = curWorld - origin` (SdfWgsl.cpp:3548).

So the gyroid's wave number IS radians per metre here, and the Lipschitz
divisor that makes sphere tracing provably safe is 2√3·k in metres.

FIRST SEED
----------
This is a first seed of a NEW Zone, so generation is allowed
(FIRST_MOVER_AUTHORING.md §7 rule 8: "Scratch builds are allowed only for a
first seed"). Every later change to this Zone must PATCH this file's output in
place, never regenerate it. The stage → verify → atomic-rename discipline at
the bottom of this script is kept for that reason.
"""

from __future__ import annotations

import json
import math
import os
import shutil
import sys

# =============================================================================
# Constants of the Zone. Every one of these is a real measurement, not a taste.
# =============================================================================

ZONE_DIR = os.path.join("saves", "zones", "The Gyroid Reliquary")
ZONE_PATH = os.path.join(ZONE_DIR, "zone.json")

PERSON = "Zach"                        # the Person whose authority this is under
AGENT = "opencode (space-bunny-free)"  # who typed it
SESSION = "gyroid-reliquary-2026-09-29"

# --- The Reliquary's dimensions, in metres -----------------------------------
# A cubic domain, because a gyroid is cubic-periodic: give it a non-cubic box
# and the surface is sheared into something no longer minimal.
HALF = 30.0                            # membrane half-extent -> a 60 m cube
FLOOR_Y = -21.0                        # the glass floor
OCULUS_Y = 21.0                        # where the great ring hangs

# --- The gyroid ---------------------------------------------------------------
CELL_M = 12.0                          # one cubic cell of the lattice, in metres
K = 2.0 * math.pi / CELL_M             # 0.5235988 rad/m, i.e. a 12 m period
                                       # -> five bays across the 60 m cube
THICK = 0.42                           # half-thickness of the membrane, in |g|
# |∇g| ≤ 2√3·k EXACTLY: every partial of g is a difference of two products of
# sines and cosines, so each is bounded by 2k, and three of them give 2√3·k.
# Dividing by a value at or above that supremum turns the implicit function
# into a lower bound on the true distance, which is the one thing sphere
# tracing requires — with it, the marcher can never step through the membrane.
# The cost of that honesty is steps; the engine buys them back with its
# proof-based range hierarchy and over-relaxed marching.
LIPSCHITZ = 2.0 * math.sqrt(3.0) * K + 0.02   # 1.8338

# --- The Heart ---------------------------------------------------------------
HEART_SHELL_R = 8.0                   # the woven ball's radius, metres
HEART_CELL_M = 3.0                     # its lattice is four times finer
HEART_K = 2.0 * math.pi / HEART_CELL_M
HEART_LIP = 2.0 * math.sqrt(3.0) * HEART_K + 0.02

# --- The Piers ---------------------------------------------------------------
PIER_COUNT = 12
PIER_RING_R = 21.0                     # ring radius, metres
PIER_R = 1.9                           # pier radius, metres
PIER_H = 17.0                          # pier half-height, metres
PIER_CELL_M = 1.6                      # flute cells
PIER_K = 2.0 * math.pi / PIER_CELL_M
PIER_LIP = 2.0 * math.sqrt(3.0) * PIER_K + 0.02

# --- The Oculus ---------------------------------------------------------------
SHAFT_R = 6.0                          # the shaft the membrane is cut away for
RING_MAJOR_X = 17.0
RING_MAJOR_Z = 13.26                   # an ellipse, not a circle
RING_MINOR = 0.7

# =============================================================================
# OntoMath AST constructors.
# Opcodes are MathNode::Op from src/Singularity/OntoMath/ScalarForm.hpp:420-470,
# APPEND-ONLY; every value used here already exists. TransFactor::Kind is
# 0=Sin 1=Cos 2=Exp 3=Ln (ScalarForm.hpp:179). A Term's `trans` list
# MULTIPLIES (ScalarForm.cpp:44-49) — that is how a product of a sine and a
# cosine, which is one whole term of a gyroid, is written in one term.
# =============================================================================


def c(v):
    """A ScalarLeaf holding a constant."""
    return {"op": 0, "scalarForm": {"terms": [{"c": float(v), "factors": {}}]}}


def tr(kind, var, scale=1.0, shift=0.0):
    return {"kind": kind, "var": var, "scale": float(scale), "shift": float(shift)}


def trig(coef, *trans):
    """A ScalarLeaf: `coef * sin(...) * cos(...)` — one product, one term."""
    return {"op": 0, "scalarForm": {"terms": [{"c": float(coef), "factors": {}, "trans": list(trans)}]}}


def v(name):
    """A ValueLeaf: the ambient point's `name` component."""
    return {"op": 1, "var": name}


def vec3(x, y, z):
    return {"op": 2, "children": [x, y, z]}


def add(a, b):
    return {"op": 4, "children": [a, b]}


def sub(a, b):
    return {"op": 5, "children": [a, b]}


def mul(a, b):
    return {"op": 6, "children": [a, b]}


def length(a):
    return {"op": 11, "children": [a]}


def dist(a, b):
    return {"op": 15, "children": [a, b]}


def inter(a, b):
    """max(a, b) — the CSG intersection, and also plain `max`, which is how
    every box and cylinder below is written: a max over the axes IS a box."""
    return {"op": 21, "children": [a, b]}


def diff(a, b):
    """max(a, -b) — the CSG difference."""
    return {"op": 22, "children": [a, b]}


def div(a, b):
    return {"op": 23, "children": [a, b]}


def power(a, b):
    return {"op": 24, "children": [a, b]}


def absolute(a):
    return {"op": 25, "children": [a]}


def clamp01(a):
    """clamp(a, 0, 1) — the falloff used everywhere, so every field reaches
    exactly zero instead of stopping at a discontinuity."""
    return clamp(0.0, a, 1.0)


def clamp(lo, val, hi):
    return {"op": 26, "children": [val, c(lo), c(hi)]}


def root(a):
    return {"op": 27, "children": [a]}


def noise(a):
    return {"op": 29, "children": [a]}


def p():
    return v("p")


def bell(radius, softness, centre=None):
    """A radial bell envelope in [0,1] — 1 at the centre, exactly 0 past
    `radius + softness`, with a linear ramp across `softness` so the edge is
    a real falloff and not a cut."""
    d = dist(p(), centre) if centre is not None else length(p())
    return clamp01(sub(c(1.0), div(d, c(radius + softness))))


# =============================================================================
# The gyroid, and the shapes derived from it. All in metres.
# =============================================================================


def gyroid(k, phase_x=0.0, phase_y=0.0, phase_z=0.0):
    """g = sin(kx+px)·cos(ky+py) + sin(ky+py)·cos(kz+pz) + sin(kz+pz)·cos(kx+px)

    A term may hold any number of trans factors, so each sine·cosine product is
    one ScalarLeaf and the three of them are summed as MathNodes.
    """
    sx, sy, sz = tr(0, "x", k, phase_x), tr(0, "y", k, phase_y), tr(0, "z", k, phase_z)
    cx, cy, cz = tr(1, "x", k, phase_x), tr(1, "y", k, phase_y), tr(1, "z", k, phase_z)
    return add(add(trig(1.0, sx, cy), trig(1.0, sy, cz)), trig(1.0, sz, cx))


def gyroid_shell(k, thickness, lipschitz, phase=(0.0, 0.0, 0.0)):
    """A signed distance to the membrane |g| = thickness that is safe to march.

    Dividing by a value at or above |∇g|'s supremum produces a lower bound on
    the true distance. That is the whole requirement of sphere tracing, and it
    is why this is a divisor derived from a bound and not a tuned constant.
    """
    return mul(c(1.0 / lipschitz), sub(absolute(gyroid(k, *phase)), c(thickness)))


def sd_sphere(radius):
    """length(p) - r. Written out, not selected out of a prim enum."""
    return sub(length(p()), c(radius))


def sd_cylinder_y(radius, half_height):
    """A Y-axis capped cylinder as max(sqrt(x²+z²) - r, |y| - h)."""
    radial = sub(root(add(power(v("x"), c(2.0)), power(v("z"), c(2.0)))), c(radius))
    return inter(radial, sub(absolute(v("y")), c(half_height)))


def sd_ellipse_ring(major_x, major_z, minor, y_squash=1.0):
    """A ring in the XZ plane whose tube rides an ELLIPSE and is squashed in y.

    sqrt((x/a)² + (z/b)² - 1)² + y²/s² , all under a root, minus the tube
    radius. The only way to reach this shape through SdfPrim::Torus would be a
    non-uniform scale on the transform, which is a lie about the geometry and
    would also break the SDF's meaning under the inverse-transpose normal. Here
    the ellipse simply IS the distance function.
    """
    radial = sub(
        root(add(div(power(v("x"), c(2.0)), c(major_x * major_x)),
                 div(power(v("z"), c(2.0)), c(major_z * major_z)))),
        c(1.0))
    return sub(root(add(power(radial, c(2.0)),
                        div(power(v("y"), c(2.0)), c(y_squash * y_squash)))),
               c(minor))


# =============================================================================
# Property value wrapper — PropertyValueJson.cpp:19, {"t":..., "v"/"x","y","z"}.
# Nothing here is hidden state: every one is a registered, Law-readable path.
# =============================================================================


def pv(t, value):
    if t == "int":
        return {"t": "int", "v": int(value)}
    return {"t": t, "v": value}


def pv3(x, y, z):
    return {"t": "vec3", "x": float(x), "y": float(y), "z": float(z)}


# =============================================================================
# Object assembly
# =============================================================================


def mat4_at(x, y, z):
    return [1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            float(x), float(y), float(z), 1.0]


def sdf_expr(math_node):
    """An SdfNode leaf carrying a full OntoMath expression AS its distance.

    SdfPrim::Expr == 7 (Sdf.hpp:21-31). This is the substrate that lets a
    Person's mathematics be the geometry: evalLeaf evaluates `mathNode` and uses
    its value as the signed distance (Sdf.cpp:466-476).
    """
    return {"op": 0, "prim": 7, "dims": [0.5, 0.5, 0.5],
            "offset": [0.0, 0.0, 0.0], "p0": 0.0, "p1": 0.0, "t": 0.5,
            "mathNode": math_node}


def field_object(object_id, display, material, pos, math_node, extent,
                 cell_size, props, role):
    """Every Object in this Zone is a Field whose shape is OntoMath.

    `shapeKind`/`geometryType` 10 == ShapeKind::Field — the First-Mover seed
    bootstrap substrate ObjectTypes.hpp:32-58 reserves. `renderMode` 0 is Auto,
    which on the WebGPU build (`earthcall_webgpu`, THE app) raymarches the
    analytic form exactly and never falls back to a cached tessellation.
    """
    x, y, z = pos
    ex, ey, ez = (extent, extent, extent) if isinstance(extent, (int, float)) else extent
    return {
        "objectID": object_id,
        "shapeKind": 10,
        "geometryType": 10,
        "shapeParams": [1.0, 1.0, 1.0, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
        "transform": mat4_at(x, y, z),
        "center": [float(x), float(y), float(z)],
        "materialId": material,
        "renderMode": 0,
        "rotationResponsiveness": 10.0,
        "authoritativeAxis": [0, 1, 0],
        "targetRotation": [0.0, 0.0, 0.0],
        "x2D": 100.0, "y2D": 100.0, "zOrder2D": 0.0,
        "field": sdf_expr(math_node),
        "fieldExtent": [float(ex), float(ey), float(ez)],
        "fieldCellSize": float(cell_size),
        "authoredProperties": dict(props, **{
            "displayName": pv("string", display),
            "reliquary.role": pv("string", role),
        }),
    }


# =============================================================================
# THE GEOMETRY
# =============================================================================


def build_membrane():
    """THE MEMBRANE — the Reliquary's whole architecture, in one expression.

        membrane = max( max( (|g| − t)/L ,  ‖p‖ − R ) ,  −shaft )

    Read it inside out:

      * `(|g| − t)/L` is the infinite gyroid surface — the minimal surface with
        no edge, no seam and no end. It is PERIODIC, and that matters more than
        it first appears: the gyroid is not a hollow lattice but a SOLID that
        fills all of space, so this term alone is negative a long way from
        anywhere. A Reliquary built on it alone has no exterior at all — you
        cannot stand outside a thing made of it, because outside is also inside.
        (The verification test found this the hard way: 1031 of 3945 rays
        launched from 46 m away reported themselves embedded in the wall.)
      * `‖p‖ − R` intersects that infinite surface with a 30 m ball, which is
        what turns the effect into a PLACE: a hollow sphere of woven stone you
        can approach, stand outside, and look into.
      * `−shaft` is the Oculus — a 6 m cylinder of subtracted membrane straight
        up through the crown, the only opening in the Reliquary and the reason
        the light at its centre can be seen from anywhere in the labyrinth.

    Every term is 1-Lipschitz and a max of 1-Lipschitz functions is
    1-Lipschitz, so the whole expression is a true distance and the marcher can
    never step through the wall. That is asserted, not assumed — see the
    conservativeness check in tests/zones/gyroid_reliquary_test.cpp.
    """
    infinite = gyroid_shell(K, THICK, LIPSCHITZ)
    vessel = sub(length(p()), c(HALF))                 # inside the 30 m ball
    opened = mul(c(-1.0), sd_cylinder_y(SHAFT_R, HALF))
    return field_object(
        "reliquary.membrane",
        "The Membrane — one gyroid, cut to a sphere, opened at the crown",
        "reliquary.alabaster",
        (0.0, 0.0, 0.0),
        inter(inter(infinite, vessel), opened),
        HALF * 1.05,
        1.5,
        {
            "reliquary.surface": pv("string", "triply-periodic minimal surface (gyroid)"),
            "reliquary.cellMetres": pv("float", CELL_M),
            "reliquary.waveNumberPerMetre": pv("float", K),
            "reliquary.halfThicknessG": pv("float", THICK),
            "reliquary.lipschitzDivisor": pv("float", LIPSCHITZ),
            "reliquary.lipschitzProof": pv(
                "string",
                f"|grad g| <= 2*sqrt(3)*k = {2 * math.sqrt(3) * K:.4f} <= {LIPSCHITZ:.4f}; "
                "the divisor is a proved lower bound on the true distance, not a tuning"),
            "reliquary.oculusRadiusMetres": pv("float", SHAFT_R),
            "reliquary.vesselRadiusMetres": pv("float", HALF),
            "reliquary.mathIsUnbounded": pv("bool", True),
            "reliquary.boundingNote": pv(
                "string",
                "The gyroid is PERIODIC and the gyroid is a SOLID: it fills all "
                "of space, so the bare |g| = t surface is negative a long way from "
                "anywhere and a Reliquary built on it alone has no exterior to "
                "stand outside of. What gives this Zone a boundary is the "
                f"authored ‖p‖ − {HALF} m term intersected into the expression, "
                "not the proxy AABB. Raising that radius is the one-number way to "
                f"grow the Reliquary; fieldExtent ({HALF * 1.05} m) only follows it."),
            "reliquary.doctrine": pv(
                "string",
                "A Zone is a bound in a continuum. This one's bound is a surface "
                "that cannot be built, only evaluated."),
        },
        "membrane",
    )


def build_heart():
    """THE HEART — a solid ball woven out of the Reliquary's own lattice.

        heart = max( ‖p‖ − 8 ,  −g/L )

    The second term is the negative of the gyroid's own distance, so the region
    it carves is g > 0: the g-positive half of space, which is a labyrinth.
    max() of the two keeps what is both inside the ball and inside that
    labyrinth — a ball with the corridors cut clean through it. You can see
    into the Heart, through it, and out the far side, and what you are seeing is
    the same surface the whole Reliquary is made of at a third of its cell size.

    This is NOT the same as intersecting the ball with the gyroid's SURFACE.
    That was the first thing written here and it is wrong: a surface
    intersected with a solid is a curve, and `max(ball, g/L)` collapses to the
    ball itself with about a tenth of a metre of fluting on its skin, because
    the lattice's positive range is tiny next to the ball's. The Heart has to
    be a gyroid SOLID, not a gyroid shell. The test is what caught it.
    """
    ball = sub(length(p()), c(HEART_SHELL_R))
    labyrinth = mul(c(-1.0 / HEART_LIP), gyroid(HEART_K))
    return field_object(
        "reliquary.heart",
        "The Heart — a ball woven solid out of the Reliquary's own labyrinth",
        "reliquary.gold",
        (0.0, 0.0, 0.0),
        inter(ball, labyrinth),
        HEART_SHELL_R,
        0.35,
        {
            "reliquary.surface": pv("string", "ball intersected with the gyroid half-space g > 0"),
            "reliquary.cellMetres": pv("float", HEART_CELL_M),
            "reliquary.radiusMetres": pv("float", HEART_SHELL_R),
            "reliquary.waveNumberPerMetre": pv("float", HEART_K),
            "reliquary.lipschitzDivisor": pv("float", HEART_LIP),
            "reliquary.solidFractionNominal": pv("float", 0.5),
            "reliquary.doctrine": pv(
                "string",
                "The two labyrinths meet here and neither can tell which is "
                "which. That is the Reliquary's whole claim."),
        },
        "heart",
    )


def build_piers():
    """THE TWELVE PIERS — a ring of columns, each cored out by the gyroid.

    max( cylinder , g/L ) keeps what is both inside the column and inside the
    labyrinth, so a solid pier becomes a pier with the corridors cut clean
    through it — a column whose flutes ARE the minimal surface, at 1.6 m cells.
    Twelve of them, on a 21 m ring.

    The divisor matters here in a way it does not for the Membrane: `max` is
    only a well-mixed CSG when both of its arguments are distances in the same
    unit. The cylinder is in metres and the raw gyroid is in |g| units, so
    without the divisor the g term would simply swamp a 1.9 m radius and the
    piers would come out as loose cylinders of raw labyrinth with no column
    left in them. Divided, both are metres and the intersection is the one a
    Person reading the expression would expect.
    """
    piers = []
    column = inter(sd_cylinder_y(PIER_R, PIER_H), mul(c(1.0 / PIER_LIP), gyroid(PIER_K)))
    for i in range(PIER_COUNT):
        theta = 2.0 * math.pi * i / PIER_COUNT
        px, pz = PIER_RING_R * math.cos(theta), PIER_RING_R * math.sin(theta)
        piers.append(field_object(
            f"reliquary.pier.{i:02d}",
            f"Pier {i + 1} of {PIER_COUNT} — the labyrinth cored through the column",
            "reliquary.basalt",
            (px, 0.0, pz),
            column,
            PIER_H,
            0.4,
            {
                "reliquary.surface": pv("string", "cylinder intersected with the gyroid half-space g > 0"),
                "reliquary.index": pv("int", i + 1),
                "reliquary.of": pv("int", PIER_COUNT),
                "reliquary.cellMetres": pv("float", PIER_CELL_M),
                "reliquary.lipschitzDivisor": pv("float", PIER_LIP),
                "reliquary.angleDegrees": pv("float", math.degrees(theta)),
            },
            "pier",
        ))
    return piers


def build_floor():
    """THE GLASS FLOOR — two crossed wave trains under the Reliquary.

    y - h(x, z) = 0 is the one shape here the marcher recognises as a
    heightfield (Sdf.cpp:971-991): a `Sub` whose left side is y and whose right
    side does not read y. h is a long swell crossed with a short interference
    ripple, both in metres, so the floor is a lake of glass with two wave trains
    running across it.

    `reliquary.analyticGradient` is FALSE, and that is recorded rather than
    wished away: the analytic-gradient emitter refuses any ScalarLeaf carrying
    factors or trans (SdfWgsl.cpp:712-721), and every wave in this Zone is a
    trans. So the floor takes central-difference normals. It is still exact as
    a surface — only the normal is differenced instead of derived.
    """
    h = add(
        mul(c(0.55), trig(1.0, tr(0, "x", 1.1), tr(1, "z", 1.1))),
        mul(c(0.16), trig(1.0, tr(0, "x", 4.3, 1.3), tr(1, "z", 3.7, 0.4))),
    )
    return field_object(
        "reliquary.floor",
        "The Glass Floor — two crossed wave trains",
        "reliquary.glass",
        (0.0, FLOOR_Y, 0.0),
        sub(v("y"), h),
        (HALF, 3.0, HALF),
        0.75,
        {
            "reliquary.surface": pv("string", "heightfield y - h(x, z)"),
            "reliquary.heightMetres": pv("float", FLOOR_Y),
            "reliquary.swellMetres": pv("float", 0.55),
            "reliquary.rippleMetres": pv("float", 0.16),
            "reliquary.provenHeightfield": pv("bool", True),
            "reliquary.analyticGradient": pv("bool", False),
            "reliquary.analyticGradientRefusal": pv(
                "string",
                "isDifferentiableAst refuses a ScalarLeaf with factors or trans "
                "(SdfWgsl.cpp:712-721); every wave here is a trans, so normals are "
                "central-differenced. The surface itself is exact."),
            "reliquary.doctrine": pv(
                "string",
                "The one surface in the Reliquary the machine can prove is a "
                "heightfield, and the one place it declines to differentiate it."),
        },
        "floor",
    )


def build_oculus_rings():
    """THE OCULUS RINGS — a great ellipse hanging over the shaft, and a second
    squashed ring tilted across it. Two authored distance functions; no
    transform scaling anywhere, so neither ring is a lie about its shape."""
    specs = [
        (RING_MAJOR_X, RING_MAJOR_Z, RING_MINOR, 1.0, OCULUS_Y,
         "Oculus Ring I — a gold ellipse over the shaft"),
        (RING_MAJOR_X * 0.62, RING_MAJOR_X * 0.62, RING_MINOR * 0.7, 2.4, OCULUS_Y - 4.5,
         "Oculus Ring II — squashed and dropped through the first"),
    ]
    rings = []
    for i, (mx, mz, minor, ys, y, display) in enumerate(specs):
        rings.append(field_object(
            f"reliquary.oculus.ring.{i}",
            display,
            "reliquary.brass",
            (0.0, y, 0.0),
            sd_ellipse_ring(mx, mz, minor, ys),
            max(mx, mz) * 1.15,
            0.5,
            {
                "reliquary.surface": pv("string", "elliptical torus, authored in the distance function"),
                "reliquary.majorXMetres": pv("float", mx),
                "reliquary.majorZMetres": pv("float", mz),
                "reliquary.minorMetres": pv("float", minor),
                "reliquary.ySquash": pv("float", ys),
                "reliquary.heightMetres": pv("float", y),
            },
            "oculus_ring",
        ))
    return rings


def build_vigil():
    """THE VIGIL — forty-eight beads set on the great ellipse, each one a
    witness that the lattice continues above the eye.

    These are the one place a parameterization substrate is used: a bead is a
    parameter (SdfPrim::Sphere with three radii), not a kind of thing, which is
    exactly the line ObjectTypes.hpp:32-58 draws for First-Mover seed work.
    """
    beads = []
    count = 48
    for i in range(count):
        theta = 2.0 * math.pi * i / count
        bx, bz = RING_MAJOR_X * math.cos(theta), RING_MAJOR_Z * math.sin(theta)
        beads.append({
            "objectID": f"reliquary.vigil.{i:02d}",
            "shapeKind": 10,
            "geometryType": 10,
            "shapeParams": [0.34, 0.34, 0.34, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
            "transform": mat4_at(bx, OCULUS_Y, bz),
            "center": [float(bx), float(OCULUS_Y), float(bz)],
            "materialId": "reliquary.brass",
            "renderMode": 0,
            "rotationResponsiveness": 10.0,
            "authoritativeAxis": [0, 1, 0],
            "targetRotation": [0.0, 0.0, 0.0],
            "x2D": 100.0, "y2D": 100.0, "zOrder2D": 0.0,
            "field": {"op": 0, "prim": 0, "dims": [0.34, 0.34, 0.34],
                      "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": 0.5},
            "fieldExtent": [0.5, 0.5, 0.5],
            "authoredProperties": {
                "displayName": pv("string", f"Vigil {i + 1} of {count}"),
                "reliquary.role": pv("string", "vigil_bead"),
                "reliquary.index": pv("int", i + 1),
                "reliquary.of": pv("int", count),
            },
        })
    return beads


# =============================================================================
# THE LIGHT
#
# A FieldNode is a being, not a render struct (FieldNode.cpp:42-54). The
# Zone's radiant constitution is
#
#     E_i(p, t) = rho_i(p, t) · chi_i(p, t) · alpha_i(p, omega, t)
#
# and every factor below is authored OntoMath with its numbers registered as
# properties, so a Law can read and change any of them.
# =============================================================================


def piecewise(math_node, input_var="x"):
    """A single unbounded Piecewise over an authored expression.

    `input_var` is the variable the DISCRETE piece bounds are stated in, and it
    is NOT free choice. `emitPiecewise` resolves it through `pointComponent`
    (SdfWgsl.cpp:664), which binds x, y, z, t, n, omega.*, wi.* and wo.* — and
    nothing else. Declaring `"input": "p"` therefore REFUSES the whole shader:
    "a field expression names the variable 'p', which has no binding in this
    shader expression context", once per frame, per source, forever.

    The subtlety worth keeping: `p` IS legal inside a piece's mathNode, because
    `emitMathNode` special-cases a ValueLeaf of `p` before it ever reaches
    pointComponent. So `length(p)` compiles and `"input": "p"` does not, and
    they are one character apart. Every other Zone in the tree says "x"; the
    first version of this said "p" and cost an afternoon of log spam.

    Every piece here is unbounded, so the input only has to be legal, not
    meaningful. "x" is both.
    """
    return {"input": input_var, "pieces": [{"hasLo": False, "hasHi": False, "mathNode": math_node}]}


def scalar_field(math_node, amplitude=1.0):
    return {"mode": "AST", "baseDensity": 1.0, "frequency": 1.0,
            "amplitude": float(amplitude), "astDefinition": piecewise(math_node)}


def null_vector_field():
    return {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0}


def light_props(name, color, intensity, ambient=0.06, attenuation=(1.0, 0.0, 0.0)):
    return {
        "displayName": pv("string", name),
        "light.source": pv("bool", True),
        "light.enabled": pv("bool", True),
        "light.color": pv3(*color),
        "light.intensity": pv("float", intensity),
        "light.ambient": pv("float", ambient),
        "light.diffuse": pv("float", 1.0),
        "light.specular": pv("float", 1.0),
        "light.attenuation.constant": pv("float", attenuation[0]),
        "light.attenuation.linear": pv("float", attenuation[1]),
        "light.attenuation.quadratic": pv("float", attenuation[2]),
    }


def phase_chroma(inner_r, softness, inner, outer):
    """Chroma as a function of radius: warm at the source, cool at the edge.

    Each channel lerps between two authored vec3s by one shared bell envelope,
    so the gradient is radial and legible rather than a rainbow nobody chose.
    """
    w = bell(inner_r, softness)
    inv = sub(c(1.0), w)
    return vec3(*[add(mul(w, c(hi)), mul(inv, c(lo))) for lo, hi in zip(inner, outer)])


def gyroid_chroma(k, bias, gain, offsets):
    """Chroma read off the gyroid's OWN phase: one sine·cosine product per
    channel, at 120° apart. The result is a triply-periodic colour field on
    exactly the lattice the geometry is cut from, so colour and form are two
    readings of one expression and the iridescence can never drift off the
    surface the way a hand-authored texture would."""
    return vec3(*[
        add(mul(c(bias), c(1.0)),
            mul(c(gain), trig(1.0, tr(0, axis_a, k, off), tr(1, axis_b, k, off))))
        for axis_a, axis_b, off in (("x", "y", offsets[0]),
                                     ("y", "z", offsets[1]),
                                     ("z", "x", offsets[2]))
    ])


def omni(strength):
    """Angular emission with no direction preference — the honest description of
    a source that is itself a mathematical surface rather than a lamp."""
    return c(strength)


# --- The five radiant beings, plus the Zone's canonical root ------------------


def build_spatial_root():
    """THE RELIQUARY'S OWN LIGHT — the Zone's canonical continuous field.

    rho peaks in the open channels, where |g| is small, and is exactly zero at
    the membrane, so the labyrinth glows and the walls stay walls. The `t` term
    breathes the whole Zone by 8% at 0.22 Hz. Nothing in C++ decides that; the
    number lives in the expression, and the expression is a registered property.

    The bell envelope is not decoration. Without it this light is rho = 0.276 at
    the origin and rho = 0.276 at nine hundred metres — the same number — so
    the Zone's own glow is an infinite fog that happens to be thin, and looking
    away from the Reliquary shows no boundary at all. A source with no envelope
    is a source with no edge, and the test that reads this back caught exactly
    that before the Zone was ever opened.
    """
    g = gyroid(K * 0.5)
    breath = trig(1.0, tr(0, "t", 1.3823))          # 0.22 Hz at 1 rad/s
    rho = mul(
        mul(mul(c(0.30), clamp01(sub(c(1.0), div(absolute(g), c(1.05))))),
            bell(HALF * 1.4, HALF * 0.5)),
        add(c(0.92), mul(c(0.08), breath)))
    return {
        "id": "reliquary.own-light",
        "origin": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(rho),
        "vectorField": null_vector_field(),
        "lightChroma": piecewise(gyroid_chroma(K * 0.5, 0.62, 0.38, (0.0, 2.0944, 4.1888))),
        "lightAngular": piecewise(omni(0.85)),
        "authoredProperties": light_props(
            "The Reliquary's Own Light — breathing in the channels",
            (0.86, 0.82, 1.0), 0.85, ambient=0.10, attenuation=(1.0, 0.004, 0.0)),
    }


def build_heart_source():
    """THE HEART'S RADIANCE — the source, in its own terms.

    A bell of radius 9 m in gold, warming outward through amber to a deep
    ember, with a 0.11 Hz swell so the Reliquary's pulse never quite repeats
    on a count a Person could keep.
    """
    swell = trig(1.0, tr(0, "t", 0.6912))
    rho = mul(bell(HEART_SHELL_R, 4.0), add(c(0.90), mul(c(0.10), swell)))
    return {
        "id": "reliquary.heart-source",
        "origin": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(mul(c(3.4), rho)),
        "vectorField": null_vector_field(),
        "lightChroma": piecewise(phase_chroma(
            HEART_SHELL_R, 5.0,
            inner=(1.00, 0.62, 0.16),      # ember at the edge
            outer=(1.00, 0.95, 0.78))),    # white-gold at the centre
        "lightAngular": piecewise(omni(2.6)),
        "authoredProperties": light_props(
            "The Heart's Radiance — gold, cooling to ember",
            (1.0, 0.78, 0.34), 2.4, ambient=0.05, attenuation=(1.0, 0.0, 0.006)),
    }


def build_labyrinth_medium():
    """THE LABYRINTH MEDIUM — fog that exists only where the surface is not.

    volumeDensity peaks in the channels, reaches exactly zero at the membrane,
    and is bounded by a strict bell so it never leaks outside the Zone. Deep
    indigo, because the Heart is gold and the one thing this Zone must never be
    is monochrome.

    There is no `volumeOccluder` here even though FieldNode serializes one
    (FieldNode.cpp:32-34): the WGSL volume transport never reads it, so
    authoring one would be a field no law could see take effect. The membrane
    is opaque geometry instead, so the medium is bounded by the marching
    surface itself — the channel in front of a wall glows, and the wall behind
    it is never seen through. That is a real occlusion and it is the honest one.
    """
    g = gyroid(K)
    density = mul(
        mul(c(0.85), clamp01(sub(c(1.0), div(absolute(g), c(1.20))))),
        bell(HALF * 1.5, HALF * 0.6))
    return {
        "id": "reliquary.labyrinth-medium",
        "origin": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(density, amplitude=0.6),
        "vectorField": {
            "mode": "AST", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 1.0,
            "astDefinition": piecewise(vec3(
                trig(1.0, tr(0, "x", 0.31), tr(1, "z", 0.31)),
                trig(1.0, tr(0, "y", 0.27, 1.1), tr(1, "x", 0.27, 1.1)),
                trig(1.0, tr(0, "z", 0.23, 2.2), tr(1, "y", 0.23, 2.2)),
            )),
        },
        "volumeDensity": piecewise(density),
        "volumeExtinction": piecewise(mul(c(1.35), density)),
        "volumeScattering": piecewise(mul(c(0.90), density)),
        "volumeChroma": piecewise(phase_chroma(
            HALF, HALF * 0.5,
            inner=(0.36, 0.30, 0.78),      # violet at the centre
            outer=(0.16, 0.20, 0.52))),    # deep indigo at the walls
        "authoredProperties": dict(light_props(
            "The Labyrinth Medium — indigo fog, bounded by the membrane itself",
            (0.30, 0.30, 0.72), 0.0, ambient=0.0), **{
            "radiance.role": pv("string", "participating medium, not a source"),
            "reliquary.densityPeak": pv("float", 0.85),
            "reliquary.envelopeRadiusMetres": pv("float", HALF * 1.5),
            "reliquary.noVolumeOccluder": pv(
                "string",
                "FieldNode serializes volumeOccluder but the WGSL volume "
                "transport never reads it; the membrane's own opaque surface "
                "does the occluding, so no occluder is authored."),
        }),
    }


def build_shaft_beam():
    """THE SHAFT BEAM — the light escaping through the Oculus.

    A cylinder of emissive medium on the Reliquary's axis, chroma rising from
    white-gold at the floor to a cold blue-white at the opening. This is the
    Zone's only vertical, and it is the first thing a Person will see.
    """
    r = root(add(power(v("x"), c(2.0)), power(v("z"), c(2.0))))
    within = clamp01(sub(c(1.0), div(r, c(SHAFT_R * 1.15))))
    vertical = clamp01(sub(c(1.0), div(absolute(v("y")), c(22.0))))
    density = mul(mul(c(2.20), within), vertical)
    rise = clamp(0.0, div(add(v("y"), c(21.0)), c(42.0)), 1.0)
    chroma = vec3(
        add(c(0.22), mul(c(0.78), rise)),
        add(c(0.34), mul(c(0.60), rise)),
        add(c(0.62), mul(c(0.34), rise)),
    )
    return {
        "id": "reliquary.shaft-beam",
        "origin": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(mul(c(1.90), density)),
        "vectorField": null_vector_field(),
        "lightChroma": piecewise(chroma, input_var="y"),
        "lightAngular": piecewise(c(1.15)),
        "volumeDensity": piecewise(density),
        "volumeExtinction": piecewise(mul(c(0.70), density)),
        "volumeEmission": piecewise(mul(c(1.50), density)),
        "volumeChroma": piecewise(chroma, input_var="y"),
        "authoredProperties": dict(light_props(
            "The Shaft Beam — the Reliquary's only vertical",
            (1.0, 0.94, 0.76), 1.6, ambient=0.03, attenuation=(1.0, 0.0, 0.0)), **{
            "radiance.role": pv("string", "emissive participating medium"),
            "reliquary.shaftRadiusMetres": pv("float", SHAFT_R),
            "reliquary.beamHalfHeightMetres": pv("float", 22.0),
            "reliquary.chromaAtFloor": pv3(0.22, 0.34, 0.62),
            "reliquary.chromaAtOculus": pv3(1.00, 0.94, 0.96),
        }),
    }


def build_iris_halo():
    """THE IRIS HALO — a cold counter-light ringing the Heart at 16 m.

    The Heart is gold. This is teal, and it is the reason the gold reads as
    gold: without a complement on the far side of it, a warm source in a dark
    field is only an orange blob. The chroma is a two-point blend across the
    halo's own width, written out in full.
    """
    r = length(p())
    band = clamp01(sub(c(1.0), div(absolute(sub(r, c(16.0))), c(3.2))))
    density = mul(mul(c(0.30), band), bell(30.0, 8.0))
    chroma = phase_chroma(16.0, 3.2,
                          inner=(0.10, 0.86, 0.82),    # teal on the band
                          outer=(0.10, 0.42, 0.74))    # deep blue off it
    return {
        "id": "reliquary.iris-halo",
        "origin": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(mul(c(1.10), density)),
        "vectorField": null_vector_field(),
        "lightChroma": piecewise(chroma),
        "lightAngular": piecewise(c(1.00)),
        "volumeDensity": piecewise(density),
        "volumeExtinction": piecewise(mul(c(0.50), density)),
        "authoredProperties": dict(light_props(
            "The Iris Halo — teal at 16 m, the Heart's complement",
            (0.12, 0.78, 0.80), 0.9, ambient=0.04, attenuation=(1.0, 0.006, 0.0)), **{
            "radiance.role": pv("string", "counter-light ring"),
            "reliquary.haloRadiusMetres": pv("float", 16.0),
            "reliquary.haloWidthMetres": pv("float", 3.2),
        }),
    }


def build_nimbus():
    """THE NIMBUS — Perlin-warmed cloud outside the Reliquary.

    Without it, looking out through the Oculus shows a void. With it, the
    Oculus opens onto weather. The envelope is a strict bell, so the cloud
    never leaks back inside and double-lights the labyrinth.
    """
    body = noise(mul(c(0.045), p()))
    density = mul(mul(c(0.55), clamp01(add(c(0.62), body))),
                  bell(96.0, 34.0))
    return {
        "id": "reliquary.nimbus",
        "origin": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(mul(c(0.50), density)),
        "vectorField": null_vector_field(),
        "lightChroma": piecewise(gyroid_chroma(K * 0.25, 0.34, 0.30, (0.0, 2.0944, 4.1888))),
        "lightAngular": piecewise(omni(0.60)),
        "volumeDensity": piecewise(density),
        "volumeExtinction": piecewise(mul(c(0.40), density)),
        "volumeChroma": piecewise(vec3(
            add(c(0.10), mul(c(0.34), trig(1.0, tr(0, "x", 0.08), tr(1, "y", 0.08)))),
            add(c(0.13), mul(c(0.26), trig(1.0, tr(0, "y", 0.07, 2.1), tr(1, "z", 0.07, 2.1)))),
            add(c(0.30), mul(c(0.44), trig(1.0, tr(0, "z", 0.06, 4.2), tr(1, "x", 0.06, 4.2)))),
        )),
        "authoredProperties": dict(light_props(
            "The Nimbus — weather on the far side of the Oculus",
            (0.20, 0.18, 0.44), 0.35, ambient=0.05, attenuation=(1.0, 0.0, 0.0)), **{
            "radiance.role": pv("string", "participating medium, faint"),
            "reliquary.noiseScalePerMetre": pv("float", 0.045),
            "reliquary.envelopeRadiusMetres": pv("float", 96.0),
        }),
    }


# =============================================================================
# THE MATERIALS — paint lives on the Material being and materials are shared,
# so each is written once and named by every Object that resolves it.
# =============================================================================


def material(name, base, ambient, diffuse, specular, shininess, opacity=1.0, color_expr=None):
    m = {
        "name": name,
        "baseColor": [float(x) for x in base],
        "opacity": float(opacity),
        "shininess": float(shininess),
        "specular": float(specular),
        "ambient": float(ambient),
        "diffuse": float(diffuse),
    }
    if color_expr is not None:
        m["colorExpr"] = color_expr
    return m


def build_materials():
    """Five materials, each carrying an authored `colorExpr` where the surface
    ought to be more than one colour.

    The membrane's is the important one: an opal, banded in three orthogonal
    wave trains at 120°, phase-locked to a gyroid at half the architectural
    wave number — so the colour bands are twice the size of the surface's cells
    and the two read as related rather than identical. Colour and form are two
    readings of one family of expressions, which is the only reason the
    iridescence cannot drift off the lattice the way a painted texture would.
    """
    return [
        material("reliquary.alabaster", (0.86, 0.86, 0.90), 0.30, 0.70, 0.55, 48.0,
                 color_expr=piecewise(gyroid_chroma(K * 0.5, 0.70, 0.30, (0.0, 2.0944, 4.1888)))),
        material("reliquary.gold", (1.0, 0.78, 0.34), 0.34, 0.72, 1.30, 96.0,
                 color_expr=piecewise(gyroid_chroma(HEART_K * 0.5, 0.66, 0.34,
                                                   (0.0, 2.0944, 4.1888)))),
        material("reliquary.basalt", (0.10, 0.10, 0.13), 0.16, 0.62, 0.70, 40.0,
                 color_expr=piecewise(gyroid_chroma(PIER_K * 0.5, 0.30, 0.22,
                                                   (0.0, 2.0944, 4.1888)))),
        material("reliquary.glass", (0.05, 0.07, 0.12), 0.10, 0.34, 1.60, 220.0, opacity=0.62,
                 color_expr=piecewise(vec3(
                     add(c(0.05), mul(c(0.30), trig(1.0, tr(0, "x", 0.55), tr(1, "z", 0.55)))),
                     add(c(0.08), mul(c(0.34), trig(1.0, tr(0, "x", 0.55, 2.1), tr(1, "z", 0.55, 2.1)))),
                     add(c(0.16), mul(c(0.52), trig(1.0, tr(0, "x", 0.55, 4.2), tr(1, "z", 0.55, 4.2)))),
                 ))),
        material("reliquary.brass", (0.78, 0.60, 0.26), 0.28, 0.70, 1.10, 110.0),
    ]


# =============================================================================
# ASSEMBLY
# =============================================================================


def build_zone():
    objects = [build_membrane(), build_heart()]
    objects.extend(build_piers())
    objects.append(build_floor())
    objects.extend(build_oculus_rings())
    objects.extend(build_vigil())

    fields = [
        build_heart_source(),
        build_labyrinth_medium(),
        build_shaft_beam(),
        build_iris_halo(),
        build_nimbus(),
    ]
    # FieldNode::toJson writes a key only when the Piecewise has pieces
    # (FieldNode.cpp:14-30), so a null here would be a key that never
    # round-trips. Drop it rather than write something that cannot survive.
    for f in fields:
        for key in [k for k, val in f.items() if val is None]:
            del f[key]

    return {
        "identifier": "The Gyroid Reliquary",
        "name": "The Gyroid Reliquary",
        "authors": [PERSON],
        "injected_by": (
            f"{AGENT} — session {SESSION}. A Zone authored entirely in OntoMath, "
            f"under the authority of {PERSON}. No engine change was made to admit "
            f"it: no new class, no new enum value, no new method."),
        "scope": "Local",
        "deletable": {PERSON: True},
        "qualities": {
            "ownerKind": "person",
            "reliquary.surface": "triply-periodic minimal surface (gyroid)",
            "reliquary.cellMetres": "12",
            "reliquary.baysAcross": "5",
            "reliquary.oculusRadiusMetres": "6",
        },
        "parentZone": "",
        "owner": PERSON,
        "spatialRoot": build_spatial_root(),
        "spatialFields": fields,
        "materials": build_materials(),
        "world": {"objects": objects, "relations": [], "laws": []},
        "formationRelations": {"relations": []},
        "lexemes": [],
    }


def main():
    print(f"Building 'The Gyroid Reliquary' -> {ZONE_PATH}")

    zone = build_zone()
    objects = zone["world"]["objects"]

    os.makedirs(ZONE_DIR, exist_ok=True)
    staged = ZONE_PATH + ".staged"
    with open(staged, "w") as f:
        json.dump(zone, f, indent=2)

    # --- verify the staged file BEFORE it is allowed to replace anything -----
    with open(staged) as f:
        v = json.load(f)
    assert v["identifier"] == "The Gyroid Reliquary", "the Zone lost its identity"
    assert v["owner"] == PERSON, f"the Person's ownership of this Zone was lost"
    assert v["injected_by"], "no author was recorded"
    assert len(objects) == 2 + PIER_COUNT + 1 + 2 + 48, f"object count drifted: {len(objects)}"

    # Every Expr shape must still carry its mathematics, or evalLeaf falls
    # through to an empty RPN and the shape becomes empty space (Sdf.cpp:477-489).
    expr_shapes = [o for o in objects if o["field"]["prim"] == 7]
    assert all(o["field"].get("mathNode") for o in expr_shapes), \
        "an Expr shape lost its mathematics and would render as empty space"
    assert len(expr_shapes) == 17, f"expected 17 authored shapes, got {len(expr_shapes)}"

    assert len(v["spatialFields"]) == 5, "a radiant being was lost"
    assert v["spatialRoot"]["field"]["astDefinition"]["pieces"], "the root light lost its math"
    assert len(v["materials"]) == 5

    # Every material an object names must exist. MaterialManager::get matches on
    # the name with the "material." prefix stripped (MaterialManager.cpp:18-33),
    # so an object naming a material that was not written falls back to the
    # default white one and the Zone silently loses its whole palette.
    named = {m["name"] for m in v["materials"]}
    for o in objects:
        assert o["materialId"] in named, f"{o['objectID']} names a missing material"

    if os.path.exists(ZONE_PATH):
        backup = ZONE_PATH + ".bak"
        shutil.copy2(ZONE_PATH, backup)
        print(f"  backed up the existing Zone to {backup}")

    os.replace(staged, ZONE_PATH)
    print(f"  {len(objects)} objects ({len(expr_shapes)} authored in OntoMath, "
          f"{len(objects) - len(expr_shapes)} parameterized beads)")
    print(f"  1 spatial root + {len(v['spatialFields'])} radiant beings, "
          f"{len(v['materials'])} materials")
    print(f"  gyroid k = {K:.6f} rad/m ({CELL_M} m cells, 5 bays across 60 m), "
          f"Lipschitz divisor {LIPSCHITZ:.4f}")
    print(f"  SUCCESS: {ZONE_PATH}")


if __name__ == "__main__":
    sys.exit(main())
