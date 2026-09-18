import json

with open('saves/worlds/synthesis_studio.json', 'r') as f:
    data = json.load(f)

# Find the object with objectID = "state.studio"
def find_obj(data):
    if isinstance(data, dict):
        if data.get('objectID') == 'state.studio':
            return data
        for k, v in data.items():
            res = find_obj(v)
            if res: return res
    elif isinstance(data, list):
        for item in data:
            res = find_obj(item)
            if res: return res
    return None

obj = find_obj(data)
if obj:
    if 'authoredProperties' not in obj:
        obj['authoredProperties'] = {}
    obj['authoredProperties']['lastStrokeX'] = {"t": "double", "v": 0.0}
    obj['authoredProperties']['lastStrokeY'] = {"t": "double", "v": 0.0}
    obj['authoredProperties']['lastStrokeZ'] = {"t": "double", "v": 0.0}
    obj['authoredProperties']['strokeSpacing'] = {"t": "double", "v": 0.1}
    with open('saves/worlds/synthesis_studio.json', 'w') as f:
        json.dump(data, f, indent=2)
    print("Patched synthesis_studio.json with lastStrokeX/Y/Z and strokeSpacing.")
else:
    print("state.studio object not found!")
