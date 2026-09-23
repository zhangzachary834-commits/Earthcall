#!/usr/bin/env python3
"""
generate_northern_veil.py
Generates the 'Northern Veil' zone for Earthcall.
Showcases landed Volumetric V0-V4 (D, sigma_t, sigma_s, C_v, Phi, E_v).
"""

import json
import math
import os

def scalar_node(val):
    return {
        "op": 0,
        "scalarForm": {
            "terms": [{
                "c": float(val),
                "factors": {}
            }]
        }
    }

def var_node(name):
    return {
        "op": 1,
        "var": str(name)
    }

def vec3_node(x, y, z):
    return {
        "op": 2,
        "children": [scalar_node(x), scalar_node(y), scalar_node(z)]
    }

def mul_node(a, b):
    # MathNode::Op::Scale = 6. In Earthcall's typed OntoMath this admits
    # Scalar×Scalar as well as scalar/vector scaling. Op 4 is Add; using it
    # here turns every intended density/emission product into a sum and fills
    # the whole volume proxy ("big white box").
    return {
        "op": 6,
        "children": [a, b]
    }

def add_node(a, b):
    return {
        "op": 4,
        "children": [a, b]
    }

def sub_node(a, b):
    return {
        "op": 5,
        "children": [a, b]
    }

def scale_node(s, node):
    return {
        "op": 6,
        "children": [scalar_node(s), node]
    }

def length_node(node):
    return {
        "op": 11,
        "children": [node]
    }

def div_node(a, b):
    return {
        "op": 23,
        "children": [a, b]
    }

def abs_node(node):
    return {
        "op": 25,
        "children": [node]
    }

def clamp_node(val_node, min_val, max_val):
    return {
        "op": 26,
        "children": [val_node, scalar_node(min_val), scalar_node(max_val)]
    }

def perlin_node(p_node):
    return {
        "op": 29,
        "children": [p_node]
    }

def piecewise(math_node, var_input="x"):
    return {
        "input": var_input,
        "pieces": [
            {
                "hasLo": False,
                "hasHi": False,
                "mathNode": math_node
            }
        ]
    }

def make_curtain_shape(half_w, half_h, w_z, z_amp1, z_freq1, z_amp2, z_freq2, ray_freq, noise_scale, z_amp3=0.0, z_freq3=0.0):
    # 1. X envelope: Smooth lateral boundary taper so curtains span the horizon without harsh cuts
    env_x = clamp_node(
        sub_node(scalar_node(1.0), div_node(abs_node(var_node("x")), scalar_node(half_w))),
        0.0, 1.0
    )
    
    # 2. Y envelope: Crisp auroral arc lower boundary (fast rise), with long ethereal exospheric fade
    bottom_fade = clamp_node(
        scale_node(0.25, add_node(var_node("y"), scalar_node(half_h))),
        0.0, 1.0
    )
    top_fade = clamp_node(
        sub_node(scalar_node(1.0), div_node(add_node(var_node("y"), scalar_node(half_h)), scalar_node(2.0 * half_h))),
        0.0, 1.0
    )
    env_y = mul_node(bottom_fade, top_fade)
    
    # 3. Z envelope: 3D undulating serpentine ribbon with harmonic wave folds
    # Grand sweeping arc + secondary folding creates natural line-of-sight limb brightening
    fold_1 = {
        "op": 0,
        "scalarForm": {
            "terms": [{
                "c": -float(z_amp1),
                "factors": {},
                "trans": [{
                    "kind": 0, # Sin
                    "var": "x",
                    "scale": float(z_freq1),
                    "shift": 0.0
                }]
            }]
        }
    }
    fold_2 = {
        "op": 0,
        "scalarForm": {
            "terms": [{
                "c": -float(z_amp2),
                "factors": {},
                "trans": [{
                    "kind": 1, # Cos
                    "var": "x",
                    "scale": float(z_freq2),
                    "shift": 0.5
                }]
            }]
        }
    }
    z_diff = add_node(add_node(var_node("z"), fold_1), fold_2)
    if z_amp3 != 0.0 and z_freq3 != 0.0:
        fold_3 = {
            "op": 0,
            "scalarForm": {
                "terms": [{
                    "c": -float(z_amp3),
                    "factors": {},
                    "trans": [{
                        "kind": 0, # Sin
                        "var": "x",
                        "scale": float(z_freq3),
                        "shift": 1.2
                    }]
                }]
            }
        }
        z_diff = add_node(z_diff, fold_3)
        
    env_z = clamp_node(
        sub_node(scalar_node(1.0), div_node(abs_node(z_diff), scalar_node(w_z))),
        0.0, 1.0
    )
    
    # 4. Vertical ray striations: Atmospheric drapery and folds (continuous glowing sheet, NOT searchlights!)
    # Continuous base drapery (0.68 to 1.0) with multi-frequency harmonic ripples:
    fluting = add_node(
        scalar_node(0.68),
        add_node(
            {
                "op": 0,
                "scalarForm": {
                    "terms": [{
                        "c": 0.20,
                        "factors": {},
                        "trans": [{
                            "kind": 1, # Cos
                            "var": "x",
                            "scale": float(ray_freq),
                            "shift": 0.0
                        }]
                    }]
                }
            },
            {
                "op": 0,
                "scalarForm": {
                    "terms": [{
                        "c": 0.12,
                        "factors": {},
                        "trans": [{
                            "kind": 1, # Cos
                            "var": "x",
                            "scale": float(ray_freq * 2.2),
                            "shift": 0.8
                        }]
                    }]
                }
            }
        )
    )
    
    # 5. Perlin wisps and turbulent cosmic eddies (feathers the edges without black holes)
    noise = clamp_node(
        add_node(
            scalar_node(0.70),
            scale_node(0.30, perlin_node(scale_node(noise_scale, var_node("p"))))
        ),
        0.0, 1.0
    )
    
    # Multiplied product: continuous sheet * drapery folds * turbulent cosmic wisps
    sheet = mul_node(env_x, mul_node(env_y, env_z))
    details = mul_node(fluting, noise)
    return mul_node(sheet, details)

def make_emissive_vec3(r, g, b, time_rate=0.3):
    """
    E_v(p, omega, t) = vec3(r, g, b) * (0.85 + 0.15 * cos(time_rate * t))
    Calibrated so (E_v / sigma_t) stays within [0.4, 0.85] range.
    No white blowout!
    """
    time_mod = add_node(
        scalar_node(0.85),
        {
            "op": 0,
            "scalarForm": {
                "terms": [{
                    "c": 0.15,
                    "factors": {},
                    "trans": [{
                        "kind": 1, # Cos
                        "var": "t",
                        "scale": float(time_rate),
                        "shift": 0.0
                    }]
                }]
            }
        }
    )
    r_val = mul_node(scalar_node(r), time_mod)
    g_val = mul_node(scalar_node(g), time_mod)
    b_val = mul_node(scalar_node(b), time_mod)
    return {
        "op": 2,
        "children": [r_val, g_val, b_val]
    }

def make_phase_forward(g_val=0.35):
    """
    Phi(p, wi, wo) = 1.0 + 3.0 * g * dot(wi, wo)
    """
    dot_x = mul_node(var_node("wi.x"), var_node("wo.x"))
    dot_y = mul_node(var_node("wi.y"), var_node("wo.y"))
    dot_z = mul_node(var_node("wi.z"), var_node("wo.z"))
    dot_term = add_node(dot_x, add_node(dot_y, dot_z))
    return add_node(scalar_node(1.0), scale_node(3.0 * g_val, dot_term))

def make_box_object(obj_id, center, dims, material_id, display_name, face_color):
    half_x, half_y, half_z = dims[0] / 2.0, dims[1] / 2.0, dims[2] / 2.0
    x, y, z = center[0], center[1], center[2]
    return {
        "objectID": str(obj_id),
        "center": [float(x), float(y), float(z)],
        "authoritativeAxis": [0, 1, 0],
        "geometryType": 10,
        "shapeKind": 10,
        "materialId": str(material_id),
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeParams": [1, 1, 1, float(half_y), 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": [
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            float(x), float(y), float(z), 1.0
        ],
        "x2D": 100,
        "y2D": 100,
        "zOrder2D": 0,
        "faceColors": [
            [float(face_color[0]), float(face_color[1]), float(face_color[2])],
            [float(face_color[0]), float(face_color[1]), float(face_color[2])],
            [float(face_color[0]), float(face_color[1]), float(face_color[2])],
            [float(face_color[0]), float(face_color[1]), float(face_color[2])],
            [float(face_color[0]), float(face_color[1]), float(face_color[2])],
            [float(face_color[0]), float(face_color[1]), float(face_color[2])]
        ],
        "field": {
            "op": 0,
            "prim": 1, # SdfPrim::Box
            "dims": [float(half_x), float(half_y), float(half_z)],
            "offset": [0, 0, 0],
            "p0": 0,
            "p1": 0,
            "t": 0.5
        },
        "fieldExtent": [float(half_x + 0.15), float(half_y + 0.15), float(half_z + 0.15)],
        "authoredProperties": {
            "displayName": {"t": "string", "v": str(display_name)}
        }
    }

def build_zone():
    spatial_fields = []
    
    # 1. Primary Emerald Aurora Curtain (557.7 nm atomic oxygen green)
    # Majestic undulating S-curve across the northern sky with luminous limb brightening (Ref 2, 3, 5)
    curtain_1_shape = make_curtain_shape(
        half_w=85.0, half_h=28.0, w_z=3.4,
        z_amp1=20.0, z_freq1=0.024,
        z_amp2=7.5, z_freq2=0.062,
        ray_freq=0.32, noise_scale=0.08,
        z_amp3=3.0, z_freq3=0.15
    )
    curtain_1_density = scale_node(0.75, curtain_1_shape)
    curtain_1_extinction = scale_node(0.12, curtain_1_shape)
    curtain_1_scattering = scale_node(0.04, curtain_1_shape)
    curtain_1_chroma = vec3_node(0.10, 0.98, 0.45)
    curtain_1_phase = make_phase_forward(0.40)
    # Radiant electric emerald with brilliant white-green core on limb folds
    curtain_1_emission = mul_node(curtain_1_shape, make_emissive_vec3(0.45, 3.40, 1.15, time_rate=0.32))
    
    spatial_fields.append({
        "id": "northern_veil.aurora.primary-emerald-curtain",
        "origin": [0.0, 68.0, 80.0],
        "scale": [180.0, 60.0, 64.0],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": piecewise(curtain_1_density),
        "volumeExtinction": piecewise(curtain_1_extinction),
        "volumeScattering": piecewise(curtain_1_scattering),
        "volumeChroma": piecewise(curtain_1_chroma),
        "volumePhase": piecewise(curtain_1_phase),
        "volumeEmission": piecewise(curtain_1_emission),
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Primary Emerald Aurora Curtain (557.7nm O-I)"},
            "aurora.role": {"t": "string", "v": "Dominant Oxygen Green Drapery"},
            "aurora.spectrum": {"t": "string", "v": "557.7 nm Forbidden Atomic Oxygen Green"},
            "volumetric.channels": {"t": "string", "v": "V0 D + V1 sigma_t + V2 sigma_s/C_v + V3 Phi + V4 E_v"}
        }
    })
    
    # 2. Secondary Cyan Ribbon (High altitude N2+ / O2+ ionization)
    # Graceful intertwining ribbon creating multi-layer celestial depth (Ref 2)
    curtain_2_shape = make_curtain_shape(
        half_w=75.0, half_h=26.0, w_z=3.0,
        z_amp1=-17.0, z_freq1=0.028,
        z_amp2=6.0, z_freq2=0.072,
        ray_freq=0.36, noise_scale=0.09,
        z_amp3=-2.5, z_freq3=0.18
    )
    curtain_2_density = scale_node(0.65, curtain_2_shape)
    curtain_2_extinction = scale_node(0.10, curtain_2_shape)
    curtain_2_scattering = scale_node(0.03, curtain_2_shape)
    curtain_2_chroma = vec3_node(0.08, 0.88, 0.98)
    curtain_2_phase = make_phase_forward(0.25)
    # Luminous electric turquoise / cyan ribbon
    curtain_2_emission = mul_node(curtain_2_shape, make_emissive_vec3(0.25, 2.30, 2.90, time_rate=0.40))
    
    spatial_fields.append({
        "id": "northern_veil.aurora.secondary-cyan-ribbon",
        "origin": [15.0, 80.0, 110.0],
        "scale": [160.0, 56.0, 56.0],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": piecewise(curtain_2_density),
        "volumeExtinction": piecewise(curtain_2_extinction),
        "volumeScattering": piecewise(curtain_2_scattering),
        "volumeChroma": piecewise(curtain_2_chroma),
        "volumePhase": piecewise(curtain_2_phase),
        "volumeEmission": piecewise(curtain_2_emission),
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Secondary Cyan Ionization Ribbon"},
            "aurora.role": {"t": "string", "v": "Upper Tropospheric Fast Ribbon"},
            "aurora.spectrum": {"t": "string", "v": "470.9 nm First Negative Nitrogen Band"},
            "volumetric.channels": {"t": "string", "v": "V0 D + V1 sigma_t + V2 sigma_s/C_v + V3 Phi + V4 E_v"}
        }
    })
    
    # 3. Accent Violet-Magenta Crest (High altitude N2 molecular corona)
    # Exospheric purple/magenta crown soaring high above the green drapery (Ref 1 & 4)
    curtain_3_shape = make_curtain_shape(
        half_w=80.0, half_h=24.0, w_z=4.0,
        z_amp1=15.0, z_freq1=0.022,
        z_amp2=6.5, z_freq2=0.055,
        ray_freq=0.25, noise_scale=0.07,
        z_amp3=2.0, z_freq3=0.12
    )
    curtain_3_density = scale_node(0.55, curtain_3_shape)
    curtain_3_extinction = scale_node(0.08, curtain_3_shape)
    curtain_3_scattering = scale_node(0.02, curtain_3_shape)
    curtain_3_chroma = vec3_node(0.88, 0.20, 0.95)
    curtain_3_phase = scalar_node(1.0)
    # Radiant celestial violet / hot magenta
    curtain_3_emission = mul_node(curtain_3_shape, make_emissive_vec3(2.50, 0.45, 2.80, time_rate=0.24))
    
    spatial_fields.append({
        "id": "northern_veil.aurora.accent-violet-crest",
        "origin": [-12.0, 102.0, 92.0],
        "scale": [170.0, 50.0, 60.0],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": piecewise(curtain_3_density),
        "volumeExtinction": piecewise(curtain_3_extinction),
        "volumeScattering": piecewise(curtain_3_scattering),
        "volumeChroma": piecewise(curtain_3_chroma),
        "volumePhase": piecewise(curtain_3_phase),
        "volumeEmission": piecewise(curtain_3_emission),
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Accent Violet-Magenta Celestial Crest"},
            "aurora.role": {"t": "string", "v": "Exospheric High-Altitude Corona"},
            "aurora.spectrum": {"t": "string", "v": "427.8 nm N2+ Molecular Violet/Magenta"},
            "volumetric.channels": {"t": "string", "v": "V0 D + V1 sigma_t + V2 sigma_s/C_v + V3 Phi + V4 E_v"}
        }
    })
    
    # 4. Towering Crimson Ray Pillar (630.0 nm atomic oxygen)
    # Majestic vertical salmon-rose / crimson ray column rising into the stars (Ref 1)
    curtain_4_shape = make_curtain_shape(
        half_w=65.0, half_h=30.0, w_z=3.2,
        z_amp1=13.0, z_freq1=0.030,
        z_amp2=5.0, z_freq2=0.085,
        ray_freq=0.28, noise_scale=0.10,
        z_amp3=2.0, z_freq3=0.16
    )
    curtain_4_density = scale_node(0.50, curtain_4_shape)
    curtain_4_extinction = scale_node(0.08, curtain_4_shape)
    curtain_4_scattering = scale_node(0.02, curtain_4_shape)
    curtain_4_chroma = vec3_node(0.98, 0.18, 0.32)
    curtain_4_phase = make_phase_forward(0.30)
    # Deep, glowing atomic oxygen crimson / salmon-rose
    curtain_4_emission = mul_node(curtain_4_shape, make_emissive_vec3(2.80, 0.40, 0.75, time_rate=0.36))
    
    spatial_fields.append({
        "id": "northern_veil.aurora.deep-crimson-fringe",
        "origin": [6.0, 76.0, 65.0],
        "scale": [140.0, 64.0, 48.0],
        "field": {"mode": "Procedural", "baseDensity": 1.0, "frequency": 1.0, "amplitude": 1.0},
        "vectorField": {"mode": "Procedural", "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0, "frequency": 1.0, "amplitude": 0.0},
        "volumeDensity": piecewise(curtain_4_density),
        "volumeExtinction": piecewise(curtain_4_extinction),
        "volumeScattering": piecewise(curtain_4_scattering),
        "volumeChroma": piecewise(curtain_4_chroma),
        "volumePhase": piecewise(curtain_4_phase),
        "volumeEmission": piecewise(curtain_4_emission),
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Deep Crimson Aurora Fringe (630.0nm O-I)"},
            "aurora.role": {"t": "string", "v": "Lower Ray Fringe"},
            "aurora.spectrum": {"t": "string", "v": "630.0 nm Excited Atomic Oxygen Red"},
            "volumetric.channels": {"t": "string", "v": "V0 D + V1 sigma_t + V2 sigma_s/C_v + V3 Phi + V4 E_v"}
        }
    })
    
    # --- Planetary Environment & Staging ---
    objects = []
    
    # 1. Mirror Lake ice segments (Z from -20 to 220, X from -30 to 30)
    for z in range(-20, 220, 40):
        for x in [-30.0, 30.0]:
            obj = make_box_object(
                obj_id=f"northern_veil.ice.plate_x{int(x)}_z{z}",
                center=[x, -0.6, float(z)],
                dims=[60.0, 1.0, 40.0],
                material_id="material.cathedral_basalt",
                display_name=f"Obsidian Mirror Lake Plate (Z={z})",
                face_color=[0.02, 0.03, 0.05]
            )
            obj["authoredProperties"]["material.specular"] = {"t": "float", "v": 0.88}
            obj["authoredProperties"]["material.diffuse"] = {"t": "float", "v": 0.12}
            objects.append(obj)
    
    # 2. Central Observation Dais at [0, 0, 0]
    dais_obj = make_box_object(
        obj_id="northern_veil.dais.platform",
        center=[0.0, -0.4, 0.0],
        dims=[16.0, 0.8, 16.0],
        material_id="material.cathedral_basalt",
        display_name="Observation Dais (Spawn)",
        face_color=[0.08, 0.10, 0.14]
    )
    dais_obj["authoredProperties"]["material.specular"] = {"t": "float", "v": 0.45}
    dais_obj["authoredProperties"]["material.diffuse"] = {"t": "float", "v": 0.55}
    objects.append(dais_obj)
    
    # 3. Perimeter Mountain Silhouettes (West at X=-55, East at X=+55)
    for i, z in enumerate([30.0, 70.0, 110.0, 150.0, 190.0]):
        # West mountain
        m_west = make_box_object(
            obj_id=f"northern_veil.mountain.west_{i}",
            center=[-55.0, 12.0 + (i % 3) * 4.0, z],
            dims=[28.0, 36.0 + (i % 2) * 12.0, 36.0],
            material_id="material.cathedral_basalt",
            display_name=f"Western Ridge Silhouette {i+1}",
            face_color=[0.015, 0.02, 0.03]
        )
        objects.append(m_west)
        # East mountain
        m_east = make_box_object(
            obj_id=f"northern_veil.mountain.east_{i}",
            center=[55.0, 14.0 + ((i + 1) % 3) * 4.0, z],
            dims=[28.0, 40.0 + (i % 2) * 8.0, 36.0],
            material_id="material.cathedral_basalt",
            display_name=f"Eastern Ridge Silhouette {i+1}",
            face_color=[0.015, 0.02, 0.03]
        )
        objects.append(m_east)
        
    # 4. Inscribed Stele of Volumetric Sovereignty
    stele_obj = make_box_object(
        obj_id="northern_veil.stele.monolith",
        center=[0.0, 1.6, 10.0],
        dims=[2.8, 5.6, 1.2],
        material_id="material.cathedral_basalt",
        display_name="Stele of the Northern Veil (V0-V4 Sovereignty)",
        face_color=[0.14, 0.16, 0.22]
    )
    stele_obj["authoredProperties"]["inscription.constitution"] = {
        "t": "string",
        "v": "rho_source != V_transport != D_medium; D != sigma_t != sigma_s != C_v != Phi != E_v"
    }
    stele_obj["authoredProperties"]["inscription.authors"] = {
        "t": "string",
        "v": "Zachary Zhang & Gemini Spark"
    }
    objects.append(stele_obj)

    zone = {
        "identifier": "Northern Veil",
        "name": "Northern Veil",
        "authors": [
            "Zachary Zhang",
            "Gemini Spark"
        ],
        "injected_by": "Gemini Spark",
        "scope": "global",
        "deletable": False,
        "qualities": [
            "aurora",
            "volumetric",
            "night",
            "participating_media",
            "ontomath",
            "luminous",
            "showcase",
            "v0-v4",
            "northern_veil"
        ],
        "parentZone": "",
        "owner": "default",
        "spatialRoot": {
            "id": "northern_veil.celestial-vault-root",
            "origin": [0.0, 95.0, 80.0],
            "scale": [4.0, 4.0, 1.5],
            "field": {
                "mode": "AST",
                "baseDensity": 1,
                "frequency": 1,
                "amplitude": 1,
                "ast": {
                    "input": "x",
                    "pieces": [{
                        "hasLo": False,
                        "hasHi": False,
                        "mathNode": {
                            "op": 0,
                            "scalarForm": {
                                "terms": [{
                                    "c": 0.0,
                                    "factors": {}
                                }]
                            }
                        }
                    }]
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
                "displayName": {"t": "string", "v": "Starlit Arctic Night Sky"},
                "light.source": {"t": "bool", "v": True},
                "light.ambient": {"t": "float", "v": 0.025},
                "light.diffuse": {"t": "float", "v": 0.18},
                "light.specular": {"t": "float", "v": 0.12},
                "light.intensity": {"t": "float", "v": 0.35},
                "light.color": {"t": "vec3", "v": [0.06, 0.08, 0.16]}
            }
        },
        "spatialFields": spatial_fields,
        "materials": {},
        "world": {
            "objects": objects
        },
        "formationRelations": [],
        "lexemes": []
    }
    
    out_dir = "saves/zones/Northern Veil"
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "zone.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(zone, f, indent=2)
    print(f"Generated {out_path} successfully!")
    print(f"  - Spatial fields: {len(spatial_fields)}")
    print(f"  - Objects: {len(objects)}")

if __name__ == "__main__":
    build_zone()
