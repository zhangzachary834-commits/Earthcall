from datetime import datetime
import json
import os
import uuid

# --- Helper Methods ---

def create_property(prop_type, value):
    return {"type": prop_type, "value": value}

def pv(prop_type, value):
    return create_property(prop_type, value)

def build_being(kind, properties, owner=None, id_str=None):
    if id_str is None:
        id_str = str(uuid.uuid4())
    b = {
        "id": id_str,
        "kind": kind,
        "properties": properties,
        "authors": ["Zach"],
        "injected_by": "Go Setup Script",
        "createdAt": datetime.utcnow().isoformat() + "Z"
    }
    if owner:
        b["owner"] = owner
    return b

def build_law(name, action_model, conditions=None):
    return {
        "id": str(uuid.uuid4()),
        "name": name,
        "authors": ["Zach"],
        "injected_by": "Go Setup Script",
        "createdAt": datetime.utcnow().isoformat() + "Z",
        "actionModel": action_model,
        "conditions": conditions or [],
        "enabled": True,
        "authorityLevel": 0
    }

def build_relation(relation_type, a, b):
    return {
        "id": str(uuid.uuid4()),
        "relation_type": relation_type,
        "a": a,
        "b": b,
        "authors": ["Zach"],
        "injected_by": "Go Setup Script",
        "createdAt": datetime.utcnow().isoformat() + "Z"
    }


# --- Domain Constants ---
ZONE_ID = "zone-go"

# Properties
COLOR_BLACK = [0.1, 0.1, 0.1, 1.0]
COLOR_WHITE = [0.9, 0.9, 0.9, 1.0]
COLOR_BOARD = [0.85, 0.7, 0.45, 1.0]

# --- Build the Go App World ---

def build_go_world():
    world = {
        "formatVersion": 2,
        "name": "Go",
        "zones": [
            {
                "id": ZONE_ID,
                "name": "Go Game",
                "objects": [],
                "laws": [],
                "relations": [],
                "formations": []
            }
        ]
    }
    z = world["zones"][0]

    board_id = "go_board"
    board = build_being("object", {
        "shape": pv("string", "Cube"),
        "position": pv("vec3", [0.0, 0.0, 0.0]),
        "scale": pv("vec3", [19.0, 0.5, 19.0]),
        "color": pv("vec4", COLOR_BOARD)
    }, id_str=board_id)
    z["objects"].append(board)

    game_state_id = "go_state"
    game_state = build_being("object", {
        "current_turn": pv("string", "black")
    }, id_str=game_state_id)
    z["objects"].append(game_state)

    for x in range(19):
        for y in range(19):
            ix_id = f"intersection_{x}_{y}"
            ix = build_being("object", {
                "grid_x": pv("double", float(x)),
                "grid_y": pv("double", float(y)),
                "is_empty": pv("bool", True),
                "stone_color": pv("string", "none"),
                "position": pv("vec3", [x - 9.0, 0.26, y - 9.0]),
                "scale": pv("vec3", [0.8, 0.1, 0.8]),
                "color": pv("vec4", COLOR_BOARD)
            }, id_str=ix_id)
            z["objects"].append(ix)

    # Core Go Mechanics: Hover Preview
    hover_law = build_law(
        name="Hover preview",
        action_model=[
            {"type": "ActionNode::Set", "path": "color", "value": pv("vec4", [0.6, 0.6, 0.6, 1.0])}
        ],
        conditions=[
            {"type": "ConditionNode::Equals", "path": "is_empty", "value": pv("bool", True)},
            {"type": "ConditionNode::RelationExists", "relation_type": "is-hovering"}
        ]
    )
    z["laws"].append(hover_law)

    # Core Go Mechanics: Place Stones
    place_black_law = build_law(
        name="Place Black Stone",
        action_model=[
            {"type": "ActionNode::Set", "path": "is_empty", "value": pv("bool", False)},
            {"type": "ActionNode::Set", "path": "stone_color", "value": pv("string", "black")},
            {"type": "ActionNode::Set", "path": "color", "value": pv("vec4", COLOR_BLACK)},
            {"type": "ActionNode::Set", "path": "shape", "value": pv("string", "Sphere")},
            {"type": "ActionNode::Set", "path": "scale", "value": pv("vec3", [0.9, 0.5, 0.9])},
            {"type": "ActionNode::SetRemote", "target": game_state_id, "path": "current_turn", "value": pv("string", "white")}
        ],
        conditions=[
            {"type": "ConditionNode::Equals", "path": "is_empty", "value": pv("bool", True)},
            {"type": "ConditionNode::RelationExists", "relation_type": "is-interacting"},
            {"type": "ConditionNode::RemoteEquals", "target": game_state_id, "path": "current_turn", "value": pv("string", "black")}
        ]
    )
    z["laws"].append(place_black_law)

    place_white_law = build_law(
        name="Place White Stone",
        action_model=[
            {"type": "ActionNode::Set", "path": "is_empty", "value": pv("bool", False)},
            {"type": "ActionNode::Set", "path": "stone_color", "value": pv("string", "white")},
            {"type": "ActionNode::Set", "path": "color", "value": pv("vec4", COLOR_WHITE)},
            {"type": "ActionNode::Set", "path": "shape", "value": pv("string", "Sphere")},
            {"type": "ActionNode::Set", "path": "scale", "value": pv("vec3", [0.9, 0.5, 0.9])},
            {"type": "ActionNode::SetRemote", "target": game_state_id, "path": "current_turn", "value": pv("string", "black")}
        ],
        conditions=[
            {"type": "ConditionNode::Equals", "path": "is_empty", "value": pv("bool", True)},
            {"type": "ConditionNode::RelationExists", "relation_type": "is-interacting"},
            {"type": "ConditionNode::RemoteEquals", "target": game_state_id, "path": "current_turn", "value": pv("string", "white")}
        ]
    )
    z["laws"].append(place_white_law)

    return world

if __name__ == "__main__":
    os.makedirs("saves/worlds", exist_ok=True)
    with open("saves/worlds/go_app.json", "w") as f:
        json.dump(build_go_world(), f, indent=2)
    print("Go world save generated.")
