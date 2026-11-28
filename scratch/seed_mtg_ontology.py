import json
import os

zone_json = {
    "objects": [],
    "formationRelations": [],
    "laws": []
}

def add_object(obj):
    zone_json["objects"].append(obj)

def add_relation(rel):
    zone_json["formationRelations"].append(rel)

# 1. State Object
add_object({
    "objectID": "state.mtg",
    "shapeKind": 0,
    "authors": ["Gemini"],
    "authoredProperties": {
        "turnPhase": {"t": "string", "v": "main1"},
        "priorityPlayer": {"t": "int", "v": 1},
        "stackCounter": {"t": "int", "v": 0}
    }
})

# 2. Categories
add_object({
    "objectID": "category.mtg.card",
    "shapeKind": 0,
    "authors": ["Gemini"],
    "authoredProperties": {
        "isCard": {"t": "bool", "v": True}
    }
})

# 3. Formations (Zones)
formations = ["battlefield", "stack", "graveyard.1", "graveyard.2", "hand.1", "hand.2", "library.1", "library.2"]
for f in formations:
    add_object({
        "objectID": f"mtg.formation.{f}",
        "shapeKind": 0,
        "authors": ["Gemini"],
        "authoredProperties": {
            "isFormation": {"t": "bool", "v": True}
        }
    })

# 4. Test Cards
add_object({
    "objectID": "card.grizzly_bears_1",
    "shapeKind": 3,
    "geometryType": 0,
    "shapeParams": [0.3, 0.01, 0.4, 0,0,0, 0,0,0],
    "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, -2,0,0, 1],
    "authors": ["Gemini"],
    "authoredProperties": {
        "mtg.name": {"t": "string", "v": "Grizzly Bears"},
        "mtg.power": {"t": "int", "v": 2},
        "mtg.toughness": {"t": "int", "v": 2},
        "mtg.damage": {"t": "int", "v": 0},
        "mtg.type": {"t": "string", "v": "Creature"}
    }
})
add_relation({
    "type": "instance-of",
    "entityA": "card.grizzly_bears_1",
    "entityB": "category.mtg.card",
    "directed": True,
    "authors": ["Gemini"]
})
add_relation({
    "type": "member-of",
    "entityA": "card.grizzly_bears_1",
    "entityB": "mtg.formation.battlefield",
    "directed": True,
    "authors": ["Gemini"],
    "authoredProperties": {
        "mtg.layoutIndex": {"t": "int", "v": 0}
    }
})

add_object({
    "objectID": "card.lightning_bolt_1",
    "shapeKind": 3,
    "geometryType": 0,
    "shapeParams": [0.3, 0.01, 0.4, 0,0,0, 0,0,0],
    "transform": [1,0,0,0, 0,1,0,0, 0,0,1,0, 2,0,0, 1],
    "authors": ["Gemini"],
    "authoredProperties": {
        "mtg.name": {"t": "string", "v": "Lightning Bolt"},
        "mtg.type": {"t": "string", "v": "Instant"}
    }
})
add_relation({
    "type": "instance-of",
    "entityA": "card.lightning_bolt_1",
    "entityB": "category.mtg.card",
    "directed": True,
    "authors": ["Gemini"]
})
add_relation({
    "type": "member-of",
    "entityA": "card.lightning_bolt_1",
    "entityB": "mtg.formation.hand.1",
    "directed": True,
    "authors": ["Gemini"],
    "authoredProperties": {
        "mtg.layoutIndex": {"t": "int", "v": 0}
    }
})

os.makedirs("saves/zones/MTG", exist_ok=True)
with open("saves/zones/MTG/zone.json", "w") as f:
    json.dump(zone_json, f, indent=2)

print("Created saves/zones/MTG/zone.json")
