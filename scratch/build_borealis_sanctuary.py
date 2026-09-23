#!/usr/bin/env python3
"""
Borealis Sanctuary Complete Zone Generator
==========================================
Generates saves/zones/Borealis Sanctuary/zone.json
and saves/worlds/borealis_sanctuary.json
"""

import json
import os
import math

def num(c):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(c), "factors": {}}]}}

def var(name):
    return {"op": 1, "var": name}

def vec3_construct(x, y, z):
    return {"op": 2, "children": [x, y, z]}

def add(a, b):
    return {"op": 4, "children": [a, b]}

def sub(a, b):
    return {"op": 5, "children": [a, b]}

def scale(a, b):
    return {"op": 6, "children": [a, b]}

def length_node(p):
    return {"op": 11, "children": [p]}

def div(a, b):
    return {"op": 23, "children": [a, b]}

def abs_node(a):
    return {"op": 25, "children": [a]}

def clamp_node(val, lo, hi):
    return {"op": 26, "children": [val, lo, hi]}

def noise_node(p):
    return {"op": 29, "children": [p]}

def sin_node(c, v, sc=1.0, sh=0.0):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(c), "factors": {}, "trans": [{"kind": 0, "var": v, "scale": float(sc), "shift": float(sh)}]}]}}

def cos_node(c, v, sc=1.0, sh=0.0):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(c), "factors": {}, "trans": [{"kind": 1, "var": v, "scale": float(sc), "shift": float(sh)}]}]}}

def pw(node, input_var="x"):
    return {
        "input": input_var,
        "pieces": [
            {
                "hasLo": False,
                "hasHi": False,
                "mathNode": node
            }
        ]
    }

# -----------------------------------------------------------------------------
# 1. V1 COMPARISON PAIR: Shared Density, Airy vs Dense Extinction
# -----------------------------------------------------------------------------
# Shared Density: 1.6 * clamp(1.0 - (length(p) - 0.7 * noise(0.9 * p)) / 2.4, 0.0, 1.0)
v1_shared_density = pw(
    scale(
        num(1.6),
        clamp_node(
            sub(
                num(1.0),
                div(
                    sub(
                        length_node(var("p")),
                        scale(num(0.7), noise_node(scale(num(0.9), var("p"))))
                    ),
                    num(2.4)
                )
            ),
            num(0.0),
            num(1.0)
        )
    )
)

v1_airy_extinction = pw(num(0.16))    # Low extinction: luminous and airy, high transmittance
v1_dense_extinction = pw(num(2.85))   # High extinction: dense and absorbing, heavy attenuation
v1_shared_scattering = pw(num(0.16))  # Neutral scattering
v1_shared_chroma = pw(vec3_construct(num(1.0), num(1.0), num(1.0))) # Neutral white

# -----------------------------------------------------------------------------
# 2. V2 COMPARISON PAIR: Shared Density & Extinction, Emerald vs Violet
# -----------------------------------------------------------------------------
# Shared Density: 1.8 * clamp(1.0 - (abs(z - 0.8*sin(0.6*x)) + 0.6*length(p) - 0.4*noise(p)) / 2.0, 0.0, 1.0)
v2_shared_density = pw(
    scale(
        num(1.8),
        clamp_node(
            sub(
                num(1.0),
                div(
                    add(
                        abs_node(sub(var("z"), sin_node(0.8, "x", sc=0.6, sh=0.0))),
                        sub(
                            scale(num(0.6), length_node(var("p"))),
                            scale(num(0.4), noise_node(var("p")))
                        )
                    ),
                    num(2.0)
                )
            ),
            num(0.0),
            num(1.0)
        )
    )
)

v2_shared_extinction = pw(num(0.65)) # Identical extinction for both!

# Emerald / Cyan scattering & chroma:
v2_emerald_scattering = pw(num(1.25)) # Strong scattering
v2_emerald_chroma = pw(vec3_construct(num(0.08), num(0.98), num(0.68))) # Vibrant emerald-cyan

# Violet / Magenta scattering & chroma:
v2_violet_scattering = pw(num(0.28))  # Weaker scattering
v2_violet_chroma = pw(vec3_construct(num(0.78), num(0.16), num(0.92)))  # Celestial violet-magenta

# -----------------------------------------------------------------------------
# 3. HERO CURTAIN: Aurora-inspired Participating Medium
# -----------------------------------------------------------------------------
# D(p,t): Undulating flowing ribbon curtain
# z_fold = 2.6*sin(0.18*x) + 1.2*cos(0.35*x)
# Sheet envelope S = clamp(1.0 - abs(z - z_fold) / 1.8, 0.0, 1.0)
# Striation R = 0.65 + 0.35*sin(1.1*x)
# Noise turbulence N = 0.4 * noise(0.14 * p)
# Composed D = 2.0 * S * clamp(R + N, 0.0, 1.8)
hero_density = pw(
    scale(
        num(2.0),
        scale(
            clamp_node(
                sub(
                    num(1.0),
                    div(
                        abs_node(
                            sub(
                                var("z"),
                                add(
                                    sin_node(2.6, "x", sc=0.18, sh=0.0),
                                    cos_node(1.2, "x", sc=0.35, sh=0.0)
                                )
                            )
                        ),
                        num(1.8)
                    )
                ),
                num(0.0),
                num(1.0)
            ),
            clamp_node(
                add(
                    add(num(0.65), sin_node(0.35, "x", sc=1.1, sh=0.0)),
                    scale(num(0.4), noise_node(scale(num(0.14), var("p"))))
                ),
                num(0.0),
                num(1.8)
            )
        )
    )
)

# Extinction sigma_t(p,t):
# sigma_t = 0.25 + 0.55 * clamp(0.45 - 0.07*y, 0.0, 1.0) + 0.12 * sin(0.22*x)
hero_extinction = pw(
    add(
        num(0.25),
        add(
            scale(
                num(0.55),
                clamp_node(
                    sub(num(0.45), scale(num(0.07), var("y"))),
                    num(0.0),
                    num(1.0)
                )
            ),
            sin_node(0.12, "x", sc=0.22, sh=0.0)
        )
    )
)

# Scattering sigma_s(p,t):
# sigma_s = 0.35 + 0.75 * clamp(0.5 + 0.5*cos(0.32*x) + 0.25*noise(0.18*p), 0.0, 1.0)
hero_scattering = pw(
    add(
        num(0.35),
        scale(
            num(0.75),
            clamp_node(
                add(
                    add(num(0.5), cos_node(0.5, "x", sc=0.32, sh=0.0)),
                    scale(num(0.25), noise_node(scale(num(0.18), var("p"))))
                ),
                num(0.0),
                num(1.0)
            )
        )
    )
)

# Medium Chroma C_v(p,t):
hero_chroma_g = clamp_node(
    sub(
        add(num(0.85), sin_node(0.18, "x", sc=0.16, sh=0.0)),
        scale(num(0.035), var("y"))
    ),
    num(0.12),
    num(0.98)
)

hero_chroma_r = clamp_node(
    add(
        add(num(0.12), scale(num(0.055), var("y"))),
        sin_node(0.24, "x", sc=0.22, sh=0.0)
    ),
    num(0.05),
    num(0.92)
)

hero_chroma_b = clamp_node(
    add(
        add(num(0.45), scale(num(0.048), var("y"))),
        cos_node(0.28, "x", sc=0.18, sh=0.0)
    ),
    num(0.12),
    num(0.98)
)

hero_chroma = pw(vec3_construct(hero_chroma_r, hero_chroma_g, hero_chroma_b))

# -----------------------------------------------------------------------------
# Build FieldNodes
# -----------------------------------------------------------------------------
spatial_root = {
    "id": "borealis.primary-atmosphere-root",
    "origin": [0.0, 12.0, 0.0],
    "scale": [1.0, 1.0, 1.0],
    "field": {
        "mode": "AST",
        "baseDensity": 1,
        "frequency": 1,
        "amplitude": 1,
        "ast": {
            "input": "x",
            "pieces": [
                {
                    "hasLo": False,
                    "hasHi": False,
                    "mathNode": num(0.0)
                }
            ]
        }
    },
    "vectorField": {
        "mode": "Procedural",
        "baseFlowX": 0.0,
        "baseFlowY": 0.0,
        "baseFlowZ": 0.0,
        "frequency": 1.0,
        "amplitude": 0.0
    },
    "authoredProperties": {
        "displayName": {"t": "string", "v": "Celestial Zenith Starlight"},
        "light.source": {"t": "bool", "v": True},
        "light.ambient": {"t": "float", "v": 0.04},
        "light.diffuse": {"t": "float", "v": 0.55},
        "light.specular": {"t": "float", "v": 0.35},
        "light.intensity": {"t": "float", "v": 0.85},
        "light.color": {"t": "vec3", "v": [0.14, 0.18, 0.32]}
    }
}

spatial_fields = [
    # Station 1: V1 Comparison Pair
    {
        "id": "borealis.v1.airy-transmitting-cloud",
        "origin": [-12.0, 5.0, 25.0],
        "scale": [4.5, 3.5, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": v1_shared_density,
        "volumeExtinction": v1_airy_extinction,
        "volumeScattering": v1_shared_scattering,
        "volumeChroma": v1_shared_chroma,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "V1 Airy Transmitting Cloud (sigma_t = 0.16)"},
            "borealis.station": {"t": "string", "v": "Station I: V1 Extinction Sovereignty"},
            "borealis.regime": {"t": "string", "v": "Low Extinction (Airy / Transmitting)"}
        }
    },
    {
        "id": "borealis.v1.dense-absorbing-cloud",
        "origin": [12.0, 5.0, 25.0],
        "scale": [4.5, 3.5, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": v1_shared_density,
        "volumeExtinction": v1_dense_extinction,
        "volumeScattering": v1_shared_scattering,
        "volumeChroma": v1_shared_chroma,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "V1 Dense Absorbing Cloud (sigma_t = 2.85)"},
            "borealis.station": {"t": "string", "v": "Station I: V1 Extinction Sovereignty"},
            "borealis.regime": {"t": "string", "v": "High Extinction (Dense / Absorbing)"}
        }
    },
    # Station 2: V2 Comparison Pair
    {
        "id": "borealis.v2.emerald-cyan-surge",
        "origin": [-12.0, 5.0, 60.0],
        "scale": [4.5, 3.5, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": v2_shared_density,
        "volumeExtinction": v2_shared_extinction,
        "volumeScattering": v2_emerald_scattering,
        "volumeChroma": v2_emerald_chroma,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "V2 Emerald-Cyan Surge (sigma_s = 1.25, emerald/cyan chroma)"},
            "borealis.station": {"t": "string", "v": "Station II: V2 Scattering & Chroma Sovereignty"},
            "borealis.albedo": {"t": "string", "v": "High Scattering Albedo (Luminous Emerald)"}
        }
    },
    {
        "id": "borealis.v2.violet-magenta-whisper",
        "origin": [12.0, 5.0, 60.0],
        "scale": [4.5, 3.5, 4.5],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": v2_shared_density,
        "volumeExtinction": v2_shared_extinction,
        "volumeScattering": v2_violet_scattering,
        "volumeChroma": v2_violet_chroma,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "V2 Violet-Magenta Whisper (sigma_s = 0.28, violet/magenta chroma)"},
            "borealis.station": {"t": "string", "v": "Station II: V2 Scattering & Chroma Sovereignty"},
            "borealis.albedo": {"t": "string", "v": "Low Scattering Albedo (Mystic Violet)"}
        }
    },
    # Station 3: Hero Auroral Curtain
    {
        "id": "borealis.hero.auroral-curtain",
        "origin": [0.0, 16.0, 105.0],
        "scale": [22.0, 12.0, 10.0],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": hero_density,
        "volumeExtinction": hero_extinction,
        "volumeScattering": hero_scattering,
        "volumeChroma": hero_chroma,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "The Celestial Aurora Veil (Hero V1+V2 Curtain)"},
            "borealis.station": {"t": "string", "v": "Station III: The Celestial Aurora Veil"},
            "borealis.role": {"t": "string", "v": "hero_aurora_curtain"}
        }
    }
]

# -----------------------------------------------------------------------------
# Build World Objects (Floor, Beacons, Pillars, Stelae)
# -----------------------------------------------------------------------------
objects = []

def make_obj(obj_id, name, center, dims, mat_id="material.cathedral_basalt", shape_kind=10, face_color=[0.8, 0.8, 0.85]):
    # shape_kind 10 = Box (SdfPrim::Box)
    # shapeParams for Box: dims[0], dims[1], dims[2]
    # field definition for Box
    half_x = dims[0]
    half_y = dims[1]
    half_z = dims[2]
    transform = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        center[0], center[1], center[2], 1.0
    ]
    return {
        "objectID": obj_id,
        "center": center,
        "authoritativeAxis": [0, 1, 0],
        "geometryType": shape_kind,
        "shapeKind": shape_kind,
        "materialId": mat_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": transform,
        "x2D": 100,
        "y2D": 100,
        "zOrder2D": 0,
        "faceColors": [face_color for _ in range(6)],
        "field": {
            "op": 0,
            "prim": 1,
            "dims": [half_x, half_y, half_z],
            "offset": [0, 0, 0],
            "p0": 0,
            "p1": 0,
            "t": 0.5
        },
        "fieldExtent": [half_x + 0.15, half_y + 0.15, half_z + 0.15],
        "authoredProperties": {
            "displayName": {"t": "string", "v": name}
        }
    }

# 1. Floor segments along the sanctuary promenade (Z in [-15, 135])
for z_pos in range(-10, 140, 20):
    objects.append(
        make_obj(
            f"borealis.floor.seg_{z_pos}",
            f"Promenade Pavement Z={z_pos}",
            [0.0, -0.6, float(z_pos)],
            [14.0, 0.5, 10.0],
            mat_id="material.cathedral_basalt",
            face_color=[0.18, 0.20, 0.25]
        )
    )

# 2. Station 1 Reference Beacons (Z = 35) behind airy and dense clouds
objects.append(
    make_obj(
        "borealis.station1.beacon-left",
        "Station I Golden Beacon (Airy Cloud Reference)",
        [-12.0, 5.0, 35.0],
        [1.2, 5.0, 1.2],
        face_color=[1.0, 0.85, 0.3]
    )
)
objects.append(
    make_obj(
        "borealis.station1.beacon-right",
        "Station I Golden Beacon (Dense Cloud Reference)",
        [12.0, 5.0, 35.0],
        [1.2, 5.0, 1.2],
        face_color=[1.0, 0.85, 0.3]
    )
)

# 3. Station 2 Reference Crystal Pillars (Z = 70) behind emerald and violet clouds
objects.append(
    make_obj(
        "borealis.station2.pillar-left",
        "Station II Astral Quartz Needle (Emerald Surge Reference)",
        [-12.0, 5.0, 70.0],
        [1.2, 5.0, 1.2],
        face_color=[0.85, 1.0, 1.0]
    )
)
objects.append(
    make_obj(
        "borealis.station2.pillar-right",
        "Station II Astral Quartz Needle (Violet Whisper Reference)",
        [12.0, 5.0, 70.0],
        [1.2, 5.0, 1.2],
        face_color=[0.85, 1.0, 1.0]
    )
)

# 4. Station 3 Celestial Needles behind Hero Curtain (Z = 125)
for i, x_pos in enumerate([-16.0, -8.0, 0.0, 8.0, 16.0]):
    objects.append(
        make_obj(
            f"borealis.station3.needle_{i}",
            f"Celestial Spire {i+1} (Hero Curtain Background)",
            [x_pos, 16.0, 125.0],
            [0.8, 16.0, 0.8],
            face_color=[0.75, 0.85, 1.0]
        )
    )

# 5. Pedagogical Inscription Stelae
stelae_data = [
    {
        "id": "borealis.stele.entrance",
        "title": "Stele 0: The True Participating-Medium Substrate",
        "pos": [0.0, 1.5, 0.0],
        "inscription": (
            "THE CONSTITUTION OF VOLUMETRIC SOVEREIGNTY\n"
            "------------------------------------------\n"
            "rho_source != V_transport != D_medium\n"
            "D_medium   != sigma_t     != sigma_s != C_v\n\n"
            "Here in Borealis Sanctuary, light does not meet flat surfaces or fake alpha meshes.\n"
            "Participating medium is continuous mathematical reality:\n"
            "1. volume.density.ast    D(p,t)        -> scalar existence & spatial shape\n"
            "2. volume.extinction.ast sigma_t(p,t)  -> scalar optical thickness & transmittance\n"
            "3. volume.scattering.ast sigma_s(p,t)  -> scalar scattering albedo & radiance\n"
            "4. volume.chroma.ast     C_v(p,t)      -> vec3 spectral medium color\n\n"
            "Proceed down the promenade to witness the sovereignty of each channel."
        )
    },
    {
        "id": "borealis.stele.station1",
        "title": "Stele I: Volumetric V1 Extinction Sovereignty",
        "pos": [0.0, 1.5, 25.0],
        "inscription": (
            "STATION I: V1 EXTINCTION SOVEREIGNTY (D != sigma_t)\n"
            "---------------------------------------------------\n"
            "Behold the two floating clouds flanking this dais:\n"
            "- Left Cloud:  volume.extinction.ast = 0.16 (Airy / Transmitting)\n"
            "- Right Cloud: volume.extinction.ast = 2.85 (Dense / Attenuating)\n\n"
            "Their density mathematics D(p,t) are BYTE-IDENTICAL. Both clouds have the\n"
            "exact same geometric bounds, noise folds, and mass of medium.\n\n"
            "Look through them to the golden beacons placed behind at Z=35:\n"
            "The Left Beacon shines through the airy cloud with high transmittance (T ~ 0.75).\n"
            "The Right Beacon is severely darkened and extinguished (T ~ 0.02).\n"
            "Extinction governs optical attenuation independently from spatial density."
        )
    },
    {
        "id": "borealis.stele.station2",
        "title": "Stele II: Volumetric V2 Scattering & Chroma Sovereignty",
        "pos": [0.0, 1.5, 60.0],
        "inscription": (
            "STATION II: V2 SCATTERING & CHROMA SOVEREIGNTY (sigma_s & C_v)\n"
            "--------------------------------------------------------------\n"
            "Observe the two serpentine clouds flanking this dais:\n"
            "- Left Cloud:  sigma_s = 1.25, C_v = vec3(0.08, 0.98, 0.68) (Emerald/Cyan)\n"
            "- Right Cloud: sigma_s = 0.28, C_v = vec3(0.78, 0.16, 0.92) (Violet/Magenta)\n\n"
            "Both clouds share the EXACT SAME density D(p,t) AND the EXACT SAME\n"
            "extinction sigma_t(p,t) = 0.65.\n\n"
            "Because sigma_t is identical, background quartz needles at Z=70 attenuate\n"
            "at the exact same physical rate e^(-sigma_t * ds) through both media.\n"
            "The radiant emerald surge versus mystical violet glow is entirely authored\n"
            "by volume.scattering.ast and volume.chroma.ast without modifying D or sigma_t."
        )
    },
    {
        "id": "borealis.stele.station3",
        "title": "Stele III: The Celestial Aurora Veil (Hero V1+V2 Synthesis)",
        "pos": [0.0, 1.5, 85.0],
        "inscription": (
            "STATION III: THE CELESTIAL AURORA VEIL\n"
            "--------------------------------------\n"
            "Above you rises the Hero Aurora Curtain (span: 44m wide, 24m tall, 20m deep).\n"
            "It unites all four authored participating-medium channels into living harmony:\n\n"
            "1. D(p,t): Thin undulating ribbon sheet (z_fold = 2.6*sin(0.18*x) + 1.2*cos(0.35*x)),\n"
            "   striated by vertical ray harmonic waves and Perlin noise turbulence,\n"
            "   forming organic folds, wisps, and transparent gaps.\n"
            "2. sigma_t(p,t): Vertical altitude gradient (0.25 -> 0.80) that attenuates deeply\n"
            "   at the rooted base while fading to delicate gossamer transparency at the crest.\n"
            "3. sigma_s(p,t): Harmonic wave patches (0.35 -> 1.10) producing shimmering folds\n"
            "   where auroral rays catch intense scattering radiance.\n"
            "4. C_v(p,t): Continuous spectral shift from lower oxygen green (557.7 nm) to\n"
            "   mid turquoise/cyan, upper nitrogen violet, and high-altitude crimson fringes (630 nm).\n\n"
            "Gaze past the curtain to see the 5 Celestial Needles at Z=125 truncated by opaque scene depth."
        )
    }
]

for stele in stelae_data:
    obj = make_obj(
        stele["id"],
        stele["title"],
        stele["pos"],
        [1.5, 1.5, 0.4],
        mat_id="material.cathedral_basalt",
        face_color=[0.3, 0.35, 0.45]
    )
    obj["authoredProperties"]["stele.inscription"] = {"t": "string", "v": stele["inscription"]}
    objects.append(obj)

# -----------------------------------------------------------------------------
# Construct Zone Document
# -----------------------------------------------------------------------------
zone_doc = {
    "identifier": "Borealis Sanctuary",
    "name": "Borealis Sanctuary",
    "authors": ["Zachary Zhang", "Gemini Spark"],
    "injected_by": "Gemini Spark",
    "scope": "global",
    "deletable": False,
    "qualities": [
        "sanctuary",
        "observatory",
        "volumetric",
        "ontomath",
        "aurora",
        "participating_media"
    ],
    "parentZone": "",
    "owner": "default",
    "spatialRoot": spatial_root,
    "spatialFields": spatial_fields,
    "materials": [],
    "world": {
        "objects": objects
    },
    "formationRelations": [],
    "lexemes": []
}

os.makedirs("saves/zones/Borealis Sanctuary", exist_ok=True)
zone_path = "saves/zones/Borealis Sanctuary/zone.json"
with open(zone_path, "w") as f:
    json.dump(zone_doc, f, indent=2)

print(f"Wrote {zone_path} ({len(spatial_fields)} spatial fields, {len(objects)} world objects)")

# -----------------------------------------------------------------------------
# Construct Companion World/Session Save
# -----------------------------------------------------------------------------
world_doc = {
    "saveName": "borealis_sanctuary",
    "version": 1,
    "cameraPos": [0.0, 3.5, -8.0],
    "cameraFront": [0.0, 0.15, 1.0],
    "cameraUp": [0.0, 1.0, 0.0],
    "yaw": 90.0,
    "pitch": 8.0,
    "activeZone": "Borealis Sanctuary",
    "zones": ["Borealis Sanctuary"],
    "zoneRefs": ["Borealis Sanctuary"],
    "objects": objects
}

os.makedirs("saves/worlds", exist_ok=True)
world_path = "saves/worlds/borealis_sanctuary.json"
with open(world_path, "w") as f:
    json.dump(world_doc, f, indent=2)

print(f"Wrote {world_path}")
