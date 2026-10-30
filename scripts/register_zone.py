import json
import os
import msgpack

def register_zone():
    repo_root = "/Users/zacharyzhang/Documents/GitHub/Earthcall"
    world_file = os.path.join(repo_root, "saves", "worlds", "chess_app.ecform")
    zone_file = os.path.join(repo_root, "saves", "zones", "Neural Network", "zone.json")
    
    with open(world_file, "rb") as f:
        packed_data = msgpack.unpackb(f.read(), raw=False)
        
    world_json_str = packed_data["MigrationRoot"]
    world_data = json.loads(world_json_str)
        
    with open(zone_file, "r") as f:
        zone_data = json.load(f)
        
    if "zones" not in world_data:
        world_data["zones"] = []
        
    exists = any(z.get("identifier") == "Neural Network" for z in world_data["zones"])
    if not exists:
        world_data["zones"].append(zone_data)
        
        if "zoneRefs" in world_data:
            world_data["zoneRefs"].append("Neural Network")
            
        new_world_json_str = json.dumps(world_data)
        packed_data["MigrationRoot"] = new_world_json_str
        
        with open(world_file, "wb") as f:
            f.write(msgpack.packb(packed_data, use_bin_type=True))
            
        print("Registered 'Neural Network' zone in chess_app.ecform")
    else:
        print("Zone already registered.")

if __name__ == "__main__":
    register_zone()
