import json

path = "saves/zones/MTG/zone.json"
with open(path, "r") as f:
    data = json.load(f)

# Clear existing test law
if "mtg-pass-priority" in data.get("authoredLaws", {}):
    del data["authoredLaws"]["mtg-pass-priority"]

phases = [
    ("untap", "upkeep"),
    ("upkeep", "draw"),
    ("draw", "main1"),
    ("main1", "combat"),
    ("combat", "main2"),
    ("main2", "end"),
    ("end", "untap")
]

for current_phase, next_phase in phases:
    data["authoredLaws"][f"mtg-pass-priority-{current_phase}"] = {
        "name": f"mtg-pass-priority-{current_phase}",
        "authors": ["Gemini"],
        "enabled": True,
        "eventFilter": {
            "event": "pointer-click",
            "bindings": {
                "self": "subject"
            }
        },
        "conditionModel": {
            "kind": 3,  # All
            "children": [
                {
                    "kind": 8,  # Identity
                    "path": "@self",
                    "otherId": "button.mtg.pass"
                },
                {
                    "kind": 0,  # Compare
                    "path": "@state.mtg.turnPhase",
                    "op": 0,  # ==
                    "operand": {"t": "string", "v": current_phase}
                }
            ]
        },
        "actionModel": {
            "kind": 0,  # Set
            "path": "@state.mtg.turnPhase",
            "op": 0,
            "operand": {"t": "string", "v": next_phase}
        }
    }

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Added phase transition Laws!")
