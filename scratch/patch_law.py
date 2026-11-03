import json

with open("saves/laws/law-art-stroke-draw/law.json", "r") as f:
    law = json.load(f)

# The conditionModel children has the `dragX` condition at index 5.
# Let's replace it with the new condition.
conds = law["law"]["conditionModel"]["children"]
# Find the any_of that has dragX/dragY
for i, c in enumerate(conds):
    if c.get("kind") == 4:
        # replace it with the new any_of
        conds[i] = {
            "kind": 4, # any_of
            "children": [
                {
                    "kind": 0,
                    "op": 1,
                    "operand": {"t": "double", "v": 0.0},
                    "path": "@interaction-channel.dragX"
                },
                {
                    "kind": 0,
                    "op": 1,
                    "operand": {"t": "double", "v": 0.0},
                    "path": "@interaction-channel.dragY"
                }
            ]
        }
        break

# Now append the MathCondition (kind 6) to the end of conds
conds.append({
    "kind": 6,
    "function": {
        "pieces": [
            {
                "expr": {
                    "terms": [
                        {"c": 1.0, "factors": {"px": 2.0}},
                        {"c": -2.0, "factors": {"px": 1.0, "lx": 1.0}},
                        {"c": 1.0, "factors": {"lx": 2.0}},
                        {"c": 1.0, "factors": {"py": 2.0}},
                        {"c": -2.0, "factors": {"py": 1.0, "ly": 1.0}},
                        {"c": 1.0, "factors": {"ly": 2.0}},
                        {"c": 1.0, "factors": {"pz": 2.0}},
                        {"c": -2.0, "factors": {"pz": 1.0, "lz": 1.0}},
                        {"c": 1.0, "factors": {"lz": 2.0}},
                        {"c": -1.0, "factors": {"s": 2.0}}
                    ]
                }
            }
        ]
    },
    "bindings": {
        "px": "@interaction-channel.pointerWorldX",
        "py": "@interaction-channel.pointerWorldY",
        "pz": "@interaction-channel.pointerWorldZ",
        "lx": "@state.studio.lastStrokeX",
        "ly": "@state.studio.lastStrokeY",
        "lz": "@state.studio.lastStrokeZ",
        "s": "@state.studio.strokeSpacing"
    },
    "lo": {"t": "double", "v": 0.0}
})

with open("saves/laws/law-art-stroke-draw/law.json", "w") as f:
    json.dump(law, f, indent=2)

