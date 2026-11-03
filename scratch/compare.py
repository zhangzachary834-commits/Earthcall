import json

with open("saves/zones/Chess/zone.json", "r") as f:
    j = json.load(f)

for m in j["materials"]:
    if m["name"] == "chess.board":
        print(f"zone.json Face 2 size: {m['faceTextures'][2].get('size')}")
        print(f"zone.json Face 2 length: {len(m['faceTextures'][2]['pixelsB64'])}")
        print(f"zone.json Face 2 hash: {hash(m['faceTextures'][2]['pixelsB64'])}")
