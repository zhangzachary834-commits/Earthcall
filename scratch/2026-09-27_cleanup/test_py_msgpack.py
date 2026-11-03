import json
import msgpack

with open("saves/zones/Chess/zone.json", "r") as f:
    d = json.load(f)

wrapper = {"MigrationRoot": json.dumps(d, separators=(',', ':'))}
with open("test_chess_py.ecform", "wb") as f:
    msgpack.pack(wrapper, f, use_bin_type=True)
