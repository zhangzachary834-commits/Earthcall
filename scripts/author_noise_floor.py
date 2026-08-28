#!/usr/bin/env python3
import json
import os

AUTHOR = "Antigravity"
ZONE_ID = "NoiseFloorWorld"

def pv(t, v):
    return {"t": t, "v": v}

def math_scalar(v):
    return {
        "op": 0,
        "scalarForm": {
            "terms": [
                {"c": float(v), "factors": {}}
            ]
        }
    }

def math_var(name):
    return {
        "op": 1,
        "var": name
    }

def math_scale(a, b):
    return {
        "op": 6,
        "children": [a, b]
    }

def math_sub(a, b):
    return {
        "op": 5,
        "children": [a, b]
    }

def math_add(a, b):
    return {
        "op": 4,
        "children": [a, b]
    }

def math_noise(a):
    return {
        "op": 29,
        "children": [a]
    }

def math_vec3(x, y, z):
    return {
        "op": 2,
        "children": [math_scalar(x), math_scalar(y), math_scalar(z)]
    }

# Use a lower frequency for larger rolling hills, and higher amplitude
freq = 0.02
amp = 15.0
offset_p = math_add(math_var("p"), math_vec3(100.0, 0.0, 100.0))

sdf_math = math_sub(
    math_var("y"),
    math_scale(
        math_scalar(amp),
        math_noise(math_scale(math_scalar(freq), offset_p))
    )
)

field_node = {
    "op": 0,     # Leaf
    "prim": 7,   # Expr
    "mathNode": sdf_math
}

floor_object = {
    "objectID": "perlin-ground-plane",
    "shapeKind": 10,  # Field
    "geometryType": 10,
    "shapeParams": [0.0]*9,
    "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-2,0,1],
    "center": [0,-2,0],
    "materialId": "material.grass",
    "baseline": "ground",
    "authoredProperties": {
        "displayName": pv("string", "Perlin Ground Plane")
    },
    "fieldExtent": 1000.0,
    "field": field_node
}

grass_material = {
    "name": "grass",
    "baseColor": [0.15, 0.55, 0.15],
    "opacity": 1.0,
    "shininess": 0.0,
    "specular": 0.0,
    "ambient": 0.3,
    "diffuse": 0.8
}

zone_json = {
    "being": "zone",
    "name": "Perlin Noise Floor Zone",
    "identifier": ZONE_ID,
    "owner": AUTHOR,
    "parentZone": "",
    "scope": "Local",
    "qualities": {},
    "world": {
        "objects": [floor_object],
        "laws": []
    },
    "materials": [grass_material],
    "formationRelations": [],
    "lexemes": []
}

session_json = {
    "saveFormat": "zone-identity-v1",
    "zoneRefs": [{"identifier": ZONE_ID}],
    "currentZoneId": ZONE_ID,
    "zones": [zone_json],
    "authoredLaws": {
        "laws": [],
        "triggers": {},
        "formationMembers": [],
        "rete": {"alphaNodes": [], "betaNodes": [], "facts": [], "agenda": []}
    },
    "concepts": {"concepts": []},
    "playerBody": [0, 5, 0],
    "cameraPos": [0, 5, 5],
    "cameraFront": [0, 0, -1],
    "cameraUp": [0, 1, 0],
    "yaw": -90.0,
    "pitch": 0.0
}

os.makedirs(f"saves/zones/{ZONE_ID}", exist_ok=True)
with open(f"saves/zones/{ZONE_ID}/zone.json", "w") as f:
    json.dump(zone_json, f, indent=2)

os.makedirs("saves/worlds", exist_ok=True)
with open("saves/worlds/noise_floor.json", "w") as f:
    json.dump(session_json, f, indent=2)

print("Generated saves/worlds/noise_floor.json and saves/zones/NoiseFloorWorld/zone.json")
