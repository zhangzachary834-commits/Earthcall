import json
import msgpack
import sys

if len(sys.argv) < 3:
    print("Usage: python3 scripts/edit_save.py <input.ecform> <output.json>")
    print("Example: python3 scripts/edit_save.py saves/zones/Chess/zone.ecform saves/zones/Chess/zone.json")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

try:
    with open(input_file, "rb") as f:
        data = msgpack.unpack(f, raw=False)
except Exception as e:
    print(f"Failed to read msgpack file: {e}")
    sys.exit(1)

if "MigrationRoot" in data:
    try:
        parsed = json.loads(data["MigrationRoot"])
        with open(output_file, "w") as f:
            json.dump(parsed, f, indent=2)
        print(f"✅ Successfully exported to {output_file}.")
        print(f"You can now edit this JSON file. If you delete {input_file}, Earthcall will automatically load your JSON and compile it back into a new .ecform file on the next save!")
    except Exception as e:
        print(f"Failed to parse JSON: {e}")
        sys.exit(1)
else:
    print(f"❌ Error: {input_file} does not contain a MigrationRoot. It might not be a valid Earthcall zone save.")
