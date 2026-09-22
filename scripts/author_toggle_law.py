import json

def pv(type_name, value):
    return {"t": type_name, "v": value}

def compare(path, op, value_node):
    return {"kind": 0, "path": path, "op": op, "value": value_node}

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
    "action": seq(
        map_path("transform.scale.x", {"v": "transform.scale.x"}, [
            {"c": 1000.0, "factors": {}},
            {"c": -1.0, "factors": {"v": 1.0}}
        ]),
        map_path("transform.scale.y", {"v": "transform.scale.y"}, [
            {"c": 1000.0, "factors": {}},
            {"c": -1.0, "factors": {"v": 1.0}}
        ]),
        map_path("transform.scale.z", {"v": "transform.scale.z"}, [
            {"c": 1000.0, "factors": {}},
            {"c": -1.0, "factors": {"v": 1.0}}
        ])
    ),
    "authors": ["person.zach"]
}

# Wait, if scale is 1000, 1000 - 1000 = 0, 1000 - 0 = 1000.
# BUT what if the scale is NOT 1000? 
# Maybe just move it underground?
# position.y = -10000.0 - position.y ?
# -10000 - 0 = -10000. -10000 - (-10000) = 0.
