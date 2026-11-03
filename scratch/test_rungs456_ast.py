import json

def scalar_node(val):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(val), "factors": {}}]}}

def scalar_poly_node(terms):
    return {"op": 0, "scalarForm": {"terms": terms}}

def vec3_node(x, y, z):
    return {
        "op": 2,
        "children": [scalar_node(x), scalar_node(y), scalar_node(z)]
    }

def vec3_expr_node(rx, gy, bz):
    return {
        "op": 2,
        "children": [rx, gy, bz]
    }

def var_node(name):
    return {"op": 1, "var": name}

def dist_node(pt_node, center_vec3):
    return {"op": 15, "children": [pt_node, center_vec3]}

def add_node(a, b):
    return {"op": 4, "children": [a, b]}

def sub_node(a, b):
    return {"op": 5, "children": [a, b]}

def scale_node(scalar, expr):
    return {"op": 6, "children": [scalar, expr]}

def div_node(num, denom):
    return {"op": 23, "children": [num, denom]}

def pow_node(base, exp):
    return {"op": 24, "children": [base, exp]}

def clamp_node(val, lo, hi):
    return {"op": 26, "children": [val, lo, hi]}

def noise_node(vec_expr):
    return {"op": 29, "children": [vec_expr]}

print("AST helpers defined.")
