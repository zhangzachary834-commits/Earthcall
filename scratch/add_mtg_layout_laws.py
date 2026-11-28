import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {})

# Layout law for stack: places cards at [order * 1.5, 0.5, -2.0]
# wait, the order might be 1, 2, 3...
# we'll use (order - 1) maybe, or just order.
stack_layout = {
  "name": "mtg-layout-stack",
  "authors": ["Gemini"],
  "enabled": True,
  "eventFilter": {
    "event": "WhileTrue",
    "bindings": { "self": "subject" }
  },
  "conditionModel": {
    "kind": 0, # Compare
    "path": "@self.mtg.zone",
    "op": 0,
    "operand": {"t": "string", "v": "stack"}
  },
  "actionModel": {
    "kind": 8, # Map
    "path": "@self.position",
    "function": {
      "input": "x",
      "pieces": [
        {
          "mathNode": {
            "op": 2, # VectorConstruct
            "children": [
              {
                "op": 0,
                "scalarForm": { "terms": [ {"c": 1.5, "factors": {"order": 1.0}} ] }
              },
              {
                "op": 0,
                "scalarForm": { "terms": [ {"c": 0.5} ] }
              },
              {
                "op": 0,
                "scalarForm": { "terms": [ {"c": -2.0} ] }
              }
            ]
          }
        }
      ]
    },
    "bindings": {
      "order": "@self.mtg.stackOrder"
    }
  }
}

# Add a similar law for the hand, to snap them back to the hand!
# Hand position: [order * 1.5, 0.5, 2.0]
hand_layout = {
  "name": "mtg-layout-hand",
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
    "operand": {"t": "string", "v": "hand.1"}
  },
  "actionModel": {
    "kind": 8,
    "path": "@self.position",
    "function": {
      "input": "x",
      "pieces": [
        {
          "mathNode": {
            "op": 2,
            "children": [
              {
                "op": 0,
                "scalarForm": { "terms": [ {"c": 1.5, "factors": {"order": 1.0}} ] }
              },
              {
                "op": 0,
                "scalarForm": { "terms": [ {"c": 0.5} ] }
              },
              {
                "op": 0,
                "scalarForm": { "terms": [ {"c": 2.0} ] }
              }
            ]
          }
        }
      ]
    },
    "bindings": {
      "order": "@self.mtg.handOrder"
    }
  }
}

laws["mtg-layout-stack"] = stack_layout
laws["mtg-layout-hand"] = hand_layout

data["authoredLaws"] = laws

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Injected layout laws!")
