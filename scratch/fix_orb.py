import json
path = "saves/zones/Ambient Zone/zone.json"
with open(path, "r") as f:
    data = json.load(f)

orb = data['world']['objects'][0]
orb['authoritativeAxis'] = [0.0,1.0,0.0]
orb['center'] = [0.0,0.0,0.0]
orb['faceColors'] = [[0.8,0.8,0.9], [0.8,0.8,0.9], [0.8,0.8,0.9], [0.8,0.8,0.9], [0.8,0.8,0.9], [0.8,0.8,0.9]]
orb['geometryType'] = 1 # analytical
orb['renderMode'] = 0
orb['rotationResponsiveness'] = 10.0
orb['materialId'] = "material.default"
orb['targetRotation'] = [0.0, 0.0, 0.0]
orb['x2D'] = 100.0
orb['y2D'] = 100.0
orb['zOrder2D'] = 0

with open(path, "w") as f:
    json.dump(data, f, indent=2)
print("Fixed missing fields!")
