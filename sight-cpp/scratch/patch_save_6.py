import json

path = 'saves/games/20260727_114355.json'
with open(path, 'r') as f:
    data = json.load(f)

# 1. Update concept-shape-3d to have a member
concepts = data.get('concepts', {}).get('concepts', [])
for concept in concepts:
    if concept.get('id') == 'concept-shape-3d':
        if 'members' not in concept or len(concept['members']) == 0:
            concept['members'] = [
                {
                    "kind": 2, # Sphere or whatever
                    "params": [0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    "relativeTransform": [
                        1.0, 0.0, 0.0, 0.0,
                        0.0, 1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 1.0
                    ]
                }
            ]

# 2. Update law-shape-3d
laws = data.get('authoredLaws', {}).get('laws', [])
for law in laws:
    if law.get('id') == 'law-shape-3d':
        # Add conditionModel
        law['conditionModel'] = {
            "kind": 0,
            "path": "active3DMode",
            "op": 0,
            "operand": {
                "t": "string",
                "v": "Create"
            }
        }
        # Update actionModel spawnPlacementPath
        if 'actionModel' in law:
            law['actionModel']['spawnPlacementPath'] = '@subject.cursorSpawnTransform'
            # ensure no spawnParentPath
            if 'spawnParentPath' in law['actionModel']:
                del law['actionModel']['spawnParentPath']

with open(path, 'w') as f:
    json.dump(data, f, indent=4)
print('Patched successfully!')
