import json
with open("saves/worlds/synthesis_studio_living.json", "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {}).get("laws", [])
while_true = [L for L in laws if L.get("activation") == 1]

def collect_paths(node):
    if not node: return []
    out = []
    if "path" in node:
        p = node["path"]
        if isinstance(p, str): out.append(p)
        elif isinstance(p, dict) and "segments" in p: out.append(".".join(p["segments"]))
    if "operandPath" in node:
        p = node["operandPath"]
        if isinstance(p, str): out.append(p)
        elif isinstance(p, dict) and "segments" in p: out.append(".".join(p["segments"]))
    for child in node.get("children", []):
        out.extend(collect_paths(child))
    return out

for i, L in enumerate(while_true):
    c_paths = collect_paths(L.get("conditionModel", {}))
    a_paths = collect_paths(L.get("actionModel", {}))
    print(f"Law {i}: cond_paths={c_paths}, action_paths={a_paths}")

