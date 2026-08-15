import json
import os

SAVE_FILE = "saves/games/20260727_142514_QuickSave.json"

if not os.path.exists(SAVE_FILE):
    print(f"Save file {SAVE_FILE} not found!")
    exit(1)

with open(SAVE_FILE, 'r') as f:
    save_data = json.load(f)

# 1. Ensure the gamemode-controller Object exists
controller_found = False
for zone in save_data.get('zones', []):
    for obj in zone.get('world', {}).get('objects', []):
        if obj.get('objectID') == 'gamemode-controller':
            controller_found = True
            break
    if controller_found:
        break

if not controller_found:
    print("Injecting gamemode-controller object...")
    new_obj = {
        "objectID": "gamemode-controller",
        "shapeKind": 0,
        "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
        "center": [0,0,0],
        "authoredProperties": {
            "worldPhysics": {"t": "bool", "v": True},
            "flying": {"t": "bool", "v": False}
        }
    }
    if 'zones' in save_data and len(save_data['zones']) > 0:
        save_data['zones'][0]['world']['objects'].append(new_obj)

# 2. Inject Laws to bind worldPhysics to gravity/kinematics
authored = save_data.setdefault('authoredLaws', {})
laws = authored.setdefault('laws', [])

def law_exists(law_id):
    return any(l.get('id') == law_id for l in laws)

if not law_exists('law-physics-enabler'):
    laws.append({
        "id": "law-physics-enabler",
        "name": "Enable Physics",
        "enabled": True,
        "authority": 0,
        "activation": 1, # WhileTrue
        "scope": 0, # Subject
        "drives": True,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": ["Player"],
        "conditionModel": {
            "kind": 0, # Compare
            "path": "@gamemode-controller.worldPhysics",
            "op": 0, # Eq
            "operand": {"t": "bool", "v": True}
        },
        "actionModel": {
            "kind": 6, # Parallel
            "children": [
                {
                    "kind": 0, # Set
                    "path": "@physics-gravity.enabled",
                    "operand": {"t": "bool", "v": True}
                },
                {
                    "kind": 0, # Set
                    "path": "@physics-kinematics.enabled",
                    "operand": {"t": "bool", "v": True}
                }
            ]
        },
        "provenance": [
            {
                "type": "authored-by",
                "entityA": "law-physics-enabler",
                "entityB": "Player",
                "directed": True,
                "weight": 1.0,
                "events": [{"description": "authored-by", "deltaWeight": 1.0, "timestamp": 1783903356}],
                "attachment": {"enabled": False}
            }
        ]
    })
    authored.setdefault('formationMembers', []).append('law-physics-enabler')

if not law_exists('law-physics-disabler'):
    laws.append({
        "id": "law-physics-disabler",
        "name": "Disable Physics",
        "enabled": True,
        "authority": 0,
        "activation": 1, # WhileTrue
        "scope": 0, # Subject
        "drives": True,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": ["Player"],
        "conditionModel": {
            "kind": 0, # Compare
            "path": "@gamemode-controller.worldPhysics",
            "op": 0, # Eq
            "operand": {"t": "bool", "v": False}
        },
        "actionModel": {
            "kind": 6, # Parallel
            "children": [
                {
                    "kind": 0, # Set
                    "path": "@physics-gravity.enabled",
                    "operand": {"t": "bool", "v": False}
                },
                {
                    "kind": 0, # Set
                    "path": "@physics-kinematics.enabled",
                    "operand": {"t": "bool", "v": False}
                }
            ]
        },
        "provenance": [
            {
                "type": "authored-by",
                "entityA": "law-physics-disabler",
                "entityB": "Player",
                "directed": True,
                "weight": 1.0,
                "events": [{"description": "authored-by", "deltaWeight": 1.0, "timestamp": 1783903356}],
                "attachment": {"enabled": False}
            }
        ]
    })
    authored.setdefault('formationMembers', []).append('law-physics-disabler')

with open(SAVE_FILE, 'w') as f:
    json.dump(save_data, f, indent=2)
print("Successfully injected GameMode laws and controller object.")
