#!/usr/bin/env python3
"""
author_sky_celestial_singularity.py

Authors the Masterwork Celestial Radiance Engine & Astral Orrery in the sky
above the 28-Room Radiance Gallery using advanced SDF geometry:
- Regular & Semi-Regular Convex Polyhedra (Octahedra, Dodecahedra, Icosahedra, Rhombic Dodecahedra)
- CSG Subtraction & Carving (Hollow Coronal Spheres, Slotted Astrolabe Retes, Cassini-Division Rings)
- Smooth-Union Organic Star Blends (sminK)
- Morphing Celestial Hybrids
"""

import json, math, os

# Golden ratio
PHI = (1.0 + math.sqrt(5.0)) * 0.5

def mat4_identity():
    return [1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0]

def mat4_translate(tx, ty, tz):
    return [1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            tx,  ty,  tz,  1.0]

def mat4_mul(A, B):
    C = [0.0] * 16
    for col in range(4):
        for row in range(4):
            val = 0.0
            for k in range(4):
                val += A[k * 4 + row] * B[col * 4 + k]
            C[col * 4 + row] = val
    return C

def mat4_rotate_x(rad):
    c = math.cos(rad)
    s = math.sin(rad)
    return [1.0, 0.0, 0.0, 0.0,
            0.0,   c,   s, 0.0,
            0.0,  -s,   c, 0.0,
            0.0, 0.0, 0.0, 1.0]

def mat4_rotate_y(rad):
    c = math.cos(rad)
    s = math.sin(rad)
    return [  c, 0.0,  -s, 0.0,
            0.0, 1.0, 0.0, 0.0,
              s, 0.0,   c, 0.0,
            0.0, 0.0, 0.0, 1.0]

def mat4_rotate_z(rad):
    c = math.cos(rad)
    s = math.sin(rad)
    return [  c,   s, 0.0, 0.0,
             -s,   c, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0]

# =============================================================================
# ADVANCED SDF GEOMETRY BUILDERS
# =============================================================================

def sdf_leaf(prim, dims, offset=(0, 0, 0), p0=0.0, p1=0.0):
    return {
        "op": 0, # Leaf
        "prim": prim,
        "dims": [dims[0], dims[1], dims[2]],
        "offset": [offset[0], offset[1], offset[2]],
        "p0": p0,
        "p1": p1,
        "t": 0.5
    }

def sdf_binary(op, child_a, child_b, t=0.5):
    return {
        "op": op, # 1: Morph, 2: Union, 3: Intersect, 4: Subtract, 5: SmoothUnion
        "prim": 0,
        "dims": [0.0, 0.0, 0.0],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": t,
        "children": [child_a, child_b]
    }

def sdf_convex(planes):
    # SdfPrim::Convex = 8
    return {
        "op": 0, # Leaf
        "prim": 8, # Convex
        "dims": [0.0, 0.0, 0.0],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": 0.5,
        "planes": planes
    }

def make_polyhedron_planes(kind, radius):
    planes = []
    if kind == "octahedron":
        inv = 1.0 / math.sqrt(3.0)
        for sx in [-1.0, 1.0]:
            for sy in [-1.0, 1.0]:
                for sz in [-1.0, 1.0]:
                    planes.append([sx * inv, sy * inv, sz * inv, radius])
    elif kind == "dodecahedron":
        normals = []
        for s1 in [-1.0, 1.0]:
            for s2 in [-1.0, 1.0]:
                normals.append([0.0, s1, s2 * PHI])
                normals.append([s1, s2 * PHI, 0.0])
                normals.append([s2 * PHI, 0.0, s1])
        for nx, ny, nz in normals:
            inv_len = 1.0 / math.sqrt(nx*nx + ny*ny + nz*nz)
            planes.append([nx * inv_len, ny * inv_len, nz * inv_len, radius])
    elif kind == "icosahedron":
        normals = []
        for sx in [-1.0, 1.0]:
            for sy in [-1.0, 1.0]:
                for sz in [-1.0, 1.0]:
                    normals.append([sx, sy, sz])
        inv_phi = 1.0 / PHI
        for s1 in [-1.0, 1.0]:
            for s2 in [-1.0, 1.0]:
                normals.append([0.0, s1 * PHI, s2 * inv_phi])
                normals.append([s1 * PHI, s2 * inv_phi, 0.0])
                normals.append([s2 * inv_phi, 0.0, s1 * PHI])
        for nx, ny, nz in normals:
            inv_len = 1.0 / math.sqrt(nx*nx + ny*ny + nz*nz)
            planes.append([nx * inv_len, ny * inv_len, nz * inv_len, radius])
    elif kind == "rhombic":
        inv_sqrt2 = 1.0 / math.sqrt(2.0)
        for s1 in [-1.0, 1.0]:
            for s2 in [-1.0, 1.0]:
                planes.append([s1 * inv_sqrt2, s2 * inv_sqrt2, 0.0, radius])
                planes.append([s1 * inv_sqrt2, 0.0, s2 * inv_sqrt2, radius])
                planes.append([0.0, s1 * inv_sqrt2, s2 * inv_sqrt2, radius])
    return planes

def make_sdf_object(obj_id, material_id, center, transform, field_node, extent, display_name=None, role=None):
    obj = {
        "authoritativeAxis": [0, 1, 0],
        "center": center,
        "faceColors": [[1, 1, 1]] * 6,
        "geometryType": 10,
        "materialId": material_id,
        "objectID": obj_id,
        "renderMode": 0,
        "rotationResponsiveness": 10,
        "shapeKind": 10,
        "drawFieldModel": True,
        "shapeParams": [1, 1, 1, 0.5, 0.35, 0.15, 2, 0.25, 0.12, 100, 100],
        "targetRotation": [0, 0, 0],
        "transform": transform,
        "x2D": 100,
        "y2D": 100,
        "zOrder2D": 0,
        "field": field_node,
        "fieldExtent": extent
    }
    if display_name or role:
        obj["authoredProperties"] = {}
        if display_name:
            obj["authoredProperties"]["displayName"] = {"t": "string", "v": display_name}
        if role:
            obj["authoredProperties"]["celestial.role"] = {"t": "string", "v": role}
    return obj

def build_sky_celestial_apparatus():
    objects = []
    OX, OY, OZ = 0.0, 52.0, 87.5
    rx90 = mat4_rotate_x(math.pi / 2.0)
    ry90 = mat4_rotate_y(math.pi / 2.0)

    # =========================================================================
    # 1. CORE ASTRAL SINGULARITY & ERGOSPHERE (Advanced Organic & CSG Shapes)
    # =========================================================================
    # Nucleus: Smooth Union (sminK) of an obsidian sphere with 3 orthogonal pulsating lobes!
    core_sph = sdf_leaf(0, [4.2, 4.2, 4.2], p0=4.2)
    lobe_x = sdf_leaf(3, [5.4, 2.8, 2.8]) # Ellipsoid along X
    lobe_y = sdf_leaf(3, [2.8, 5.4, 2.8]) # Ellipsoid along Y
    nucleus_cross = sdf_binary(5, core_sph, lobe_x, t=0.65) # SmoothUnion
    nucleus_shape = sdf_binary(5, nucleus_cross, lobe_y, t=0.65) # SmoothUnion

    T_core = mat4_translate(OX, OY, OZ)
    objects.append(make_sdf_object(
        "celestial-nucleus-core",
        "celestial.obsidian.core",
        [OX, OY, OZ],
        T_core,
        nucleus_shape,
        extent=[6.5, 6.5, 6.5],
        display_name="Astral Singularity Nucleus (Organic Flux Kernel)",
        role="singularity-core"
    ))

    # Inner Ergosurface Torus (Frame-dragging boundary with CSG notches)
    ergo_torus = sdf_leaf(6, [5.8, 0.48, 0.0])
    ergo_slot = sdf_leaf(1, [6.5, 0.35, 0.35])
    ergo_shape = sdf_binary(4, ergo_torus, ergo_slot) # CSG Subtract
    T_ergo = mat4_mul(mat4_translate(OX, OY, OZ), mat4_mul(mat4_rotate_x(0.21), rx90))
    objects.append(make_sdf_object(
        "celestial-nucleus-ergosphere",
        "celestial.amethyst.resonance",
        [OX, OY, OZ],
        T_ergo,
        ergo_shape,
        extent=[7.0, 7.0, 2.0],
        display_name="Ergosurface Frame-Dragging Slotted Torus",
        role="ergosphere"
    ))

    # Coronal Stellar Focus Shell (Hollow Sphere CSG: Outer Sphere - Inner Sphere)
    corona_outer = sdf_leaf(0, [6.4, 6.4, 6.4], p0=6.4)
    corona_inner = sdf_leaf(0, [5.9, 5.9, 5.9], p0=5.9)
    corona_shape = sdf_binary(4, corona_outer, corona_inner) # CSG Subtract hollow shell
    objects.append(make_sdf_object(
        "celestial-nucleus-corona",
        "celestial.gold.electrum",
        [OX, OY, OZ],
        T_core,
        corona_shape,
        extent=[7.5, 7.5, 7.5],
        display_name="Radiant Coronal Focus Hollow Shell",
        role="singularity-corona"
    ))

    # Quadrupole Quantum Flux Nodes: 4 Faceted Rhombic Dodecahedra!
    quad_planes = make_polyhedron_planes("rhombic", 0.75)
    quad_node_shape = sdf_convex(quad_planes)
    quad_nodes = [
        ("alpha", [3.8, 2.5, 3.8]),
        ("beta",  [-3.8, 2.5, -3.8]),
        ("gamma", [-3.8, -2.5, 3.8]),
        ("delta", [3.8, -2.5, -3.8])
    ]
    for qname, qoff in quad_nodes:
        qx, qy, qz = OX + qoff[0], OY + qoff[1], OZ + qoff[2]
        T_q = mat4_translate(qx, qy, qz)
        objects.append(make_sdf_object(
            f"celestial-flux-node-{qname}",
            "celestial.ruby.nodal",
            [qx, qy, qz],
            T_q,
            quad_node_shape,
            extent=[1.2, 1.2, 1.2],
            display_name=f"Quantum Flux Rhombic Dodecahedron {qname.capitalize()}",
            role="quantum-node"
        ))

    # =========================================================================
    # 2. THE GRAND KEPLERIAN ARMILLARY ORRERY (7 Concentric Gyro Rings)
    # =========================================================================
    # Ring 1: Equatorial Accretion Ring (Torus CSG-slotted with radial teeth)
    ring1_torus = sdf_leaf(6, [20.0, 0.88, 0.0])
    ring1_slot_x = sdf_leaf(1, [22.0, 0.30, 0.30])
    ring1_slot_z = sdf_leaf(1, [0.30, 22.0, 0.30])
    ring1_slots = sdf_binary(2, ring1_slot_x, ring1_slot_z) # Union
    ring1_shape = sdf_binary(4, ring1_torus, ring1_slots) # Subtract
    T_ring1 = mat4_mul(mat4_translate(OX, OY, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-ring-01-equatorial",
        "celestial.gold.stellar",
        [OX, OY, OZ],
        T_ring1,
        ring1_shape,
        extent=[22.5, 22.5, 2.5],
        display_name="Armillary Ring I — Celestial Slotted Equator",
        role="armillary-ring"
    ))

    # Ring 2: Ecliptic Zodiac Ring (Torus with Cassini-style perimeter bevel)
    ring2_torus = sdf_leaf(6, [26.0, 0.78, 0.0])
    ring2_bevel = sdf_leaf(6, [26.0, 0.25, 0.0])
    ring2_shape = sdf_binary(4, ring2_torus, ring2_bevel) # Subtract bevel groove
    T_rot_ecliptic = mat4_mul(mat4_rotate_z(math.radians(23.4)), rx90)
    T_ring2 = mat4_mul(mat4_translate(OX, OY, OZ), T_rot_ecliptic)
    objects.append(make_sdf_object(
        "celestial-ring-02-ecliptic",
        "celestial.bronze.armillary",
        [OX, OY, OZ],
        T_ring2,
        ring2_shape,
        extent=[28.5, 28.5, 2.5],
        display_name="Armillary Ring II — Ecliptic Chrono-Path",
        role="armillary-ring"
    ))

    # Ring 3: Prime Celestial Meridian (Vertical Y-Z)
    ring3_torus = sdf_leaf(6, [32.0, 0.72, 0.0])
    T_ring3 = mat4_mul(mat4_translate(OX, OY, OZ), ry90)
    objects.append(make_sdf_object(
        "celestial-ring-03-meridian",
        "celestial.gold.electrum",
        [OX, OY, OZ],
        T_ring3,
        ring3_torus,
        extent=[34.5, 34.5, 2.5],
        display_name="Armillary Ring III — Celestial Prime Meridian",
        role="armillary-ring"
    ))

    # Ring 4: Equinoctial Colure (Vertical X-Y)
    ring4_torus = sdf_leaf(6, [32.0, 0.68, 0.0])
    T_ring4 = mat4_translate(OX, OY, OZ)
    objects.append(make_sdf_object(
        "celestial-ring-04-colure",
        "celestial.silver.quicksilver",
        [OX, OY, OZ],
        T_ring4,
        ring4_torus,
        extent=[34.5, 34.5, 2.5],
        display_name="Armillary Ring IV — Equinoctial Colure",
        role="armillary-ring"
    ))

    # Ring 5 & 6: Tropics of Cancer & Capricorn (Parallel circles at Y = OY +/- 10.5)
    r_tropic = math.sqrt(max(1.0, 32.0**2 - 10.5**2)) # ~30.23
    for t_sign, t_name in [(1.0, "cancer"), (-1.0, "capricorn")]:
        ty = OY + t_sign * 10.5
        t_torus = sdf_leaf(6, [r_tropic, 0.58, 0.0])
        T_tropic = mat4_mul(mat4_translate(OX, ty, OZ), rx90)
        objects.append(make_sdf_object(
            f"celestial-ring-tropic-{t_name}",
            "celestial.bronze.armillary",
            [OX, ty, OZ],
            T_tropic,
            t_torus,
            extent=[r_tropic + 2.0, r_tropic + 2.0, 2.0],
            display_name=f"Armillary Tropic Ring — {t_name.capitalize()}",
            role="armillary-ring"
        ))

    # Ring 7: Precessional Polar Gyro (R = 36.0, 45 degree tilt)
    ring7_torus = sdf_leaf(6, [36.0, 0.72, 0.0])
    T_rot_prec = mat4_mul(mat4_rotate_x(math.radians(45.0)), mat4_rotate_y(math.radians(45.0)))
    T_ring7 = mat4_mul(mat4_translate(OX, OY, OZ), T_rot_prec)
    objects.append(make_sdf_object(
        "celestial-ring-07-precessional",
        "celestial.crystal.sapphire",
        [OX, OY, OZ],
        T_ring7,
        ring7_torus,
        extent=[39.0, 39.0, 2.5],
        display_name="Armillary Ring VII — Precessional Polar Gyro",
        role="armillary-ring"
    ))

    # Ring 8: Great Outer Zodiac Framework (Grand Fluted Double-Ring)
    ring8_main = sdf_leaf(6, [42.0, 1.10, 0.0])
    ring8_inner = sdf_leaf(6, [39.5, 0.45, 0.0])
    ring8_shape = sdf_binary(2, ring8_main, ring8_inner) # Union
    T_ring8 = mat4_mul(mat4_translate(OX, OY, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-ring-08-zodiac",
        "celestial.marble.astral",
        [OX, OY, OZ],
        T_ring8,
        ring8_shape,
        extent=[45.5, 45.5, 3.5],
        display_name="Armillary Ring VIII — Great Fluted Zodiac Framework",
        role="armillary-ring"
    ))

    # =========================================================================
    # 3. 12 ZODIAC HOUSE MARKERS: FACETED REGULAR POLYHEDRA
    # =========================================================================
    # Alternating Octahedra and Dodecahedra!
    octa_planes = make_polyhedron_planes("octahedron", 1.25)
    dodeca_planes = make_polyhedron_planes("dodecahedron", 1.20)
    octa_shape = sdf_convex(octa_planes)
    dodeca_shape = sdf_convex(dodeca_planes)

    for i in range(12):
        angle = i * (2.0 * math.pi / 12.0)
        zx = OX + 42.0 * math.cos(angle)
        zz = OZ + 42.0 * math.sin(angle)
        zy = OY
        T_znode = mat4_translate(zx, zy, zz)
        if i % 2 == 0:
            mat_z = "celestial.ruby.nodal"
            z_shape = octa_shape
            z_title = f"Zodiac House Octahedral Crystal {i+1}"
        else:
            mat_z = "celestial.crystal.sapphire"
            z_shape = dodeca_shape
            z_title = f"Zodiac House Dodecahedral Solenoid {i+1}"

        objects.append(make_sdf_object(
            f"celestial-zodiac-node-{i+1:02d}",
            mat_z,
            [zx, zy, zz],
            T_znode,
            z_shape,
            extent=[1.8, 1.8, 1.8],
            display_name=z_title,
            role="zodiac-node"
        ))

        # Ecliptic Chrono-Gear Teeth: Chamfered RoundBox teeth on the tilted 23.4 ring
        local_v = [26.0 * math.cos(angle), 26.0 * math.sin(angle), 0.0]
        rad23 = math.radians(23.4)
        c23, s23 = math.cos(rad23), math.sin(rad23)
        ex = OX + (local_v[0] * c23 - local_v[1] * s23)
        ey = OY + (local_v[0] * s23 + local_v[1] * c23)
        ez = OZ
        T_eg = mat4_translate(ex, ey, ez)
        tooth_shape = sdf_leaf(2, [0.65, 0.45, 0.45], p0=0.15) # RoundBox
        objects.append(make_sdf_object(
            f"celestial-ecliptic-tooth-{i+1:02d}",
            "celestial.bronze.armillary",
            [ex, ey, ez],
            T_eg,
            tooth_shape,
            extent=[1.2, 1.0, 1.0],
            display_name=f"Ecliptic Chrono-Gear Tooth {i+1}",
            role="ecliptic-tooth"
        ))

    # =========================================================================
    # 4. SACRED GEOMETRIC KEPLERIAN PLANETARY SATELLITES
    # =========================================================================
    # Each planet corresponds to a Keplerian Platonic Polyhedron / Morph!
    # 1. Mercury: Polished Octahedron
    # 2. Venus: Regular Icosahedron (20 equilateral triangle faces)
    # 3. Sol / Sun: Stellated Organic Solar Core (Sphere sminK 6 cones)
    # 4. Mars: Regular Dodecahedron (12 pentagon faces)
    # 5. Jupiter: Rhombic Dodecahedron with 4 satellite moonlets
    # 6. Saturn: Flattened Ellipsoid + Cassini-Division dual-torus ring!
    planets_data = [
        ("mercury", "celestial.silver.quicksilver", 13.5,  30.0,
         sdf_convex(make_polyhedron_planes("octahedron", 0.95)), 1.2),
        ("venus",   "celestial.gold.electrum",     16.5, 110.0,
         sdf_convex(make_polyhedron_planes("icosahedron", 1.15)), 1.4),
        ("sol",     "celestial.gold.stellar",      21.5, 195.0,
         sdf_binary(5, sdf_leaf(0, [1.6, 1.6, 1.6], p0=1.6), sdf_leaf(3, [2.2, 1.2, 1.2]), t=0.5), 2.5),
        ("mars",    "celestial.ruby.nodal",        25.0, 275.0,
         sdf_convex(make_polyhedron_planes("dodecahedron", 1.10)), 1.3),
        ("jupiter", "celestial.crystal.sapphire",  30.5,  75.0,
         sdf_convex(make_polyhedron_planes("rhombic", 1.85)), 2.2),
        ("saturn",  "celestial.bronze.armillary",   35.5, 335.0,
         sdf_leaf(3, [1.8, 1.4, 1.8]), 2.2), # Ellipsoid
    ]

    for pname, pmat, porbit, pdeg, pshape, pext in planets_data:
        prad = math.radians(pdeg)
        px = OX + porbit * math.cos(prad)
        pz = OZ + porbit * math.sin(prad)
        py = OY + 2.5 * math.sin(prad * 2.0)
        T_p = mat4_translate(px, py, pz)
        objects.append(make_sdf_object(
            f"celestial-planet-{pname}",
            pmat,
            [px, py, pz],
            T_p,
            pshape,
            extent=[pext, pext, pext],
            display_name=f"Keplerian Platonic Satellite — {pname.capitalize()}",
            role="celestial-planet"
        ))

    # Saturn's Cassini-Division Ring: Torus minus inner division notch
    saturn_rad = math.radians(335.0)
    sx = OX + 35.5 * math.cos(saturn_rad)
    sz = OZ + 35.5 * math.sin(saturn_rad)
    sy = OY + 2.5 * math.sin(saturn_rad * 2.0)
    sat_ring_main = sdf_leaf(6, [3.8, 0.35, 0.0])
    sat_ring_gap = sdf_leaf(6, [3.8, 0.08, 0.0]) # gap
    sat_ring_csg = sdf_binary(4, sat_ring_main, sat_ring_gap) # Subtract Cassini division
    T_sring = mat4_mul(mat4_translate(sx, sy, sz), mat4_mul(mat4_rotate_x(0.35), rx90))
    objects.append(make_sdf_object(
        "celestial-planet-ring-saturn",
        "celestial.gold.electrum",
        [sx, sy, sz],
        T_sring,
        sat_ring_csg,
        extent=[4.5, 4.5, 1.0],
        display_name="Saturnian Cassini-Division Armillary Halo",
        role="planet-ring"
    ))

    # Sol's Corona Solar Flare Torus
    sol_rad = math.radians(195.0)
    sol_x = OX + 21.5 * math.cos(sol_rad)
    sol_z = OZ + 21.5 * math.sin(sol_rad)
    sol_y = OY + 2.5 * math.sin(sol_rad * 2.0)
    sol_ring = sdf_leaf(6, [2.8, 0.28, 0.0])
    T_sol_ring = mat4_mul(mat4_translate(sol_x, sol_y, sol_z), mat4_mul(mat4_rotate_x(0.4), rx90))
    objects.append(make_sdf_object(
        "celestial-planet-ring-sol",
        "celestial.gold.electrum",
        [sol_x, sol_y, sol_z],
        T_sol_ring,
        sol_ring,
        extent=[3.5, 3.5, 0.8],
        display_name="Solar Coronal Halo Ring",
        role="planet-ring"
    ))

    # Accretion Infall Stream Pearls: Alternating Spheres and Micro-Octahedra
    for s in range(16):
        theta = s * 0.45
        r_spiral = 6.0 + 1.2 * theta
        sx = OX + r_spiral * math.cos(theta)
        sz = OZ + r_spiral * math.sin(theta)
        sy = OY + (s - 8) * 0.25
        T_s = mat4_translate(sx, sy, sz)
        if s % 2 == 0:
            mat_s = "celestial.crystal.cyan"
            shape_s = sdf_leaf(0, [0.48, 0.48, 0.48], p0=0.48)
        else:
            mat_s = "celestial.gold.stellar"
            shape_s = sdf_convex(make_polyhedron_planes("octahedron", 0.50))

        objects.append(make_sdf_object(
            f"celestial-accretion-pearl-{s+1:02d}",
            mat_s,
            [sx, sy, sz],
            T_s,
            shape_s,
            extent=[0.8, 0.8, 0.8],
            display_name=f"Accretion Infall Flux Nodule {s+1}",
            role="accretion-pearl"
        ))

    # =========================================================================
    # 5. TOWERING RELATIVISTIC BIPOLAR JET SPIRE (Zenith & Nadir)
    # =========================================================================
    # Zenith Magnetic Collimation Choke Coils (CSG Tapered Fluted Toruses)
    zenith_chokes = [
        (62.0, 11.5, 0.68),
        (72.0,  9.5, 0.60),
        (82.0,  7.5, 0.54),
        (94.0,  5.8, 0.48),
        (106.0, 4.2, 0.42),
        (118.0, 2.8, 0.36)
    ]
    for cy, cr, cw in zenith_chokes:
        T_zc = mat4_mul(mat4_translate(OX, cy, OZ), rx90)
        choke_torus = sdf_leaf(6, [cr, cw, 0.0])
        objects.append(make_sdf_object(
            f"celestial-jet-choke-zenith-{int(cy)}",
            "celestial.gold.stellar",
            [OX, cy, OZ],
            T_zc,
            choke_torus,
            extent=[cr + 1.5, cr + 1.5, 1.5],
            display_name=f"Zenith Magnetic Collimation Choke Y={int(cy)}",
            role="jet-choke"
        ))

    # Zenith Collimation Pylons (4 vertical faceted obelisks)
    for pi, (pdx, pdz) in enumerate([(6.5, 0.0), (-6.5, 0.0), (0.0, 6.5), (0.0, -6.5)]):
        py_center = 88.0
        T_zpylon = mat4_translate(OX + pdx, py_center, OZ + pdz)
        pylon_col = sdf_leaf(4, [0.45, 14.0, 0.0]) # Cylinder
        objects.append(make_sdf_object(
            f"celestial-zenith-pylon-{pi+1}",
            "celestial.crystal.cyan",
            [OX + pdx, py_center, OZ + pdz],
            T_zpylon,
            pylon_col,
            extent=[1.0, 15.0, 1.0],
            display_name=f"Zenith Jet Guide Needle {pi+1}",
            role="jet-guide"
        ))

    # Zenith Apex Spire: Morphing Cone-to-Octahedron Needle at Y = 126.0m!
    T_apex = mat4_translate(OX, 126.0, OZ)
    apex_cone = sdf_leaf(5, [1.8, 8.0, 0.0])
    apex_octa = sdf_convex(make_polyhedron_planes("octahedron", 4.0))
    apex_shape = sdf_binary(1, apex_cone, apex_octa, t=0.4) # Morph blend
    objects.append(make_sdf_object(
        "celestial-zenith-apex-spire",
        "celestial.crystal.cyan",
        [OX, 126.0, OZ],
        T_apex,
        apex_shape,
        extent=[2.5, 9.0, 2.5],
        display_name="Zenith Astral Spire Apex Faceted Needle",
        role="zenith-apex"
    ))

    # Nadir Collimation Choke Coils (Y = 44 down to 18)
    nadir_chokes = [
        (44.0, 11.5, 0.68),
        (36.0,  9.5, 0.60),
        (28.0,  7.8, 0.54),
        (18.0,  6.2, 0.48)
    ]
    for cy, cr, cw in nadir_chokes:
        T_nc = mat4_mul(mat4_translate(OX, cy, OZ), rx90)
        choke_n = sdf_leaf(6, [cr, cw, 0.0])
        objects.append(make_sdf_object(
            f"celestial-jet-choke-nadir-{int(cy)}",
            "celestial.bronze.armillary",
            [OX, cy, OZ],
            T_nc,
            choke_n,
            extent=[cr + 1.5, cr + 1.5, 1.5],
            display_name=f"Nadir Gravitational Choke Y={int(cy)}",
            role="jet-choke"
        ))

    # Nadir Gravitational Anchor: CSG Hollowed Crucible (Outer Sphere minus Inner Cone)
    T_anchor = mat4_translate(OX, 8.5, OZ)
    anc_sphere = sdf_leaf(0, [3.2, 3.2, 3.2], p0=3.2)
    anc_cone = sdf_leaf(5, [2.2, 3.5, 0.0]) # Cone carving interior
    anc_crucible = sdf_binary(4, anc_sphere, anc_cone) # Subtract
    objects.append(make_sdf_object(
        "celestial-nadir-gravity-anchor",
        "celestial.obsidian.core",
        [OX, 8.5, OZ],
        T_anchor,
        anc_crucible,
        extent=[4.0, 4.0, 4.0],
        display_name="Nadir Gravitational Crucible Anchor",
        role="nadir-anchor"
    ))

    # =========================================================================
    # 6. 8 RESONANT GRAVITATIONAL PYLONS & FLOATING HALO RINGS
    # =========================================================================
    for i in range(8):
        angle = i * (2.0 * math.pi / 8.0)
        px = OX + 28.0 * math.cos(angle)
        pz = OZ + 28.0 * math.sin(angle)
        py = OY
        R_yaw = mat4_rotate_y(-angle)
        T_monolith = mat4_mul(mat4_translate(px, py, pz), R_yaw)

        # Pylon: RoundBox with an intersected diamond core
        pylon_base = sdf_leaf(2, [4.2, 1.1, 1.1], p0=0.25)
        pylon_gem = sdf_convex(make_polyhedron_planes("octahedron", 1.4))
        pylon_shape = sdf_binary(5, pylon_base, pylon_gem, t=0.45) # SmoothUnion

        objects.append(make_sdf_object(
            f"celestial-pylon-{i+1:02d}",
            "celestial.crystal.sapphire",
            [px, py, pz],
            T_monolith,
            pylon_shape,
            extent=[5.5, 2.0, 2.0],
            display_name=f"Resonant Crystalline Pylon {i+1}",
            role="gravitational-pylon"
        ))

        # Levitating Slotted Toroidal Halo above each Pylon
        halo_torus = sdf_leaf(6, [1.8, 0.24, 0.0])
        halo_slot = sdf_leaf(1, [2.2, 0.15, 0.15])
        halo_shape = sdf_binary(4, halo_torus, halo_slot) # CSG Subtract notch
        T_halo = mat4_mul(mat4_translate(px, py + 3.8, pz), rx90)
        objects.append(make_sdf_object(
            f"celestial-pylon-halo-{i+1:02d}",
            "celestial.gold.stellar",
            [px, py + 3.8, pz],
            T_halo,
            halo_shape,
            extent=[2.5, 2.5, 0.8],
            display_name=f"Pylon Levitation Slotted Halo {i+1}",
            role="pylon-halo"
        ))

    # =========================================================================
    # 7. THE FIRMAMENT DAIS & HIGH CELESTIAL PROMENADE (Y = 22.0)
    # =========================================================================
    DY = 22.0
    cardinal_decks = [
        ("north", [OX, DY, OZ + 18.0], [3.2, 0.35, 7.5]),
        ("south", [OX, DY, OZ - 18.0], [3.2, 0.35, 7.5]),
        ("east",  [OX + 18.0, DY, OZ], [7.5, 0.35, 3.2]),
        ("west",  [OX - 18.0, DY, OZ], [7.5, 0.35, 3.2]),
    ]
    for bname, bcenter, bdims in cardinal_decks:
        Tb = mat4_translate(*bcenter)
        deck_shape = sdf_leaf(2, bdims, p0=0.12) # RoundBox
        objects.append(make_sdf_object(
            f"skybridge-deck-{bname}",
            "celestial.marble.astral",
            bcenter,
            Tb,
            deck_shape,
            extent=[bdims[0] + 0.6, bdims[1] + 0.6, bdims[2] + 0.6],
            display_name=f"Firmament Dais Deck — {bname.capitalize()}",
            role="skybridge-deck"
        ))
        # Pedestal (Fluted cylinder) & Faceted Armillary Sextant
        p_offset = 6.2
        if bname == "north": px, pz = OX, OZ + 18.0 + p_offset
        elif bname == "south": px, pz = OX, OZ - 18.0 - p_offset
        elif bname == "east": px, pz = OX + 18.0 + p_offset, OZ
        elif bname == "west": px, pz = OX - 18.0 - p_offset, OZ
        Tp = mat4_translate(px, DY + 0.85, pz)
        pedestal_shape = sdf_leaf(4, [0.9, 0.85, 0.0])
        objects.append(make_sdf_object(
            f"skybridge-pedestal-{bname}",
            "celestial.gold.stellar",
            [px, DY + 0.85, pz],
            Tp,
            pedestal_shape,
            extent=[1.4, 1.4, 1.4],
            display_name=f"Celestial Observation Dais — {bname.capitalize()}",
            role="skybridge-pedestal"
        ))

        # Sextant Astrolabe Ring: Torus intersected with Box quadrant
        T_sex = mat4_mul(mat4_translate(px, DY + 2.1, pz), mat4_rotate_y(math.pi / 4.0))
        sex_torus = sdf_leaf(6, [0.85, 0.14, 0.0])
        sex_box = sdf_leaf(1, [1.0, 1.0, 0.5], offset=[0.5, 0.5, 0.0]) # Quadrant cut
        sex_shape = sdf_binary(3, sex_torus, sex_box) # CSG Intersect
        objects.append(make_sdf_object(
            f"skybridge-sextant-{bname}",
            "celestial.bronze.armillary",
            [px, DY + 2.1, pz],
            T_sex,
            sex_shape,
            extent=[1.2, 1.2, 0.8],
            display_name=f"Armillary Observation Quadrant Sextant — {bname.capitalize()}",
            role="observation-sextant"
        ))

    # Intercardinal Connecting Skybridges (NE, NW, SE, SW)
    intercardinals = [
        ("northeast", [OX + 12.8, DY, OZ + 12.8], math.radians(-45.0)),
        ("northwest", [OX - 12.8, DY, OZ + 12.8], math.radians(45.0)),
        ("southeast", [OX + 12.8, DY, OZ - 12.8], math.radians(45.0)),
        ("southwest", [OX - 12.8, DY, OZ - 12.8], math.radians(-45.0)),
    ]
    for iname, icenter, irot in intercardinals:
        T_ic = mat4_mul(mat4_translate(*icenter), mat4_rotate_y(irot))
        bridge_shape = sdf_leaf(2, [5.5, 0.32, 2.4], p0=0.10) # RoundBox
        objects.append(make_sdf_object(
            f"skybridge-octagonal-{iname}",
            "celestial.marble.astral",
            icenter,
            T_ic,
            bridge_shape,
            extent=[6.5, 0.8, 3.2],
            display_name=f"Firmament Octagonal Cloister Bridge — {iname.capitalize()}",
            role="octagonal-bridge"
        ))

    # 8 Astral Brazier Beacons: Fluted pedestal topped with Faceted Octahedron Crystal!
    octa_flame = sdf_convex(make_polyhedron_planes("octahedron", 0.55))
    for bi in range(8):
        b_angle = bi * (2.0 * math.pi / 8.0) + (math.pi / 8.0)
        bx = OX + 21.0 * math.cos(b_angle)
        bz = OZ + 21.0 * math.sin(b_angle)
        by = DY + 0.9
        T_br = mat4_translate(bx, by, bz)
        brazier_base = sdf_leaf(4, [0.48, 0.9, 0.0])
        objects.append(make_sdf_object(
            f"skybridge-brazier-{bi+1:02d}",
            "celestial.gold.electrum",
            [bx, by, bz],
            T_br,
            brazier_base,
            extent=[0.8, 1.4, 0.8],
            display_name=f"Astral Flame Brazier Beacon {bi+1}",
            role="brazier-beacon"
        ))

        # Floating Faceted Octahedral Flame Crystal
        T_flame = mat4_translate(bx, by + 1.35, bz)
        mat_fl = "celestial.ruby.nodal" if (bi % 2 == 0) else "celestial.crystal.cyan"
        objects.append(make_sdf_object(
            f"skybridge-flame-{bi+1:02d}",
            mat_fl,
            [bx, by + 1.35, bz],
            T_flame,
            octa_flame,
            extent=[0.8, 0.8, 0.8],
            display_name=f"Astral Octahedral Fire Crystal {bi+1}",
            role="brazier-flame"
        ))

    # Firmament Dais Central Oculus Architrave Rim (CSG Fluted Double-Torus)
    T_dais_ring = mat4_mul(mat4_translate(OX, DY - 0.2, OZ), rx90)
    dais_t1 = sdf_leaf(6, [12.5, 1.25, 0.0])
    dais_t2 = sdf_leaf(6, [11.0, 0.50, 0.0])
    dais_rim_shape = sdf_binary(2, dais_t1, dais_t2) # Union
    objects.append(make_sdf_object(
        "skybridge-central-aperture-ring",
        "celestial.bronze.armillary",
        [OX, DY - 0.2, OZ],
        T_dais_ring,
        dais_rim_shape,
        extent=[15.0, 15.0, 2.5],
        display_name="Firmament Dais Central Fluted Oculus Rim",
        role="skybridge-rim"
    ))

    # Ground-level Grand Celestial Oculus Architrave (Roof of 28-Room Central Gallery)
    T_oculus_rim = mat4_mul(mat4_translate(OX, 0.2, OZ), rx90)
    oculus_t1 = sdf_leaf(6, [8.8, 0.90, 0.0])
    oculus_t2 = sdf_leaf(6, [7.8, 0.40, 0.0])
    oculus_rim_shape = sdf_binary(2, oculus_t1, oculus_t2) # Union
    objects.append(make_sdf_object(
        "gallery-sky-oculus-architrave",
        "celestial.gold.stellar",
        [OX, 0.2, OZ],
        T_oculus_rim,
        oculus_rim_shape,
        extent=[11.0, 11.0, 2.2],
        display_name="Grand Celestial Oculus Architrave",
        role="oculus-architrave"
    ))

    # =========================================================================
    # 8. ASCENDING SPIRAL LEVITATION DISKS (Ascension from Gallery to Dais)
    # =========================================================================
    num_steps = 14
    for st in range(num_steps):
        s_prog = st / float(num_steps)
        s_ang = s_prog * (1.5 * math.pi)
        s_rad = 14.5 + 2.0 * math.sin(s_prog * math.pi)
        sx = OX + s_rad * math.cos(s_ang)
        sz = OZ + s_rad * math.sin(s_ang)
        sy = 1.5 + s_prog * (DY - 2.5)
        T_step = mat4_translate(sx, sy, sz)
        # Chamfered levitation disk: RoundBox or Cylinder with bevel
        step_shape = sdf_leaf(2, [1.4, 0.15, 1.4], p0=0.08) # RoundBox
        objects.append(make_sdf_object(
            f"celestial-ascension-step-{st+1:02d}",
            "celestial.marble.astral",
            [sx, sy, sz],
            T_step,
            step_shape,
            extent=[1.8, 0.5, 1.8],
            display_name=f"Ascension Levitation Chamfered Disk {st+1}",
            role="ascension-step"
        ))

    # 4 Grand Vertical Flux Colonnades (Anchoring Skybridge to Gallery)
    col_coords = [
        (OX + 14.0, OZ + 14.0),
        (OX - 14.0, OZ + 14.0),
        (OX + 14.0, OZ - 14.0),
        (OX - 14.0, OZ - 14.0),
    ]
    for ci, (cx, cz) in enumerate(col_coords):
        cy = DY / 2.0
        T_col = mat4_translate(cx, cy, cz)
        col_shape = sdf_leaf(4, [0.65, cy, 0.0]) # Cylinder
        objects.append(make_sdf_object(
            f"celestial-flux-colonnade-{ci+1}",
            "celestial.bronze.armillary",
            [cx, cy, cz],
            T_col,
            col_shape,
            extent=[1.2, cy + 0.5, 1.2],
            display_name=f"Celestial Flux Colonnade Pillar {ci+1}",
            role="flux-colonnade"
        ))

    return objects

def create_celestial_spatial_root():
    OX, OY, OZ = 0.0, 52.0, 87.5

    # 1. Advanced Multi-Harmonic Relativistic Quasar Jet rho(p, t):
    rho_ast = {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 6, # Scale
                    "children": [
                        {
                            "op": 23, # Div
                            "children": [
                                {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 8.5, "factors": {}}
                                        ]
                                    }
                                },
                                {
                                    "op": 0,
                                    "scalarForm": {
                                        "terms": [
                                            {"c": 1.0, "factors": {}},
                                            {"c": 0.004, "factors": {"x": 2.0}},
                                            {"c": 0.004, "factors": {"z": 2.0}},
                                            {"c": 0.0008, "factors": {"y": 2.0}}
                                        ]
                                    }
                                }
                            ]
                        },
                        {
                            "op": 0,
                            "scalarForm": {
                                "terms": [
                                    {"c": 1.0, "factors": {}},
                                    {
                                        "c": 0.28,
                                        "factors": {},
                                        "trans": [
                                            {"kind": 0, "scale": 1.8, "var": "t"}
                                        ]
                                    },
                                    {
                                        "c": 0.14,
                                        "factors": {},
                                        "trans": [
                                            {"kind": 1, "scale": 3.2, "shift": 0.7854, "var": "t"}
                                        ]
                                    }
                                ]
                            }
                        }
                    ]
                }
            }
        ]
    }

    # 2. Cosmic Chromatic Spectrum chi(p, t):
    chi_ast = {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 2, # VectorConstruct
                    "children": [
                        {
                            "op": 0,
                            "scalarForm": {
                                "terms": [
                                    {"c": 0.92, "factors": {}},
                                    {
                                        "c": 0.22,
                                        "factors": {},
                                        "trans": [{"kind": 1, "scale": 1.2, "var": "t"}]
                                    },
                                    {
                                        "c": 0.10,
                                        "factors": {},
                                        "trans": [{"kind": 0, "scale": 2.4, "var": "t"}]
                                    }
                                ]
                            }
                        },
                        {
                            "op": 0,
                            "scalarForm": {
                                "terms": [
                                    {"c": 0.78, "factors": {}},
                                    {
                                        "c": 0.18,
                                        "factors": {},
                                        "trans": [{"kind": 0, "scale": 1.2, "shift": 1.0, "var": "t"}]
                                    },
                                    {
                                        "c": 0.08,
                                        "factors": {},
                                        "trans": [{"kind": 1, "scale": 2.4, "var": "t"}]
                                    }
                                ]
                            }
                        },
                        {
                            "op": 0,
                            "scalarForm": {
                                "terms": [
                                    {"c": 0.98, "factors": {}},
                                    {
                                        "c": 0.25,
                                        "factors": {},
                                        "trans": [{"kind": 1, "scale": 1.2, "shift": 2.1, "var": "t"}]
                                    },
                                    {
                                        "c": 0.12,
                                        "factors": {},
                                        "trans": [{"kind": 0, "scale": 3.0, "var": "t"}]
                                    }
                                ]
                            }
                        }
                    ]
                }
            }
        ]
    }

    # 3. Multi-Lobe Relativistic Jet + Equatorial Flare alpha(p, omega, t):
    alpha_ast = {
        "input": "x",
        "pieces": [
            {
                "mathNode": {
                    "op": 0, # ScalarLeaf
                    "scalarForm": {
                        "terms": [
                            {"c": 0.60, "factors": {}},
                            {"c": 4.50, "factors": {"omega.y": 4.0}},
                            {"c": 1.80, "factors": {"omega.y": 2.0}},
                            {"c": 0.85, "factors": {"omega.x": 2.0}}
                        ]
                    }
                }
            }
        ]
    }

    spatial_root = {
        "position": [OX, OY, OZ],
        "scale": [1.0, 1.0, 1.0],
        "authoredProperties": {
            "light.source": {"t": "bool", "v": True},
            "light.enabled": {"t": "bool", "v": True},
            "light.intensity": {"t": "float", "v": 4.2},
            "light.color": {"t": "vec3", "x": 1.0, "y": 0.95, "z": 0.88},
            "light.ambient": {"t": "float", "v": 0.32},
            "light.diffuse": {"t": "float", "v": 0.92},
            "light.specular": {"t": "float", "v": 1.5},
            "light.attenuation.constant": {"t": "float", "v": 1.0},
            "light.attenuation.linear": {"t": "float", "v": 0.0},
            "light.attenuation.quadratic": {"t": "float", "v": 0.0}
        },
        "field": {
            "mode": "AST",
            "baseDensity": 1.0,
            "frequency": 1.0,
            "amplitude": 1.0,
            "astDefinition": rho_ast
        },
        "lightChroma": chi_ast,
        "lightAngular": alpha_ast
    }

    return spatial_root

def get_celestial_materials():
    return [
        {
            "ambient": 0.1,
            "baseColor": [0.03, 0.02, 0.04],
            "diffuse": 0.25,
            "name": "celestial.obsidian.core",
            "opacity": 1.0,
            "shininess": 96.0,
            "specular": 2.5
        },
        {
            "ambient": 0.35,
            "baseColor": [1.0, 0.84, 0.32],
            "diffuse": 0.88,
            "name": "celestial.gold.stellar",
            "opacity": 1.0,
            "shininess": 48.0,
            "specular": 1.8
        },
        {
            "ambient": 0.30,
            "baseColor": [0.98, 0.92, 0.65],
            "diffuse": 0.85,
            "name": "celestial.gold.electrum",
            "opacity": 1.0,
            "shininess": 56.0,
            "specular": 1.7
        },
        {
            "ambient": 0.22,
            "baseColor": [0.82, 0.54, 0.28],
            "diffuse": 0.82,
            "name": "celestial.bronze.armillary",
            "opacity": 1.0,
            "shininess": 36.0,
            "specular": 1.2
        },
        {
            "ambient": 0.35,
            "baseColor": [0.15, 0.45, 0.98],
            "diffuse": 0.75,
            "name": "celestial.crystal.sapphire",
            "opacity": 1.0,
            "shininess": 64.0,
            "specular": 2.2
        },
        {
            "ambient": 0.40,
            "baseColor": [0.12, 0.92, 0.96],
            "diffuse": 0.75,
            "name": "celestial.crystal.cyan",
            "opacity": 1.0,
            "shininess": 64.0,
            "specular": 2.0
        },
        {
            "ambient": 0.25,
            "baseColor": [0.95, 0.95, 0.98],
            "diffuse": 0.90,
            "name": "celestial.marble.astral",
            "opacity": 1.0,
            "shininess": 32.0,
            "specular": 1.1
        },
        {
            "ambient": 0.40,
            "baseColor": [0.95, 0.12, 0.24],
            "diffuse": 0.85,
            "name": "celestial.ruby.nodal",
            "opacity": 1.0,
            "shininess": 80.0,
            "specular": 2.4
        },
        {
            "ambient": 0.35,
            "baseColor": [0.65, 0.25, 0.95],
            "diffuse": 0.80,
            "name": "celestial.amethyst.resonance",
            "opacity": 1.0,
            "shininess": 64.0,
            "specular": 2.0
        },
        {
            "ambient": 0.28,
            "baseColor": [0.88, 0.92, 0.96],
            "diffuse": 0.85,
            "name": "celestial.silver.quicksilver",
            "opacity": 1.0,
            "shininess": 84.0,
            "specular": 2.2
        }
    ]

def main():
    print("Generating Masterwork Sky Celestial Apparatus with Advanced SDF Geometry...")
    sky_objects = build_sky_celestial_apparatus()
    print(f"Generated {len(sky_objects)} celestial sky objects.")

    celestial_root = create_celestial_spatial_root()
    celestial_materials = get_celestial_materials()

    # 1. Update Radiance Gallery zone.json
    zone_path = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/zones/Radiance Gallery/zone.json"
    with open(zone_path, "r") as f:
        gallery_zone = json.load(f)

    existing_mat_map = {m["name"]: m for m in gallery_zone.get("materials", [])}
    for m in celestial_materials:
        existing_mat_map[m["name"]] = m
    gallery_zone["materials"] = list(existing_mat_map.values())

    existing_objs = gallery_zone.get("world", {}).get("objects", [])
    base_objs = [o for o in existing_objs if not o["objectID"].startswith("celestial-") and not o["objectID"].startswith("skybridge-") and not o["objectID"].startswith("gallery-sky-")]
    total_objs = base_objs + sky_objects
    gallery_zone["world"]["objects"] = total_objs
    gallery_zone["objects"] = total_objs # Dual-read compatibility
    gallery_zone["spatialRoot"] = celestial_root

    with open(zone_path, "w") as f:
        json.dump(gallery_zone, f, indent=2)
    print(f"Updated {zone_path} with {len(total_objs)} total objects ({len(sky_objects)} celestial).")

    # 2. Update radiance_gallery.json world
    world_path = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/worlds/radiance_gallery.json"
    gallery_world = {
        "saveFormat": "zone-identity-v1",
        "identifier": "radiance_gallery",
        "name": "Radiance Gallery & Astral Singularity",
        "owner": "zacharyzhang",
        "description": "The 28-Room OntoMath Radiance Demonstration Gallery beneath the Celestial Astral Orrery in the Sky.",
        "currentZone": 0,
        "currentZoneId": "Radiance Gallery",
        "cameraPos": [0.0, 24.5, 65.0],      # Situated on the Firmament Dais Skybridge
        "cameraFront": [0.0, 0.45, 0.89],     # Looking up at pitch ~27 deg into the turning celestial rings
        "cameraUp": [0.0, 1.0, 0.0],
        "flying": True,
        "pitch": 27.0,
        "yaw": 90.0,
        "zoneRefs": [
            {
                "identifier": "Radiance Gallery",
                "kind": "gallery"
            }
        ],
        "zones": [
            gallery_zone
        ],
        "objects": total_objs,
        "materials": gallery_zone["materials"],
        "authoredLaws": {},
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Gemini Spark"],
        "injected_by": "Gemini Spark"
    }
    with open(world_path, "w") as f:
        json.dump(gallery_world, f, indent=2)
    print(f"Updated {world_path}.")

    # 3. Create standalone Celestial World: celestial_radiance_engine.json
    standalone_world_path = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/worlds/celestial_radiance_engine.json"
    standalone_zone_dir = "/Users/zacharyzhang/Documents/GitHub/Earthcall/saves/zones/Celestial Radiance Engine"
    os.makedirs(standalone_zone_dir, exist_ok=True)
    standalone_zone_path = os.path.join(standalone_zone_dir, "zone.json")
    standalone_zone = {
        "identifier": "Celestial Radiance Engine",
        "name": "Celestial Radiance Engine",
        "owner": "zacharyzhang",
        "parentZone": "",
        "scope": "public",
        "deletable": True,
        "spatialRoot": celestial_root,
        "materials": celestial_materials,
        "world": {
            "objects": sky_objects,
            "laws": [],
            "relations": []
        },
        "objects": sky_objects, # Dual-read compatibility
        "authoredLaws": {},
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Gemini Spark"],
        "injected_by": "Gemini Spark"
    }
    with open(standalone_zone_path, "w") as f:
        json.dump(standalone_zone, f, indent=2)
    print(f"Wrote standalone zone: {standalone_zone_path}.")

    standalone_world = {
        "saveFormat": "zone-identity-v1",
        "identifier": "celestial_radiance_engine",
        "name": "Celestial Radiance Engine",
        "owner": "zacharyzhang",
        "description": "The Masterwork Celestial Singularity and Relativistic Astral Orrery in the Sky.",
        "cameraPos": [0.0, 24.5, 65.0],
        "cameraFront": [0.0, 0.45, 0.89],
        "cameraUp": [0.0, 1.0, 0.0],
        "flying": True,
        "pitch": 27.0,
        "yaw": 90.0,
        "currentZone": 0,
        "currentZoneId": "Celestial Radiance Engine",
        "zoneRefs": [
            {
                "identifier": "Celestial Radiance Engine",
                "kind": "celestial"
            }
        ],
        "zones": [
            standalone_zone
        ],
        "objects": sky_objects,
        "materials": celestial_materials,
        "authoredLaws": {},
        "formationRelations": [],
        "lexemes": [],
        "authors": ["Gemini Spark"],
        "injected_by": "Gemini Spark"
    }
    with open(standalone_world_path, "w") as f:
        json.dump(standalone_world, f, indent=2)
    print(f"Wrote standalone world: {standalone_world_path}.")

if __name__ == "__main__":
    main()
