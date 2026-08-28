import json
import os

def pv(type_name, value):
    return {"t": type_name, "v": value}

def compare(path, op, value_node):
    return {"kind": 0, "path": path, "op": op, "operand": value_node}

def all_of(*conditions):
    return {"kind": 3, "conditions": list(conditions)}

def map_path(path, bindings, terms):
    return {
        "kind": 8,
        "path": path,
        "bindings": bindings,
        "function": {"pieces": [{"expr": {"terms": terms}}]}
    }

def seq(*actions):
    return {"kind": 1, "actions": list(actions)}

law = {
    "identifier": "law-toggle-ground",
    "name": "toggle-ground-visibility",
    "activation": 0,  # OnEvent
    "eventTypes": ["key-pressed"],
    "condition": all_of(
        compare("@event.subject", 0, pv("string", "key.g")),
        compare("baseline", 0, pv("string", "ground"))
    ),
    "action": map_path("position.y", {"v": "position.y"}, [
        {"c": -1000.0, "factors": {}},
        {"c": -1.0, "factors": {"v": 1.0}}
    ]),
    "authors": ["person.zach"]
}

for world_file in ["saves/worlds/noise_floor.json", "saves/zones/Home/zone.json"]:
    if os.path.exists(world_file):
        with open(world_file, "r") as f:
            d = json.load(f)
        
        target_list = d.get("laws") if "laws" in d else d.get("world", {}).get("laws")
        if target_list is None:
            if "world" in d:
                d["world"]["laws"] = []
                target_list = d["world"]["laws"]
            else:
                d["laws"] = []
                target_list = d["laws"]
        
        # Remove old injected law
        target_list[:] = [l for l in target_list if l.get("identifier") != "law-toggle-ground"]
        target_list.append(law)
        
        with open(world_file, "w") as f:
            json.dump(d, f, indent=2)
        print(f"Injected into {world_file}")

