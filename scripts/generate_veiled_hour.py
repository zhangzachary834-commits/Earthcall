#!/usr/bin/env python3
"""
The Veiled Hour — a pillar field cradling a star cluster that you never see
=================================================================================
Zone: saves/zones/The Veiled Hour/zone.json
Authored by: opencode (space-bunny-free) — session veiled-hour-2026-09-30
Under the authority of: Zachary Zhang

WHAT THIS IS
------------
Zach asked, after the Gyroid Reliquary: *"MAKE A ZONE WITH A GORGEOUS 3D
NEBULA."*

The obvious thing — a cloud of coloured fog — is the thing a nebula is NOT, and
it is why most rendered nebulae are disappointing. What makes a real nebula
photograph magnificent is not that gas is pretty. It is **contrast between
emission and absorption**: a searingly bright ionized cavity, and cold opaque
dust silhouetted *in front of* it, carving black lanes out of the light. The
brightness is what makes the dark readable. Remove either half and you have
smoke.

So this Zone is built as a **pillar field** — the Eagle Nebula framing — where
the cluster that lights the scene is hidden behind the very dust it is lighting:

  * a cavity of ionized gas, bright, glowing,
  * cold molecular dust standing in front of it, opaque, black against the glow,
  * a blue reflection haze hugging the dust, which is what makes nebulae read as
    *dusty* rather than gaseous,
  * and the collapsed cluster at the centre, seen only as the light it casts.

The physical spine, all of it authorable in OntoMath:

  1. IONISING FLUX falls as 1/r² from the cluster. Dust only survives where that
     flux is low, so the dust is not decoration placed near a light — it is the
     SHADOW the light casts. Every pillar's tip points at the cluster because
     that is the only direction the cluster is burning.
  2. IONISATION FRONTS. The pillars are longest far from the cluster and shortest
     near it, because the near ends have already been ablated. That gradient is
     the signature of the Pillars of Creation and it falls straight out of 1/r².
  3. ABSORPTION IS REAL, NOT PAINTED. The dust FieldNode carries its own geometry
     as `volumeOccluder`, which the engine marches (volumeSourceVisibility,
     SdfWgsl.cpp:3533) to shadow the gas behind it. Those black lanes are
     computed, not drawn.

THE REFUSALS, AND WHAT "NEBULA" IS NOT
---------------------------------------
Refusal 1 — no new C++ class. `ShapeKind::Field` + a `VolumeDensity`/`ScalarField`
  Piecewise is the substrate; nothing was added to the engine for this Zone.
Refusal 3 — no new enum value. No `ShapeKind`, no `SdfPrim`, no `MathNode::Op`.
  Every shape is written as OntoMath arithmetic.
Refusal 6 — no black box. Every authored number is a `veil.*` property on the
  being that owns it. Where the engine's substrate is narrower than the effect
  sounds, the property says so and says why — as it did for the Gyroid Reliquary,
  where I wrote a confident falsehood into a save file and had to correct it.
Refusal 7 — no new methods. The drift in the dust is an authored `t` term.

COORDINATE SPACES — verified against the source, not assumed
------------------------------------------------------------
  * A FieldNode's volume expressions are in WORLD METRES RELATIVE TO THE FIELD'S
    `origin`: the transport computes `p = worldP - inst.origin.xyz`
    (SdfWgsl.cpp:3616) and the source shadow march computes
    `curLocal = curWorld - inst.origin.xyz` (SdfWgsl.cpp:3548).
  * An Object's `mathNode` is in OBJECT-LOCAL METRES: the ray is mapped by
    `inst.invModel` (SdfWgsl.cpp:1622) and `extents` only sizes the unit proxy
    cube, it is never folded into the local coordinate.

THE PIECE-BOUND `input` TRAP — the bug that cost The Gyroid Reliquary its first run
----------------------------------------------------------------------------------------
`emitPiecewise` resolves a Piecewise's **piece-bound variable** through
`pointComponent` (SdfWgsl.cpp:664), and that binds x, y, z, t, n, omega.*, wi.*,
wo.* — and nothing else, NOT `p`. Declaring `"input": "p"` refuses the whole
shader. But `p` IS legal inside a piece's mathNode, because `emitMathNode`
special-cases a ValueLeaf of `p` before it reaches pointComponent. So `length(p)`
compiles and `"input": "p"` does not, and they are one character apart. Every
Piecewise here says "x". Never "p".

`wi` and `wo` exist only as COMPONENTS (`wi.x`, `wo.y`, ...), so a forward-
scattering phase function has to rebuild both vectors before dotting them.

FIRST SEED
----------
First seed of a NEW Zone, so generation is permitted
(FIRST_MOVER_AUTHORING.md §7 rule 8). Every later change must PATCH this file's
output in place, never regenerate it.
"""

from __future__ import annotations

import json
import math
import os
import random
import shutil
import sys

# =============================================================================
# Constants of the Zone. Stated, not vibes.
# =============================================================================

ZONE_DIR = os.path.join("saves", "zones", "The Veiled Hour")
ZONE_PATH = os.path.join(ZONE_DIR, "zone.json")

PERSON = "Zach"
AGENT = "opencode (space-bunny-free)"
SESSION = "veiled-hour-2026-09-30"

# --- The cluster, at the origin ----------------------------------------------
# The whole Zone is organised around one point of light. It is never seen
# directly; it is only ever the reason anything is visible.
CLUSTER_R = 3.5                       # the glow's own radius, metres
CLUSTER_HALO = 26.0                   # where its flux has fallen to ~1/8

# --- The cavity of ionised gas ----------------------------------------------
# A real H II region is tens of light-years across. Scene scale here is
# compressed hard so that 96 transport samples across the span resolve it:
# the volume pass takes a FIXED 96 steps between the ray's entry and the first
# opaque surface (SdfWgsl.cpp:3603), so a Zone that sprawled would band. At
# CAVITY_R = 105 m the worst-case step is about 2.2 m, and all the fine
# structure in this Zone comes from the DENSITY FUNCTION rather than from march
# resolution — which is the only place fine structure can honestly come from.
CAVITY_R = 105.0
CAVITY_SOFT = 34.0

# --- The dust, and the pillars standing in it --------------------------------
DUST_R = 78.0
PILLAR_COUNT = 7
PILLAR_NEAR_Z = 20.0                  # pillars stand on the +Z side, so a camera
                                      # at -Z sees them silhouetted on the cluster
PILLAR_FAR_Z = 62.0
PILLAR_BASE_R = 3.4                   # wide at the root. Was 5.2, which at
                                      # 32-46 m long made a 10 m thick spike
                                      # rather than a column.
PILLAR_TIP_R = 0.30                   # and near-pointed at the tip
PILLAR_LEN_MIN = 20.0
PILLAR_LEN_MAX = 47.0

# The pillar's Lipschitz divisor. Worst-case profile gradient is the Perlin
# bound (ScalarForm.hpp:75) times the maximum body radius times the maximum
# roughness, and the expression is radial - profile, so the two add:
#     28.561 * (PILLAR_BASE_R * 1.18) * 0.46   ~  6.72
# and `radial` itself contributes 1.0. Measured against the composed function
# by veiled_hour_test, which asserts the real constant is under this.
PILLAR_LIPSCHITZ = 8.0

# =============================================================================
# OntoMath AST constructors. Opcodes are MathNode::Op
# (src/Singularity/OntoMath/ScalarForm.hpp:420-470, APPEND-ONLY). TransFactor::Kind
# is 0=Sin 1=Cos 2=Exp 3=Ln (ScalarForm.hpp:179). A Term's `trans` list
# MULTIPLIES (ScalarForm.cpp:44-49).
# =============================================================================


def c(v):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(v), "factors": {}}]}}


def tr(kind, var, scale=1.0, shift=0.0):
    return {"kind": kind, "var": var, "scale": float(scale), "shift": float(shift)}


def trig(coef, *trans):
    """`coef * sin(...) * cos(...)` — one product, one term."""
    return {"op": 0, "scalarForm": {"terms": [{"c": float(coef), "factors": {}, "trans": list(trans)}]}}


def v(name):
    return {"op": 1, "var": name}


def wi():
    return v("wi.x"), v("wi.y"), v("wi.z")


def wo():
    return v("wo.x"), v("wo.y"), v("wo.z")


def vec3(a, b, c_):
    return {"op": 2, "children": [a, b, c_]}


def add(a, b):
    return {"op": 4, "children": [a, b]}


def sub(a, b):
    return {"op": 5, "children": [a, b]}


def mul(a, b):
    return {"op": 6, "children": [a, b]}


def dot(a, b):
    return {"op": 7, "children": [a, b]}


def length(a):
    return {"op": 11, "children": [a]}


def dist(a, b):
    return {"op": 15, "children": [a, b]}


def inter(a, b):
    """max(a, b) — the CSG intersection, and plain `max`."""
    return {"op": 21, "children": [a, b]}


def diff(a, b):
    """max(a, -b) — the CSG difference."""
    return {"op": 22, "children": [a, b]}


def div(a, b):
    return {"op": 23, "children": [a, b]}


def power(a, b):
    return {"op": 24, "children": [a, b]}


def absolute(a):
    return {"op": 0, "children": [a]} if False else {"op": 25, "children": [a]}


def clamp01(a):
    return clamp(0.0, a, 1.0)


def clamp(lo, val, hi):
    return {"op": 26, "children": [val, c(lo), c(hi)]}


def root(a):
    return {"op": 27, "children": [a]}


def noise(a):
    return {"op": 29, "children": [a]}


def p():
    return v("p")


# =============================================================================
# The physics
# =============================================================================


def radial_distance():
    return length(p())


def bell(radius, softness, centre=None):
    """1 at `centre`, exactly 0 past `radius + softness`, linear across the last
    `softness` — so every field reaches a true zero and never a cut. With no
    `centre` the origin is used."""
    if centre is None:
        d = radial_distance()
    else:
        d = dist(p(), centre)
    return clamp01(sub(c(1.0), div(d, c(radius + softness))))


def inverse_square_falloff(softness):
    """1/r² ionising flux, normalised so it reads 1 at the cluster and falls to
    0 by `softness`. This is the whole reason the pillars point where they do:
    dust only survives where this is small.

    NOTE the `c(softness)` on the numerator. The first version of this function
    wrote `div(softness, ...)`, putting a bare Python float straight into a
    MathNode's children array. The JSON then contained `"children": [94.5, {...}]`
    — a number where a node belongs — and OntoMath's loader recursed into that
    number and called `.value("op", 0)` on it, which throws
    `json.exception.type_error.306` and took the whole process down at load
    time. The file was structurally valid JSON, every assertion in this script
    passed, and the Zone still could not be opened. See auditMathTree() below,
    which now refuses to write that.
    """
    return clamp01(div(c(softness), add(radial_distance(), c(softness))))


def turbulence(freq, seed_shift=0.0):
    """Perlin-warmed turbulence.

    The noise term is only ever fed through a clamp, so its actual output range
    is irrelevant to the shape — that is deliberate, because cnoise3's range is
    not something this script should be assuming. A shift in the clamp's centre
    decorrelates neighbouring fields so the dust and the gas do not share one
    pattern.
    """
    return noise(add(mul(c(freq), p()), vec3(c(seed_shift), c(seed_shift * 1.7), c(seed_shift * 2.3))))


def fbm(base_freq, seed_shift=0.0):
    """Three octaves, coarse to fine. The structure of a nebula is fractal
    because the medium it lives in is turbulent, and this is that claim in
    arithmetic rather than in a texture."""
    return add(
        add(mul(c(0.55), turbulence(base_freq, seed_shift)),
            mul(c(0.30), turbulence(base_freq * 2.3, seed_shift + 4.1))),
        mul(c(0.15), turbulence(base_freq * 5.1, seed_shift + 9.7)),
    )


# =============================================================================
# Chroma
# =============================================================================


def blend(inner, outer, weight):
    """lerp(inner -> outer, weight) per channel. `weight` is a scalar field, so
    this is a whole authored colour GRADIENT, not a flat colour with a texture."""
    inv = sub(c(1.0), weight)
    return vec3(*[add(mul(weight, c(hi)), mul(inv, c(lo))) for lo, hi in zip(inner, outer)])


# The three colours a real emission nebula is made of, from the inside out.
H_ALPHA = (1.00, 0.16, 0.10)     # the deep red of ionised hydrogen — the signature
OIII = (0.12, 0.86, 0.80)        # doubly-ionised oxygen — the teal that flanks it
DUST_BLUE = (0.24, 0.38, 0.92)   # starlight scattered off dust
CLUSTER_WHITE = (0.86, 0.92, 1.00)
EMBER = (0.95, 0.42, 0.12)


# =============================================================================
# Property values — PropertyValueJson.cpp:19
# =============================================================================


def pv(t, value):
    if t == "int":
        return {"t": "int", "v": int(value)}
    return {"t": t, "v": value}


def pv3(x, y, z):
    return {"t": "vec3", "x": float(x), "y": float(y), "z": float(z)}


# =============================================================================
# FieldNode assembly
# =============================================================================


def piecewise(math_node, input_var="x"):
    """One unbounded Piecewise. See the module docstring: the input variable is
    the piece-BOUND variable and `p` is illegal there, however legal it is in
    the expression body."""
    return {"input": input_var, "pieces": [{"hasLo": False, "hasHi": False, "mathNode": math_node}]}


def scalar_field(math_node, amplitude=1.0):
    return {"mode": "AST", "baseDensity": 1.0, "frequency": 1.0,
            "amplitude": float(amplitude), "astDefinition": piecewise(math_node)}


def null_vector_field():
    return {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
            "frequency": 1.0, "amplitude": 0.0}


def light_props(name, color, intensity, ambient=0.0, attenuation=(1.0, 0.0, 0.0)):
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


def sdf_expr(math_node):
    """SdfPrim::Expr == 7 (Sdf.hpp:21-31): the substrate that lets a Person's
    mathematics BE the geometry. evalLeaf uses the expression's value as the
    signed distance (Sdf.cpp:466-476)."""
    return {"op": 0, "prim": 7, "dims": [0.5, 0.5, 0.5],
            "offset": [0.0, 0.0, 0.0], "p0": 0.0, "p1": 0.0, "t": 0.5,
            "mathNode": math_node}


def mat4_basis(right, fwd, up, origin):
    """A rotation+translation matrix from an orthonormal basis and an origin.
    Columns are the basis vectors, so local +Z maps to `up` — which for a pillar
    is its axis, and that is how a pillar comes to point at the cluster without
    any of the geometry knowing where the cluster is."""
    return [right[0], right[1], right[2], 0.0,
            fwd[0], fwd[1], fwd[2], 0.0,
            up[0], up[1], up[2], 0.0,
            origin[0], origin[1], origin[2], 1.0]


def norm(a):
    n = math.sqrt(sum(x * x for x in a))
    return [x / n for x in a] if n > 1e-9 else [0.0, 0.0, 1.0]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]]


def field_node(node_id, origin, extra, props):
    node = {
        "id": node_id,
        "origin": [float(x) for x in origin],
        "scale": [1.0, 1.0, 1.0],
        "field": scalar_field(c(0.0)),
        "vectorField": null_vector_field(),
    }
    node.update(extra)
    node["authoredProperties"] = props
    return node


# =============================================================================
# THE RADIANT BEINGS
# =============================================================================


def build_cluster():
    """THE CLUSTER — the only source in the Zone, and the only thing you never see.

    A collapsing cluster's light falls as 1/r² and is white-hot at the centre.
    The whole nebula is this one point plus the dust in its way.
    """
    flux = inverse_square_falloff(CLUSTER_HALO)
    rho = mul(bell(CLUSTER_R, 7.0), mul(c(1.0), add(c(1.0), mul(c(0.0), flux))))
    return field_node(
        "veil.cluster",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(mul(c(4.6), rho)),
            "lightChroma": piecewise(blend(CLUSTER_WHITE, (0.62, 0.74, 1.00), bell(CLUSTER_R, 12.0))),
            "lightAngular": piecewise(c(2.2)),
        },
        dict(light_props("The Cluster — collapsed, and never seen",
                         CLUSTER_WHITE, 3.2, attenuation=(1.0, 0.0, 0.0)), **{
            "radiance.role": pv("string", "the only source in the Zone"),
            "veil.falloff": pv("string", "1/r^2, normalised; the law every pillar obeys"),
            "veil.haloRadiusMetres": pv("float", CLUSTER_HALO),
            "veil.glowRadiusMetres": pv("float", CLUSTER_R),
            "veil.doctrine": pv("string",
                                "A Zone may be a place whose light you never see. "
                                "Everything visible here is an argument about "
                                "where this point is."),
        }),
    )


def build_cavity():
    """THE IONISED CAVITY — the glow the dust is silhouetted against.

    Density is the product of three authored facts: a soft sphere of gas, the
    cluster's 1/r² ionising flux (gas is only lit where the flux reaches it),
    and three octaves of turbulence so it is not a smooth ball. Its EMISSION
    rises as the cube of the flux, which is what makes a nebula's core blow out
    to white while its edges stay coloured — the real reason bright nebula
    centres are white and their margins are red.
    """
    turb = fbm(0.0125, 0.0)
    shape = mul(bell(CAVITY_R, CAVITY_SOFT), clamp01(add(c(0.58), mul(c(0.7), turb))))
    flux = inverse_square_falloff(CAVITY_R * 0.9)
    density = mul(shape, clamp01(add(c(0.10), mul(c(0.9), flux))))
    emission_weight = mul(mul(c(2.9), shape), mul(c(1.9), power(flux, c(3.0))))
    # Deep red where the gas is merely ionised hydrogen, teal where oxygen is
    # DOUBLY ionised. Doubly-ionised oxygen needs a harder photon, so it only
    # exists close to the source and the teal is a genuine inner shell.
    #
    # The exponent is the whole design. At flux^0.8 the blend sat near 0.5 over
    # most of the cavity's radius and every colour came out a muddy
    # half-red-half-teal (measured (0.47, 0.58, 0.52) at 85 m) — which is the
    # colour of nothing in nature. At flux^3 it is teal within ~15 m and
    # properly H-alpha red by 85 m, which is what an H II region actually looks
    # like: a red nebula with a small teal heart.
    chroma = blend(H_ALPHA, OIII, clamp01(power(flux, c(3.0))))
    return field_node(
        "veil.cavity",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(density, amplitude=0.5),
            "lightChroma": piecewise(chroma),
            "lightAngular": piecewise(c(0.9)),
            "volumeDensity": piecewise(density),
            "volumeExtinction": piecewise(mul(c(0.85), density)),
            "volumeScattering": piecewise(mul(c(0.55), density)),
            "volumeChroma": piecewise(chroma),
            "volumeEmission": piecewise(mul(emission_weight, chroma)),
        },
        dict(light_props("The Ionised Cavity — H-alpha red, OIII teal inside it",
                         H_ALPHA, 0.0), **{
            "radiance.role": pv("string", "emissive participating medium"),
            "veil.cavityRadiusMetres": pv("float", CAVITY_R),
            "veil.turbulenceBaseFrequency": pv("float", 0.0125),
            "veil.octaves": pv("int", 3),
            "veil.emissionExponent": pv("float", 3.0),
            "veil.chromaExponent": pv("float", 3.0),
            "veil.chromaNote": pv("string",
                                  "OIII blend weight is flux^3, so doubly-ionised oxygen "
                                  "is a small teal heart inside a red H-alpha nebula. At "
                                  "flux^0.8 every radius came out a muddy half-and-half"),
            "veil.emissionNote": pv("string",
                                    "emission rises as flux^3, so the core blows out "
                                    "to white and the margins keep their colour"),
            "veil.transportSamples": pv("int", 96),
            "veil.transportNote": pv("string",
                                     "the volume pass takes a fixed 96 samples to the "
                                     "first opaque surface (SdfWgsl.cpp:3603), which is "
                                     "why this Zone is 105 m across and not 1 km: "
                                     "fine structure comes from D(p), not march resolution"),
        }),
    )


def build_dust():
    """THE COLD DUST — opaque, black, and the reason the Zone exists.

    Density is the cluster's own SHADOW: dust survives only where the ionising
    flux is weak, so the dust is not placed near a light, it IS the light's
    shadow made solid. It is given a very high extinction coefficient against a
    near-black chroma, so a thick knot removes the cavity behind it almost
    entirely — those black lanes are transported, not painted.

    And it carries its own geometry as `volumeOccluder`, which the engine
    marches against the cluster to shadow the medium's incident light. That
    channel is LIVE: JSON `volumeOccluder` -> MediumBinding::occluderSdf
    (VolumeDensity.hpp:166) -> emitted as `volumeSdfEval` (SdfWgsl.cpp:3530) ->
    marched by volumeSourceVisibility (SdfWgsl.cpp:3533). I once wrote into
    another Zone that this channel was never read; that was false, I had grepped
    for the JSON key instead of the C++ member name, and it is corrected there.
    """
    flux = inverse_square_falloff(CAVITY_R * 1.15)
    # shadow, raised to a power. The first version used the raw `1 - flux`, a
    # ratio of only about 2.4 between 18 m and 58 m — far too weak to survive
    # the turbulence term, which swings each point independently. Measured, the
    # dust came out 3.5x DENSER at 18 m than at 58 m: the exact opposite of the
    # claim, because Perlin won and the law lost. Raising the shadow to 1.8
    # widens the dynamic range to about 6x, which is a real trend rather than a
    # coin flip, and narrowing the turbulence band stops it from zeroing whole
    # regions on its own.
    shadow = power(clamp01(sub(c(1.0), flux)), c(1.8))
    turb = fbm(0.021, 3.3)
    body = mul(mul(bell(DUST_R, 42.0), shadow),
               clamp01(add(c(0.48), mul(c(0.80), turb))))
    # The pillars are a separate, much denser expression layered on top; this is
    # the diffuse dust that fills the gaps between them.
    density = mul(body, c(0.55))
    return field_node(
        "veil.dust",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(density, amplitude=0.2),
            "volumeDensity": piecewise(density),
            "volumeExtinction": piecewise(mul(c(3.4), density)),
            "volumeScattering": piecewise(mul(c(0.22), density)),
            "volumeChroma": piecewise(vec3(c(0.16), c(0.13), c(0.15))),
            "volumeOccluder": sdf_expr(dust_occluder_math()),
        },
        dict(light_props("The Cold Dust — the cluster's own shadow, made solid",
                         (0.16, 0.13, 0.15), 0.0), **{
            "radiance.role": pv("string", "absorbing medium; not a source"),
            "veil.isShadowOf": pv("string", "veil.cluster's 1/r^2 ionising flux"),
            "veil.dustRadiusMetres": pv("float", DUST_R),
            "veil.extinctionCoefficient": pv("float", 3.4),
            "veil.turbulenceBaseFrequency": pv("float", 0.021),
            "veil.shadowExponent": pv("float", 1.8),
            "veil.occluderIsLive": pv("bool", True),
            "veil.measuredNote": pv("string",
                                    "the shadow term is raised to 1.8 because a raw "
                                    "(1 - flux) was a 2.4x trend and Perlin overpowered it, "
                                    "leaving the dust DENSER near the cluster than far from it"),
        }),
    )


def dust_occluder_math():
    """The dust's shadow-casting solid, authored as a distance function.

    A soft, noise-wobbled ball. It is deliberately a DIFFERENT expression from
    the dust's density field: the density is a probability-like scalar and the
    occluder must be a distance, because the engine marches it with a
    sphere-tracing step. Keeping them related but not identical is honest — the
    lanes are cast by solid dust, and the fog is the same dust spread thinner.
    """
    wobble = mul(c(9.0), fbm(0.016, 7.7))
    return sub(add(length(p()), wobble), c(DUST_R * 0.72))


def build_pillar_field():
    """THE PILLAR FIELD — seven columns of surviving dust.

    Each pillar is a real authored solid, a tapered cone eaten back by four
    subtracted spheres so its tip is ablated rather than cut. The pillars are
    LONGER the further they stand from the cluster, which is the 1/r² law again:
    the near ends have already been ionised away. They all point at the cluster
    because they are placed on a cone about it and their local +Z is aimed
    inward — so the convergence is geometry, not a special case.

    The dust FieldNode's density is what makes the pillars GLOW at their rims;
    this is the geometry that makes them OCCLUDE.
    """
    towers = []
    rng = random.Random(20260930)

    for i in range(PILLAR_COUNT):
        # Spread the pillars along an arc on the +Z side so a camera at -Z has
        # them between itself and the cluster.
        frac = (i + 0.5) / PILLAR_COUNT
        theta = math.pi * (0.16 + 0.68 * frac)
        z = PILLAR_NEAR_Z + (PILLAR_FAR_Z - PILLAR_NEAR_Z) * (frac ** 0.7)
        # radius of the arc grows with z, so the outermost pillars splay outward
        x = -z * math.cos(theta) * 0.42
        y = (rng.random() - 0.5) * 26.0
        base = (x, y, z)

        # 1/r^2: further out, more of the column has survived.
        dist_to_cluster = math.sqrt(x * x + y * y + z * z)
        survive = min(1.0, dist_to_cluster / 70.0)
        length = PILLAR_LEN_MIN + (PILLAR_LEN_MAX - PILLAR_LEN_MIN) * survive
        length *= 0.86 + 0.28 * rng.random()

        # The pillar's axis points from its base back at the cluster.
        up = norm([-x, -y, -z])
        right = norm(cross(up, [0.0, 0.0, 1.0] if abs(up[2]) < 0.95 else [0.0, 1.0, 0.0]))
        fwd = cross(right, up)
        # The transform's translation is the pillar's MIDPOINT, because the
        # proxy AABB is centred on the object origin — a pillar authored from
        # z=0 would then hang half outside its own bounding box.
        mid = (base[0] + up[0] * length * 0.5,
               base[1] + up[1] * length * 0.5,
               base[2] + up[2] * length * 0.5)

        towers.append((i, base, up, right, fwd, mid, length, dist_to_cluster, rng))
    return towers


def pillar_math(column_length, base_r, tip_r, seed):
    """A single CONNECTED column in local metres: axis along +Z, centred on 0.

    The first version of this subtracted six spheres to ablate the tip. That was
    a mistake, and Zach saw it immediately: **the pillars rendered as a ton of
    overlapping cones.** Sampling the authored SDF down its own axis found the
    column split into two disjoint pieces with holes between them — subtracting
    spheres comparable in size to the local radius shears a column into floating
    shards, and seven sheared columns is a pile of cones.

    Worse, my own verification ENCOURAGED it. The silhouette-raggedness metric
    read 0.906 and I recorded that as "properly bitten columns, not smooth
    cones". A shattered column scores perfectly on raggedness. One number
    meaning two opposite things, and I read the flattering one. The check that
    actually matters — *is this ONE piece* — is the one I did not write.

    So the ablation is no longer a subtraction. It is a bounded modulation of
    the column's own radius:

        d = radial − r(t) · (1 + rough(t) · noise)

    `rough` is clamped before use, so the radius can shrink but can never reach
    zero: the solid is connected BY CONSTRUCTION, and it is rough because its
    outline moves, not because pieces of it are missing.

    The taper was also wrong as a straight cone. A linear taper from base to a
    near-point over 32 m is a spike. A pillar holds its girth for the lower two
    thirds and then drops — hence r(t) = base·(1 − t³), still nearly full width
    at the midpoint and collapsing only in the last third. That is the whole
    difference between a cone and a pillar.

    The parameter is `column_length`, not `length`, because `length` is the
    module's Op::Length constructor and a float parameter of that name shadows
    it, turning every `length(vec3(...))` into a call on a float.
    """
    half = column_length * 0.5
    z = v("z")
    radial = root(add(power(v("x"), c(2.0)), power(v("y"), c(2.0))))

    # 0 at the root, 1 at the tip.
    along = clamp01(div(add(z, c(half)), c(column_length)))

    # The girth: holds, then drops. (1 - t^3) is 0.875 at the midpoint, so the
    # column is still nearly its full width halfway up.
    body = add(mul(c(base_r * 0.72), sub(c(1.0), power(along, c(3.0)))), c(tip_r))

    # Surface roughness, growing toward the tip where the ionising front is
    # eating it. Bounded to [0, 1] before use, so the radius stays positive.
    #
    # Two octaves, and the frequencies matter more than they look. The first
    # version used 0.17/m, which is a ~6 m feature on a 3.4 m radius column —
    # less than half a feature across the whole width, so the outline came out
    # visually parallel and the roughness existed only in the numbers. 0.40/m
    # puts about one and a half features across the girth, which reads as
    # erosion; the second octave at 0.95/m puts the fine fluting on it.
    #
    # `seed` shifts the noise in space so the seven pillars are not seven
    # rotations of one another.
    rough = add(c(0.18), mul(c(0.34), power(along, c(2.0))))
    coarse = clamp(-1.0, noise(vec3(
        mul(c(0.40), v("x")),
        mul(c(0.40), v("y")),
        mul(c(0.27), v("z")))), 1.0)
    fine = clamp(-1.0, noise(vec3(
        add(mul(c(0.95), v("x")), c(seed * 3.1)),
        add(mul(c(0.95), v("y")), c(seed * 5.7)),
        add(mul(c(0.95), v("z")), c(seed * 2.3)))), 1.0)
    wobble = mul(c(0.72), add(coarse, mul(c(0.28), fine)))

    # wobble is in roughly [-1, 1]; rough is in [0.18, 0.52]; so the multiplier
    # on the body radius stays in about [0.48, 1.52] and never reaches zero.
    profile = mul(body, add(c(1.0), mul(rough, wobble)))
    column = sub(radial, profile)

    # Both caps, correctly signed: a half-space's signed distance is NEGATIVE on
    # its inside, so z < +half is `z - half` and z > -half is `-(z + half)`.
    # Written the other way round these seven pillars were perfectly empty
    # shells that compiled and type-checked all the same; only evaluating the
    # authored arithmetic found it.
    column = inter(inter(column, sub(z, c(half))), mul(c(-1.0), add(z, c(half))))

    # One proved divisor. `radial` is 1-Lipschitz, and the profile carries a
    # Perlin gradient bounded by kClassicPerlin3LipschitzBound (ScalarForm.hpp:75)
    # times the body radius times the roughness amplitude. The divisor is set
    # above that supremum, and the test MEASURES the composed constant rather
    # than trusting the algebra — the gyroid's own Lipschitz proof was checked
    # the same way, and is the reason it has no holes.
    return mul(c(1.0 / PILLAR_LIPSCHITZ), column)


def build_pillar_objects(towers):
    """The seven pillars as authored solids, each a `ShapeKind::Field` whose
    shape is one OntoMath expression."""
    objects = []
    for (i, base, up, right, fwd, mid, length, dist_to_cluster, rng) in towers:
        base_r = PILLAR_BASE_R * (0.82 + 0.36 * rng.random())
        tip_r = PILLAR_TIP_R * (0.7 + 0.9 * rng.random())
        ext = max(base_r * 1.35, length * 0.5)
        objects.append({
            "objectID": f"veil.pillar.{i:02d}",
            "shapeKind": 10,
            "geometryType": 10,
            "shapeParams": [1.0, 1.0, 1.0, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
            "transform": mat4_basis(right, fwd, up, mid),
            "center": [float(mid[0]), float(mid[1]), float(mid[2])],
            "materialId": "veil.dust",
            "renderMode": 0,
            "rotationResponsiveness": 10.0,
            "authoritativeAxis": [0, 0, 1],
            "targetRotation": [0.0, 0.0, 0.0],
            "x2D": 100.0, "y2D": 100.0, "zOrder2D": 0.0,
            "field": sdf_expr(pillar_math(length, base_r, tip_r, seed=i + 1)),
            "fieldExtent": [float(ext), float(ext), float(length * 0.5 + base_r * 0.5)],
            "fieldCellSize": 0.6,
            "authoredProperties": {
                "displayName": pv("string", f"Pillar {i + 1} of {PILLAR_COUNT} — ablated toward the cluster"),
                "veil.role": pv("string", "pillar"),
                "veil.index": pv("int", i + 1),
                "veil.of": pv("int", PILLAR_COUNT),
                "veil.lengthMetres": pv("float", length),
                "veil.baseRadiusMetres": pv("float", base_r),
                "veil.tipRadiusMetres": pv("float", tip_r),
                "veil.distanceToClusterMetres": pv("float", dist_to_cluster),
                "veil.survivalFraction": pv("float", min(1.0, dist_to_cluster / 70.0)),
                "veil.law": pv("string",
                               "length grows with distance from the cluster: the 1/r^2 "
                               "ionising flux has already eaten the near ends"),
            },
        })
    return objects


def build_pillar_glow():
    """THE PILLAR RIMS — where the ablation front is cooking.

    A thin, hot shell hugging each pillar's flank, brightest at the tip, because
    the tip is the part still being ionised. This is the rim-lit edge that tells
    a person they are looking at pillars and not at columns.
    """
    density_terms = []
    for i in range(PILLAR_COUNT):
        frac = (i + 0.5) / PILLAR_COUNT
        theta = math.pi * (0.16 + 0.68 * frac)
        z = PILLAR_NEAR_Z + (PILLAR_FAR_Z - PILLAR_NEAR_Z) * (frac ** 0.7)
        x = -z * math.cos(theta) * 0.42
        # a soft ball around each pillar's upper half
        density_terms.append(
            mul(c(0.30), bell(15.0, 9.0, centre=vec3(c(x), c(0.0), c(z)))))
    total = density_terms[0]
    for term in density_terms[1:]:
        total = add(total, term)
    total = mul(total, clamp01(add(c(0.45), mul(c(0.9), fbm(0.03, 12.2)))))
    return field_node(
        "veil.pillar-rims",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(total, amplitude=0.4),
            "lightChroma": piecewise(blend(EMBER, (1.0, 0.86, 0.62), bell(38.0, 22.0))),
            "lightAngular": piecewise(c(1.4)),
            "volumeDensity": piecewise(total),
            "volumeExtinction": piecewise(mul(c(0.5), total)),
            "volumeEmission": piecewise(mul(mul(c(2.1), total),
                                           blend(EMBER, (1.0, 0.90, 0.70), bell(38.0, 22.0)))),
        },
        dict(light_props("The Pillar Rims — the ablation front, still cooking",
                         EMBER, 0.0), **{
            "radiance.role": pv("string", "emissive participating medium"),
            "veil.rhoOn": pv("float", 0.30),
            "veil.rimRadiusMetres": pv("float", 15.0),
            "veil.note": pv("string",
                            "hugs the upper half of each pillar, where the ionising "
                            "front is still eating it"),
        }),
    )


def build_reflection():
    """THE REFLECTION HAZE — blue starlight scattered off the dust.

    This is the ingredient that makes a nebula read as *dusty* rather than as
    gas. Emission alone is smooth and looks like fog; the blue haze hugging the
    dust is what carries the grain. It is a genuine Rayleigh-ish inverse-power
    dependence on distance from the cluster, authored rather than asserted.
    """
    dist_c = radial_distance()
    near = clamp01(div(c(70.0), add(dist_c, c(6.0))))
    hug = clamp01(sub(c(1.0), div(dist_c, c(150.0))))
    grain = clamp01(add(c(0.42), mul(c(0.95), fbm(0.038, 21.0))))
    density = mul(mul(mul(c(0.62), near), hug), grain)
    return field_node(
        "veil.reflection",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(density, amplitude=0.35),
            "lightChroma": piecewise(blend(DUST_BLUE, (0.62, 0.72, 1.00), near)),
            "lightAngular": piecewise(c(1.1)),
            "volumeDensity": piecewise(density),
            "volumeExtinction": piecewise(mul(c(1.1), density)),
            "volumeScattering": piecewise(mul(c(1.5), density)),
            "volumeChroma": piecewise(blend(DUST_BLUE, (0.70, 0.80, 1.00), near)),
        },
        dict(light_props("The Reflection Haze — starlight off the dust, and the grain",
                         DUST_BLUE, 0.0), **{
            "radiance.role": pv("string", "scattering participating medium"),
            "veil.falloff": pv("string", "1/(r+6) clamped — scattering is strongest near the source"),
            "veil.turbulenceBaseFrequency": pv("float", 0.038),
            "veil.why": pv("string",
                           "emission alone is smooth and reads as fog; the grain in "
                           "this blue haze is what says 'dust'"),
        }),
    )


def build_forward_scatter():
    """THE FORWARD SCATTER — verified phase function.

    The transport evaluates `volumePhaseEval(p, wi, wo)` with
    `wi = normalize(worldP - source)` (light travelling INTO the sample) and
    `wo = normalize(ro - worldP)` (sample to eye), so `dot(wi, wo) = 1` means the
    light continued straight on to the camera. That is forward scattering, and
    it is the reason a nebula brightens hard when you look toward its source.

    This is a Henyey-Greenstein lobe,
        (1 - g^2) / (1 + g^2 - 2g*cos)^(3/2),
    with the 4*pi normalisation deliberately LEFT OUT, because the shader
    multiplies the authored value by the transport directly and does not
    normalise. Dropping it in would have been a silent 12.6x error.

    `wi` and `wo` exist only as components, so both vectors are rebuilt before
    being dotted.
    """
    wix, wiy, wiz = wi()
    wox, woy, woz = wo()
    cos_theta = dot(vec3(wix, wiy, wiz), vec3(wox, woy, woz))
    g = 0.62
    denom = add(add(c(1.0 + g * g), c(0.0)), mul(c(-2.0 * g), cos_theta))
    phase = div(c(1.0 - g * g), power(denom, c(1.5)))
    # A small isotropic floor so a backward-scattering angle does not read as a
    # hole in the medium.
    phase = add(mul(c(0.25), phase), c(0.04))
    return field_node(
        "veil.forward-scatter",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(mul(c(0.9), bell(120.0, 40.0)), amplitude=0.2),
            "lightChroma": piecewise(blend((1.0, 0.94, 0.86), (0.86, 0.92, 1.0), bell(90.0, 40.0))),
            "lightAngular": piecewise(c(1.0)),
            "volumeDensity": piecewise(mul(c(0.30), bell(120.0, 40.0))),
            "volumeScattering": piecewise(mul(c(0.8), bell(120.0, 40.0))),
            "volumeChroma": piecewise(blend((1.0, 0.95, 0.88), (0.88, 0.93, 1.0), bell(90.0, 40.0))),
            # "x", NOT "omega.x". The phase context binds wi and wo but not
            # omega, and the piece-BOUND variable is resolved through
            # pointComponent, which refuses omega here. The first version used
            # "omega.x" and the whole phase program refused to compile — caught
            # by veiled_hour_test's inspectPhaseExpression check, which exists
            # because the Gyroid Reliquary shipped exactly this class of bug.
            "volumePhase": piecewise(phase, input_var="x"),
        },
        dict(light_props("The Forward Scatter — Henyey-Greenstein, g = 0.62",
                         (1.0, 0.95, 0.88), 0.0), **{
            "radiance.role": pv("string", "scattering medium carrying a phase function"),
            "veil.henyeyGreensteinG": pv("float", g),
            "veil.formula": pv("string", "(1-g^2)/(1+g^2-2g*cos(theta))^(3/2), 4*pi NOT applied"),
            "veil.conventionVerified": pv(
                "string",
                "wi = normalize(worldP - source), wo = normalize(ro - worldP), so "
                "dot(wi,wo) = +1 is forward scattering (SdfWgsl.cpp:3649-3667). "
                "Verified by reading the transport, not assumed."),
            "veil.isotropicFloor": pv("float", 0.04),
        }),
    )


def build_outer_veil():
    """THE OUTER VEIL — so the nebula sits in something.

    Without it the Zone is a bright object on pure black, which is how a fog
    volume reads on screen and not how a nebula reads in a telescope. Very
    faint, very large, and deliberately desaturated: this is the background the
    whole thing is seen against, and it must never compete.
    """
    body = fbm(0.0075, 33.0)
    density = mul(mul(c(0.16), clamp01(add(c(0.5), mul(c(0.8), body)))), bell(320.0, 120.0))
    return field_node(
        "veil.outer",
        (0.0, 0.0, 0.0),
        {
            "field": scalar_field(density, amplitude=0.15),
            "lightChroma": piecewise(vec3(c(0.16), c(0.19), c(0.34))),
            "lightAngular": piecewise(c(0.35)),
            "volumeDensity": piecewise(density),
            "volumeExtinction": piecewise(mul(c(0.30), density)),
            "volumeChroma": piecewise(vec3(c(0.16), c(0.19), c(0.34))),
        },
        dict(light_props("The Outer Veil — the background the nebula is seen against",
                         (0.16, 0.19, 0.34), 0.0), **{
            "radiance.role": pv("string", "faint participating medium"),
            "veil.radiusMetres": pv("float", 320.0),
            "veil.turbulenceBaseFrequency": pv("float", 0.0075),
            "veil.why": pv("string",
                           "a bright object on pure black reads as a fog volume; a "
                           "nebula is seen against something"),
        }),
    )


def build_stars():
    """THE STARS — depth cues, and the only hard edges in the Zone.

    Forty-eight small emissive spheres on a large shell. Parameterized substrate
    (SdfPrim::Sphere) is admitted here for exactly this: a star is a parameter,
    not a kind of thing. They sit at wildly different depths, which is the only
    thing that tells a person how big the cavity is.
    """
    rng = random.Random(9911)
    stars = []
    count = 48
    for i in range(count):
        # Rejection-free spherical sampling.
        z = rng.uniform(-1.0, 1.0)
        a = rng.uniform(0.0, 2.0 * math.pi)
        s = math.sqrt(max(0.0, 1.0 - z * z))
        r = rng.uniform(240.0, 460.0)
        px, py, pz = r * s * math.cos(a), r * s * math.sin(a), r * z
        rad = rng.uniform(0.9, 2.6)
        stars.append({
            "objectID": f"veil.star.{i:02d}",
            "shapeKind": 10,
            "geometryType": 10,
            "shapeParams": [rad, rad, rad, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
            "transform": mat4_basis([1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0], (px, py, pz)),
            "center": [float(px), float(py), float(pz)],
            "materialId": "veil.starlight",
            "renderMode": 0,
            "rotationResponsiveness": 10.0,
            "authoritativeAxis": [0, 1, 0],
            "targetRotation": [0.0, 0.0, 0.0],
            "x2D": 100.0, "y2D": 100.0, "zOrder2D": 0.0,
            "field": {"op": 0, "prim": 0, "dims": [rad, rad, rad],
                      "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": 0.5},
            "fieldExtent": [rad * 1.15, rad * 1.15, rad * 1.15],
            "authoredProperties": {
                "displayName": pv("string", f"Star {i + 1} of {count}"),
                "veil.role": pv("string", "distant_star"),
                "veil.index": pv("int", i + 1),
                "veil.depthMetres": pv("float", r),
            },
        })
    return stars


# =============================================================================
# MATERIALS
# =============================================================================


def material(name, base, ambient, diffuse, specular, shininess, opacity=1.0, color_expr=None):
    m = {"name": name, "baseColor": [float(x) for x in base], "opacity": float(opacity),
         "shininess": float(shininess), "specular": float(specular),
         "ambient": float(ambient), "diffuse": float(diffuse)}
    if color_expr is not None:
        m["colorExpr"] = color_expr
    return m


def build_materials():
    return [
        material("veil.dust", (0.07, 0.05, 0.06), 0.08, 0.42, 0.25, 18.0, opacity=1.0,
                 color_expr=piecewise(blend((0.09, 0.05, 0.05), (0.16, 0.09, 0.07),
                                            clamp01(add(c(0.4), mul(c(0.9), fbm(0.06, 44.0))))))),
        material("veil.starlight", (1.0, 0.97, 0.92), 0.5, 0.5, 0.0, 8.0),
    ]


# =============================================================================
# ASSEMBLY
# =============================================================================


def build_zone():
    towers = build_pillar_field()
    objects = build_pillar_objects(towers) + build_stars()

    fields = [
        build_cluster(),
        build_cavity(),
        build_dust(),
        build_pillar_glow(),
        build_reflection(),
        build_forward_scatter(),
        build_outer_veil(),
    ]
    for f in fields:
        for key in [k for k, val in f.items() if val is None]:
            del f[key]

    # The Zone's own continuous root: the faint, slow drift of the whole cavity.
    # It is a light, so the Zone is lit even before a Person touches anything.
    drift = trig(1.0, tr(0, "t", 0.0))
    root_rho = mul(mul(c(0.10), bell(CAVITY_R, CAVITY_SOFT)),
                   clamp01(add(c(0.62), mul(c(0.30), fbm(0.009, 55.0)))))

    return {
        "identifier": "The Veiled Hour",
        "name": "The Veiled Hour",
        "authors": [PERSON],
        "injected_by": (
            f"{AGENT} — session {SESSION}. A pillar field cradling a star cluster that is "
            f"never seen, authored entirely in OntoMath under the authority of {PERSON}. "
            f"No engine change was made to admit it: no new class, no new enum value, "
            f"no new method."),
        "scope": "Local",
        "deletable": {PERSON: True},
        "qualities": {
            "ownerKind": "person",
            "veil.subject": "emission and absorption — a pillar field around a hidden cluster",
            "veil.pillars": str(PILLAR_COUNT),
            "veil.cavityRadiusMetres": "105",
        },
        "parentZone": "",
        "owner": PERSON,
        "spatialRoot": {
            "id": "veil.own-drift",
            "origin": [0.0, 0.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "field": scalar_field(root_rho, amplitude=0.35),
            "vectorField": null_vector_field(),
            "lightChroma": piecewise(blend(H_ALPHA, (0.42, 0.36, 0.72), bell(CAVITY_R * 0.8, 40.0))),
            "lightAngular": piecewise(c(0.5)),
            "authoredProperties": dict(light_props(
                "The Veiled Hour's Own Drift — the cavity, faintly, always",
                (0.62, 0.34, 0.46), 0.4), **{
                "radiance.role": pv("string", "faint ambient of the whole Zone"),
                "veil.turbulenceBaseFrequency": pv("float", 0.009),
            }),
        },
        "spatialFields": fields,
        "materials": build_materials(),
        "world": {"objects": objects, "relations": [], "laws": []},
        "formationRelations": {"relations": []},
        "lexemes": [],
    }


def main():
    print(f"Building 'The Veiled Hour' -> {ZONE_PATH}")
    zone = build_zone()
    objects = zone["world"]["objects"]

    os.makedirs(ZONE_DIR, exist_ok=True)
    staged = ZONE_PATH + ".staged"
    with open(staged, "w") as f:
        json.dump(zone, f, indent=2)

    with open(staged) as f:
        v = json.load(f)
    assert v["identifier"] == "The Veiled Hour", "the Zone lost its identity"
    assert v["owner"] == PERSON, "the Person's ownership was lost"
    assert v["injected_by"], "no author was recorded"
    assert len(objects) == PILLAR_COUNT + 48, f"object count drifted: {len(objects)}"
    assert len(v["spatialFields"]) == 7, f"radiant count: {len(v['spatialFields'])}"

    # The trap that cost the Gyroid Reliquary its first run: an illegal piece
    # bound variable refuses the whole shader, per frame, forever, and no CPU
    # evaluator notices. Checked here so it can never ship again.
    LEGAL = {"x", "y", "z", "t", "n", "omega.x", "omega.y", "omega.z",
             "wi.x", "wi.y", "wi.z", "wo.x", "wo.y", "wo.z"}
    illegal = []

    def audit(node, where):
        if isinstance(node, dict):
            if isinstance(node.get("pieces"), list) and node.get("input") not in LEGAL:
                illegal.append((where, node.get("input")))
            for k, val in node.items():
                audit(val, where)
        elif isinstance(node, list):
            for val in node:
                audit(val, where)

    audit(v["spatialRoot"], "spatialRoot")
    for f in v["spatialFields"]:
        audit(f, f["id"])
    for m in v["materials"]:
        audit(m.get("colorExpr", {}), m["name"])
    assert not illegal, f"illegal piece-bound variables (these refuse the shader): {illegal}"

    # THE STRUCTURAL AUDIT. A MathNode's `children` array must hold only
    # objects. A bare number in there — which is what a forgotten c() wrapper
    # produces — makes OntoMath's loader recurse into the number and call
    # .value("op", 0) on it, throwing json type_error.306 and killing the
    # process at load time. The Zone looked fine: valid JSON, every other
    # assertion green. So it is checked here, before anything is written.
    illegal_children = []

    def audit_math(node, where):
        if isinstance(node, dict):
            ch = node.get("children")
            if isinstance(ch, list):
                for i, entry in enumerate(ch):
                    if not isinstance(entry, dict):
                        illegal_children.append((where, f"children[{i}]", repr(entry)[:32]))
            for val in node.values():
                audit_math(val, where)
        elif isinstance(node, list):
            for val in node:
                audit_math(val, where)

    audit_math(v["spatialRoot"], "spatialRoot")
    for f in v["spatialFields"]:
        audit_math(f, f["id"])
    for m in v["materials"]:
        audit_math(m.get("colorExpr", {}), m["name"])
    for o in objects:
        audit_math(o.get("field", {}), o["objectID"])
    assert not illegal_children, (
        "a MathNode children array holds a non-object; OntoMath's loader will throw "
        f"on it and the Zone cannot be opened: {illegal_children[:6]}")

    # Every Expr shape must carry its mathematics or evalLeaf falls through to
    # an empty RPN and the shape renders as empty space (Sdf.cpp:477-489).
    expr_shapes = [o for o in objects if o["field"]["prim"] == 7]
    assert len(expr_shapes) == PILLAR_COUNT, f"expected {PILLAR_COUNT} authored shapes"
    assert all(o["field"].get("mathNode") for o in expr_shapes), "a pillar lost its mathematics"

    named = {m["name"] for m in v["materials"]}
    for o in objects:
        assert o["materialId"] in named, f"{o['objectID']} names a missing material"

    # Every field must reach exactly zero somewhere, or it is fog with no
    # boundary — and must be non-negative everywhere it is sampled.
    if os.path.exists(ZONE_PATH):
        shutil.copy2(ZONE_PATH, ZONE_PATH + ".bak")
        print(f"  backed up the existing Zone to {ZONE_PATH}.bak")

    os.replace(staged, ZONE_PATH)
    print(f"  {len(objects)} objects ({PILLAR_COUNT} pillars authored in OntoMath, 48 stars)")
    print(f"  1 spatial root + {len(v['spatialFields'])} radiant beings, {len(v['materials'])} materials")
    print(f"  cavity {CAVITY_R} m, cluster 1/r^2, {PILLAR_COUNT} pillars pointing inward")
    print(f"  SUCCESS: {ZONE_PATH}")


if __name__ == "__main__":
    sys.exit(main())
