import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {})

laws["mtg-sba-lethal-damage"] = {
    "name": "mtg-sba-lethal-damage",
    "authors": ["Gemini"],
    "enabled": True,
    "eventFilter": {
        "event": "WhileTrue",
        "bindings": { "self": "subject" }
    },
    "conditionModel": {
        "kind": 3,
        "children": [
            {
                "kind": 0,
                "path": "@self.mtg.zone",
                "op": 0,
                "operand": {"t": "string", "v": "battlefield"}
            },
            {
                "kind": 0,
                "path": "@self.mtg.damage",
                "op": 3, # >=
                "operandPath": "@self.mtg.toughness"
            }
        ]
    },
    "actionModel": {
        "kind": 17,
        "children": [
            {
                "kind": 0, # Set
                "path": "@self.mtg.zone",
                "operand": {"t": "string", "v": "graveyard"}
            },
            {
                "kind": 8, # Map
                "path": "@self.mtg.graveyardOrder",
                "function": { "input": "x", "pieces": [ { "mathNode": { "op": 1, "var": "c" } } ] },
                "bindings": { "c": "@state.mtg.graveyardCounter" }
            },
            {
                "kind": 1, # Add
                "path": "@state.mtg.graveyardCounter",
                "operand": {"t": "number", "v": 1.0}
            }
        ]
    }
}

# Layout law for graveyard
laws["mtg-layout-graveyard"] = {
    "name": "mtg-layout-graveyard",
    "authors": ["Gemini"],
    "enabled": True,
    "eventFilter": {
        "event": "WhileTrue",
        "bindings": { "self": "subject" }
    },
    "conditionModel": {
        "kind": 0,
        "path": "@self.mtg.zone",
        "op": 0,
        "operand": {"t": "string", "v": "graveyard"}
    },
    "actionModel": {
        "kind": 17,
        "children": [
            {
                "kind": 8, # Map
                "path": "@self.position.x",
                "function": {
                    "input": "x",
                    "pieces": [ { "mathNode": { "op": 0, "scalarForm": { "terms": [ {"c": -2.0, "factors": {"i": 1.0}} ] } } } ]
                },
                "bindings": { "i": "@self.mtg.graveyardOrder" }
            },
            {
                "kind": 0,
                "path": "@self.position.y",
                "operand": {"t": "number", "v": 0.5}
            },
            {
                "kind": 0,
                "path": "@self.position.z",
                "operand": {"t": "number", "v": 4.0}
            }
        ]
    }
}

# Initialize graveyardCounter in state.mtg
for obj in data["objects"]:
    if obj["objectID"] == "state.mtg":
        obj["authoredProperties"]["graveyardCounter"] = {"t": "number", "v": 0.0}

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Implemented Step 6: State-Based Actions!")
