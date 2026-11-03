import json
with open("saves/zones/Chess/zone.json", "r") as f:
    j = json.load(f)

for m in j["materials"]:
    if m["name"] == "chess.board":
        json_b64 = m['faceTextures'][2]['pixelsB64']
        
with open("ecform_b64.txt", "r") as f:
    ecform_b64 = f.read()

print(f"Are they identical? {json_b64 == ecform_b64}")
