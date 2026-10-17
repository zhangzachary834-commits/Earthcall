import json

path = "saves/zones/MTG/zone.json"
with open(path, "r") as f:
    data = json.load(f)

new_objects = []
lexemes = []

formations = ["mtg.formation.battlefield", "mtg.formation.stack", 
              "mtg.formation.graveyard.1", "mtg.formation.graveyard.2",
              "mtg.formation.hand.1", "mtg.formation.hand.2",
              "mtg.formation.library.1", "mtg.formation.library.2"]
abstract_ids = formations + ["category.mtg.card", "state.mtg"]

# Add Lexemes
for aid in abstract_ids:
    lexemes.append({
        "id": aid,
        "symbol": aid
    })

# Filter objects
for obj in data.get("objects", []):
    if obj.get("objectID") not in abstract_ids:
        new_objects.append(obj)

data["objects"] = new_objects
data["lexemes"] = lexemes

# Add tracks-state relation
if "formationRelations" not in data:
    data["formationRelations"] = []

data["formationRelations"].append({
    "type": "tracks-state",
    "entityA": "MTG",
    "entityB": "state.mtg",
    "directed": True,
    "authors": ["Gemini"],
    "authoredProperties": {
        "turnPhase": {"t": "string", "v": "main1"},
        "priorityPlayer": {"t": "int", "v": 1},
        "stackCounter": {"t": "int", "v": 0}
    }
})

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Migrated abstract objects to Lexemes and Relations!")
