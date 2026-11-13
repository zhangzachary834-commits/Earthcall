import json

path = "saves/zones/Ambient Zone/zone.ecform"
with open(path, "rb") as f:
    raw = f.read()

try:
    data = json.loads(raw.decode('utf-8'))
except:
    import msgpack
    data = msgpack.unpackb(raw)

print(type(data))
