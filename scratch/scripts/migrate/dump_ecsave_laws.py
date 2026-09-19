import zlib
import msgpack
import sys
import json

def read_ecsave(path):
    with open(path, 'rb') as f:
        data = f.read()
    try:
        decompressed = zlib.decompress(data)
        obj = msgpack.unpackb(decompressed, raw=False)
        print(json.dumps(obj.get('authoredLaws', {}).get('laws', []), indent=2))
    except Exception as e:
        print(f"Error: {e}")

read_ecsave(sys.argv[1])
