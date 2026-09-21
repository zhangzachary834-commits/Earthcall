#!/usr/bin/env python3
"""
author_sky_celestial_singularity.py

Authors the Celestial Radiant Orrery and Astral Singularity in the sky
above the 28-Room Radiance Gallery.
"""

import json, math, os

def mat4_identity():
    return [1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0]

def mat4_translate(tx, ty, tz):
    return [1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            tx,  ty,  tz,  1.0]

def mat4_mul(A, B):
    C = [0.0] * 16
    for col in range(4):
        for row in range(4):
            val = 0.0
            for k in range(4):
                val += A[k * 4 + row] * B[col * 4 + k]
            C[col * 4 + row] = val
    return C

def mat4_rotate_x(rad):
    c = math.cos(rad)
    s = math.sin(rad)
    return [1.0, 0.0, 0.0, 0.0,
            0.0,   c,   s, 0.0,
            0.0,  -s,   c, 0.0,
            0.0, 0.0, 0.0, 1.0]

def mat4_rotate_y(rad):
    c = math.cos(rad)
    s = math.sin(rad)
    return [  c, 0.0,  -s, 0.0,
            0.0, 1.0, 0.0, 0.0,
              s, 0.0,   c, 0.0,
            0.0, 0.0, 0.0, 1.0]

def mat4_rotate_z(rad):
    c = math.cos(rad)
    s = math.sin(rad)
    return [  c,   s, 0.0, 0.0,
             -s,   c, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0]

def make_sdf_object(obj_id, material_id, center, transform, prim, dims, extent, display_name=None, role=None):
    obj = {
        "authoritativeAxis": [0, 1, 0],
        "center": center,
        "faceColors": [[1, 1, 1]] * 6,
        "geometryType": 10,
        "materialId": material_id,
        "objectID": obj_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeKind": 10,
        "drawFieldModel": True,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": transform,
        "x2D": 100,
        "y2D": 100,
        "zOrder2D": 0,
        "field": {
            "op": 0,
            "prim": prim,
            "dims": dims,
            "offset": [0, 0, 0],
            "p0": dims[0] if prim == 0 else 0,
            "p1": 0,
            "t": 0.5
        },
        "fieldExtent": extent
    }
    if display_name or role:
        obj["authoredProperties"] = {}
        if display_name:
            obj["authoredProperties"]["displayName"] = {"t": "string", "v": display_name}
        if role:
            obj["authoredProperties"]["celestial.role"] = {"t": "string", "v": role}
    return obj

def build_sky_celestial_apparatus():
    objects = []
    OX, OY, OZ = 0.0, 52.0, 87.5

    # 1. Singularity Core (Obsidian Sphere)
    T_core = mat4_translate(OX, OY, OZ)
    objects.append(make_sdf_object(
        "celestial-nucleus-core",
        "celestial.obsidian.core",
        [OX, OY, OZ],
        T_core,
        prim=0, # Sphere
        dims=[4.5, 4.5, 4.5],
        extent=[5.5, 5.5, 5.5],
        display_name="Astral Singularity Nucleus",
        role="singularity-core"
    ))

    # 2. Outer Coronal Focus Shell
    objects.append(make_sdf_object(
        "celestial-nucleus-corona",
        "celestial.gold.stellar",
        [OX, OY, OZ],
        T_core,
        prim=0, # Sphere
        dims=[5.6, 5.6, 5.6],
        extent=[6.5, 6.5, 6.5],
        display_name="Radiant Singularity Corona",
        role="singularity-corona"
    ))

    # 3. Gyroscopic Armillary Rings
    rx90 = mat4_rotate_x(math.pi / 2.0)

    # Ring Alpha (Equatorial Accretion Ring)
    T_ring_alpha = mat4_mul(mat4_translate(OX, OY, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-ring-alpha-equatorial",
        "celestial.gold.stellar",
        [OX, OY, OZ],
        T_ring_alpha,
        prim=6, # Torus
        dims=[20.0, 0.85, 0.0],
        extent=[22.0, 22.0, 2.5],
        display_name="Astrolabe Ring Alpha — Equatorial Accretion",
        role="armillary-ring"
    ))

    # Ring Beta (Ecliptic Ring)
    T_rot_beta = mat4_mul(mat4_rotate_z(math.pi / 4.0), rx90)
    T_ring_beta = mat4_mul(mat4_translate(OX, OY, OZ), T_rot_beta)
    objects.append(make_sdf_object(
        "celestial-ring-beta-ecliptic",
        "celestial.bronze.armillary",
        [OX, OY, OZ],
        T_ring_beta,
        prim=6, # Torus
        dims=[26.0, 0.70, 0.0],
        extent=[28.5, 28.5, 2.5],
        display_name="Astrolabe Ring Beta — Ecliptic Chrono-Ring",
        role="armillary-ring"
    ))

    # Ring Gamma (Meridian Ring)
    T_rot_gamma = mat4_mul(mat4_rotate_x(math.pi / 4.0), mat4_identity())
    T_ring_gamma = mat4_mul(mat4_translate(OX, OY, OZ), T_rot_gamma)
    objects.append(make_sdf_object(
        "celestial-ring-gamma-meridian",
        "celestial.gold.stellar",
        [OX, OY, OZ],
        T_ring_gamma,
        prim=6, # Torus
        dims=[32.0, 0.65, 0.0],
        extent=[34.5, 34.5, 2.5],
        display_name="Astrolabe Ring Gamma — Polar Meridian Gyro",
        role="armillary-ring"
    ))

    # Ring Delta (Great Zodiac Outer Ring)
    ry90 = mat4_rotate_y(math.pi / 2.0)
    T_ring_delta = mat4_mul(mat4_translate(OX, OY, OZ), ry90)
    objects.append(make_sdf_object(
        "celestial-ring-delta-zodiac",
        "celestial.marble.astral",
        [OX, OY, OZ],
        T_ring_delta,
        prim=6, # Torus
        dims=[38.0, 0.90, 0.0],
        extent=[41.0, 41.0, 3.0],
        display_name="Astrolabe Ring Delta — Great Outer Zodiac Ring",
        role="armillary-ring"
    ))

    # 4. 12 Zodiac Nodal Cubes
    for i in range(12):
        angle = i * (2.0 * math.pi / 12.0)
        ny = OY + 38.0 * math.cos(angle)
        nz = OZ + 38.0 * math.sin(angle)
        nx = OX
        T_node = mat4_translate(nx, ny, nz)
        objects.append(make_sdf_object(
            f"celestial-zodiac-node-{i+1:02d}",
            "celestial.ruby.nodal",
            [nx, ny, nz],
            T_node,
            prim=1, # Box
            dims=[0.9, 0.9, 0.9],
            extent=[1.4, 1.4, 1.4],
            display_name=f"Zodiac Marker Nodal Prism {i+1}",
            role="zodiac-node"
        ))

    # 5. 8 Resonant Floating Monoliths
    for i in range(8):
        angle = i * (2.0 * math.pi / 8.0)
        px = OX + 27.0 * math.cos(angle)
        pz = OZ + 27.0 * math.sin(angle)
        py = OY
        R_yaw = mat4_rotate_y(-angle)
        T_monolith = mat4_mul(mat4_translate(px, py, pz), R_yaw)
        objects.append(make_sdf_object(
            f"celestial-pylon-{i+1:02d}",
            "celestial.crystal.sapphire",
            [px, py, pz],
            T_monolith,
            prim=2, # RoundBox
            dims=[3.5, 0.9, 0.9],
            extent=[4.5, 1.6, 1.6],
            display_name=f"Resonant Gravitational Pylon {i+1}",
            role="gravitational-pylon"
        ))

    # 6. Upper & Lower Relativistic Jet Collars
    T_upper_collar = mat4_mul(mat4_translate(OX, OY + 16.0, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-jet-collar-zenith",
        "celestial.gold.stellar",
        [OX, OY + 16.0, OZ],
        T_upper_collar,
        prim=6, # Torus
        dims=[9.0, 0.60, 0.0],
        extent=[11.0, 11.0, 2.0],
        display_name="Zenith Relativistic Jet Collar",
        role="jet-collar"
    ))

    T_lower_collar = mat4_mul(mat4_translate(OX, OY - 16.0, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-jet-collar-nadir",
        "celestial.gold.stellar",
        [OX, OY - 16.0, OZ],
        T_lower_collar,
        prim=6, # Torus
        dims=[9.0, 0.60, 0.0],
        extent=[11.0, 11.0, 2.0],
        display_name="Nadir Relativistic Jet Collar",
        role="jet-collar"
    ))

    # 7. The Firmament Dais (Skybridge Observation Deck at Y = 22.0m)
    DY = 22.0
    bridge_extents = [
        ("north", [OX, DY, OZ + 18.0], [3.0, 0.35, 7.0]),
        ("south", [OX, DY, OZ - 18.0], [3.0, 0.35, 7.0]),
        ("east",  [OX + 18.0, DY, OZ], [7.0, 0.35, 3.0]),
        ("west",  [OX - 18.0, DY, OZ], [7.0, 0.35, 3.0]),
    ]
    for bname, bcenter, bdims in bridge_extents:
        Tb = mat4_translate(*bcenter)
        objects.append(make_sdf_object(
            f"skybridge-deck-{bname}",
            "celestial.marble.astral",
            bcenter,
            Tb,
            prim=1, # Box
            dims=bdims,
            extent=[bdims[0] + 0.5, bdims[1] + 0.5, bdims[2] + 0.5],
            display_name=f"Firmament Dais Deck — {bname.capitalize()}",
            role="skybridge-deck"
        ))
        p_offset = 5.5
        if bname == "north": px, pz = OX, OZ + 18.0 + p_offset
        elif bname == "south": px, pz = OX, OZ - 18.0 - p_offset
        elif bname == "east": px, pz = OX + 18.0 + p_offset, OZ
        elif bname == "west": px, pz = OX - 18.0 - p_offset, OZ
        Tp = mat4_translate(px, DY + 0.75, pz)
        objects.append(make_sdf_object(
            f"skybridge-pedestal-{bname}",
            "celestial.gold.stellar",
            [px, DY + 0.75, pz],
            Tp,
            prim=4, # Cylinder
            dims=[0.8, 0.75, 0.8],
            extent=[1.2, 1.2, 1.2],
            display_name=f"Celestial Observation Dais — {bname.capitalize()}",
            role="skybridge-pedestal"
        ))

    T_dais_ring = mat4_mul(mat4_translate(OX, DY - 0.2, OZ), rx90)
    objects.append(make_sdf_object(
        "skybridge-central-aperture-ring",
        "celestial.bronze.armillary",
        [OX, DY - 0.2, OZ],
        T_dais_ring,
        prim=6, # Torus
        dims=[12.0, 1.2, 0.0],
        extent=[14.5, 14.5, 2.2],
        display_name="Firmament Dais Central Oculus Rim",
        role="skybridge-rim"
    ))

    T_oculus_rim = mat4_mul(mat4_translate(OX, 0.2, OZ), rx90)
    objects.append(make_sdf_object(
        "gallery-sky-oculus-architrave",
        "celestial.gold.stellar",
        [OX, 0.2, OZ],
        T_oculus_rim,
        prim=6, # Torus
        dims=[8.5, 0.8, 0.0],
        extent=[10.5, 10.5, 2.0],
        display_name="Grand Celestial Oculus Architrave",
        role="oculus-architrave"
    ))

    return objects

def create_celestial_spatial_root():
    OX, OY, OZ = 0.0, 52.0, 87.5
    rho_ast = {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 1,
                    "children": [
                        {
                            "op": 3,
                            "children": [
                                {
                                    "op": 23,
                                    "children": [
                                        {"op": 0, "scalarForm": {"terms": [{"c": 6.8, "factors": {}}]}},
                                        {
                                            "op": 0,
                                            "scalarForm": {
                                                "terms": [
                                                    {"c": 1.0, "factors": {}},
                                                    {"c": 0.006, "factors": {"x": 2}},
                                                    {"c": 0.006, "factors": {"y": 2}},
                                                    {"c": 0.006, "factors": {"z": 2}}
                                                ]
                                            }
                                        }
                                    ]
                                },
                                {
                                    "op": 1,
                                    "children": [
                                        {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {}}]}},
                                        {
                                            "op": 3,
                                            "children": [
                                                {"op": 0, "scalarForm": {"terms": [{"c": 0.35, "factors": {}}]}},
                                                {
                                                    "op": 9,
                                                    "children": [
                                                        {"op": 0, "scalarForm": {"terms": [{"c": 2.4, "factors": {"t": 1}}]}}
                                                    ]
                                                }
                                            ]
                                        }
                                    ]
                                }
                            ]
                        },
                        {
                            "op": 3,
                            "children": [
                                {"op": 0, "scalarForm": {"terms": [{"c": 0.65, "factors": {}}]}},
                                {
                                    "op": 10,
                                    "children": [
                                        {
                                            "op": 0,
                                            "scalarForm": {
                                                "terms": [
                                                    {"c": 0.35, "factors": {"x": 1}},
                                                    {"c": -2.8, "factors": {"t": 1}}
                                                ]
                                            }
                                        }
                                    ]
                                }
                            ]
                        },
                        {
                            "op": 23,
                            "children": [
                                {"op": 0, "scalarForm": {"terms": [{"c": 4.2, "factors": {}}]}},
                                {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {}},
                                            {"c": 0.15, "factors": {"x": 2}},
                                            {"c": 0.15, "factors": {"z": 2}}
                                        ]
                                    }
                                }
                            ]
                        }
                    ]
                }
            }
        ]
    }

    chi_ast = {
        "input": "x",
        "pieces": [
            {
                "vectorForm": {
                    "x": {
                        "op": 1,
                        "children": [
                            {"op": 0, "scalarForm": {"terms": [{"c": 0.85, "factors": {}}]}},
                            {
                                "op": 3,
                                "children": [
                                    {"op": 0, "scalarForm": {"terms": [{"c": 0.60, "factors": {}}]}},
                                    {
                                        "op": 10,
                                        "children": [
                                            {"op": 0, "scalarForm": {"terms": [{"c": 0.12, "factors": {"x": 1}}, {"c": -1.5, "factors": {"t": 1}}]}}
                                        ]
                                    }
                                ]
                            }
                        ]
                    },
                    "y": {
                        "op": 1,
                        "children": [
                            {"op": 0, "scalarForm": {"terms": [{"c": 0.65, "factors": {}}]}},
                            {
                                "op": 3,
                                "children": [
                                    {"op": 0, "scalarForm": {"terms": [{"c": 0.50, "factors": {}}]}},
                                    {
                                        "op": 10,
                                        "children": [
                                            {"op": 0, "scalarForm": {"terms": [{"c": 0.12, "factors": {"x": 1}}, {"c": -1.5, "factors": {"t": 1}}, {"c": -2.09, "factors": {}}]}}
                                        ]
                                    }
                                ]
                            }
                        ]
                    },
                    "z": {
                        "op": 1,
                        "children": [
                            {"op": 0, "scalarForm": {"terms": [{"c": 0.95, "factors": {}}]}},
                            {
                                "op": 3,
                                "children": [
                                    {"op": 0, "scalarForm": {"terms": [{"c": 0.55, "factors": {}}]}},
                                    {
                                        "op": 10,
                                        "children": [
                                            {"op": 0, "scalarForm": {"terms": [{"c": 0.12, "factors": {"x": 1}}, {"c": -1.5, "factors": {"t": 1}}, {"c": -4.18, "factors": {}}]}}
                                        ]
                                    }
                                ]
                            }
                        ]
                    }
                }
            }
        ]
    }

    alpha_ast = {
        "input": "omega",
        "pieces": [
            {
                "mathNode": {
                    "op": 1,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.50, "factors": {}}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 3.2, "factors": {"y": 4}}]}},
                        {
                            "op": 3,
                            "children": [
                                {"op": 0, "scalarForm": {"terms": [{"c": 4.5, "factors": {}}]}},
                                {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 0.5, "factors": {"x": 2}},
                                            {"c": 0.5, "factors": {"z": 2}}
                                        ]
                                    }
                                }
                            ]
                        }
                    ]
                }
            }
        ]
    }

    spatial_root = {
        "position": [OX, OY, OZ],
        "scale": [1.0, 1.0, 1.0],
        "authoredProperties": {
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.intensity": {"t": "float", "v": 3.2},
            "light.color": {"t": "vec3", "x": 1.0, "y": 0.94, "z": 0.82},
            "light.ambient": {"t": "float", "v": 0.28},
            "light.diffuse": {"t": "float", "v": 0.85},
            "light.specular": {"t": "float", "v": 1.2},
            "light.attenuation.constant": {"t": "float", "v": 1.0},
            "light.attenuation.linear": {"t": "float", "v": 0.0},
            "light.attenuation.quadratic": {"t": "float", "v": 0.0}
        },
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": rho_ast
        },
        "lightChroma": chi_ast,
        "lightAngular": alpha_ast
    }

    return spatial_root

def get_celestial_materials():
    return [
        {
            "ambient": 0.1,
            "baseColor": [0.06, 0.05, 0.08],
            "diffuse": 0.4,
            "emissive": [0.0, 0.0, 0.0],
            "name": "celestial.obsidian.core",
            "roughness": 0.05,
            "specular": 1.6
        },
        {
            "ambient": 0.25,
            "baseColor": [0.98, 0.85, 0.42],
            "diffuse": 0.9,
            "emissive": [0.15, 0.12, 0.05],
            "name": "celestial.gold.stellar",
            "roughness": 0.2,
            "specular": 1.4
        },
        {
            "ambient": 0.2,
            "baseColor": [0.82, 0.58, 0.35],
            "diffuse": 0.85,
            "emissive": [0.05, 0.03, 0.01],
            "name": "celestial.bronze.armillary",
            "roughness": 0.25,
            "specular": 1.0
        },
        {
            "ambient": 0.2,
            "baseColor": [0.22, 0.55, 0.98],
            "diffuse": 0.8,
            "emissive": [0.1, 0.2, 0.4],
            "name": "celestial.crystal.sapphire",
            "roughness": 0.1,
            "specular": 1.5
        },
        {
            "ambient": 0.2,
            "baseColor": [0.94, 0.94, 0.98],
            "diffuse": 0.85,
            "emissive": [0.0, 0.0, 0.0],
            "name": "celestial.marble.astral",
            "roughness": 0.15,
            "specular": 0.9
        },
        {
            "ambient": 0.3,
            "baseColor": [0.95, 0.15, 0.28],
            "diffuse": 0.9,
            "emissive": [0.35, 0.05, 0.10],
            "name": "celestial.ruby.nodal",
            "roughness": 0.1,
            "specular": 1.6
        }
    ]

def main():
    print("Generating Sky Celestial Apparatus...")
    sky_objects = build_sky_celestial_apparatus()
    print(f"Generated {len(sky_objects)} celestial sky objects.")

    celestial_root = create_celestial_spatial_root()
    celestial_materials = get_celestial_materials()

    # 1. Update Radiance Gallery zone.json
    zone_path = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/zones/Radiance Gallery/zone.json"
    with open(zone_path, "r") as f:
        gallery_zone = json.load(f)

    existing_mat_names = {m["name"] for m in gallery_zone.get("materials", [])}
    for m in celestial_materials:
        if m["name"] not in existing_mat_names:
            gallery_zone.setdefault("materials", []).append(m)

    existing_objs = gallery_zone.get("world", {}).get("objects", [])
    base_objs = [o for o in existing_objs if not o["objectID"].startswith("celestial-") and not o["objectID"].startswith("skybridge-") and not o["objectID"].startswith("gallery-sky-")]
    total_objs = base_objs + sky_objects
    gallery_zone["world"]["objects"] = total_objs
    gallery_zone["objects"] = total_objs # Dual-read compatibility!
    gallery_zone["spatialRoot"] = celestial_root

    with open(zone_path, "w") as f:
        json.dump(gallery_zone, f, indent=2)
    print(f"Updated {zone_path} with {len(total_objs)} total objects.")

    # 2. Update radiance_gallery.json world
    world_path = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/worlds/radiance_gallery.json"
    gallery_world = {
        "saveFormat": "zone-identity-v1",
        "identifier": "radiance_gallery",
        "name": "Radiance Gallery & Astral Singularity",
        "owner": "zacharyzhang",
        "description": "The 28-Room OntoMath Radiance Demonstration Gallery beneath the Celestial Astral Orrery in the Sky.",
        "currentZone": 0,
        "currentZoneId": "Radiance Gallery",
        "cameraPos": [0.0, 24.5, 65.0],      # Situated on the Firmament Dais Skybridge!
        "cameraFront": [0.0, 0.45, 0.89],     # Looking up at pitch ~27 deg into the turning celestial rings!
        "cameraUp": [0.0, 1.0, 0.0],
        "flying": True,
        "pitch": 27.0,
        "yaw": 90.0,
        "zoneRefs": [
            {
                "identifier": "Radiance Gallery",
                "kind": "gallery"
            }
        ],
        "zones": [
            gallery_zone
        ],
        "objects": total_objs,
        "materials": gallery_zone["materials"],
        "authoredLaws": {},
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Gemini Spark"],
        "injected_by": "Gemini Spark"
    }
    with open(world_path, "w") as f:
        json.dump(gallery_world, f, indent=2)
    print(f"Updated {world_path}.")

    # 3. Create standalone Celestial World: celestial_radiance_engine.json
    standalone_world_path = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/worlds/celestial_radiance_engine.json"
    standalone_zone_dir = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/zones/Celestial Radiance Engine"
    os.makedirs(standalone_zone_dir, exist_ok=True)
    standalone_zone_path = os.path.join(standalone_zone_dir, "zone.json")
    standalone_zone = {
        "identifier": "Celestial Radiance Engine",
        "name": "Celestial Radiance Engine",
        "owner": "zacharyzhang",
        "parentZone": "",
        "scope": "public",
        "deletable": True,
        "spatialRoot": celestial_root,
        "materials": celestial_materials,
        "world": {
            "objects": sky_objects,
            "laws": [],
            "relations": []
        },
        "objects": sky_objects, # Dual-read compatibility!
        "authoredLaws": {},
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Gemini Spark"],
        "injected_by": "Gemini Spark"
    }
    with open(standalone_zone_path, "w") as f:
        json.dump(standalone_zone, f, indent=2)
    print(f"Wrote standalone zone: {standalone_zone_path}.")

    standalone_world = {
        "saveFormat": "zone-identity-v1",
        "identifier": "celestial_radiance_engine",
        "name": "Celestial Radiance Engine",
        "owner": "zacharyzhang",
        "description": "The Hyper-Dimensional Celestial Singularity and Gyroscopic Astral Orrery in the Sky.",
        "cameraPos": [0.0, 24.5, 65.0],
        "cameraFront": [0.0, 0.45, 0.89],
        "cameraUp": [0.0, 1.0, 0.0],
        "flying": True,
        "pitch": 27.0,
        "yaw": 90.0,
        "currentZone": 0,
        "currentZoneId": "Celestial Radiance Engine",
        "zoneRefs": [
            {
                "identifier": "Celestial Radiance Engine",
                "kind": "celestial"
            }
        ],
        "zones": [
            standalone_zone
        ],
        "objects": sky_objects,
        "materials": celestial_materials,
        "authoredLaws": {},
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Gemini Spark"],
        "injected_by": "Gemini Spark"
    }
    with open(standalone_world_path, "w") as f:
        json.dump(standalone_world, f, indent=2)
    print(f"Wrote standalone world: {standalone_world_path}.")

if __name__ == "__main__":
    main()
