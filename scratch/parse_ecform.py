import msgpack
import sys
import json

with open(sys.argv[1], 'rb') as f:
    data = f.read()

try:
    unpacked = msgpack.unpackb(data, raw=False)
    if "MigrationRoot" in unpacked:
        parsed = json.loads(unpacked["MigrationRoot"])
        print(json.dumps(parsed, indent=2))
    else:
        print(json.dumps(unpacked, indent=2))
except Exception as e:
    print("Error:", e)
