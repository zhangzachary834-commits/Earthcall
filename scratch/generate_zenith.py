import json
import math
import os

objects = []
materials = []

def add_material(name, color, shininess=32.0, specular=0.5, ambient=0.2, diffuse=0.8, opacity=1.0):
    materials.append({
        "name": name,
        "baseColor": color,
        "opacity": opacity,
        "shininess": shininess,
        "specular": specular,
        "ambient": ambient,
        "diffuse": diffuse,
        "faceTextures": []
    })

add_material("zenith.gold", [1.0, 0.84, 0.0], 128.0, 1.0, 0.4, 0.9)
add_material("zenith.obsidian", [0.05, 0.05, 0.08], 64.0, 0.8, 0.1, 0.7)
add_material("zenith.marble", [0.9, 0.9, 0.95], 16.0, 0.3, 0.3, 0.8)
add_material("zenith.glow_blue", [0.2, 0.6, 1.0], 10.0, 0.1, 1.0, 1.0)
add_material("zenith.glow_pink", [1.0, 0.3, 0.7], 10.0, 0.1, 1.0, 1.0)

def params(r=0, ry=0, rz=0, halfH=0, majorR=0, minorR=0, paraboloidA=0, ovoidAsym=0, fillet=0, width2D=0, height2D=0):
    return [r, ry, rz, halfH, majorR, minorR, paraboloidA, ovoidAsym, fillet, width2D, height2D]

# Altar
objects.append({
    "objectID": "zenith.altar.base",
    "center": [0, 1, 0],
    "shapeKind": 9, # RoundedBox
    "materialId": "material.zenith.marble",
    "shapeParams": params(r=4, ry=1, rz=4, fillet=0.5),
    "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,1,0,1]
})

# Floating Gold Torus above Altar
objects.append({
    "objectID": "zenith.altar.halo",
    "center": [0, 8, 0],
    "shapeKind": 8, # Torus
    "materialId": "material.zenith.gold",
    "shapeParams": params(majorR=6, minorR=0.5),
    "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,8,0,1]
})

# Pillars forming a circle
num_pillars = 12
radius = 20
for i in range(num_pillars):
    angle = i * (2 * math.pi / num_pillars)
    x = round(math.cos(angle) * radius, 3)
    z = round(math.sin(angle) * radius, 3)
    
    # Pillar Base
    objects.append({
        "objectID": f"zenith.pillar.{i}.base",
        "center": [x, 5, z],
        "shapeKind": 3, # Cylinder
        "materialId": "material.zenith.obsidian",
        "shapeParams": params(r=2, halfH=5),
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, x,5,z,1]
    })
    
    # Floating glowing orb on top
    orb_mat = "material.zenith.glow_blue" if i % 2 == 0 else "material.zenith.glow_pink"
    objects.append({
        "objectID": f"zenith.pillar.{i}.orb",
        "center": [x, 12, z],
        "shapeKind": 2, # Sphere
        "materialId": orb_mat,
        "shapeParams": params(r=1.5),
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, x,12,z,1]
    })

# Floor
objects.append({
    "objectID": "zenith.floor",
    "center": [0, -1, 0],
    "shapeKind": 0, # Cube
    "materialId": "material.zenith.obsidian",
    "shapeParams": params(r=100, ry=1, rz=100),
    "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-1,0,1]
})

zone = {
    "world": {
        "name": "Celestial Zenith",
        "objects": objects
    },
    "materials": materials,
    "formationRelations": [],
    "lexemes": []
}

os.makedirs("saves/zones/Celestial Zenith", exist_ok=True)
with open("saves/zones/Celestial Zenith/zone.json", "w") as f:
    json.dump(zone, f, indent=2)

print("Zone generated!")
