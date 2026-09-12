import json

with open("saves/worlds/synthesis_studio_living.json", "r") as f:
    data = json.load(f)

laws = data.get("authoredLaws", {}).get("laws", [])
while_true = [L for L in laws if L.get("activation") == 1]

def reads_qualified_root(node):
    if not node: return False
    p = node.get("path")
    if isinstance(p, str) and p.startswith("@"): return True
    if isinstance(p, dict) and p.get("segments") and p["segments"][0].startswith("@"): return True
    p = node.get("operandPath")
    if isinstance(p, str) and p.startswith("@"): return True
    if isinstance(p, dict) and p.get("segments") and p["segments"][0].startswith("@"): return True
    for child in node.get("children", []):
        if reads_qualified_root(child): return True
    return False

def is_quantifier(node):
    k = node.get("kind")
    return k in [6, 7] # ForAll, ForAny

alpha_counter = 1

def compile_to_rete(node, left_id=0):
    global alpha_counter
    if not node: return []
    kind = node.get("kind")
    if kind == 3: # All
        children = node.get("children", [])
        if not children: return [] if left_id == 0 else [left_id]
        currents = [left_id]
        for child in children:
            if reads_qualified_root(child) or is_quantifier(child):
                continue
            next_ids = []
            for left in currents:
                t = compile_to_rete(child, left)
                if not t: return []
                next_ids.extend(t)
            currents = next_ids
        if len(currents) == 1 and currents[0] == 0:
            return []
        return currents
    elif kind == 4: # Any
        children = node.get("children", [])
        for child in children:
            if reads_qualified_root(child): return []
        terminals = []
        for child in children:
            t = compile_to_rete(child, left_id)
            terminals.extend(t)
        return terminals
    else: # Compare, etc
        if reads_qualified_root(node): return []
        a_id = alpha_counter
        alpha_counter += 1
        if left_id == 0:
            return [a_id]
        b_id = alpha_counter
        alpha_counter += 1
        return [b_id]

for i, L in enumerate(while_true):
    c = L.get("conditionModel")
    terms = compile_to_rete(c)
    print(f"Law {i} terminals: {terms}")

