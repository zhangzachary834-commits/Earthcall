import json
import os

def build_ml():
    repo_root = "/Users/zacharyzhang/Documents/GitHub/Earthcall"
    
    # 1. Update Zone objects
    zone_dir = os.path.join(repo_root, "saves", "zones", "Neural Network")
    zone_file = os.path.join(zone_dir, "zone.json")
    
    with open(zone_file, "r") as f:
        zone_data = json.load(f)
        
    # We will use the first object as template
    base_object = zone_data["world"]["objects"][0].copy()
    
    def make_node(obj_id, tx, ty, tz, color):
        obj = json.loads(json.dumps(base_object))
        obj["objectID"] = obj_id
        obj["transform"][12] = tx
        obj["transform"][13] = ty
        obj["transform"][14] = tz
        obj["faceColors"] = [color for _ in range(6)]
        obj["shapeKind"] = 2 # Sphere
        if "shape" in obj:
            obj["shape"]["kind"] = 2
        return obj

    # Keep original 5 objects if we want, or clear them.
    # The prompt says "I DONT SEE THE NN ZONE". So they probably haven't entered it.
    # Let's just append to be safe.
    # Filter out previous nn nodes if they exist
    objects = [o for o in zone_data["world"]["objects"] if not o["objectID"].startswith("node-")]
    
    # Create the 6 nodes
    n_in1  = make_node("node-in-1",  -4.0, 0.5, -2.0, [1.0, 0.0, 0.0])  # Red, input 1
    n_in2  = make_node("node-in-2",  -4.0, 1.0,  2.0, [0.0, 1.0, 0.0])  # Green, input 2 (bias)
    n_w1   = make_node("node-w-1",   -2.0, 0.0, -2.0, [1.0, 1.0, 0.0])  # Yellow, weight 1
    n_w2   = make_node("node-w-2",   -2.0, 0.0,  2.0, [1.0, 1.0, 0.0])  # Yellow, weight 2
    n_out  = make_node("node-out",    0.0, 0.0,  0.0, [0.0, 0.0, 1.0])  # Blue, prediction
    n_tgt  = make_node("node-target", 2.0, 1.5,  0.0, [1.0, 1.0, 1.0])  # White, target

    objects.extend([n_in1, n_in2, n_w1, n_w2, n_out, n_tgt])
    zone_data["world"]["objects"] = objects
    
    with open(zone_file, "w") as f:
        json.dump(zone_data, f)
        
    print("Updated Neural Network/zone.json with 6 nodes.")
    
    # 2. Forward Pass Law
    forward_law_dir = os.path.join(repo_root, "saves", "laws", "law-nn-forward")
    os.makedirs(forward_law_dir, exist_ok=True)
    
    forward_law = {
        "identifier": "law-nn-forward",
        "authors": ["Player"],
        "injected_by": "Antigravity - ML",
        "law": {
            "id": "law-nn-forward",
            "name": "Neural Network Forward Pass",
            "enabled": True,
            "authority": 0,
            "activation": 0,
            "scope": 0, # Universe
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Player"],
            "conditionModel": { "kind": 3, "children": [] },
            "actionModel": {
                "kind": 5, # Sequence
                "children": [
                    {
                        "kind": 8, # Map
                        "path": "@node-out.position.y",
                        "bindings": {
                            "i1": "@node-in-1.position.y",
                            "i2": "@node-in-2.position.y",
                            "w1": "@node-w-1.position.y",
                            "w2": "@node-w-2.position.y"
                        },
                        "function": {
                            "pieces": [
                                {
                                    "expr": {
                                        "terms": [
                                            { "c": 1.0, "factors": { "w1": 1, "i1": 1 } },
                                            { "c": 1.0, "factors": { "w2": 1, "i2": 1 } }
                                        ]
                                    }
                                }
                            ]
                        }
                    }
                ]
            },
            "provenance": []
        },
        "triggers": []
    }
    
    with open(os.path.join(forward_law_dir, "law.json"), "w") as f:
        json.dump(forward_law, f, indent=4)
        
    # 3. Training Law (Gradient Descent)
    train_law_dir = os.path.join(repo_root, "saves", "laws", "law-nn-train")
    os.makedirs(train_law_dir, exist_ok=True)
    
    # Using Flow (kind=9). dW/dt = lr * (Target - Out) * In
    # We will use lr = 5.0 (since it's per second)
    lr = 5.0
    
    train_law = {
        "identifier": "law-nn-train",
        "authors": ["Player"],
        "injected_by": "Antigravity - ML",
        "law": {
            "id": "law-nn-train",
            "name": "Neural Network Gradient Descent",
            "enabled": True,
            "authority": 0,
            "activation": 0,
            "scope": 0,
            "drives": False,
            "retrigger": 0,
            "conditionMode": "all",
            "authors": ["Player"],
            "conditionModel": { "kind": 3, "children": [] },
            "actionModel": {
                "kind": 5, # Sequence
                "children": [
                    {
                        "kind": 9, # Flow
                        "path": "@node-w-1.position.y",
                        "bindings": {
                            "out": "@node-out.position.y",
                            "tgt": "@node-target.position.y",
                            "i1": "@node-in-1.position.y"
                        },
                        "function": {
                            "pieces": [
                                {
                                    "expr": {
                                        "terms": [
                                            { "c": lr, "factors": { "tgt": 1, "i1": 1 } },
                                            { "c": -lr, "factors": { "out": 1, "i1": 1 } }
                                        ]
                                    }
                                }
                            ]
                        }
                    },
                    {
                        "kind": 9, # Flow
                        "path": "@node-w-2.position.y",
                        "bindings": {
                            "out": "@node-out.position.y",
                            "tgt": "@node-target.position.y",
                            "i2": "@node-in-2.position.y"
                        },
                        "function": {
                            "pieces": [
                                {
                                    "expr": {
                                        "terms": [
                                            { "c": lr, "factors": { "tgt": 1, "i2": 1 } },
                                            { "c": -lr, "factors": { "out": 1, "i2": 1 } }
                                        ]
                                    }
                                }
                            ]
                        }
                    }
                ]
            },
            "provenance": []
        },
        "triggers": []
    }
    
    with open(os.path.join(train_law_dir, "law.json"), "w") as f:
        json.dump(train_law, f, indent=4)
        
    print("Created ML laws: law-nn-forward, law-nn-train")

if __name__ == "__main__":
    build_ml()
