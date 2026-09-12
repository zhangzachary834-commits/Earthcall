import json

with open("saves/worlds/synthesis_studio_living.json", "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {}).get("laws", [])
print(f"Total laws: {len(laws)}")

while_true = [L for L in laws if L.get("activation") == 1]
print(f"WhileTrue laws: {len(while_true)}")

def has_bare_path(node):
    if not node: return False
    if "path" in node:
        p = node["path"]
        if isinstance(p, str) and not p.startswith("@"):
            return True
    if "operandPath" in node:
        p = node["operandPath"]
        if isinstance(p, str) and not p.startswith("@"):
            return True
    for child in node.get("children", []):
        if has_bare_path(child): return True
    return False

bare_cond = 0
bare_action = 0
for i, L in enumerate(while_true):
    c_bare = has_bare_path(L.get("conditionModel", {}))
    a_bare = has_bare_path(L.get("actionModel", {}))
    if c_bare: bare_cond += 1
    if a_bare: bare_action += 1
    if not c_bare and not a_bare:
        print(f"Law {i} is purely subject independent!")

print(f"Condition bare: {bare_cond}")
print(f"Action bare: {bare_action}")
