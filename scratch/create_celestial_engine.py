import json

zone = {
    "identifier": "Celestial Radiance Engine",
    "name": "Celestial Radiance Engine",
    "objects": []
}

# Add a giant glowing central core (Sphere)
zone["objects"].append({
    "identifier": "central_core",
    "kind": 1, # Sphere
    "transform": [
        20.0, 0, 0, 0,
        0, 20.0, 0, 0,
        0, 0, 20.0, 0,
        0, 30.0, 0, 1.0
    ],
    "faceColors": [[0.0, 0.8, 1.0, 1.0]], # Cyan glow
    "physicsState": {"mass": 0.0, "isKinematic": True},
    "radiance": {"intensity": 5.0, "color": [0.0, 0.8, 1.0], "radius": 100.0}
})

# Add orbital rings (Torus)
for i in range(3):
    scale = 40.0 + (i * 15.0)
    zone["objects"].append({
        "identifier": f"orbital_ring_{i}",
        "kind": 4, # Torus
        "transform": [
            scale, 0, 0, 0,
            0, scale, 0, 0,
            0, 0, scale, 0,
            0, 30.0, 0, 1.0
        ],
        "faceColors": [[1.0, 0.2, 0.8, 1.0]], # Neon pink
        "physicsState": {"mass": 0.0, "isKinematic": True},
        "radiance": {"intensity": 2.0, "color": [1.0, 0.2, 0.8], "radius": 50.0}
    })

# Add floating crystalline pillars (Cylinders)
for x in [-80, 80]:
    for z in [-80, 80]:
        zone["objects"].append({
            "identifier": f"pillar_{x}_{z}",
            "kind": 3, # Cylinder
            "transform": [
                5.0, 0, 0, 0,
                0, 40.0, 0, 0,
                0, 0, 5.0, 0,
                x, 20.0, z, 1.0
            ],
            "faceColors": [[0.8, 0.9, 1.0, 0.9]],
            "physicsState": {"mass": 0.0, "isKinematic": True}
        })

# Save to the zones folder
import os
os.makedirs("saves/zones/Celestial Radiance Engine", exist_ok=True)
with open("saves/zones/Celestial Radiance Engine/zone.json", "w") as f:
    json.dump(zone, f, indent=2)

print("Created Celestial Radiance Engine!")
