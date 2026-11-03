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

def p_node():
    return {"op": 1, "var": "p"}

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

def noise_node(vec_expr):
    return {"op": 29, "children": [vec_expr]}

# Room 1: Baseline Uniform
# rho_1 = 1.05 + 0.12*cos(0.3*z) + 0.08*sin(0.3*x + 12.0)
def make_room1_ast():
    terms = [
        {"c": 1.05, "factors": {}},
        {"c": 0.12, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.3, "shift": 0.0}]},
        {"c": 0.08, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.3, "shift": 12.0}]}
    ]
    return scalar_poly_node(terms)

# Room 2: Strong Falloff
# rho_2 = 4.0 / (1.0 + 0.45 * [(p.x + 20)^2 + (p.y + 9.5)^2 + p.z^2])
# denom = 221.6125 + 18.0*x + 8.55*y + 0.45*x^2 + 0.45*y^2 + 0.45*z^2
def make_room2_ast():
    terms = [
        {"c": 221.6125, "factors": {}},
        {"c": 18.0, "factors": {"x": 1.0}},
        {"c": 8.55, "factors": {"y": 1.0}},
        {"c": 0.45, "factors": {"x": 2.0}},
        {"c": 0.45, "factors": {"y": 2.0}},
        {"c": 0.45, "factors": {"z": 2.0}}
    ]
    denom = scalar_poly_node(terms)
    num = scalar_node(4.0)
    return div_node(num, denom)

# Room 3: Halo / Nested Shell
# shell1: 2.2 / (1.0 + 3.0 * (dist(p, c) - 2.5)^2)
# shell2: 1.8 / (1.0 + 2.0 * (dist(p, c) - 5.5)^2)
# total = shell1 + shell2 + 0.15
def make_room3_ast():
    c = vec3_node(0.0, -9.5, 0.0)
    d = dist_node(p_node(), c)
    
    # shell 1
    delta1 = sub_node(d, scalar_node(2.5))
    delta1_sq = pow_node(delta1, scalar_node(2.0))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(3.0), delta1_sq))
    shell1 = div_node(scalar_node(2.2), denom1)
    
    # shell 2
    delta2 = sub_node(d, scalar_node(5.5))
    delta2_sq = pow_node(delta2, scalar_node(2.0))
    denom2 = add_node(scalar_node(1.0), scale_node(scalar_node(2.0), delta2_sq))
    shell2 = div_node(scalar_node(1.8), denom2)
    
    shells = add_node(shell1, shell2)
    return add_node(shells, scalar_node(0.15))

# Room 4: Multi-Lobe Chamber
# 3 focal lobes:
# L1 at (20.0, -9.5, 4.0), peak 2.6, k 0.75
# L2 at (16.5, -9.5, -3.0), peak 2.4, k 0.75
# L3 at (23.5, -9.5, -3.0), peak 2.4, k 0.75
def make_room4_ast():
    c1 = vec3_node(20.0, -9.5, 4.0)
    d1_sq = pow_node(dist_node(p_node(), c1), scalar_node(2.0))
    denom1 = add_node(scalar_node(1.0), scale_node(scalar_node(0.75), d1_sq))
    lobe1 = div_node(scalar_node(2.6), denom1)
    
    c2 = vec3_node(16.5, -9.5, -3.0)
    d2_sq = pow_node(dist_node(p_node(), c2), scalar_node(2.0))
    denom2 = add_node(scalar_node(1.0), scale_node(scalar_node(0.75), d2_sq))
    lobe2 = div_node(scalar_node(2.4), denom2)
    
    c3 = vec3_node(23.5, -9.5, -3.0)
    d3_sq = pow_node(dist_node(p_node(), c3), scalar_node(2.0))
    denom3 = add_node(scalar_node(1.0), scale_node(scalar_node(0.75), d3_sq))
    lobe3 = div_node(scalar_node(2.4), denom3)
    
    lobes12 = add_node(lobe1, lobe2)
    lobes123 = add_node(lobes12, lobe3)
    return add_node(lobes123, scalar_node(0.12))

# Room 5: Procedural / Cathedral Chamber
# baseDome = 2.5 / (1.0 + 0.03 * dist(p, (40, -9.5, 0))^2)
# lattice = 1.0 + 0.45*cos(0.8*(x-40))*cos(0.8*z) + 0.25*cos(1.6*(x-40)) + 0.25*cos(1.6*z)
# noise = 0.35 * cnoise3(0.18 * p)
def make_room5_ast():
    c5 = vec3_node(40.0, -9.5, 0.0)
    d5_sq = pow_node(dist_node(p_node(), c5), scalar_node(2.0))
    denom5 = add_node(scalar_node(1.0), scale_node(scalar_node(0.03), d5_sq))
    base_dome = div_node(scalar_node(2.5), denom5)
    
    # 0.8*(x-40) = 0.8*x - 32.0; 1.6*(x-40) = 1.6*x - 64.0
    lattice_terms = [
        {"c": 0.60, "factors": {}},
        {"c": 0.25, "factors": {}, "trans": [
            {"kind": 1, "var": "x", "scale": 0.8, "shift": -32.0},
            {"kind": 1, "var": "z", "scale": 0.8, "shift": 0.0}
        ]},
        {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.6, "shift": -64.0}]},
        {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 1.6, "shift": 0.0}]}
    ]
    lattice = scalar_poly_node(lattice_terms)
    
    scaled_p = scale_node(scalar_node(0.18), p_node())
    noise = noise_node(scaled_p)
    scaled_noise = scale_node(scalar_node(0.25), noise)
    
    modulation = add_node(lattice, scaled_noise)
    return scale_node(base_dome, modulation)

def make_gallery_piecewise():
    return {
        "input": "x",
        "pieces": [
            {
                "hasLo": False,
                "hasHi": True,
                "hi": -30.0,
                "includeHi": True,
                "mathNode": make_room1_ast()
            },
            {
                "hasLo": True,
                "lo": -30.0,
                "includeLo": False,
                "hasHi": True,
                "hi": -10.0,
                "includeHi": True,
                "mathNode": make_room2_ast()
            },
            {
                "hasLo": True,
                "lo": -10.0,
                "includeLo": False,
                "hasHi": True,
                "hi": 10.0,
                "includeHi": True,
                "mathNode": make_room3_ast()
            },
            {
                "hasLo": True,
                "lo": 10.0,
                "includeLo": False,
                "hasHi": True,
                "hi": 30.0,
                "includeHi": True,
                "mathNode": make_room4_ast()
            },
            {
                "hasLo": True,
                "lo": 30.0,
                "includeLo": False,
                "hasHi": False,
                "mathNode": make_room5_ast()
            }
        ]
    }

pw = make_gallery_piecewise()
with open('/Users/zacharyzhang/Documents/GitHub/Earthcall/scratch/gallery_piecewise.json', 'w') as f:
    json.dump(pw, f, indent=2)
print("Wrote /Users/zacharyzhang/Documents/GitHub/Earthcall/scratch/gallery_piecewise.json")
