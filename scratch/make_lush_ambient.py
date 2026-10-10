import json
import os
import copy

def make_law(voice, c, scale, shift):
    law_id = f"law-ambient-pan-{voice}"
    law = {
        "authors": ["Zach"],
        "identifier": law_id,
        "injected_by": "Antigravity, authorized by Zach",
        "law": {
            "id": law_id,
            "name": f"Ambient Sound Panning - {voice}",
            "enabled": True,
            "authority": 0,
            "activation": 1,
            "scope": 1,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach"],
            "conditionSubjects": [],
            "targets": [],
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
            },
            "provenance": []
        },
        "triggers": []
    }
    
    os.makedirs(f"saves/laws/{law_id}", exist_ok=True)
    with open(f"saves/laws/{law_id}/law.json", "w") as f:
        json.dump(law, f, indent=4)

# Create Laws
make_law("root", 6.0, 0.2, 0.0)
make_law("fifth", -5.0, 0.35, 0.0)
make_law("ninth", 4.0, 0.15, 0.0)

# Patch Zone
zone_path = "saves/zones/Ambient Zone/zone.json"
with open(zone_path, "r") as f:
    zone = json.load(f)

# Clear old orbs
zone['world']['objects'] = [o for o in zone['world']['objects'] if not o.get('objectID', '').startswith('panning-sound-orb')]

# Remove old law-ambient-pan from lawRefs, add new ones
if "lawRefs" not in zone:
    zone["lawRefs"] = []
if "law-ambient-pan" in zone["lawRefs"]:
    zone["lawRefs"].remove("law-ambient-pan")

for l in ["law-ambient-pan-root", "law-ambient-pan-fifth", "law-ambient-pan-ninth"]:
    if l not in zone["lawRefs"]:
        zone["lawRefs"].append(l)

def make_orb(voice, freq, lowpass, amp):
    return {
        "objectID": f"panning-sound-orb-{voice}",
        "authoritativeAxis": [0.0, 1.0, 0.0],
        "center": [0.0, 0.0, 0.0],
        "faceColors": [[0.8, 0.8, 0.9]] * 6,
        "geometryType": 1,
        "materialId": "material.default",
        "renderMode": 0,
        "rotationResponsiveness": 10.0,
        "shape": {
            "kind": 1,
            "params": {
                "r": 0.5
            }
        },
        "shapeKind": 1,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 100.0, 100.0],
        "targetRotation": [0.0, 0.0, 0.0],
        "transform": [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0],
        "x2D": 100.0,
        "y2D": 100.0,
        "zOrder2D": 0,
        "authoredProperties": {
            "acoustic.isSoundEmitter": {"t": "string", "v": "true"},
            "acoustic.voice": {"t": "string", "v": voice},
            "acoustic.waveType": {"t": "string", "v": "sine"},
            "acoustic.frequency": {"t": "float", "v": freq},
            "acoustic.lowpassCutoff": {"t": "float", "v": lowpass},
            "acoustic.amplitude": {"t": "float", "v": amp}
        }
    }

zone['world']['objects'].append(make_orb("root", 110.0, 250.0, 0.5))
zone['world']['objects'].append(make_orb("fifth", 164.81, 300.0, 0.4))
zone['world']['objects'].append(make_orb("ninth", 246.94, 400.0, 0.25))

with open(zone_path, "w") as f:
    json.dump(zone, f, indent=2)

print("Created lush ambient chord!")
