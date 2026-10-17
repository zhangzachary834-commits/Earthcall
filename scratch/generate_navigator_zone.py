#!/usr/bin/env python3
"""Generate the Personal Ourverse Navigator Zone and Law data.

Implements Zach's specification in docs/Agenda/Tasks/Specific Tasks/Zones and Ourverse/Personal_Ourverse_Navigator/Personal_Ourverse_Navigator.md:
"beautiful personal Ourverse navigator with little 2D/3D islands representing each world."
"""
import json
import os
from pathlib import Path

def create_transform(x, y, z, sx=1.0, sy=1.0, sz=1.0):
    # 4x4 column-major matrix
    return [
        sx, 0.0, 0.0, 0.0,
        0.0, sy, 0.0, 0.0,
        0.0, 0.0, sz, 0.0,
        x,   y,   z,   1.0
    ]

def make_obj(object_id, material_id, shape_kind, shape_params, center, scale=(1.0, 1.0, 1.0), authored_props=None):
    tx, ty, tz = center
    sx, sy, sz = scale
    obj = {
        "objectID": object_id,
        "materialId": material_id,
        "shapeKind": shape_kind,
        "geometryType": shape_kind,
        "shapeParams": shape_params,
        "center": [tx, ty, tz],
        "transform": create_transform(tx, ty, tz, sx, sy, sz),
        "authoritativeAxis": [0.0, 1.0, 0.0],
        "rotationResponsiveness": 10.0,
        "renderMode": 0,
        "targetRotation": [0.0, 0.0, 0.0],
        "faceColors": [[1.0, 1.0, 1.0] for _ in range(6)]
    }
    if authored_props:
        obj["authoredProperties"] = authored_props
    return obj

def build_navigator_zone():
    materials = [
        # Sanctum
        {"name": "navigator.sanctum.base", "baseColor": [0.78, 0.86, 0.45], "ambient": 0.35, "diffuse": 0.80, "specular": 0.3, "shininess": 16.0, "opacity": 1.0},
        {"name": "navigator.sanctum.rock", "baseColor": [0.55, 0.48, 0.38], "ambient": 0.25, "diffuse": 0.75, "specular": 0.2, "shininess": 8.0, "opacity": 1.0},
        {"name": "navigator.sanctum.gold", "baseColor": [0.98, 0.84, 0.28], "ambient": 0.40, "diffuse": 0.90, "specular": 1.0, "shininess": 64.0, "opacity": 1.0},
        {"name": "navigator.sanctum.beacon", "baseColor": [1.0, 0.92, 0.40], "ambient": 0.65, "diffuse": 1.0, "specular": 1.0, "shininess": 128.0, "opacity": 1.0},
        # Temple
        {"name": "navigator.temple.base", "baseColor": [0.24, 0.32, 0.55], "ambient": 0.30, "diffuse": 0.70, "specular": 0.8, "shininess": 48.0, "opacity": 1.0},
        {"name": "navigator.temple.rock", "baseColor": [0.18, 0.22, 0.32], "ambient": 0.20, "diffuse": 0.65, "specular": 0.2, "shininess": 8.0, "opacity": 1.0},
        {"name": "navigator.temple.silver", "baseColor": [0.82, 0.86, 0.95], "ambient": 0.40, "diffuse": 0.85, "specular": 1.0, "shininess": 80.0, "opacity": 1.0},
        {"name": "navigator.temple.beacon", "baseColor": [0.35, 0.65, 1.0], "ambient": 0.65, "diffuse": 1.0, "specular": 1.0, "shininess": 128.0, "opacity": 1.0},
        # Cavern
        {"name": "navigator.cavern.base", "baseColor": [0.15, 0.20, 0.26], "ambient": 0.25, "diffuse": 0.70, "specular": 0.4, "shininess": 16.0, "opacity": 1.0},
        {"name": "navigator.cavern.rock", "baseColor": [0.12, 0.14, 0.18], "ambient": 0.18, "diffuse": 0.60, "specular": 0.1, "shininess": 8.0, "opacity": 1.0},
        {"name": "navigator.cavern.crystal", "baseColor": [0.15, 0.95, 0.78], "ambient": 0.55, "diffuse": 0.95, "specular": 1.0, "shininess": 96.0, "opacity": 1.0},
        {"name": "navigator.cavern.beacon", "baseColor": [0.20, 1.0, 0.85], "ambient": 0.65, "diffuse": 1.0, "specular": 1.0, "shininess": 128.0, "opacity": 1.0},
        # Forge
        {"name": "navigator.forge.base", "baseColor": [0.18, 0.16, 0.18], "ambient": 0.25, "diffuse": 0.70, "specular": 0.3, "shininess": 16.0, "opacity": 1.0},
        {"name": "navigator.forge.magma", "baseColor": [0.98, 0.45, 0.12], "ambient": 0.70, "diffuse": 1.0, "specular": 0.6, "shininess": 32.0, "opacity": 1.0},
        {"name": "navigator.forge.bronze", "baseColor": [0.78, 0.52, 0.22], "ambient": 0.35, "diffuse": 0.80, "specular": 0.9, "shininess": 50.0, "opacity": 1.0},
        {"name": "navigator.forge.beacon", "baseColor": [1.0, 0.38, 0.15], "ambient": 0.65, "diffuse": 1.0, "specular": 1.0, "shininess": 128.0, "opacity": 1.0},
        # Plinth
        {"name": "navigator.plinth.deck", "baseColor": [0.88, 0.90, 0.95], "ambient": 0.35, "diffuse": 0.80, "specular": 0.9, "shininess": 60.0, "opacity": 1.0},
        {"name": "navigator.plinth.gold", "baseColor": [0.92, 0.78, 0.30], "ambient": 0.40, "diffuse": 0.90, "specular": 1.0, "shininess": 70.0, "opacity": 1.0}
    ]

    objects = []

    # 1. Central Observation Plinth
    objects.append(make_obj(
        "navigator-plinth-center", "material.navigator.plinth.deck",
        0, [2.5, 0.2, 2.5], [0.0, -0.2, 0.0], [5.0, 0.4, 5.0]
    ))
    objects.append(make_obj(
        "navigator-compass-ring", "material.navigator.plinth.gold",
        0, [2.6, 0.05, 2.6], [0.0, 0.02, 0.0], [5.2, 0.1, 5.2]
    ))

    # 2. Island 1: Sanctum of Beginnings (North-West, ~-10, 1.5, -12)
    s_x, s_y, s_z = -10.0, 1.2, -12.0
    objects.append(make_obj(
        "island-sanctum-base", "material.navigator.sanctum.base",
        0, [2.5, 0.4, 2.5], [s_x, s_y, s_z], [5.0, 0.8, 5.0]
    ))
    objects.append(make_obj(
        "island-sanctum-crag", "material.navigator.sanctum.rock",
        0, [2.0, 0.6, 2.0], [s_x, s_y - 0.7, s_z], [4.0, 1.2, 4.0]
    ))
    objects.append(make_obj(
        "island-sanctum-spire", "material.navigator.sanctum.gold",
        0, [0.4, 1.8, 0.4], [s_x, s_y + 2.2, s_z - 0.8], [0.8, 3.6, 0.8]
    ))
    objects.append(make_obj(
        "island-sanctum-pillar-l", "material.navigator.sanctum.gold",
        0, [0.2, 1.1, 0.2], [s_x - 1.2, s_y + 1.5, s_z], [0.4, 2.2, 0.4]
    ))
    objects.append(make_obj(
        "island-sanctum-pillar-r", "material.navigator.sanctum.gold",
        0, [0.2, 1.1, 0.2], [s_x + 1.2, s_y + 1.5, s_z], [0.4, 2.2, 0.4]
    ))
    objects.append(make_obj(
        "island-sanctum-arch", "material.navigator.sanctum.gold",
        0, [1.4, 0.15, 0.25], [s_x, s_y + 2.7, s_z], [2.8, 0.3, 0.5]
    ))
    # Sanctum Beacon
    objects.append(make_obj(
        "navigator-beacon-sanctum", "material.navigator.sanctum.beacon",
        2, [0.65], [s_x, s_y + 1.6, s_z + 1.8], [1.3, 1.3, 1.3],
        authored_props={
            "destinationZone": {"t": "string", "v": "Sanctum of Beginnings"},
            "controlValue": {"t": "double", "v": 0.0},
            "controlLabel": {"t": "string", "v": "Sanctum of Beginnings"}
        }
    ))

    # 3. Island 2: Temple of Echoes (North-North-West, ~-3.5, 2.0, -15.5)
    t_x, t_y, t_z = -3.5, 1.8, -15.5
    objects.append(make_obj(
        "island-temple-base", "material.navigator.temple.base",
        0, [2.6, 0.45, 2.6], [t_x, t_y, t_z], [5.2, 0.9, 5.2]
    ))
    objects.append(make_obj(
        "island-temple-crag", "material.navigator.temple.rock",
        0, [2.1, 0.65, 2.1], [t_x, t_y - 0.75, t_z], [4.2, 1.3, 4.2]
    ))
    # Colonnade
    objects.append(make_obj(
        "island-temple-col-1", "material.navigator.temple.silver",
        0, [0.2, 1.1, 0.2], [t_x - 1.2, t_y + 1.5, t_z - 0.9], [0.4, 2.2, 0.4]
    ))
    objects.append(make_obj(
        "island-temple-col-2", "material.navigator.temple.silver",
        0, [0.2, 1.1, 0.2], [t_x + 1.2, t_y + 1.5, t_z - 0.9], [0.4, 2.2, 0.4]
    ))
    objects.append(make_obj(
        "island-temple-col-3", "material.navigator.temple.silver",
        0, [0.2, 1.1, 0.2], [t_x - 1.2, t_y + 1.5, t_z + 0.9], [0.4, 2.2, 0.4]
    ))
    objects.append(make_obj(
        "island-temple-col-4", "material.navigator.temple.silver",
        0, [0.2, 1.1, 0.2], [t_x + 1.2, t_y + 1.5, t_z + 0.9], [0.4, 2.2, 0.4]
    ))
    objects.append(make_obj(
        "island-temple-roof", "material.navigator.temple.base",
        0, [1.8, 0.2, 1.5], [t_x, t_y + 2.7, t_z], [3.6, 0.4, 3.0]
    ))
    # Temple Beacon
    objects.append(make_obj(
        "navigator-beacon-temple", "material.navigator.temple.beacon",
        2, [0.65], [t_x, t_y + 1.5, t_z + 1.9], [1.3, 1.3, 1.3],
        authored_props={
            "destinationZone": {"t": "string", "v": "Temple of Echoes"},
            "controlValue": {"t": "double", "v": 0.0},
            "controlLabel": {"t": "string", "v": "Temple of Echoes"}
        }
    ))

    # 4. Island 3: Cavern of Light (North-North-East, ~3.5, 1.0, -15.5)
    c_x, c_y, c_z = 3.5, 0.8, -15.5
    objects.append(make_obj(
        "island-cavern-base", "material.navigator.cavern.base",
        0, [2.6, 0.45, 2.6], [c_x, c_y, c_z], [5.2, 0.9, 5.2]
    ))
    objects.append(make_obj(
        "island-cavern-crag", "material.navigator.cavern.rock",
        0, [2.1, 0.7, 2.1], [c_x, c_y - 0.8, c_z], [4.2, 1.4, 4.2]
    ))
    # Crystals
    objects.append(make_obj(
        "island-cavern-crys-1", "material.navigator.cavern.crystal",
        0, [0.3, 1.4, 0.3], [c_x - 0.9, c_y + 1.6, c_z - 0.7], [0.6, 2.8, 0.6]
    ))
    objects.append(make_obj(
        "island-cavern-crys-2", "material.navigator.cavern.crystal",
        0, [0.4, 1.8, 0.4], [c_x, c_y + 2.0, c_z - 0.9], [0.8, 3.6, 0.8]
    ))
    objects.append(make_obj(
        "island-cavern-crys-3", "material.navigator.cavern.crystal",
        0, [0.3, 1.2, 0.3], [c_x + 0.9, c_y + 1.4, c_z - 0.6], [0.6, 2.4, 0.6]
    ))
    # Cavern Beacon
    objects.append(make_obj(
        "navigator-beacon-cavern", "material.navigator.cavern.beacon",
        2, [0.65], [c_x, c_y + 1.5, c_z + 1.9], [1.3, 1.3, 1.3],
        authored_props={
            "destinationZone": {"t": "string", "v": "Cavern of Light"},
            "controlValue": {"t": "double", "v": 0.0},
            "controlLabel": {"t": "string", "v": "Cavern of Light"}
        }
    ))

    # 5. Island 4: Character Architect Forge (North-East, ~10.0, 1.5, -12.0)
    f_x, f_y, f_z = 10.0, 1.4, -12.0
    objects.append(make_obj(
        "island-forge-base", "material.navigator.forge.base",
        0, [2.6, 0.45, 2.6], [f_x, f_y, f_z], [5.2, 0.9, 5.2]
    ))
    objects.append(make_obj(
        "island-forge-crag", "material.navigator.forge.base",
        0, [2.0, 0.65, 2.0], [f_x, f_y - 0.75, f_z], [4.0, 1.3, 4.0]
    ))
    objects.append(make_obj(
        "island-forge-crucible", "material.navigator.forge.bronze",
        0, [0.9, 0.8, 0.9], [f_x, f_y + 1.2, f_z - 0.6], [1.8, 1.6, 1.8]
    ))
    objects.append(make_obj(
        "island-forge-magma", "material.navigator.forge.magma",
        0, [0.5, 0.5, 0.5], [f_x, f_y + 1.8, f_z - 0.6], [1.0, 1.0, 1.0]
    ))
    # Forge Beacon
    objects.append(make_obj(
        "navigator-beacon-forge", "material.navigator.forge.beacon",
        2, [0.65], [f_x, f_y + 1.6, f_z + 1.8], [1.3, 1.3, 1.3],
        authored_props={
            "destinationZone": {"t": "string", "v": "Character Architect Forge"},
            "controlValue": {"t": "double", "v": 0.0},
            "controlLabel": {"t": "string", "v": "Character Architect Forge"}
        }
    ))

    # Relations
    formation_relations = [
        # Button categories
        {
            "directed": True,
            "entityA": "navigator-beacon-sanctum",
            "entityB": "category.control.button",
            "type": "instance-of",
            "weight": 1.0
        },
        {
            "directed": True,
            "entityA": "navigator-beacon-temple",
            "entityB": "category.control.button",
            "type": "instance-of",
            "weight": 1.0
        },
        {
            "directed": True,
            "entityA": "navigator-beacon-cavern",
            "entityB": "category.control.button",
            "type": "instance-of",
            "weight": 1.0
        },
        {
            "directed": True,
            "entityA": "navigator-beacon-forge",
            "entityB": "category.control.button",
            "type": "instance-of",
            "weight": 1.0
        },
        # Undirected filaments from central plinth to each island
        {
            "directed": False,
            "entityA": "navigator-plinth-center",
            "entityB": "island-sanctum-base",
            "type": "filament",
            "weight": 1.0
        },
        {
            "directed": False,
            "entityA": "navigator-plinth-center",
            "entityB": "island-temple-base",
            "type": "filament",
            "weight": 1.0
        },
        {
            "directed": False,
            "entityA": "navigator-plinth-center",
            "entityB": "island-cavern-base",
            "type": "filament",
            "weight": 1.0
        },
        {
            "directed": False,
            "entityA": "navigator-plinth-center",
            "entityB": "island-forge-base",
            "type": "filament",
            "weight": 1.0
        }
    ]

    zone_data = {
        "identifier": "Ourverse Gathering",
        "name": "Ourverse Gathering",
        "owner": "",
        "parentZone": "",
        "scope": "Global",
        "qualities": {
            "kind": "ourverse-gathering",
            "purpose": "personal-ourverse-navigator",
            "injected_by": "Gemini 3.8 Flash (High) - Personal Ourverse Navigator Prototyping Loop authorized by Zach, 2026-10-16"
        },
        "deletable": {},
        "lawRefs": [
            "law-navigator-button"
        ],
        "lexemes": [],
        "materials": materials,
        "formationRelations": formation_relations,
        "spatialRoot": {
            "id": "OurverseGathering_spatialRoot",
            "origin": [0.0, 0.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "field": {
                "amplitude": 1.0,
                "baseDensity": 1.0,
                "frequency": 1.0,
                "mode": "Procedural"
            },
            "vectorField": {
                "amplitude": 1.0,
                "baseFlowX": 0.0,
                "baseFlowY": 0.0,
                "baseFlowZ": 0.0,
                "frequency": 1.0,
                "mode": "Procedural"
            }
        },
        "world": {
            "objects": objects
        }
    }
    return zone_data

def build_navigator_law():
    return {
        "authors": [
            "author.gemini-spark"
        ],
        "identifier": "law-navigator-button",
        "injected_by": "Gemini 3.8 Flash (High) - Personal Ourverse Navigator Prototyping Loop authorized by Zach, 2026-10-16",
        "law": {
            "actionModel": {
                "children": [
                    {
                        "eventType": "control-activated",
                        "kind": 10,
                        "publishSubject": "",
                        "publishObject": ""
                    },
                    {
                        "eventType": "zone-navigation-requested",
                        "kind": 10,
                        "publishSubject": "",
                        "publishObject": ""
                    }
                ],
                "kind": 5
            },
            "activation": 0,
            "applicationLog": [],
            "authority": 0,
            "authors": [
                "author.gemini-spark"
            ],
            "conditionMode": "all",
            "conditionModel": {
                "kind": 2,
                "otherId": "category.control.button",
                "relationType": "instance-of"
            },
            "conditionSubjects": [],
            "drives": False,
            "enabled": True,
            "id": "law-navigator-button",
            "name": "Ourverse Navigator: Button Click to Zone Navigation",
            "provenance": [
                {
                    "directed": True,
                    "entityA": "law-navigator-button",
                    "entityB": "author.gemini-spark",
                    "events": [
                        {
                            "deltaWeight": 1.0,
                            "description": "authored-by",
                            "timestamp": 1787400000
                        }
                    ],
                    "type": "authored-by",
                    "weight": 1.0
                }
            ],
            "retrigger": 0,
            "scope": 0,
            "targets": []
        },
        "triggers": [
            "object-clicked"
        ]
    }

def main():
    root = Path(__file__).resolve().parents[1]
    zone_data = build_navigator_zone()
    law_data = build_navigator_law()

    # 1. Output to saves/
    save_zone_dir = root / "saves/zones/Ourverse Gathering"
    save_zone_dir.mkdir(parents=True, exist_ok=True)
    with open(save_zone_dir / "zone.json", "w") as f:
        json.dump(zone_data, f, indent=2)

    save_law_dir = root / "saves/laws/law-navigator-button"
    save_law_dir.mkdir(parents=True, exist_ok=True)
    with open(save_law_dir / "law.json", "w") as f:
        json.dump(law_data, f, indent=2)

    # 2. Output to scripts/macos/Foundation/
    foundation_zone_dir = root / "scripts/macos/Foundation/zones/Ourverse Gathering"
    foundation_zone_dir.mkdir(parents=True, exist_ok=True)
    with open(foundation_zone_dir / "zone.json", "w") as f:
        json.dump(zone_data, f, indent=2)

    foundation_law_dir = root / "scripts/macos/Foundation/laws/law-navigator-button"
    foundation_law_dir.mkdir(parents=True, exist_ok=True)
    with open(foundation_law_dir / "law.json", "w") as f:
        json.dump(law_data, f, indent=2)

    print("Successfully authored Personal Ourverse Navigator to saves/ and scripts/macos/Foundation/")

if __name__ == "__main__":
    main()
