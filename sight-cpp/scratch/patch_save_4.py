import json

path = 'saves/games/20260727_114355.json'
with open(path, 'r') as f:
    data = json.load(f)

laws = data.get('authoredLaws', {})

new_laws = {}
for law_id, law in laws.items():
    if law_id == 'law-3':
        continue
    if law_id == 'law-shape-3d':
        law['eventDescriptions'] = ['onMouseClicked']
        law['conditionDescriptions'] = [] # disable condition as event makes it trigger on click
        if 'actionModel' in law:
            law['actionModel']['spawnPlacementPath'] = '@subject.cursorSpawnTransform'
    new_laws[law_id] = law

data['authoredLaws'] = new_laws

with open(path, 'w') as f:
    json.dump(data, f, indent=4)
print('Patched authoredLaws successfully.')
