import json

path = 'saves/games/20260727_114355.json'
with open(path, 'r') as f:
    data = json.load(f)

laws = data.get('laws', [])
if not laws:
    print("No laws found.")

new_laws = []
for law in laws:
    if law['id'] == 'law-3':
        continue
    if law['id'] == 'law-shape-3d':
        law['eventDescriptions'] = ['onMouseClicked']
        law['conditionDescriptions'] = [] # disable condition as event makes it trigger on click
        if 'actionModel' in law:
            law['actionModel']['spawnPlacementPath'] = '@subject.cursorSpawnTransform'
    new_laws.append(law)

data['laws'] = new_laws

with open(path, 'w') as f:
    json.dump(data, f, indent=4)
print('Patched laws successfully.')
