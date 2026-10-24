#!/usr/bin/env python3
import json
import os

def make_transform(sx, sy, sz, tx, ty, tz):
    return [
        float(sx), 0.0, 0.0, 0.0,
        0.0, float(sy), 0.0, 0.0,
        0.0, 0.0, float(sz), 0.0,
        float(tx), float(ty), float(tz), 1.0
    ]

def make_law(identifier, name, activation, scope, triggers, condition_model, action_model, targets=None):
    return {
        "authors": [
            "author.antigravity"
        ],
        "identifier": identifier,
        "injected_by": "Antigravity, 2026-10-11 MTG Zone Law",
        "law": {
            "actionDescriptions": [
                action_model.get("desc", "action")
            ],
            "actionModel": action_model,
            "activation": activation,
            "authority": 0,
            "authors": [
                "author.antigravity"
            ],
            "conditionDescriptions": [
                condition_model.get("desc", "condition")
            ],
            "conditionMode": "all",
            "conditionModel": condition_model,
            "drives": False,
            "enabled": True,
            "id": identifier,
            "name": name,
            "retrigger": 0,
            "scope": scope,
            "targets": targets if targets is not None else []
        },
        "triggers": triggers
    }

def cond_all(children, desc="all"):
    return {"kind": 3, "children": children, "desc": desc}

def cond_identity(other_id, desc=""):
    return {"kind": 8, "otherId": other_id, "desc": desc or f"identity({other_id})"}

def cond_compare(path, op, operand=None, operand_path=None, desc=""):
    node = {"kind": 0, "path": path, "op": op, "desc": desc or f"compare({path})"}
    if operand is not None:
        node["operand"] = operand
    if operand_path is not None:
        node["operandPath"] = operand_path
    return node

def act_set(path, operand, desc=""):
    return {"kind": 0, "path": path, "operand": operand, "desc": desc or f"set({path})"}

def act_add(path, operand, desc=""):
    return {"kind": 1, "path": path, "operand": operand, "desc": desc or f"add({path})"}

def act_map_copy(target_path, source_path, desc=""):
    return {
        "kind": 8,
        "path": target_path,
        "bindings": {
            "v": source_path
        },
        "function": {
            "input": "x",
            "pieces": [
                {
                    "mathNode": {
                        "op": 0,
                        "scalarForm": {
                            "terms": [
                                {
                                    "c": 1.0,
                                    "factors": {
                                        "v": 1.0
                                    }
                                }
                            ]
                        }
                    }
                }
            ]
        },
        "desc": desc or f"map({target_path} := {source_path})"
    }

def act_seq(children, desc="sequence"):
    return {"kind": 5, "children": children, "desc": desc}

def act_publish(event_type, desc=""):
    return {"kind": 10, "eventType": event_type, "desc": desc or f"publish({event_type})"}

def act_map_phase_step(target_path="@state.mtg.phase", desc="map(phase step)"):
    return {
        "kind": 8,
        "path": target_path,
        "bindings": {
            "p": target_path
        },
        "function": {
            "input": "p",
            "pieces": [
                {
                    "lo": -0.5,
                    "hi": 5.5,
                    "includeLo": True,
                    "includeHi": True,
                    "mathNode": {
                        "op": 0,
                        "scalarForm": {
                            "terms": [
                                {
                                    "c": 1.0,
                                    "factors": {
                                        "p": 1.0
                                    }
                                },
                                {
                                    "c": 1.0,
                                    "factors": {}
                                }
                            ]
                        }
                    }
                },
                {
                    "lo": 5.5,
                    "hi": 6.5,
                    "includeLo": True,
                    "includeHi": True,
                    "mathNode": {
                        "op": 0,
                        "scalarForm": {
                            "terms": [
                                {
                                    "c": 0.0,
                                    "factors": {}
                                }
                            ]
                        }
                    }
                }
            ]
        },
        "desc": desc
    }

def main():
    laws = {}

    # 1. law-mtg-turn-cycle (evaluated before pass-phase on button.mtg.pass_phase clicks)
    laws["law-mtg-turn-cycle"] = make_law(
        identifier="law-mtg-turn-cycle",
        name="mtg-turn-cycle",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("button.mtg.pass_phase"),
            cond_compare("@state.mtg.phase", 0, {"t": "int", "v": 6}) # phase == 6
        ]),
        action_model=act_seq([
            act_add("@state.mtg.turnNumber", {"t": "int", "v": 1}),
            act_set("@state.mtg.landPlayedThisTurn", {"t": "int", "v": 0}),
            act_set("@card.forest_1.isTapped", {"t": "int", "v": 0}),
            act_set("@card.forest_1.rotation.y", {"t": "float", "v": 0.0}),
            act_set("@card.mountain_1.isTapped", {"t": "int", "v": 0}),
            act_set("@card.mountain_1.rotation.y", {"t": "float", "v": 0.0}),
            act_set("@card.grizzly_bears_1.isTapped", {"t": "int", "v": 0}),
            act_set("@card.grizzly_bears_1.rotation.y", {"t": "float", "v": 0.0}),
            act_set("@card.grizzly_bears_1.isAttacking", {"t": "int", "v": 0})
        ])
    )

    # 2. law-mtg-pass-phase (Piecewise OntoMath step: [0..5] -> p+1, [6] -> 0)
    laws["law-mtg-pass-phase"] = make_law(
        identifier="law-mtg-pass-phase",
        name="mtg-pass-phase",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_identity("button.mtg.pass_phase"),
        action_model=act_map_phase_step("@state.mtg.phase")
    )

    # 3. law-mtg-tap-forest (registered before play-forest so clicking in hand fails battlefield check)
    laws["law-mtg-tap-forest"] = make_law(
        identifier="law-mtg-tap-forest",
        name="mtg-tap-forest",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.forest_1"),
            cond_compare("zone", 0, {"t": "string", "v": "battlefield"}),
            cond_compare("isTapped", 0, {"t": "int", "v": 0})
        ]),
        action_model=act_seq([
            act_set("isTapped", {"t": "int", "v": 1}),
            act_set("rotation.y", {"t": "float", "v": 90.0}),
            act_add("@state.mtg.manaGreen", {"t": "int", "v": 1})
        ])
    )

    # 4. law-mtg-play-forest
    laws["law-mtg-play-forest"] = make_law(
        identifier="law-mtg-play-forest",
        name="mtg-play-forest",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.forest_1"),
            cond_compare("zone", 0, {"t": "string", "v": "hand"}),
            cond_compare("@state.mtg.landPlayedThisTurn", 0, {"t": "int", "v": 0})
        ]),
        action_model=act_seq([
            act_set("zone", {"t": "string", "v": "battlefield"}),
            act_set("position", {"t": "vec3", "x": -2.0, "y": 0.52, "z": 1.0}),
            act_set("@state.mtg.landPlayedThisTurn", {"t": "int", "v": 1})
        ])
    )

    # 5. law-mtg-tap-mountain (registered before play-mountain so clicking in hand fails battlefield check)
    laws["law-mtg-tap-mountain"] = make_law(
        identifier="law-mtg-tap-mountain",
        name="mtg-tap-mountain",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.mountain_1"),
            cond_compare("zone", 0, {"t": "string", "v": "battlefield"}),
            cond_compare("isTapped", 0, {"t": "int", "v": 0})
        ]),
        action_model=act_seq([
            act_set("isTapped", {"t": "int", "v": 1}),
            act_set("rotation.y", {"t": "float", "v": 90.0}),
            act_add("@state.mtg.manaRed", {"t": "int", "v": 1})
        ])
    )

    # 6. law-mtg-play-mountain
    laws["law-mtg-play-mountain"] = make_law(
        identifier="law-mtg-play-mountain",
        name="mtg-play-mountain",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.mountain_1"),
            cond_compare("zone", 0, {"t": "string", "v": "hand"}),
            cond_compare("@state.mtg.landPlayedThisTurn", 0, {"t": "int", "v": 0})
        ]),
        action_model=act_seq([
            act_set("zone", {"t": "string", "v": "battlefield"}),
            act_set("position", {"t": "vec3", "x": -0.8, "y": 0.52, "z": 1.0}),
            act_set("@state.mtg.landPlayedThisTurn", {"t": "int", "v": 1})
        ])
    )

    # 7. law-mtg-cast-grizzly-bears
    laws["law-mtg-cast-grizzly-bears"] = make_law(
        identifier="law-mtg-cast-grizzly-bears",
        name="mtg-cast-grizzly-bears",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.grizzly_bears_1"),
            cond_compare("zone", 0, {"t": "string", "v": "hand"}),
            cond_compare("@state.mtg.manaGreen", 5, {"t": "int", "v": 1}) # manaGreen >= 1
        ]),
        action_model=act_seq([
            act_add("@state.mtg.manaGreen", {"t": "int", "v": -1}),
            act_add("@state.mtg.stackCount", {"t": "int", "v": 1}),
            act_map_copy("stackOrder", "@state.mtg.stackCount"),
            act_set("zone", {"t": "string", "v": "stack"}),
            act_set("position", {"t": "vec3", "x": 0.0, "y": 0.6, "z": 0.0})
        ])
    )

    # 8. law-mtg-cast-lightning-bolt
    laws["law-mtg-cast-lightning-bolt"] = make_law(
        identifier="law-mtg-cast-lightning-bolt",
        name="mtg-cast-lightning-bolt",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.lightning_bolt_1"),
            cond_compare("zone", 0, {"t": "string", "v": "hand"}),
            cond_compare("@state.mtg.manaRed", 5, {"t": "int", "v": 1}) # manaRed >= 1
        ]),
        action_model=act_seq([
            act_add("@state.mtg.manaRed", {"t": "int", "v": -1}),
            act_add("@state.mtg.stackCount", {"t": "int", "v": 1}),
            act_map_copy("stackOrder", "@state.mtg.stackCount"),
            act_set("zone", {"t": "string", "v": "stack"}),
            act_set("position", {"t": "vec3", "x": 0.0, "y": 0.65, "z": 0.0})
        ])
    )

    # 9. law-mtg-trigger-resolve
    laws["law-mtg-trigger-resolve"] = make_law(
        identifier="law-mtg-trigger-resolve",
        name="mtg-trigger-resolve",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("button.mtg.resolve"),
            cond_compare("@state.mtg.stackCount", 4, {"t": "int", "v": 0}) # stackCount > 0
        ]),
        action_model=act_publish("resolve-stack")
    )

    # 10. law-mtg-resolve-lightning-bolt
    laws["law-mtg-resolve-lightning-bolt"] = make_law(
        identifier="law-mtg-resolve-lightning-bolt",
        name="mtg-resolve-lightning-bolt",
        activation=0,
        scope=1, # Everyone
        triggers=["resolve-stack"],
        condition_model=cond_all([
            cond_identity("card.lightning_bolt_1"),
            cond_compare("zone", 0, {"t": "string", "v": "stack"}),
            cond_compare("stackOrder", 0, operand_path="@state.mtg.stackCount")
        ]),
        action_model=act_seq([
            act_add("@state.mtg.player2Life", {"t": "int", "v": -3}),
            act_set("zone", {"t": "string", "v": "graveyard"}),
            act_set("position", {"t": "vec3", "x": 3.0, "y": 0.52, "z": -2.0}),
            act_set("stackOrder", {"t": "int", "v": 0}),
            act_add("@state.mtg.stackCount", {"t": "int", "v": -1})
        ])
    )

    # 11. law-mtg-resolve-grizzly-bears
    laws["law-mtg-resolve-grizzly-bears"] = make_law(
        identifier="law-mtg-resolve-grizzly-bears",
        name="mtg-resolve-grizzly-bears",
        activation=0,
        scope=1, # Everyone
        triggers=["resolve-stack"],
        condition_model=cond_all([
            cond_identity("card.grizzly_bears_1"),
            cond_compare("zone", 0, {"t": "string", "v": "stack"}),
            cond_compare("stackOrder", 0, operand_path="@state.mtg.stackCount")
        ]),
        action_model=act_seq([
            act_set("zone", {"t": "string", "v": "battlefield"}),
            act_set("position", {"t": "vec3", "x": 0.5, "y": 0.52, "z": 0.5}),
            act_set("stackOrder", {"t": "int", "v": 0}),
            act_add("@state.mtg.stackCount", {"t": "int", "v": -1})
        ])
    )

    # 12. law-mtg-attack-grizzly-bears
    laws["law-mtg-attack-grizzly-bears"] = make_law(
        identifier="law-mtg-attack-grizzly-bears",
        name="mtg-attack-grizzly-bears",
        activation=0,
        scope=0,
        triggers=["object-clicked"],
        condition_model=cond_all([
            cond_identity("card.grizzly_bears_1"),
            cond_compare("zone", 0, {"t": "string", "v": "battlefield"}),
            cond_compare("@state.mtg.phase", 0, {"t": "int", "v": 4}), # Combat
            cond_compare("isTapped", 0, {"t": "int", "v": 0})
        ]),
        action_model=act_seq([
            act_set("isTapped", {"t": "int", "v": 1}),
            act_set("isAttacking", {"t": "int", "v": 1}),
            act_set("rotation.y", {"t": "float", "v": 90.0}),
            act_set("position", {"t": "vec3", "x": 0.5, "y": 0.52, "z": -0.5}),
            act_add("@state.mtg.player2Life", {"t": "int", "v": -2})
        ])
    )

    # 13. law-mtg-sba-lethal-damage
    laws["law-mtg-sba-lethal-damage"] = make_law(
        identifier="law-mtg-sba-lethal-damage",
        name="mtg-sba-lethal-damage",
        activation=1, # WhileTrue
        scope=1,      # Everyone
        triggers=[],
        condition_model=cond_all([
            cond_compare("zone", 0, {"t": "string", "v": "battlefield"}),
            cond_compare("cardType", 0, {"t": "string", "v": "creature"}),
            cond_compare("damage", 5, operand_path="toughness") # damage >= toughness
        ]),
        action_model=act_seq([
            act_set("zone", {"t": "string", "v": "graveyard"}),
            act_set("position", {"t": "vec3", "x": 3.0, "y": 0.52, "z": -2.0}),
            act_set("damage", {"t": "int", "v": 0}),
            act_set("isTapped", {"t": "int", "v": 0}),
            act_set("rotation.y", {"t": "float", "v": 0.0})
        ])
    )

    # 14. law-mtg-sba-player-loss
    laws["law-mtg-sba-player-loss"] = make_law(
        identifier="law-mtg-sba-player-loss",
        name="mtg-sba-player-loss",
        activation=1, # WhileTrue
        scope=0,      # Subject
        targets=["state.mtg"],
        triggers=[],
        condition_model=cond_all([
            cond_compare("@state.mtg.player2Life", 3, {"t": "int", "v": 0}), # player2Life <= 0
            cond_compare("@state.mtg.gameOver", 0, {"t": "bool", "v": False})
        ]),
        action_model=act_set("@state.mtg.gameOver", {"t": "bool", "v": True})
    )

    # Write each law file
    for law_id, law_json in laws.items():
        law_dir = os.path.join("saves", "laws", law_id)
        os.makedirs(law_dir, exist_ok=True)
        law_path = os.path.join(law_dir, "law.json")
        with open(law_path, "w", encoding="utf-8") as f:
            json.dump(law_json, f, indent=2)
        print(f"Wrote {law_path}")

    # Materials for MTG Zone
    materials = [
        {
            "name": "mtg.table",
            "id": "material.mtg.table",
            "ambient": 0.25,
            "diffuse": 0.85,
            "specular": 0.4,
            "shininess": 24.0,
            "opacity": 1.0,
            "baseColor": [0.22, 0.12, 0.07]
        },
        {
            "name": "mtg.playmat",
            "id": "material.mtg.playmat",
            "ambient": 0.3,
            "diffuse": 0.9,
            "specular": 0.2,
            "shininess": 16.0,
            "opacity": 1.0,
            "baseColor": [0.08, 0.10, 0.14]
        },
        {
            "name": "mtg.stack",
            "id": "material.mtg.stack",
            "ambient": 0.4,
            "diffuse": 0.8,
            "specular": 0.8,
            "shininess": 64.0,
            "opacity": 1.0,
            "baseColor": [0.35, 0.18, 0.55]
        },
        {
            "name": "mtg.button.pass",
            "id": "material.mtg.button.pass",
            "ambient": 0.35,
            "diffuse": 0.9,
            "specular": 1.0,
            "shininess": 80.0,
            "opacity": 1.0,
            "baseColor": [0.85, 0.65, 0.15]
        },
        {
            "name": "mtg.button.resolve",
            "id": "material.mtg.button.resolve",
            "ambient": 0.4,
            "diffuse": 0.9,
            "specular": 1.0,
            "shininess": 90.0,
            "opacity": 1.0,
            "baseColor": [0.15, 0.65, 0.85]
        },
        {
            "name": "mtg.card.forest",
            "id": "material.mtg.card.forest",
            "ambient": 0.25,
            "diffuse": 0.85,
            "specular": 0.5,
            "shininess": 32.0,
            "opacity": 1.0,
            "baseColor": [0.18, 0.45, 0.22]
        },
        {
            "name": "mtg.card.mountain",
            "id": "material.mtg.card.mountain",
            "ambient": 0.25,
            "diffuse": 0.85,
            "specular": 0.5,
            "shininess": 32.0,
            "opacity": 1.0,
            "baseColor": [0.65, 0.25, 0.15]
        },
        {
            "name": "mtg.card.bears",
            "id": "material.mtg.card.bears",
            "ambient": 0.25,
            "diffuse": 0.85,
            "specular": 0.5,
            "shininess": 32.0,
            "opacity": 1.0,
            "baseColor": [0.45, 0.32, 0.18]
        },
        {
            "name": "mtg.card.bolt",
            "id": "material.mtg.card.bolt",
            "ambient": 0.3,
            "diffuse": 0.9,
            "specular": 0.7,
            "shininess": 48.0,
            "opacity": 1.0,
            "baseColor": [0.85, 0.35, 0.10]
        },
        {
            "name": "mtg.crystal.p1",
            "id": "material.mtg.crystal.p1",
            "ambient": 0.35,
            "diffuse": 0.85,
            "specular": 0.9,
            "shininess": 70.0,
            "opacity": 1.0,
            "baseColor": [0.2, 0.8, 0.3]
        },
        {
            "name": "mtg.crystal.p2",
            "id": "material.mtg.crystal.p2",
            "ambient": 0.35,
            "diffuse": 0.85,
            "specular": 0.9,
            "shininess": 70.0,
            "opacity": 1.0,
            "baseColor": [0.85, 0.2, 0.2]
        }
    ]

    # Objects in MTG Zone
    objects = [
        # Table (wood surface at y = 0.50)
        {
            "objectID": "object.mtg.table",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.table",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.05],
            "transform": make_transform(10.0, 0.5, 8.0, 0.0, 0.25, 0.0),
            "center": [0.0, 0.25, 0.0]
        },
        # Playmat (y = 0.51)
        {
            "objectID": "object.mtg.playmat",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.playmat",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.02],
            "transform": make_transform(8.0, 0.02, 6.0, 0.0, 0.51, 0.0),
            "center": [0.0, 0.51, 0.0]
        },
        # Stack Zone (center glowing circle at y = 0.525)
        {
            "objectID": "object.mtg.stack_zone",
            "shapeKind": 3, # Cylinder
            "materialId": "material.mtg.stack",
            "shapeParams": [1.2, 0.015, 1.2, 0.015, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": make_transform(1.5, 0.01, 1.5, 0.0, 0.522, 0.0),
            "center": [0.0, 0.522, 0.0]
        },
        # Pass Phase Button
        {
            "objectID": "button.mtg.pass_phase",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.button.pass",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.02],
            "transform": make_transform(1.0, 0.04, 0.6, 3.0, 0.535, 1.5),
            "center": [3.0, 0.535, 1.5],
            "authoredProperties": {
                "isButton": {"t": "bool", "v": True}
            }
        },
        # Resolve Button
        {
            "objectID": "button.mtg.resolve",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.button.resolve",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.02],
            "transform": make_transform(0.9, 0.04, 0.5, 0.0, 0.535, 1.2),
            "center": [0.0, 0.535, 1.2],
            "authoredProperties": {
                "isButton": {"t": "bool", "v": True}
            }
        },
        # Player 1 Life Crystal
        {
            "objectID": "counter.mtg.life1",
            "shapeKind": 2, # Sphere
            "materialId": "material.mtg.crystal.p1",
            "shapeParams": [0.2, 0.2, 0.2, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": make_transform(0.3, 0.3, 0.3, -3.2, 0.55, 2.2),
            "center": [-3.2, 0.55, 2.2]
        },
        # Player 2 Life Crystal
        {
            "objectID": "counter.mtg.life2",
            "shapeKind": 2, # Sphere
            "materialId": "material.mtg.crystal.p2",
            "shapeParams": [0.2, 0.2, 0.2, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": make_transform(0.3, 0.3, 0.3, -3.2, 0.55, -2.2),
            "center": [-3.2, 0.55, -2.2]
        },
        # Card: Forest
        {
            "objectID": "card.forest_1",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.card.forest",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.01],
            "transform": make_transform(0.7, 0.02, 1.0, -2.0, 0.53, 2.4),
            "center": [-2.0, 0.53, 2.4],
            "authoredProperties": {
                "cardName": {"t": "string", "v": "Forest"},
                "cardType": {"t": "string", "v": "land"},
                "zone": {"t": "string", "v": "hand"},
                "isTapped": {"t": "int", "v": 0}
            }
        },
        # Card: Mountain
        {
            "objectID": "card.mountain_1",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.card.mountain",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.01],
            "transform": make_transform(0.7, 0.02, 1.0, -0.8, 0.53, 2.4),
            "center": [-0.8, 0.53, 2.4],
            "authoredProperties": {
                "cardName": {"t": "string", "v": "Mountain"},
                "cardType": {"t": "string", "v": "land"},
                "zone": {"t": "string", "v": "hand"},
                "isTapped": {"t": "int", "v": 0}
            }
        },
        # Card: Grizzly Bears
        {
            "objectID": "card.grizzly_bears_1",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.card.bears",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.01],
            "transform": make_transform(0.7, 0.02, 1.0, 0.8, 0.53, 2.4),
            "center": [0.8, 0.53, 2.4],
            "authoredProperties": {
                "cardName": {"t": "string", "v": "Grizzly Bears"},
                "cardType": {"t": "string", "v": "creature"},
                "manaCost": {"t": "int", "v": 1},
                "power": {"t": "int", "v": 2},
                "toughness": {"t": "int", "v": 2},
                "damage": {"t": "int", "v": 0},
                "zone": {"t": "string", "v": "hand"},
                "isTapped": {"t": "int", "v": 0},
                "isAttacking": {"t": "int", "v": 0},
                "stackOrder": {"t": "int", "v": 0}
            }
        },
        # Card: Lightning Bolt
        {
            "objectID": "card.lightning_bolt_1",
            "shapeKind": 9, # RoundedBox
            "materialId": "material.mtg.card.bolt",
            "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.01],
            "transform": make_transform(0.7, 0.02, 1.0, 2.0, 0.53, 2.4),
            "center": [2.0, 0.53, 2.4],
            "authoredProperties": {
                "cardName": {"t": "string", "v": "Lightning Bolt"},
                "cardType": {"t": "string", "v": "instant"},
                "manaCost": {"t": "int", "v": 1},
                "zone": {"t": "string", "v": "hand"},
                "stackOrder": {"t": "int", "v": 0}
            }
        },
        # Author First Mover Object
        {
            "objectID": "author.antigravity",
            "shapeKind": 12, # Shape2D
            "materialId": "",
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": make_transform(1.0, 1.0, 1.0, 80.0, 80.0, 80.0),
            "center": [80.0, 80.0, 80.0]
        },
        # MTG Game State Object
        {
            "objectID": "state.mtg",
            "shapeKind": 12, # Shape2D
            "materialId": "",
            "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "transform": make_transform(1.0, 1.0, 1.0, 80.0, 80.0, 80.0),
            "center": [80.0, 80.0, 80.0],
            "authoredProperties": {
                "phase": {"t": "int", "v": 0},
                "turnNumber": {"t": "int", "v": 1},
                "activePlayer": {"t": "int", "v": 1},
                "landPlayedThisTurn": {"t": "int", "v": 0},
                "manaGreen": {"t": "int", "v": 0},
                "manaRed": {"t": "int", "v": 0},
                "player1Life": {"t": "int", "v": 20},
                "player2Life": {"t": "int", "v": 20},
                "stackCount": {"t": "int", "v": 0},
                "gameOver": {"t": "bool", "v": False}
            }
        }
    ]

    # Assembly of MTG Zone JSON
    zone_json = {
        "identifier": "MTG",
        "name": "Magic: The Gathering",
        "scope": "Local",
        "lawRefs": list(laws.keys()),
        "materials": materials,
        "world": {
            "objects": objects
        }
    }

    zone_path = os.path.join("saves", "zones", "MTG", "zone.json")
    os.makedirs(os.path.dirname(zone_path), exist_ok=True)
    with open(zone_path, "w", encoding="utf-8") as f:
        json.dump(zone_json, f, indent=2)
    print(f"Wrote {zone_path}")

if __name__ == "__main__":
    main()
