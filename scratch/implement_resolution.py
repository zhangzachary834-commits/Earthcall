import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {})

# 1. Update mtg-cast-card to maintain topStackOrder
cast_card_action = laws["mtg-cast-card"]["actionModel"]
cast_card_action["children"].insert(2, {
    "kind": 8, # Map
    "path": "@state.mtg.topStackOrder",
    "function": {
        "input": "x",
        "pieces": [ { "mathNode": { "op": 1, "var": "c" } } ]
    },
    "bindings": { "c": "@state.mtg.stackCounter" }
})

# Initialize topStackOrder in state.mtg
for obj in data["objects"]:
    if obj["objectID"] == "state.mtg":
        obj.setdefault("authoredProperties", {})
        obj["authoredProperties"]["topStackOrder"] = {"t": "number", "v": -1.0}
        obj["authoredProperties"]["stackCounter"] = {"t": "number", "v": 0.0}

# 2. Add button.mtg.resolve
data["objects"].append({
    "objectID": "button.mtg.resolve",
    "shapeKind": 0,
    "geometryType": 0,
    "shapeParams": [0.5, 0.1, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    "position": [2.0, 0.0, -1.0],
    "authoredProperties": {
        "label": {"t": "string", "v": "Resolve"}
    }
})

# 3. Add mtg-trigger-resolve Law
laws["mtg-trigger-resolve"] = {
    "name": "mtg-trigger-resolve",
    "authors": ["Gemini"],
    "enabled": True,
    "eventFilter": {
        "event": "pointer-click",
        "bindings": { "self": "subject" }
    },
    "conditionModel": {
        "kind": 0,
        "path": "@self.objectID",
        "op": 0,
        "operand": {"t": "string", "v": "button.mtg.resolve"}
    },
    "actionModel": {
        "kind": 9, # Publish
        "eventType": "resolve-stack"
    }
}

# 4. Add mtg-execute-resolve Law
laws["mtg-execute-resolve"] = {
    "name": "mtg-execute-resolve",
    "authors": ["Gemini"],
    "enabled": True,
    "eventFilter": {
        "event": "resolve-stack",
        "bindings": { "self": "subject" }
    },
    "conditionModel": {
        "kind": 3,
        "children": [
            {
                "kind": 0,
                "path": "@self.mtg.zone",
                "op": 0,
                "operand": {"t": "string", "v": "stack"}
            },
            {
                "kind": 0,
                "path": "@self.mtg.stackOrder",
                "op": 0,
                "operandPath": "@state.mtg.topStackOrder"
            }
        ]
    },
    "actionModel": {
        "kind": 17,
        "children": [
            {
                "kind": 0, # Set
                "path": "@self.mtg.zone",
                "operand": {"t": "string", "v": "battlefield"}
            },
            {
                "kind": 1, # Add
                "path": "@state.mtg.stackCounter",
                "operand": {"t": "number", "v": -1.0}
            },
            {
                "kind": 1, # Add
                "path": "@state.mtg.topStackOrder",
                "operand": {"t": "number", "v": -1.0}
            }
        ]
    }
}

# Also need a layout law for battlefield so cards don't overlap when resolved
laws["mtg-layout-battlefield"] = {
    "name": "mtg-layout-battlefield",
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
        "operand": {"t": "string", "v": "battlefield"}
    },
    "actionModel": {
        "kind": 17,
        "children": [
            {
                "kind": 8, # Map to layoutIndex
                "path": "@self.position.x",
                "function": {
                    "input": "x",
                    "pieces": [ { "mathNode": { "op": 0, "scalarForm": { "terms": [ {"c": 1.5, "factors": {"i": 1.0}} ] } } } ]
                },
                "bindings": { "i": "@self.mtg.layoutIndex" }
            },
            {
                "kind": 0,
                "path": "@self.position.y",
                "operand": {"t": "number", "v": 0.5}
            },
            {
                "kind": 0,
                "path": "@self.position.z",
                "operand": {"t": "number", "v": 0.0}
            }
        ]
    }
}

# For now, cards on the battlefield need layoutIndex. Let's patch the cards to have layoutIndex if they don't.
# Wait, resolving them puts them all at index 0. They will overlap.
# Let's just hardcode a counter for layoutIndex when resolving.
laws["mtg-execute-resolve"]["actionModel"]["children"].append({
    "kind": 8,
    "path": "@self.mtg.layoutIndex",
    "function": { "input": "x", "pieces": [ { "mathNode": { "op": 1, "var": "c" } } ] },
    "bindings": { "c": "@state.mtg.battlefieldCounter" }
})
laws["mtg-execute-resolve"]["actionModel"]["children"].append({
    "kind": 1,
    "path": "@state.mtg.battlefieldCounter",
    "operand": {"t": "number", "v": 1.0}
})

for obj in data["objects"]:
    if obj["objectID"] == "state.mtg":
        obj["authoredProperties"]["battlefieldCounter"] = {"t": "number", "v": 0.0}

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Implemented Step 5: Card Resolution!")
