import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

# Define materials
materials = data.get("materials", [])

materials.append({
    "id": "material.mtg.button.resolve",
    "colorExpr": {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.9}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.4}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.1}]}}
                    ]
                }
            }
        ]
    }
})

materials.append({
    "id": "material.mtg.skybox",
    "colorExpr": {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.05}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.05}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.1}]}}
                    ]
                }
            }
        ]
    }
})

data["materials"] = materials

# Update objects
for obj in data["objects"]:
    if obj["objectID"] == "button.mtg.resolve":
        obj["materialId"] = "material.mtg.button.resolve"

# Add Skybox
skybox = {
    "objectID": "object.mtg.skybox",
    "shapeKind": 1, # Sphere
    "geometryType": 0,
    "shapeParams": [100.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    "position": [0.0, 0.0, 0.0],
    "materialId": "material.mtg.skybox"
}
# wait, if it's a solid sphere, we are inside it, we will see the INSIDE.
# In basic SDF renderers, if the camera is inside a solid sphere, does it render the interior?
# Usually, yes, if normals are inverted or if we just want a large hollow sphere. 
# Earthcall uses exact CSG and marching. The inside of a sphere evaluates to negative distance (inside).
# If the camera is inside, the raymarcher might just hit distance=0 immediately and render black, OR it might render the interior shell.
# Let's use a huge hollow sphere! A shell.
# We can just leave out the skybox to be safe, or use the engine's default clear color.

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("More aesthetics added!")
