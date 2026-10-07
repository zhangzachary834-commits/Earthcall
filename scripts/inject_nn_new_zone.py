import json
import os

def inject_nn_new_zone():
    repo_root = "/Users/zacharyzhang/Documents/GitHub/Earthcall"
    zone_dir = os.path.join(repo_root, "saves", "zones", "Neural Network")
    zone_file = os.path.join(zone_dir, "zone.json")

    with open(zone_file, "r") as f:
        zone_data = json.load(f)

    zone_data["identifier"] = "Neural Network"
    zone_data["name"] = "Neural Network"

    # Use one of the existing objects as a template
    base_object = zone_data["world"]["objects"][0].copy()

    def make_node(obj_id, tx, ty, tz, color):
        obj = json.loads(json.dumps(base_object)) # deep copy
        obj["objectID"] = obj_id
        obj["transform"][12] = tx
        obj["transform"][13] = ty
        obj["transform"][14] = tz
        obj["faceColors"] = [color for _ in range(6)]
        obj["shapeKind"] = 2 # Sphere
        if "shape" in obj:
            obj["shape"]["kind"] = 2
        return obj

    node_in_1 = make_node("node-in-1", -2.0, 1.0, -2.0, [1.0, 0.0, 0.0])
    node_in_2 = make_node("node-in-2",  2.0, 0.5, -2.0, [0.0, 1.0, 0.0])
    node_out  = make_node("node-out-1", 0.0, 0.0, -4.0, [0.0, 0.0, 1.0])

    zone_data["world"]["objects"].extend([node_in_1, node_in_2, node_out])

    with open(zone_file, "w") as f:
        json.dump(zone_data, f)

    print("Patched Neural Network/zone.json")

if __name__ == "__main__":
    inject_nn_new_zone()
