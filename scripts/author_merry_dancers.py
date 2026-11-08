#!/usr/bin/env python3
"""
The Merry Dancers — a new aurora Zone, authored from physics and Zach's photographs
====================================================================================
Zone:         <data home>/zones/The Merry Dancers/zone.json
Commissioned: Zach, 2026-11-03: "MAKE ANOTHER AURORA ZONE FROM SCRATCH ... USE
              VOLUMETRICS AND RADIANCE FIELDS TO MAKE GORGEOUS, REALISTIC STYLE
              AURORA", with seven reference photographs. He also said Northern Veil
              "isn't that realistic/breathtaking-level" and is not the reference.
Authored by:  Claude Code / Claude Opus 5.5, under Zach's authority.
("The Merry Dancers" is the old Scots and Shetland name for the aurora. The name is
this agent's proposal; Zach may rename it.)

WHAT THE PHOTOGRAPHS HAVE THAT ANOTHER COLOURED FOG DOES NOT
------------------------------------------------------------
Read from Zach's seven references, and each answered below by mathematics:

 1. A curtain is a SHEET, thin and folded. It is bright where a sight line runs along
    it (limb brightening). So brightness comes from the transport integral through a
    folded sheet, not from a painted highlight.
 2. Rays follow the magnetic field. Long streaks are nearly independent of altitude,
    tilt together, and fan from a vanishing point purely by perspective.
 3. Colour is altitude, not object. Oxygen 557.7 nm green lives low with a sharp lower
    border, oxygen 630 nm red lives high and diffuse, and nitrogen makes a thin pink
    hem under strong green. One emission function carries the whole ramp.
 4. Cores overexpose toward white-mint, as on a sensor. The display has no tone
    mapper, so each emission colour carries small red and blue terms that only reach
    the eye once green has clipped.
 5. Still water doubles everything. The lake is a plane of symmetry, not a surface:
    sky, ridges and trees are even in y, and the far band has a mirrored medium.

Two kinds of light, on purpose:
  * MARCHED sheets (FieldNode media): real 3D, parallax, motion through time.
  * A RADIANCE FIELD on the sky surface: aurora rays, stars and the far ridges as a
    function of direction, one evaluation per pixel. It has no time coordinate, because
    surface colour binds none, so it is still while the sheets move.

Refusals: no C++ class, no enum value, no new method. Every being here is FieldNode,
Field Object and Material data. Every number is a named constant or authored Property.

FIRST SEED ONLY (FIRST_MOVER_AUTHORING §7 rule 8)
-------------------------------------------------
This script may write the Zone only when no zone file exists at the target. Once
installed, a later change must patch the file on disk, because what a Person saved
outranks this script. `--out` writes elsewhere for offscreen look development.
"""
from __future__ import annotations

import argparse
import json
import math
import os
import sys

ZONE_NAME = "The Merry Dancers"
PERSON = "Zach"
AGENT = "Claude Code / Claude Opus 5.5"

# =============================================================================
# OntoMath constructors. Opcodes are MathNode::Op (ScalarForm.hpp, append-only).
# TransFactor kinds: 0=Sin 1=Cos 2=Exp 3=Ln. Numbers lift to constants on the way in.
# =============================================================================


def c(v):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(v), "factors": {}}]}}


def n(a):
    return c(a) if isinstance(a, (int, float)) else a


def v(name):
    return {"op": 1, "var": name}


def balanced(op, xs):
    """Balanced, not left-deep. MathNode::typeOf used to visit each child twice per non-Let
    level, with the loader running it at every node, so load time was ~2^height. A left-deep
    chain was ruinous when this Zone was authored. That defect is fixed (2026-11-03,
    ontomath_typeof_depth_test), and shallow trees remain cheaper to load regardless."""
    xs = [n(x) for x in xs]
    while len(xs) > 1:
        xs = [{"op": op, "children": [xs[i], xs[i + 1]]} if i + 1 < len(xs) else xs[i]
              for i in range(0, len(xs), 2)]
    return xs[0]


def add(*xs):
    return balanced(4, xs)


def sub(a, b):
    return {"op": 5, "children": [n(a), n(b)]}


def mul(*xs):
    return balanced(6, xs)


def div(a, b):
    return {"op": 23, "children": [n(a), n(b)]}


def abs_(a):
    return {"op": 25, "children": [n(a)]}


def clamp(val, lo=0.0, hi=1.0):
    return {"op": 26, "children": [n(val), c(lo), c(hi)]}


def sqrt_(a):
    return {"op": 27, "children": [n(a)]}


def vec3(a, b, cc):
    return {"op": 2, "children": [n(a), n(b), n(cc)]}


def noise(a, b, cc):
    return {"op": 29, "children": [vec3(a, b, cc)]}


def let(name, value, body):
    return {"op": 41, "var": name, "children": [n(value), n(body)]}


def pow_(a, b):
    return {"op": 24, "children": [n(a), n(b)]}


def min_(a, b):
    """Op 20 is min(a, b): the CSG Union and the scalar minimum are the same formula."""
    return {"op": 20, "children": [n(a), n(b)]}


def component(a, axis):
    return {"op": 3, "arg": axis, "children": [n(a)]}


def floor3(a):
    """Componentwise floor; OntoMath's Map takes a vector."""
    return {"op": 12, "arg": "Floor", "children": [n(a)]}


def sin_(var, scale=1.0, shift=0.0, coef=1.0):
    """coef * sin(scale*var + shift); a TransFactor binds one named variable."""
    return {"op": 0, "scalarForm": {"terms": [{"c": float(coef), "factors": {},
            "trans": [{"kind": 0, "var": var, "scale": float(scale), "shift": float(shift)}]}]}}


def piecewise(node):
    return {"input": "x", "pieces": [{"mathNode": node}]}


def pv(t, value):
    return {"t": t, "v": value}


# =============================================================================
# THE SPECTRUM. Display code values: the surface has no sRGB encode and no tone
# mapper. The small off-axis terms are how a clipped core whitens (note 4).
# =============================================================================
OXYGEN_GREEN = (0.20, 1.00, 0.38)      # 557.7 nm, mint once it clips
NITROGEN_HEM = (1.00, 0.20, 0.62)      # N2 first positive, the pink lower border
OXYGEN_RED = (0.95, 0.10, 0.34)        # 630.0 nm, high and diffuse
NITROGEN_VIOLET = (0.46, 0.14, 1.00)   # N2+ blue-violet, sunlit tops of tall rays


def scaled(rgb, s):
    """weight * constant colour, so each spectral weight is evaluated once rather than
    once per colour channel."""
    return mul(s, vec3(*rgb))


def sum3(*vs):
    return add(*vs)


# =============================================================================
# THE CURTAIN. One folded sheet, one spectrum, one emission integral.
#
#   G(x,z)      z + three literal harmonics of x. The sheet's large path.
#   g           G + small drifting folds. g = 0 IS the sheet.
#   S(g)        bright thin core inside a faint glow, both compact-support.
#   u           0 at the (waving) lower border, 1 at the ray tops.
#   spectrum    pink hem, oxygen green, oxygen red, nitrogen violet, all from u.
#   rays        Perlin stretched along the field, leaning with altitude.
#
# DENSITY IS THE GATE. volumeDensity is only literal clamped factors around the sheet's
# path, cheap enough to evaluate at every sample and exactly zero off the envelope, so
# the zero-proof grid can skip empty space (about 84% of cells at 2560x1440, measured).
# The sheet itself is drawn by emission. `wz` must exceed the sheet half-width plus the
# drifting folds, or the sheet would be clipped by its own gate.
#
# Those factors are also the shape OntoMath::clampedWaveSupport reads. Routed transport
# needs exactly four media, so this three-media Zone stays on the Current lane on purpose:
# with four media, routed cost ~50% MORE here than Current at 2560x1440 (measured
# 2026-11-03; a finding for the ray-transport lane, not a verdict on it).
# =============================================================================


def wave_terms(k):
    return [sin_("x", k["k0"], k["p0"], k["a0"]),
            {"op": 0, "scalarForm": {"terms": [{"c": float(k["a1"]), "factors": {},
             "trans": [{"kind": 1, "var": "x", "scale": float(k["k1"]), "shift": float(k["p1"])}]}]}},
            sin_("x", k["k2"], k["p2"], k["a2"])]


def support_factors(k, half):
    """The literal factors, in exactly the shape the recipe matcher reads."""
    wx, wy, wz = float(half[0]), float(half[1]), float(k["wz"])
    a = float(k["support_rise"])
    y_shift = add(v("y"), wy)
    x_f = clamp(sub(1.0, div(abs_(v("x")), wx)))
    rise = clamp(mul(a, y_shift))
    fall = clamp(sub(1.0, div(add(v("y"), wy), 2.0 * wy)))
    w = wave_terms(k)
    z_f = clamp(sub(1.0, div(abs_(add(add(add(v("z"), w[0]), w[1]), w[2])), wz)))
    return [x_f, rise, fall, z_f]


def bind_sheet(k, body):
    """Bind Y (mirrored for the reflection) and xw, the ray coordinate. Lets are inlined
    as text by the emitter, so anything bound here is re-evaluated at every use. Keep
    bindings cheap and use pow() rather than a repeated Let (cost budget: ~25 trig and
    2 Perlin per sample, close to the 15 + 1 that earns Northern Veil its frame rate)."""
    Y = mul(-1.0, v("y")) if k.get("mirror") else v("y")
    return let("Y", Y, let("xw", add(v("x"), mul(k["tilt_x"], v("Y"))), body))


def sheet_mask(k):
    """S(g) = (1 - (g/w)^2)^2 with compact support; g = 0 is the sheet."""
    t = v("t")
    w = wave_terms(k)
    terms = [v("z"), w[0], w[1], w[2], sin_("fa", coef=k["af1"]), sin_("fb", coef=k["af2"])]
    body = add(*terms)
    if k.get("af3"):
        # The curl that reads from directly below, where large folds alone look like a tube.
        body = let("fc", add(mul(k["kf3"], v("x")), k["pf3"], mul(k["wf3"], t)),
                   add(*terms, sin_("fc", coef=k["af3"])))
    g = let("fa", add(mul(k["kf1"], v("x")), k["pf1"], mul(k["wf1"], t)),
        let("fb", add(mul(k["kf2"], v("x")), k["pf2"], mul(k["wf2"], t)), body))
    width = add(v("we") if k.get("w_max") else k["w"], sin_("x", k["kw"], k["pw"], k["w"] * 0.25))
    return pow_(clamp(sub(1.0, pow_(div(g, width), 2.0))), 2.0)


def altitude(k):
    """u: 0 at the lower border, 1 at the ray tops. The border wave travels."""
    return let("pb", add(mul(k["kb1"], v("x")), k["pb1"], mul(k["wb1"], v("t"))),
               div(sub(v("Y"), add(k["y0"] - k["oy"], sin_("pb", coef=k["ab1"]))), k["h"]))


def edge_fade(k):
    return clamp(div(sub(k["hx"], abs_(v("x"))), k["fade"]))


def limb_cap(k):
    """Path length through a sheet grows as 1/cos(view, normal), so a sheet seen exactly
    edge-on piles up into a laser line. Photographs show a brightened limb, not that:
    atmosphere and sensor bloom spread it. So emission carries cos + floor, which keeps
    the bright fold and caps its excess at about 1/floor."""
    grad = add(mul(k["a0"] * k["k0"], {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {},
               "trans": [{"kind": 1, "var": "x", "scale": float(k["k0"]), "shift": float(k["p0"])}]}]}}),
               sin_("x", k["k1"], k["p1"], -k["a1"] * k["k1"]),
               mul(k["a2"] * k["k2"], {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {},
               "trans": [{"kind": 1, "var": "x", "scale": float(k["k2"]), "shift": float(k["p2"])}]}]}}))
    cos_view = div(abs_(add(v("omega.z"), mul(v("Gx"), v("omega.x")))), sqrt_(add(1.0, mul(v("Gx"), v("Gx")))))
    return let("Gx", grad, clamp(add(cos_view, k["limb_floor"])))


def lattice_step(half):
    """The longest marching step a ray of this direction can take. Current transport samples
    chord/96, and the longest chord a direction can have through this box is min(2h/|omega|)
    over the axes. This is worst-case, so it is never too permissive, and a higher sample
    count only makes it conservative."""
    hx, hy, hz = float(half[0]), float(half[1]), float(half[2])
    chord = min_(min_(div(2.0 * hx, add(abs_(v("omega.x")), 0.001)), div(2.0 * hy, add(abs_(v("omega.y")), 0.001))),
                 div(2.0 * hz, add(abs_(v("omega.z")), 0.001)))
    return div(chord, 96.0)


def lattice_advance(k):
    """How far one step moves a ray along the ray coordinate xw."""
    tilt = k["tilt_x"] * (-1.0 if k.get("mirror") else 1.0)
    return mul(abs_(add(v("omega.x"), mul(tilt, v("omega.y")))), v("step"))


def resolved_width(k):
    """The sheet's own band limit. A thin sheet between two samples lands on the lattice
    differently from pixel to pixel, which is the larger part of the moire on a thin sheet.
    So a ray that steps too far across the sheet sees it wider, by exactly what its step
    cannot resolve, and the emission is divided by the same factor so the energy along the
    ray is unchanged. A ray looking up the sheet keeps the full sharpness."""
    across = mul(v("step"), clamp(add(abs_(v("omega.z")), abs_(v("omega.x")))))
    return min_(k["w_max"], sqrt_(add(k["w"] * k["w"], mul(1.44, across, across))))


def resolved(wavenumber):
    """Analytic band limit, the fix for the moire. A sine of wavenumber k along the ray is
    undersampled once k * advance reaches pi, and aliases into stripes that are not in the
    integral: at 768 samples per chord they vanish and nothing else changes (measured
    2026-11-03). The true integral of such a term over a step is near zero, so fading it
    out is the right answer as well as the free one. 1 up to k*advance = pi/2, 0 at pi."""
    return clamp(sub(2.0, mul(2.0 / math.pi * wavenumber, v("adv"))))


def curtain_density(k, half):
    """Only the literal support factors: this is what gates samples, cheaply. The sheet
    itself is drawn by emission, so wz is a tight envelope around the sheet."""
    return mul(*support_factors(k, half))


def curtain_emission(k, half):
    t = v("t")
    # Rays: one Perlin for organic irregularity, plus a comb of incommensurate sines in
    # the ray coordinate for the fine streaks. Sines cost a small fraction of a Perlin,
    # and four of them at irrational ratios do not read as periodic. They slide along
    # the sheet with time, which is what a racing ray is.
    harmonics = [("c1", "ca1", 1.0), ("c2", "ca2", 1.7), ("c3", "ca3", 2.3), ("c4", "ca4", 3.1), ("c5", "ca5", 4.3)]
    comb_terms = [mul(resolved(k[f]), sin_("cr", k[f], k["seed"] * ph, k[a]))
                  for f, a, ph in harmonics if k.get(a)]
    comb = let("adv", lattice_advance(k), let("cr", add(v("xw"), mul(k["cv"], t)), add(*comb_terms)))
    rays = clamp(add(k["r0"], comb,
                     mul(k["r1"], noise(mul(v("xw"), k["f1"]), mul(v("Y"), k["fy"]), add(mul(t, k["rt"]), k["seed"])))))
    weight_rays = let("rays", rays, add(k["base"], mul(1.0 - k["base"], pow_(v("rays"), 2.0))))
    section = let("ps", add(mul(k["ks1"], v("x")), k["ps1"], mul(k["ws1"], t)),
                  clamp(add(k["sec0"], sin_("ps", coef=k["as1"]), sin_("x", k["ks2"], k["ps2"], k["as2"]))))
    u = v("u")
    hem = mul(k["hem"], clamp(sub(1.0, mul(abs_(sub(u, 0.02)), 34.0))))
    green = mul(k["green"], clamp(mul(u, k.get("green_rise", 7.0))), pow_(clamp(sub(1.0, div(u, k["green_top"]))), 2.0))
    upper = mul(clamp(mul(sub(u, k["red_from"]), k["red_rise"])), clamp(sub(1.0, u)))
    spectrum = sum3(scaled(NITROGEN_HEM, hem), scaled(OXYGEN_GREEN, green),
                    mul(upper, vec3(k["red"] * OXYGEN_RED[0] + k["violet"] * NITROGEN_VIOLET[0],
                                    k["red"] * OXYGEN_RED[1] + k["violet"] * NITROGEN_VIOLET[1],
                                    k["red"] * OXYGEN_RED[2] + k["violet"] * NITROGEN_VIOLET[2])))
    body = let("u", altitude(k),
           let("sm", sheet_mask(k), let("wr", weight_rays, let("sc", section, let("lc", limb_cap(k), let("sp", spectrum,
               mul(k["e"], v("sm"), edge_fade(k), v("wr"), v("sc"), v("lc"), v("sp"))))))))
    if k.get("w_max"):
        body = let("we", resolved_width(k), mul(div(k["w"], v("we")), body))
    return bind_sheet(k, let("step", lattice_step(half), body))


def medium(node_id, name, origin, half, k, role):
    k = dict(k, oy=float(-origin[1] if k.get("mirror") else origin[1]))
    node = {
        "id": node_id,
        "origin": [float(a) for a in origin],
        "scale": [float(a) for a in half],
        "field": {"amplitude": 1.0, "baseDensity": 1.0, "frequency": 1.0, "mode": "Procedural"},
        "vectorField": {"amplitude": 0.0, "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
                        "frequency": 1.0, "mode": "Procedural"},
        "volumeDensity": piecewise(curtain_density(k, half)),
        # An aurora is optically thin: it shines and does not shadow itself. A tiny
        # constant keeps Beer-Lambert's (oldT-T)/sigma exact and equal to the step.
        "volumeExtinction": piecewise(c(0.00002)),
        "volumeScattering": piecewise(c(0.0)),
        "volumeChroma": piecewise(vec3(1.0, 1.0, 1.0)),
        "volumeEmission": piecewise(curtain_emission(k, half)),
        "authoredProperties": {
            "displayName": pv("string", name),
            "aurora.role": pv("string", role),
            "aurora.volumeBottom": pv("float", float(origin[1] - half[1])),
            "aurora.volumeTop": pv("float", float(origin[1] + half[1])),
            "authors": pv("list", [pv("string", PERSON)]),
            "injected_by": pv("string", AGENT),
        },
    }
    return node


HERO = dict(
    tilt_x=-0.20,
    k0=0.0058, p0=0.7, a0=165.0,
    k1=0.0147, p1=2.1, a1=58.0,
    k2=0.0391, p2=0.3, a2=15.0,
    kf1=0.0213, pf1=4.0, wf1=0.040, af1=2.4,
    kf2=0.0577, pf2=1.0, wf2=-0.075, af2=3.5,
    kf3=0.131, pf3=2.0, wf3=0.11, af3=2.0,
    # wz must hold w_max + w/4 + the three fold amplitudes.
    w=7.5, w_max=10.5, kw=0.011, pw=1.0, wz=20.5, support_rise=0.25,
    y0=200.0, kb1=0.0083, pb1=0.4, wb1=0.03, ab1=18.0, kb2=0.031, pb2=1.7, ab2=5.0,
    h=340.0, kh=0.019, ph=0.9,
    r0=0.36, r1=1.00, f1=0.085, fy=0.0040, rt=0.055, seed=3.7,
    cv=1.2, c1=0.137, ca1=0.17, c2=0.2221, ca2=0.13, c3=0.3697, ca3=0.20, c4=0.6131, ca4=0.17,
    c5=1.0727, ca5=0.15, green_rise=16.0,
    sec0=0.74, ks1=0.0071, ps1=1.1, ws1=0.045, as1=0.36, ks2=0.0187, ps2=3.0, as2=0.18,
    hem=0.14, green=1.0, green_top=0.72, red_from=0.36, red_rise=2.0, red=0.34, violet_from=0.36, violet_rise=3.0, violet=0.28,
    e=0.150, hx=800.0, fade=230.0, base=0.18, limb_floor=0.30,
)

FAR_BAND = dict(HERO,
    tilt_x=0.28,
    k0=0.0049, p0=4.1, a0=120.0,
    k1=0.0131, p1=0.2, a1=44.0,
    k2=0.0433, p2=2.0, a2=11.0,
    kf1=0.0241, pf1=1.0, wf1=-0.032, af1=2.0,
    kf2=0.0651, pf2=3.2, wf2=0.060, af2=0.8, af3=0.0,
    w=10.0, w_max=0.0, kw=0.013, pw=2.2, wz=16.0,
    y0=128.0, kb1=0.0093, pb1=2.6, wb1=-0.024, ab1=34.0, kb2=0.027, pb2=0.5, ab2=3.0,
    h=440.0, kh=0.023, ph=3.3,
    r0=0.36, r1=1.35, f1=0.075, fy=0.0036, rt=0.07, seed=11.3,
    cv=-1.5, c1=0.1733, ca1=0.18, c2=0.2851, ca2=0.13, c3=0.4567, ca3=0.11, c4=0.7411, ca4=0.08, ca5=0.0, green_rise=7.0,
    sec0=0.82, ks1=0.0059, ps1=5.0, ws1=-0.036, as1=0.30, ks2=0.0171, ps2=1.0, as2=0.16,
    hem=0.16, green_top=0.46, red_from=0.25, red_rise=2.5, red=0.70, violet_from=0.25, violet_rise=3.0, violet=0.46,
    e=0.150, hx=880.0, fade=240.0, base=0.30,
)

ARC_BOX = ((0.0, 370.0, 120.0), (800.0, 190.0, 300.0))
FAR_BOX = ((0.0, 330.0, 665.0), (880.0, 280.0, 245.0))


def mirrored(k, gain):
    """The reflection: the same sheet below the plane y = 0, dimmer, with the finest
    ray layer softened as ripples would soften it."""
    return dict(k, mirror=True, e=k["e"] * gain, ca3=k["ca3"] * 0.3, ca4=0.0)


def build_fields():
    """Two sheets and the far band's reflection, in disjoint boxes so no sample pays for
    two media. The arc's own reflection cost ~40 ms per frame at 2560x1440 for a part of
    the water the sky's reflected rays already fill, so only the band is doubled."""
    (ox, oy, oz), half = FAR_BOX
    return [
        medium("merry.aurora.overhead-arc", "The Overhead Arc", *ARC_BOX, HERO,
               "the hero sheet: folds across the zenith, the swirl of the first two photographs"),
        medium("merry.aurora.far-band", "The Far Band", *FAR_BOX, FAR_BAND,
               "over the northern ridges: green body, pink hem, red and violet ray tops"),
        medium("merry.aurora.far-band.reflection", "The Far Band, in the Lake", (ox, -oy, oz), half, mirrored(FAR_BAND, 0.70),
               "the lake as a plane of symmetry: y -> -y of the band"),
    ]


# =============================================================================
# THE SKY. A radiance function on one opaque surface: one evaluation per pixel, so
# stars stay point-sharp where a marched shell would not.
# =============================================================================
DOME_R = 960.0


STAR_CELLS = 120.0      # cells per unit of the stereographic plane
STAR_RADIUS = 0.00068   # radians: about 1.1 px at the app's 45 degree 1440p camera


def starfield():
    """Real point stars: one per cell of a stereographic plane, each a round kernel at a
    hashed position with a steep magnitude law, so many are faint and few are bright. The
    earlier thresholded noise product made irregular flecks of one brightness, which read
    as dashes and pebbles. Stereographic projection is conformal, which is what keeps every
    star round anywhere on the sky.

    Coordinates use |y| without the ripple on purpose. Every use would otherwise repeat the
    ripple's text, and stars are sharp in the still water of Zach's reference photos."""
    lift = add(1.0, div(v("ya0"), DOME_R))
    plane = vec3(mul(div(v("dx"), v("lift")), STAR_CELLS), 0.0, mul(div(v("dz"), v("lift")), STAR_CELLS))
    key = add(mul(component(v("cell"), "x"), 1.9898), mul(component(v("cell"), "z"), 7.233))
    hashes = vec3(sin_("key", 1.0, 0.37, 5375.8), sin_("key", 1.0, 1.7, 4758.5), sin_("key", 1.0, 3.1, 3758.5))
    fx = sub(sub(component(v("sg"), "x"), component(v("cell"), "x")), add(0.15, mul(0.7, component(v("hf"), "y"))))
    fz = sub(sub(component(v("sg"), "z"), component(v("cell"), "z")), add(0.15, mul(0.7, component(v("hf"), "z"))))
    # Squared angular distance in units of the star radius (ds = dtheta / lift here). In
    # these units nothing is tiny: OntoMath's Div guards small denominators, and dividing
    # by radius^2 ~ 7e-7 returned 0, which lit every faint star's whole cell as a block.
    r2 = mul(add(mul(fx, fx), mul(fz, fz)), pow_(mul(v("lift"), 1.0 / (STAR_CELLS * STAR_RADIUS)), 2.0))
    m = component(v("hf"), "x")
    magnitude = pow_(clamp(div(sub(m, 0.30), 0.70)), 26.0)
    kernel = pow_(clamp(sub(1.0, div(v("r2"), add(1.0, mul(5.0, v("mag")))))), 2.0)
    # Most of the sky is dust a little above the air, and bright stars are rare.
    shine = mul(clamp(mul(sub(m, 0.30), 40.0)), add(0.03, mul(0.34, m, m), mul(0.70, v("mag"))), kernel)
    warm = component(v("hf"), "y")
    colour = vec3(add(0.86, mul(0.28, warm)), 0.95, sub(1.14, mul(0.28, warm)))
    return let("lift", lift, let("sg", plane, let("cell", floor3(v("sg")), let("key", key, let("h3", hashes,
           let("hf", sub(v("h3"), floor3(v("h3"))), let("r2", r2, let("mag", magnitude, mul(shine, colour)))))))))


def rise_fall(e, a, b, cc, d):
    """Trapezoid in elevation: up over [a,b], down over [cc,d]. One use of e each side."""
    return mul(clamp(div(sub(e, a), b - a)), clamp(div(sub(d, e), d - cc)))


def sky_aurora():
    """THE RADIANCE FIELD: aurora as a function of DIRECTION. Field lines through the
    sky lie on meridians of the magnetic zenith, so a streak is constant in azimuth and
    long in elevation, and the whole fan converges overhead exactly as in the photos.
    One evaluation per pixel covers the whole sky behind the marched sheets, which is
    what lets the sky be full of fine rays at a fraction of a volume's cost."""
    # Surface colour binds no time coordinate, so this field is still; the marched
    # sheets in front of it carry the motion.
    # Unit azimuth vector about the magnetic zenith (tilted off true vertical).
    ax = sub(v("dx"), mul(-0.12, v("dy")))
    az = sub(v("dz"), mul(0.24, v("dy")))
    rr = sqrt_(add(mul(ax, ax), mul(az, az), 0.0009))
    macro = clamp(add(0.30, mul(1.55, noise(mul(v("A"), 2.3), mul(v("B"), 2.3), mul(v("dy"), 1.1)))))
    macro2 = clamp(add(0.22, mul(1.70, noise(add(mul(v("A"), 1.7), 5.1), add(mul(v("B"), 1.7), 2.9), add(mul(v("dy"), 1.4), 3.0)))))
    fine = clamp(add(0.38,
                     mul(1.05, noise(mul(v("A"), 15.0), mul(v("B"), 15.0), mul(v("dy"), 1.5))),
                     mul(0.80, noise(mul(v("A"), 53.0), mul(v("B"), 53.0), mul(v("dy"), 2.6)))))
    streak = add(0.30, mul(0.70, pow_(fine, 2.0)))
    # Colour is altitude: green low, then rose, then violet toward the zenith.
    e = v("dy")
    green = rise_fall(e, 0.015, 0.09, 0.14, 0.60)
    rose = rise_fall(e, 0.16, 0.36, 0.48, 0.92)
    violet = rise_fall(e, 0.34, 0.62, 0.75, 1.02)
    # Single-use Lets flatten the tree: same emitted arithmetic, but a Let does not
    # double the loader's type-check visits the way every other level does.
    body = let("macro", macro, let("macro2", macro2, let("streak", streak, let("cgreen", green, let("crose", rose, let("cviolet", violet,
           mul(v("streak"), add(mul(0.82, v("macro"), v("cgreen"), vec3(*OXYGEN_GREEN)),
                                mul(0.62, v("macro2"), v("crose"), vec3(0.95, 0.16, 0.42)),
                                mul(0.36, v("macro2"), v("cviolet"), vec3(0.40, 0.12, 0.92))))))))))
    return let("rr", rr, let("A", div(ax, v("rr")), let("B", div(az, v("rr")), body)))


SKY_ONLY_ABOVE = 190.0   # world |y| above which no ridge or treeline can appear


def sky_terms():
    """Air, aurora and stars. Sines replace Perlin for the dither, since this runs on every
    pixel of the frame."""
    horizon = clamp(sub(1.0, mul(v("dy"), 3.0)))
    h2 = mul(horizon, horizon)
    zenith = clamp(sub(1.0, mul(v("dy"), 1.05)))
    base = add(vec3(0.014, 0.022, 0.050),
               mul(zenith, vec3(0.016, 0.034, 0.078)),
               mul(h2, vec3(0.012, 0.090, 0.082)))
    dither = mul(0.0040, sin_("x", 1.713), sin_("z", 2.291, 0.7), sin_("ya", 3.137, 1.9))
    return let("aur", sky_aurora(), let("st", starfield(), let("dith", dither, let("base", base,
           add(v("base"), v("aur"), v("st"), mul(v("dith"), vec3(1.0, 1.0, 1.0)))))))


def direction_frame(body):
    """Bind the direction coordinates. Ripples jitter elevation below the plane only,
    from sines of a bare coordinate, because dy is read many times and each read repeats
    this text."""
    below = sub(1.0, clamp(mul(add(v("y"), 1.0), 1000.0)))
    ripple = mul(below, add(sin_("z", 0.31, 0.2, 0.012), sin_("z", 0.83, 1.9, 0.009), sin_("x", 0.041, 0.7, 0.008)))
    return let("ya0", abs_(v("y")),
           let("ya", mul(v("ya0"), add(1.0, ripple)),
           let("dx", div(v("x"), DOME_R),
           let("dy", div(v("ya"), DOME_R),
           let("dz", div(v("z"), DOME_R),
           let("water", sub(1.0, mul(0.36, below)), body))))))


def sky_only_color():
    return direction_frame(mul(v("water"), sky_terms()))


def sky_and_land_color():
    """The whole far world as one radiance field of direction: air, aurora, stars and
    the ridges, reflected by the lake through |y|. Marched terrain cost ~1.2 s per frame
    (measured); this costs a few Perlin calls, and a ridge 800 m away has no parallax a
    walking Person could see. Flying high will reveal it as a far matte: that limit is
    on the Person Verification list."""
    pa, pb = v("PA"), v("PB")
    ridged = sub(1.0, mul(2.2, abs_(noise(mul(pa, 2.9), mul(pb, 2.9), 3.3))))
    north = add(0.45, mul(0.55, clamp(add(0.5, mul(0.9, pb)))))
    jag = let("ja", mul(pa, 60.0), add(sin_("ja", 0.21, 1.3, 0.016), sin_("ja", 0.47, 0.2, 0.011),
                                       sin_("ja", 1.0, 0.3, 0.005), sin_("ja", 2.63, 1.1, 0.003)))
    ridge_line = add(0.030, mul(0.122, north, pow_(clamp(ridged), 1.3)), jag)
    land = clamp(mul(sub(v("rl"), v("dy")), 760.0))
    # Mostly snow, with rock in gullies that run down the fall line. The noise is stretched
    # in elevation: equal frequency on both axes read as camouflage blotches in Zach's shots.
    rockiness = clamp(add(0.84, mul(1.5, noise(mul(pa, 52.0), mul(pb, 52.0), mul(v("dy"), 21.0))),
                          mul(0.7, noise(mul(pa, 170.0), mul(pb, 170.0), mul(v("dy"), 70.0)))))
    snowline = clamp(div(sub(v("dy"), add(0.016, mul(0.20, v("rl")))), 0.012))
    rock = vec3(0.030, 0.040, 0.054)
    ridge = add(rock, mul(snowline, add(mul(rockiness, vec3(0.150, 0.195, 0.245)), vec3(0.010, 0.034, 0.026))))
    # Treeline teeth along the far shore, from sines of the azimuth coordinate.
    tooth = let("tw", mul(pa, 240.0),
                add(0.0095, sin_("tw", 0.37, 0.9, 0.0030), sin_("tw", 1.0, 0.0, 0.0018), sin_("tw", 2.41, 1.7, 0.0012)))
    trees = clamp(mul(sub(tooth, v("dy")), 1200.0))
    rh = sqrt_(add(mul(v("dx"), v("dx")), mul(v("dz"), v("dz")), 0.000001))
    body = let("rh", rh, let("PA", div(v("dx"), v("rh")), let("PB", div(v("dz"), v("rh")),
           let("rl", ridge_line,
           let("sky", sky_terms(), let("rdg", ridge, let("land", land, let("trees", trees,
           let("lit", add(mul(sub(1.0, v("land")), v("sky")), mul(v("land"), v("rdg"))),
               mul(v("water"), add(mul(sub(1.0, v("trees")), v("lit")),
                                   mul(v("trees"), vec3(0.006, 0.010, 0.012)))))))))))))
    return direction_frame(body)


def sky_pieces():
    """Pieces over y: the ridge is only computed in the band of the frame that can hold
    it. Most sky pixels skip it entirely."""
    return {"input": "y", "pieces": [
        {"lo": -SKY_ONLY_ABOVE, "includeLo": True, "hi": SKY_ONLY_ABOVE, "includeHi": True, "mathNode": sky_and_land_color()},
        {"lo": SKY_ONLY_ABOVE, "includeLo": False, "mathNode": sky_only_color()},
        {"hi": -SKY_ONLY_ABOVE, "includeHi": False, "mathNode": sky_only_color()},
    ]}


# =============================================================================
# THE NEAR SHORE. The ground under the Person, and the only marched land. Height is
# plain sines so it can use the height-grid traversal, with no noise in the distance.
# =============================================================================


SHORE_CZ = -140.0   # the shore's box is centred here; its expressions are box-local


def shoreline(xv):
    return add(13.0, mul(0.0011, xv, xv), sin_("x", 0.021, 0.5, 3.5), sin_("x", 0.063, 2.0, 1.2))


def shore_height():
    """Land above the water and BELOW the box floor off the shore, so the lake is not a
    surface at all: rays pass through y = 0 into the mirror world. The ground rises behind
    the Person and the beach is short, because a ray skimming a long flat beach exhausts
    the sphere tracer's steps and leaves striped misses along the far shore."""
    world_z = add(v("z"), SHORE_CZ)
    rise_behind = clamp(mul(world_z, -0.02), 0.0, 8.0)
    return add(-12.0, mul(13.0, clamp(div(sub(shoreline(v("x")), world_z), 12.0))), rise_behind)


def shore_sdf():
    return mul(0.7, sub(v("y"), shore_height()))


def shore_color():
    drift = clamp(add(0.80, mul(0.45, noise(mul(v("x"), 0.09), 0.5, mul(v("z"), 0.09)))))
    # Noise, not crossed sines: two sines at right angles read as woven cloth at a distance.
    fine = add(1.0, mul(0.11, noise(mul(v("x"), 0.71), 2.5, mul(v("z"), 0.71))))
    return mul(drift, fine, vec3(0.20, 0.275, 0.33))


# Spruce: a tapering spire with serrated branch tiers, one expression per tree.
def spruce(base, height, radius, phase):
    """Conservative distance to one spruce at the local origin, even in y through |y| so
    its reflection stands in the lake."""
    ring = sqrt_(add(mul(v("x"), v("x")), mul(v("z"), v("z"))))
    s_ = clamp(div(sub(v("ya"), base), height))
    # Many shallow branch tiers, with the second harmonic making each one droop. Fewer and
    # deeper tiers made the wavy bowling pins in Zach's screenshots.
    k = 118.0 / height
    tiers = add(0.84, sin_("ya", k, phase, 0.13), sin_("ya", 2.0 * k, 2.0 * phase + 1.2, 0.05))
    silhouette = mul(radius, sub(1.0, s_), tiers)
    return let("ya", abs_(v("y")),
               # Lipschitz ~ radius * sum(a * k) ~ 3, so step at 0.3 to stay conservative.
               mul(0.3, {"op": 21, "children": [sub(ring, add(silhouette, 0.05)), sub(v("ya"), base + height)]}))


def spruce_color():
    tint = add(0.015, mul(0.014, clamp(mul(abs_(v("y")), 0.05))))
    return vec3(mul(tint, 0.34), mul(tint, 0.60), mul(tint, 0.62))


LEFT_STAND = [(-96, 8, 2, 30, 6.4, 1.3), (-112, -6, 2, 44, 8.6, 0.2), (-128, 14, 2, 38, 7.4, 2.1), (-142, -14, 2, 58, 10.5, 5.2),
              (-158, 6, 2, 50, 9.0, 3.4), (-176, 20, 2, 66, 11.5, 4.2), (-192, -8, 2, 72, 12.5, 0.9), (-212, 14, 2, 56, 10.0, 2.7),
              (-232, -2, 2, 80, 14.0, 3.6), (-254, 24, 2, 62, 11.0, 1.1), (-278, 6, 2, 88, 15.0, 4.8), (-78, -12, 1.5, 17, 3.8, 1.9),
              (-60, 2, 1.0, 9, 2.4, 2.9)]
RIGHT_STAND = [(92, 6, 2, 34, 6.8, 3.1), (106, -8, 2, 52, 9.4, 1.1), (124, 12, 2, 62, 11.2, 2.3), (140, -12, 2, 74, 12.8, 5.0),
               (158, 4, 2, 58, 10.4, 4.4), (176, 18, 2, 82, 13.8, 0.4), (198, -4, 2, 92, 15.0, 1.7), (222, 20, 2, 70, 12.4, 3.9),
               (246, 2, 2, 100, 16.5, 2.5), (272, 26, 2, 76, 13.0, 0.7), (298, 8, 2, 108, 18.0, 4.1), (72, -10, 1.5, 19, 4.2, 5.6),
               (56, 0, 1.0, 8, 2.2, 3.3)]


def spruce_objects(prefix, name, trees):
    """One Object per tree. A shared Union would evaluate every tree at every marching
    step (a 13-tree stand measured ~690 ms per frame). Separately, each tree's small box
    is all it ever costs."""
    out = []
    for i, (x, z, base, height, radius, phase) in enumerate(trees):
        r = radius + 1.0
        out.append(field_object(f"{prefix}.{i:02d}", f"{name}, spruce {i + 1}", "material.merry.spruce", (x, 0.0, z),
                                (r, base + height + 1.0, r), spruce(base, height, radius, phase),
                                {"aurora.role": pv("string", "black spruce against the sky, and in the water"),
                                 "aurora.heightMetres": pv("float", float(height))}))
    return out


def sdf_expr(node):
    return {"op": 0, "prim": 7, "dims": [0.5, 0.5, 0.5], "offset": [0.0, 0.0, 0.0],
            "p0": 0.0, "p1": 0.0, "t": 0.5, "mathNode": node}


def field_object(object_id, name, material_id, center, extent, sdf, props=None):
    cx, cy, cz = center
    ap = {"displayName": pv("string", name), "authors": pv("list", [pv("string", PERSON)]),
          "injected_by": pv("string", AGENT)}
    ap.update(props or {})
    return {
        "objectID": object_id,
        "shapeKind": 10, "geometryType": 10,
        "shapeParams": [1.0, 1.0, 1.0, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
        "transform": [1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0, 0, float(cx), float(cy), float(cz), 1.0],
        "center": [float(cx), float(cy), float(cz)],
        "materialId": material_id,
        "renderMode": 0, "rotationResponsiveness": 10.0, "authoritativeAxis": [0, 1, 0],
        "targetRotation": [0.0, 0.0, 0.0], "x2D": 100.0, "y2D": 100.0, "zOrder2D": 0.0,
        "field": sdf_expr(sdf),
        "fieldExtent": [float(a) for a in extent],
        "authoredProperties": ap,
    }


def material(name, base, ambient, diffuse, specular, shininess, color_expr=None):
    m = {"name": name, "baseColor": list(base), "opacity": 1.0, "shininess": shininess,
         "specular": specular, "ambient": ambient, "diffuse": diffuse}
    if color_expr is not None:
        m["colorExpr"] = color_expr if "pieces" in color_expr else piecewise(color_expr)
    return m


def build_zone():
    objects = [
        field_object("merry.sky", "The Night Sky", "material.merry.sky", (0, 0, 0), (DOME_R, DOME_R, DOME_R),
                     sub(DOME_R, {"op": 11, "children": [v("p")]}),
                     {"aurora.role": pv("string", "the radiance field: sky glow, fanned rays, stars")}),
        field_object("merry.shore", "The Near Shore", "material.merry.shore", (0, 0, SHORE_CZ), (150.0, 10.0, 200.0),
                     shore_sdf(),
                     {"aurora.role": pv("string", "the snow under the Person")}),
    ] + spruce_objects("merry.spruce.west", "The West Stand", LEFT_STAND) + spruce_objects("merry.spruce.east", "The East Stand", RIGHT_STAND)
    materials = [
        material("merry.sky", (1, 1, 1), 1.0, 0.0, 0.0, 1.0, sky_pieces()),
        material("merry.shore", (1, 1, 1), 1.0, 0.0, 0.0, 1.0, shore_color()),
        material("merry.spruce", (1, 1, 1), 1.0, 0.0, 0.0, 1.0, spruce_color()),
    ]
    return {
        "identifier": ZONE_NAME, "name": ZONE_NAME, "scope": "Local", "deletable": {"default": True},
        "owner": PERSON, "parentZone": "", "injected_by": AGENT,
        "qualities": {"ownerKind": "person", "aurora.subject": "auroral sheets over a still lake"},
        "spatialRoot": {
            "id": "merry.moonlight",
            "origin": [-500.0, 700.0, -300.0], "scale": [1.0, 1.0, 1.0],
            # Literal-zero rho: a medium sample skips incident light entirely, since nothing
            # here scatters. No direct light reaches any surface, so every solid carries its
            # night shading in its own colour field.
            "field": {"amplitude": 1.0, "baseDensity": 1.0, "frequency": 1.0, "mode": "AST",
                      "astDefinition": piecewise(c(0.0))},
            "vectorField": {"amplitude": 0.0, "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
                            "frequency": 1.0, "mode": "Procedural"},
            "authoredProperties": {
                "displayName": pv("string", "The Night's Ambient"),
                "light.source": pv("bool", True),
                "light.color": {"t": "vec3", "x": 1.0, "y": 1.0, "z": 1.0},
                "light.intensity": pv("float", 1.0),
                "light.ambient": pv("float", 0.2),
                "light.diffuse": pv("float", 0.8),
                "light.specular": pv("float", 0.0),
                # The app's historical sight ends at 100 m. The sky is at DOME_R, and this
                # keeps it in reach while a Person flies around inside it.
                "view.farPlane": pv("float", 2000.0),
            },
        },
        "spatialFields": build_fields(),
        "materials": materials,
        "world": {"objects": objects, "relations": [], "laws": []},
        "formationRelations": [],
        "lexemes": [],
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, help="target zone.json; refused if it already exists unless --look-dev")
    ap.add_argument("--look-dev", action="store_true", help="overwrite a scratch file during look development")
    a = ap.parse_args()
    if os.path.exists(a.out) and not a.look_dev:
        sys.exit(f"refused: {a.out} exists. A live Zone is patched, never regenerated (§7 rule 8).")
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    staged = a.out + ".staged"
    with open(staged, "w") as f:
        json.dump(build_zone(), f, indent=1)
    os.replace(staged, a.out)
    print(f"wrote {a.out} ({os.path.getsize(a.out)} bytes)")


if __name__ == "__main__":
    main()
