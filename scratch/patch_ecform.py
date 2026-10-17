import json
import struct
import os

ECFORM_HEAD = b'\x81\xb3MigrationRoot\xdb'
path = "saves/zones/Ambient Zone/zone.ecform"

with open(path, "rb") as f:
    raw = f.read()

if raw[:len(ECFORM_HEAD)] == ECFORM_HEAD:
    n = struct.unpack(">I", raw[len(ECFORM_HEAD):len(ECFORM_HEAD) + 4])[0]
    text = raw[len(ECFORM_HEAD) + 4:].decode('utf-8')
    data = json.loads(text)
    
    if "lawRefs" not in data:
        data["lawRefs"] = []
    
    if "law-ambient-pan" not in data["lawRefs"]:
        data["lawRefs"].append("law-ambient-pan")
    
    new_text = json.dumps(data, ensure_ascii=False).encode('utf-8')
    new_raw = ECFORM_HEAD + struct.pack(">I", len(new_text)) + new_text
    
    with open(path, "wb") as f:
        f.write(new_raw)
    print("Patched zone.ecform!")
else:
    print("Not a MigrationRoot ecform!")
