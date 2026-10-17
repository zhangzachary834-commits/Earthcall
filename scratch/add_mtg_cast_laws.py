import json

save_path = "saves/zones/MTG/zone.json"
with open(save_path, "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {})

cast_card_law = {
  "name": "mtg-cast-card",
  "authors": ["Gemini"],
  "enabled": True,
  "eventFilter": {
    "event": "pointer-click",
    "bindings": { "self": "subject" }
  },
  "conditionModel": {
    "kind": 3,
    "children": [
      {
        "kind": 0,
        "path": "@self.mtg.zone",
        "op": 0,
        "operand": {"t": "string", "v": "hand.1"}
      },
      {
        "kind": 0,
        "path": "@state.mtg.turnPhase",
        "op": 0,
        "operand": {"t": "string", "v": "main1"}
      }
    ]
  },
  "actionModel": {
    "kind": 17,
    "children": [
      {
        "kind": 0,
        "path": "@self.mtg.zone",
        "operand": {"t": "string", "v": "stack"}
      },
      {
        "kind": 8,
        "path": "@self.mtg.stackOrder",
        "function": {
          "input": "x",
          "pieces": [
            {
              "mathNode": {
                "op": 1,
                "var": "c"
              }
            }
          ]
        },
        "bindings": {
          "c": "@state.mtg.stackCounter"
        }
      },
      {
        "kind": 1,
        "path": "@state.mtg.stackCounter",
        "operand": {"t": "number", "v": 1.0}
      }
    ]
  }
}

laws["mtg-cast-card"] = cast_card_law
data["authoredLaws"] = laws

with open(save_path, "w") as f:
    json.dump(data, f, indent=2)

print("Injected mtg-cast-card law!")
