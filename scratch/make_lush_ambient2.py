import json

def make_orb(voice, freq, amp, z_pos):
    return {
        "objectID": f"panning-sound-orb-{voice}",
        "conceptIdentifier": "",
        "shapeKind": "sphere",
        "materialId": "default",
        "transform": [1.0, 0.0, 0.0, 0.0, 
                      0.0, 1.0, 0.0, 0.0, 
                      0.0, 0.0, 1.0, 0.0, 
                      0.0, 0.0, z_pos, 1.0],
        "x2D": 100.0,
        "y2D": 100.0,
        "zOrder2D": 0,
        "authoredProperties": {
            "acoustic.isSoundEmitter": {"t": "string", "v": "true"},
            "acoustic.voice": {"t": "string", "v": voice},
            "acoustic.waveType": {"t": "string", "v": "sine"},
            "acoustic.frequency": {"t": "float", "v": freq},
            "acoustic.amplitude": {"t": "float", "v": amp}
        }
    }

def make_law(voice, c, scale, shift):
    return {
        "lawId": f"law-ambient-pan-{voice}",
        "authority": 0,
        "authors": ["ambient.creator"],
        "conditionMode": "all",
        "conditionModel": {
            "kind": 0,
            "path": "acoustic.voice",
            "op": 0,
            "operand": {"t": "string", "v": voice}
        },
        "actionModel": {
            "kind": 8,
            "path": "position.x",
            "bindings": {
                "t": "time"
            },
            "function": {
                "pieces": [
                    {
                        "expr": {
                            "terms": [
                                {
                                    "c": c,
                                    "factors": {},
                                    "trans": [
                                        {
                                            "kind": 0,
                                            "var": "t",
                                            "scale": scale,
                                            "shift": shift
                                        }
                                    ]
                                }
                            ]
                        }
                    }
                ]
            }
        }
    }

zone_path = "saves/zones/Ambient Zone/zone.json"
with open(zone_path, "r") as f:
    zone = json.load(f)

# clear old orbs
zone['world']['objects'] = [o for o in zone['world']['objects'] if not o.get('objectID', '').startswith('panning-sound-orb')]

# z = -3.0 so they pass in front of the listener without jumping
zone['world']['objects'].append(make_orb("root", 110.0, 0.08, -3.0))
zone['world']['objects'].append(make_orb("fifth", 164.81, 0.06, -3.0))
zone['world']['objects'].append(make_orb("ninth", 246.94, 0.04, -3.0))

with open(zone_path, "w") as f:
    json.dump(zone, f, indent=2)

for voice, c, scale, shift in [
    ("root", 6.0, 0.2, 0.0),
    ("fifth", -5.0, 0.35, 0.0),
    ("ninth", 4.0, 0.15, 0.0)
]:
    law = make_law(voice, c, scale, shift)
    with open(f"saves/laws/law-ambient-pan-{voice}/law.json", "w") as f:
        json.dump({"law": law}, f, indent=4)

print("Updated zone and laws with correct Z position and audible amplitudes!")
