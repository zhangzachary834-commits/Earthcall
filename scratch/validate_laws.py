import json
with open("saves/worlds/chess.json", "r") as f:
    world = json.load(f)

def check_obj(name, obj):
    if not isinstance(obj, dict):
        print(f"ERROR: Expected dict at {name}, found {type(obj)}: {obj}")
        return False
    return True

for i, law in enumerate(world["authoredLaws"]["laws"]):
    if not isinstance(law, dict):
        print(f"Law {i} is not a dict!")
        continue
    
    def walk_cond(c, path):
        if not check_obj(path, c): return
        if "children" in c:
            for idx, child in enumerate(c["children"]):
                walk_cond(child, f"{path}.children[{idx}]")
                
    def walk_act(a, path):
        if not check_obj(path, a): return
        if "children" in a:
            for idx, child in enumerate(a["children"]):
                walk_act(child, f"{path}.children[{idx}]")

    walk_cond(law.get("conditionModel", {}), f"law[{law.get('id')}].conditionModel")
    walk_act(law.get("actionModel", {}), f"law[{law.get('id')}].actionModel")

print("Validation done.")
