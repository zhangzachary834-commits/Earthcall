import json

path = "saves/zones/MTG/zone.json"
with open(path, "r") as f:
    data = json.load(f)

# Find the tracks-state relation and remove its authoredProperties
props = {}
for rel in data.get("formationRelations", []):
    if rel.get("type") == "tracks-state":
        props = rel.get("authoredProperties", {})
        if "authoredProperties" in rel:
            del rel["authoredProperties"]

# Add authoredProperties to the state.mtg lexeme
for lex in data.get("lexemes", []):
    if lex.get("id") == "state.mtg":
        lex["authoredProperties"] = props

with open(path, "w") as f:
    json.dump(data, f, indent=2)

print("Moved properties to state.mtg lexeme!")
