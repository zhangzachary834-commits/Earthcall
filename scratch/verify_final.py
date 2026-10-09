import json

with open('saves/zones/Ambient Zone/zone.json', 'r') as f:
    z = json.load(f)
    print("Zone Name:", z.get('name'))
    print("Law Refs:", z.get('lawRefs'))
    obj = z['world']['objects'][0]
    print("Object ID:", obj.get('objectID'))
    print("Authored Properties:", list(obj.get('authoredProperties', {}).keys()))

with open('saves/laws/law-ambient-pan/law.json', 'r') as f:
    l = json.load(f)
    print("Law Name:", l['law']['name'])
    print("Target:", l['law']['conditionModel']['path'], "==", l['law']['conditionModel']['operand']['v'])

