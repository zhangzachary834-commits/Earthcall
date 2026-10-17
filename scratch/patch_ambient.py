import json

path = 'saves/zones/Ambient Zone/zone.json'
with open(path, 'r') as f:
    data = json.load(f)

# Create our sound emitter object
obj = {
    "objectID": "panning-sound-orb",
    "transform": [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, -10.0, 1.0
    ],
    "shapeKind": 1, # Sphere
    "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 100.0, 100.0],
    "authoredProperties": {
        "acoustic.isSoundEmitter": {"t":"string", "v":"true"},
        "acoustic.waveType": {"t":"string", "v":"sine"},
        "acoustic.frequency": {"t":"double", "v":140.0},
        "acoustic.amplitude": {"t":"double", "v":0.6},
        "acoustic.lowpassCutoff": {"t":"double", "v":300.0}
    }
}

if 'world' not in data:
    data['world'] = {}
if 'objects' not in data['world']:
    data['world']['objects'] = []

# Remove old if exists
data['world']['objects'] = [o for o in data['world']['objects'] if o.get('objectID') != 'panning-sound-orb']
data['world']['objects'].append(obj)
data['name'] = "Ambient Zone"

with open(path, 'w') as f:
    json.dump(data, f, indent=2)
print("Patched zone.json!")
