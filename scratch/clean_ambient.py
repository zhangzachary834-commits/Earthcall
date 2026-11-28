import json

path = "saves/zones/Ambient Zone/zone.json"
with open(path, "r") as f:
    data = json.load(f)

data['world']['objects'] = [o for o in data['world']['objects'] if o.get('objectID') == 'panning-sound-orb']

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Cleaned!")
