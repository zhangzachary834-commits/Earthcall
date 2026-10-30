import json
import msgpack
import sys
import os

input_file = "saves/zones/Sanctum_of_Beginnings/zone.ecform"
output_file = "saves/zones/Sanctum_of_Beginnings/zone.json"

with open(input_file, "rb") as f:
    data = msgpack.unpack(f, raw=False)

parsed = json.loads(data["MigrationRoot"])

# Find the Zone object and inject @codec.codec.jsonArray
zone = None
for being in parsed:
    if being.get("id") == "Sanctum of Beginnings":
        zone = being
        break

if zone:
    if "properties" not in zone:
        zone["properties"] = {}
    
    # We inject the JSON directly into @codec.codec.jsonArray
    seed_json = [
        {"type": "Singular", "id": "player-1", "name": "First Mover", "telos": "lexeme.human", "position": [0, 1, 0]},
        {"type": "Relation", "source": "world-genesis", "target": "player-1", "name": "owns"},
        {"type": "Singular", "id": "shape-gen-tool", "name": "Tool", "concept": "concept.tool.shape_generator"},
        {"type": "Relation", "source": "player-1", "target": "shape-gen-tool", "name": "equipped"}
    ]
    
    zone["properties"]["codec.jsonArray"] = json.dumps(seed_json)
    zone["properties"]["state.genesis.phase"] = 0.0

with open(output_file, "w") as f:
    json.dump(parsed, f, indent=2)

os.remove(input_file)
print("Injected state!")
