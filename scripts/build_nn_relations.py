import json
import os
import copy
from datetime import datetime

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AUTHOR = "did:earthcall:dmvokvvtp4jwmhhkyzv23xanzyukspdv5aykm7okru3mdidyncga"
WORLD_PATH = os.path.join(ROOT, "saves", "worlds", "chess_app.ecform")

def relation(a_id, b_id):
    return {
        "type": "synapse",
        "entityA": a_id,
        "entityB": b_id,
        "directed": True,
        "weight": 1.0
    }

def neuron(id_str, x, color, w1=0.1, w2=0.1):
    n = {
        "identifier": id_str,
        "name": id_str,
        "authors": ["Zach"],
        "injected_by": "Antigravity - ML Relations",
        "physicsMode": 0, # Static
        "isTangible": False,
        "isVisible": True,
        "position": [x, 0.0, 0.0],
        "shapeKind": 1, # Sphere
        "shape": {"r": 0.5},
        "materialId": "material.neuron_" + id_str,
        "faceColors": [color],
        "authoredProperties": {
            "activation": 0.0
        }
    }
    if w1 is not None:
        n["authoredProperties"]["w1"] = w1
    if w2 is not None:
        n["authoredProperties"]["w2"] = w2
    return n

def law(id_str, name, act, children):
    return {
        "identifier": id_str,
        "authors": ["Zach"],
        "injected_by": "Antigravity - ML Relations",
        "law": {
            "id": id_str,
            "name": name,
            "enabled": True,
            "authority": 0,
            "activation": act, # 1 for WhileTrue, 0 for OnEvent
            "scope": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Zach"],
            "conditionModel": {"kind": 3, "children": []},
            "actionModel": {"kind": 5, "children": children},
            "provenance": []
        },
        "triggers": []
    }

def map_action(path, bindings, terms):
    return {
        "kind": 8,
        "path": path,
        "bindings": bindings,
        "function": {"pieces": [{"expr": {"terms": terms}}]}
    }

def flow_action(path, bindings, terms):
    return {
        "kind": 9,
        "path": path,
        "bindings": bindings,
        "function": {"pieces": [{"expr": {"terms": terms}}]}
    }

def build_zone():
    neurons = [
        neuron("node-in-1", -4.0, [1,0,0], w1=None, w2=None),
        neuron("node-in-2", -2.0, [0,1,0], w1=None, w2=None),
        neuron("node-h-1", 0.0, [1,1,0], w1=0.2, w2=-0.1),
        neuron("node-h-2", 2.0, [0,1,1], w1=-0.2, w2=0.1),
        neuron("node-out", 4.0, [0,0,1], w1=0.1, w2=0.1),
        neuron("node-target", 6.0, [1,1,1], w1=None, w2=None)
    ]
    
    relations = [
        relation("node-in-1", "node-h-1"), relation("node-in-2", "node-h-1"),
        relation("node-in-1", "node-h-2"), relation("node-in-2", "node-h-2"),
        relation("node-h-1", "node-out"), relation("node-h-2", "node-out")
    ]
    
    laws = []
    
    laws.append(law("law-nn-inputs", "NN Inputs Drive", 1, [
        {
            "kind": 3,
            "path": "@node-in-1.authored.activation",
            "input": "@Universe.time.seconds",
            "curveModel": {"kind": 2, "frequency": 1.0, "amplitude": 1.0, "phase": 0.0, "offset": 0.0}
        },
        {
            "kind": 3,
            "path": "@node-in-2.authored.activation",
            "input": "@Universe.time.seconds",
            "curveModel": {"kind": 2, "frequency": 1.0, "amplitude": 1.0, "phase": 1.5707963, "offset": 0.0}
        },
        map_action("@node-target.authored.activation", 
            {"i1": "@node-in-1.authored.activation", "i2": "@node-in-2.authored.activation"},
            [{"c": 1.0, "factors": {"i1": 1, "i2": 1}}]
        )
    ]))
    
    laws.append(law("law-nn-forward", "NN Forward Pass", 1, [
        map_action("@node-h-1.authored.activation", 
            {"i1": "@node-in-1.authored.activation", "i2": "@node-in-2.authored.activation", "w1": "@node-h-1.authored.w1", "w2": "@node-h-1.authored.w2"},
            [{"c": 1.0, "factors": {"i1": 1, "w1": 1}}, {"c": 1.0, "factors": {"i2": 1, "w2": 1}}]
        ),
        map_action("@node-h-2.authored.activation", 
            {"i1": "@node-in-1.authored.activation", "i2": "@node-in-2.authored.activation", "w1": "@node-h-2.authored.w1", "w2": "@node-h-2.authored.w2"},
            [{"c": 1.0, "factors": {"i1": 1, "w1": 1}}, {"c": 1.0, "factors": {"i2": 1, "w2": 1}}]
        ),
        map_action("@node-out.authored.activation", 
            {"i1": "@node-h-1.authored.activation", "i2": "@node-h-2.authored.activation", "w1": "@node-out.authored.w1", "w2": "@node-out.authored.w2"},
            [{"c": 1.0, "factors": {"i1": 1, "w1": 1}}, {"c": 1.0, "factors": {"i2": 1, "w2": 1}}]
        )
    ]))
    
    lr = 5.0
    laws.append(law("law-nn-train-out", "NN Train Output", 1, [
        flow_action("@node-out.authored.w1", 
            {"tgt": "@node-target.authored.activation", "out": "@node-out.authored.activation", "h1": "@node-h-1.authored.activation"},
            [{"c": lr, "factors": {"tgt": 1, "h1": 1}}, {"c": -lr, "factors": {"out": 1, "h1": 1}}]
        ),
        flow_action("@node-out.authored.w2", 
            {"tgt": "@node-target.authored.activation", "out": "@node-out.authored.activation", "h2": "@node-h-2.authored.activation"},
            [{"c": lr, "factors": {"tgt": 1, "h2": 1}}, {"c": -lr, "factors": {"out": 1, "h2": 1}}]
        )
    ]))
    
    laws.append(law("law-nn-train-h1", "NN Train Hidden 1", 1, [
        flow_action("@node-h-1.authored.w1",
            {"tgt": "@node-target.authored.activation", "out": "@node-out.authored.activation", "w_out": "@node-out.authored.w1", "in1": "@node-in-1.authored.activation"},
            [{"c": lr, "factors": {"tgt": 1, "w_out": 1, "in1": 1}}, {"c": -lr, "factors": {"out": 1, "w_out": 1, "in1": 1}}]
        ),
        flow_action("@node-h-1.authored.w2",
            {"tgt": "@node-target.authored.activation", "out": "@node-out.authored.activation", "w_out": "@node-out.authored.w1", "in2": "@node-in-2.authored.activation"},
            [{"c": lr, "factors": {"tgt": 1, "w_out": 1, "in2": 1}}, {"c": -lr, "factors": {"out": 1, "w_out": 1, "in2": 1}}]
        )
    ]))
    
    laws.append(law("law-nn-train-h2", "NN Train Hidden 2", 1, [
        flow_action("@node-h-2.authored.w1",
            {"tgt": "@node-target.authored.activation", "out": "@node-out.authored.activation", "w_out": "@node-out.authored.w2", "in1": "@node-in-1.authored.activation"},
            [{"c": lr, "factors": {"tgt": 1, "w_out": 1, "in1": 1}}, {"c": -lr, "factors": {"out": 1, "w_out": 1, "in1": 1}}]
        ),
        flow_action("@node-h-2.authored.w2",
            {"tgt": "@node-target.authored.activation", "out": "@node-out.authored.activation", "w_out": "@node-out.authored.w2", "in2": "@node-in-2.authored.activation"},
            [{"c": lr, "factors": {"tgt": 1, "w_out": 1, "in2": 1}}, {"c": -lr, "factors": {"out": 1, "w_out": 1, "in2": 1}}]
        )
    ]))
    
    vis_actions = []
    for n in neurons:
        vis_actions.append(map_action("@" + n["identifier"] + ".position.y",
            {"act": "@" + n["identifier"] + ".authored.activation"},
            [{"c": 1.0, "factors": {"act": 1}}]
        ))
    laws.append(law("law-nn-visuals", "NN Visuals", 1, vis_actions))

    zone_data = {
        "identifier": "Neural Network v2",
        "name": "Neural Network v2",
        "owner": AUTHOR,
        "lawRefs": [l["identifier"] for l in laws],
        "formationRelations": relations,
        "objects": neurons,
        "isSpatial": True,
        "isActive": False
    }
    
    return zone_data, laws

def patch():
    import msgpack
    zone_data, laws = build_zone()
    
    for l in laws:
        law_dir = os.path.join(ROOT, "saves", "laws", l["identifier"])
        os.makedirs(law_dir, exist_ok=True)
        with open(os.path.join(law_dir, "law.json"), "w") as f:
            json.dump(l, f, indent=4)
            
    zone_dir = os.path.join(ROOT, "saves", "zones", "Neural Network v2")
    os.makedirs(zone_dir, exist_ok=True)
    with open(os.path.join(zone_dir, "zone.json"), "w") as f:
        json.dump(zone_data, f, indent=4)
    with open(os.path.join(zone_dir, "zone.ecform"), "wb") as f:
        f.write(msgpack.packb({"MigrationRoot": json.dumps(zone_data)}))
        
    with open(WORLD_PATH, "rb") as f:
        world_pack = msgpack.unpackb(f.read())
    world = json.loads(world_pack["MigrationRoot"])
    
    if "Neural Network v2" not in world.get("zones", []):
        world.setdefault("zones", []).append("Neural Network v2")
    if "Neural Network v2" not in world.get("zoneRefs", []):
        world.setdefault("zoneRefs", []).append("Neural Network v2")
        
    world_pack["MigrationRoot"] = json.dumps(world)
    
    temp_path = WORLD_PATH + ".tmp"
    with open(temp_path, "wb") as f:
        f.write(msgpack.packb(world_pack))
    os.replace(temp_path, WORLD_PATH)
    print("Patched successfully!")

if __name__ == "__main__":
    patch()
