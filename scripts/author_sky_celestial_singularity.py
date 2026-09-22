#!/usr/bin/env python3
"""
author_sky_celestial_singularity.py

Authors the Masterwork Celestial Radiance Engine & Astral Orrery in the sky
above the 28-Room Radiance Gallery using ultra-detailed, ornate SDF geometry:
- Regular & Semi-Regular Convex Polyhedra (Octahedra, Dodecahedra, Icosahedra, Rhombic Dodecahedra)
- Intricate CSG Carving (Hollow Coronal Shells, Slotted Astrolabe Retes, Cassini Rings, Crucibles, Bezels)
- Architectural & Astronomical Micro-Details (Balustrades, Vernier Scales, Gnomons, Gimbals, Diopters)
- Planetary Epicyclic Moons, Stator Coils, and Infall Spiral Streams
- Ultra-Detailed Embellishments on the BIG HUGE RINGS (Polar Gimbal Housings, Gates of the Equinox,
  Rete Star Pointers, Hour Studs, Declination Graduations, Precessional Gyros, Escapement Ratchets)
"""

import json, math, os

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
# SDF GEOMETRIC CONSTRUCTORS
# =============================================================================

def sdf_leaf(prim, dims, offset=(0, 0, 0), p0=0.0, p1=0.0):
    return {
        "op": 0, # Leaf
        "prim": prim,
        "dims": [float(dims[0]), float(dims[1]), float(dims[2])],
        "offset": [float(offset[0]), float(offset[1]), float(offset[2])],
        "p0": float(p0),
        "p1": float(p1),
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
        "t": float(t),
        "children": [child_a, child_b]
    }

def sdf_convex(planes):
    return {
        "op": 0,
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
    core_sph = sdf_leaf(0, [4.2, 4.2, 4.2], p0=4.2)
    lobe_x = sdf_leaf(3, [5.4, 2.8, 2.8])
    lobe_y = sdf_leaf(3, [2.8, 5.4, 2.8])
    nucleus_cross = sdf_binary(5, core_sph, lobe_x, t=0.65)
    nucleus_shape = sdf_binary(5, nucleus_cross, lobe_y, t=0.65)

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

    # Inner Ergosurface Slotted Boundary Torus
    ergo_torus = sdf_leaf(6, [5.8, 0.48, 0.0])
    ergo_slot = sdf_leaf(1, [6.5, 0.35, 0.35])
    ergo_shape = sdf_binary(4, ergo_torus, ergo_slot)
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

    # Coronal Stellar Focus Shell (Hollow Sphere CSG)
    corona_outer = sdf_leaf(0, [6.4, 6.4, 6.4], p0=6.4)
    corona_inner = sdf_leaf(0, [5.9, 5.9, 5.9], p0=5.9)
    corona_shape = sdf_binary(4, corona_outer, corona_inner)
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

    # Quadrupole Quantum Flux Nodes: 4 Faceted Rhombic Dodecahedra
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
    # 2. THE GRAND KEPLERIAN ARMILLARY ORRERY (The 7 Giant Concentric Rings)
    # =========================================================================
    # Ring 1: Equatorial Accretion Ring (Torus CSG-slotted with radial graduations)
    ring1_torus = sdf_leaf(6, [20.0, 0.88, 0.0])
    ring1_slot_x = sdf_leaf(1, [22.0, 0.30, 0.30])
    ring1_slot_z = sdf_leaf(1, [0.30, 22.0, 0.30])
    ring1_slots = sdf_binary(2, ring1_slot_x, ring1_slot_z)
    ring1_shape = sdf_binary(4, ring1_torus, ring1_slots)
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
    ring2_shape = sdf_binary(4, ring2_torus, ring2_bevel)
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

    # Rings 5 & 6: Tropics of Cancer & Capricorn
    r_tropic = math.sqrt(max(1.0, 32.0**2 - 10.5**2))
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
    ring8_shape = sdf_binary(2, ring8_main, ring8_inner)
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
    # 3. ULTRA-DETAILED ORNATE EMBELLISHMENTS ON THE BIG RINGS
    # =========================================================================

    # --- A. CELESTIAL NORTH & SOUTH POLAR GIMBAL PIVOT ASSEMBLIES ---
    # Intersection of Prime Meridian (R=32) and Colure (R=32) at Y = OY +/- 32
    for p_sign, p_label in [(1.0, "north"), (-1.0, "south")]:
        pole_y = OY + p_sign * 32.0
        T_pole = mat4_translate(OX, pole_y, OZ)

        # 1. Ornate Gimbal Bearing Housing: RoundBox carved with cylindrical socket
        p_box = sdf_leaf(2, [1.8, 1.2, 1.8], p0=0.25)
        p_hole = sdf_leaf(4, [1.1, 1.5, 0.0]) # Cylinder socket along Z
        p_housing = sdf_binary(4, p_box, p_hole)
        objects.append(make_sdf_object(
            f"celestial-polar-gimbal-{p_label}",
            "celestial.bronze.armillary",
            [OX, pole_y, OZ],
            T_pole,
            p_housing,
            extent=[2.2, 1.8, 2.2],
            display_name=f"Celestial {p_label.capitalize()} Polar Gimbal Bearing Housing",
            role="polar-gimbal"
        ))

        # 2. Golden Pivot Axle Pin
        T_axle = mat4_mul(mat4_translate(OX, pole_y, OZ), rx90)
        objects.append(make_sdf_object(
            f"celestial-polar-axle-{p_label}",
            "celestial.gold.stellar",
            [OX, pole_y, OZ],
            T_axle,
            sdf_leaf(4, [0.65, 1.6, 0.0]), # Cylinder along Z
            extent=[1.0, 1.0, 2.0],
            display_name=f"Celestial {p_label.capitalize()} Axis Pivot Pin",
            role="polar-axle"
        ))

        # 3. Mounted Polar Jewel Indicator Needle
        T_needle = mat4_translate(OX, pole_y + p_sign * 1.8, OZ)
        mat_needle = "celestial.ruby.nodal" if p_sign > 0 else "celestial.crystal.sapphire"
        objects.append(make_sdf_object(
            f"celestial-polar-needle-{p_label}",
            mat_needle,
            [OX, pole_y + p_sign * 1.8, OZ],
            T_needle,
            sdf_leaf(5, [0.55, 1.6 * p_sign, 0.0]), # Cone
            extent=[0.9, 2.0, 0.9],
            display_name=f"Polar Axis Magnetic Indicator Needle — {p_label.capitalize()}",
            role="polar-needle"
        ))

    # --- B. THE 4 GATES OF THE EQUINOX & SOLSTICE NODES ---
    # Intersection of Ring I (Equator R=20) and Ring II (Ecliptic R=26)
    equinox_nodes = [
        ("vernal-aries", 0.0, "Vernal Equinox Gate (0 deg Aries)", "celestial.crystal.cyan"),
        ("autumnal-libra", math.pi, "Autumnal Equinox Gate (180 deg Libra)", "celestial.ruby.nodal"),
        ("summer-solstice", math.pi / 2.0, "Summer Solstice Apex Node (90 deg Cancer)", "celestial.gold.stellar"),
        ("winter-solstice", 3.0 * math.pi / 2.0, "Winter Solstice Nadir Node (270 deg Capricorn)", "celestial.amethyst.resonance")
    ]
    for ename, eang, edesc, emat in equinox_nodes:
        # Position on Ecliptic Ring
        local_v = [26.0 * math.cos(eang), 26.0 * math.sin(eang), 0.0]
        rad23 = math.radians(23.4)
        c23, s23 = math.cos(rad23), math.sin(rad23)
        ex = OX + (local_v[0] * c23 - local_v[1] * s23)
        ey = OY + (local_v[0] * s23 + local_v[1] * c23)
        ez = OZ
        T_eq_gate = mat4_translate(ex, ey, ez)

        # Caliper Bridge Bracket clamping Ecliptic & Equator together
        c_bracket = sdf_leaf(2, [1.5, 1.1, 1.1], p0=0.20)
        c_aperture = sdf_leaf(4, [0.65, 0.8, 0.0])
        c_gate_shape = sdf_binary(4, c_bracket, c_aperture)
        objects.append(make_sdf_object(
            f"celestial-equinox-gate-{ename}",
            "celestial.bronze.armillary",
            [ex, ey, ez],
            T_eq_gate,
            c_gate_shape,
            extent=[1.8, 1.5, 1.5],
            display_name=f"Chrono-Gate Clasp — {edesc}",
            role="equinox-gate"
        ))

        # Levitating Octahedral Gate Diamond inside each Equinox clasp
        T_eq_gem = mat4_translate(ex, ey + 1.2, ez)
        objects.append(make_sdf_object(
            f"celestial-equinox-gem-{ename}",
            emat,
            [ex, ey + 1.2, ez],
            T_eq_gem,
            sdf_convex(make_polyhedron_planes("octahedron", 0.75)),
            extent=[1.0, 1.0, 1.0],
            display_name=f"Solstitial Focus Diamond — {edesc}",
            role="equinox-gem"
        ))

    # --- C. RING I (EQUATOR): 24 HOUR GRADUATION STUDS & 8 INWARD RETE STRUTS ---
    # 24 Sidereal Hour Studs (spaced every 15 deg)
    for h in range(24):
        h_ang = h * (2.0 * math.pi / 24.0)
        hx = OX + 20.0 * math.cos(h_ang)
        hz = OZ + 20.0 * math.sin(h_ang)
        hy = OY
        T_h = mat4_translate(hx, hy, hz)
        h_mat = "celestial.gold.electrum" if (h % 6 == 0) else "celestial.silver.quicksilver"
        h_shape = sdf_leaf(2, [0.32, 0.25, 0.32], p0=0.08) # RoundBox stud
        objects.append(make_sdf_object(
            f"celestial-equator-hour-stud-{h+1:02d}",
            h_mat,
            [hx, hy, hz],
            T_h,
            h_shape,
            extent=[0.6, 0.5, 0.6],
            display_name=f"Equatorial Sidereal Hour Stud {h}:00",
            role="hour-stud"
        ))

    # 8 Inward-Facing Rete Struts (filigree spokes bridging Equator R=20 inward to R=10)
    for s_i in range(8):
        s_ang = s_i * (2.0 * math.pi / 8.0) + (math.pi / 8.0)
        mid_r = 15.0 # midpoint of spoke
        sp_x = OX + mid_r * math.cos(s_ang)
        sp_z = OZ + mid_r * math.sin(s_ang)
        sp_y = OY
        T_spoke = mat4_mul(mat4_translate(sp_x, sp_y, sp_z), mat4_rotate_y(-s_ang))

        # Spoke: Chamfered strut with circular central aperture (CSG Subtract)
        strut = sdf_leaf(2, [4.8, 0.28, 0.32], p0=0.08)
        strut_hole = sdf_leaf(4, [0.45, 0.5, 0.0]) # Cylinder
        spoke_shape = sdf_binary(4, strut, strut_hole)

        objects.append(make_sdf_object(
            f"celestial-equator-rete-spoke-{s_i+1:02d}",
            "celestial.bronze.armillary",
            [sp_x, sp_y, sp_z],
            T_spoke,
            spoke_shape,
            extent=[5.2, 0.6, 0.8],
            display_name=f"Equatorial Filigree Rete Spoke #{s_i+1}",
            role="rete-spoke"
        ))

        # Central Sapphire Medallion inside the spoke aperture
        T_med = mat4_translate(sp_x, sp_y, sp_z)
        objects.append(make_sdf_object(
            f"celestial-spoke-medallion-{s_i+1:02d}",
            "celestial.crystal.sapphire",
            [sp_x, sp_y, sp_z],
            T_med,
            sdf_leaf(4, [0.35, 0.18, 0.0]), # Cylinder
            extent=[0.6, 0.4, 0.6],
            display_name=f"Rete Spoke Sapphire Medallion #{s_i+1}",
            role="spoke-medallion"
        ))

    # --- D. RING II (ECLIPTIC): 12 DECAN CLASPS & 8 RETE STAR-POINTER NEEDLES ---
    # 12 Astrological Decan Clasp Plates (spaced every 30 deg along tilted Ecliptic)
    for d in range(12):
        d_ang = d * (2.0 * math.pi / 12.0)
        local_v = [26.0 * math.cos(d_ang), 26.0 * math.sin(d_ang), 0.0]
        rad23 = math.radians(23.4)
        c23, s23 = math.cos(rad23), math.sin(rad23)
        dx = OX + (local_v[0] * c23 - local_v[1] * s23)
        dy = OY + (local_v[0] * s23 + local_v[1] * c23)
        dz = OZ
        T_decan = mat4_translate(dx, dy, dz)

        # Tiered Decan Clasp Mount
        decan_base = sdf_leaf(2, [0.95, 0.35, 0.75], p0=0.10)
        objects.append(make_sdf_object(
            f"celestial-ecliptic-decan-clasp-{d+1:02d}",
            "celestial.gold.electrum",
            [dx, dy, dz],
            T_decan,
            decan_base,
            extent=[1.2, 0.6, 1.0],
            display_name=f"Ecliptic Decan Mounting Clasp #{d+1}",
            role="decan-clasp"
        ))

        # Embedded Cabochon Gemstone in each decan plate
        T_gem = mat4_translate(dx, dy + 0.42, dz)
        gem_mat = "celestial.ruby.nodal" if (d % 3 == 0) else "celestial.crystal.cyan"
        objects.append(make_sdf_object(
            f"celestial-ecliptic-decan-gem-{d+1:02d}",
            gem_mat,
            [dx, dy + 0.42, dz],
            T_gem,
            sdf_leaf(0, [0.28, 0.28, 0.28], p0=0.28),
            extent=[0.5, 0.5, 0.5],
            display_name=f"Decan Astronomical Cabochon #{d+1}",
            role="decan-gem"
        ))

    # 8 Astrolabe Rete Star-Pointer Needles (pointing to fixed stars)
    star_names = ["Sirius", "Vega", "Aldebaran", "Arcturus", "Rigel", "Spica", "Antares", "Polaris"]
    for sn_i, star_n in enumerate(star_names):
        star_ang = sn_i * (2.0 * math.pi / 8.0) + 0.22
        local_v = [26.0 * math.cos(star_ang), 26.0 * math.sin(star_ang), 0.0]
        rad23 = math.radians(23.4)
        c23, s23 = math.cos(rad23), math.sin(rad23)
        sx = OX + (local_v[0] * c23 - local_v[1] * s23)
        sy = OY + (local_v[0] * s23 + local_v[1] * c23)
        sz = OZ

        # Flame-pointer needle: Smooth-Union of spherical root and projecting cone needle
        T_star_root = mat4_translate(sx, sy, sz)
        s_root_orb = sdf_leaf(0, [0.45, 0.45, 0.45], p0=0.45)
        s_needle_cone = sdf_leaf(5, [0.35, 1.8, 0.0]) # Cone
        flame_needle = sdf_binary(5, s_root_orb, s_needle_cone, t=0.25)
        objects.append(make_sdf_object(
            f"celestial-rete-star-pointer-{star_n.lower()}",
            "celestial.gold.stellar",
            [sx, sy, sz],
            T_star_root,
            flame_needle,
            extent=[1.0, 2.2, 1.0],
            display_name=f"Astrolabe Rete Star Pointer — {star_n}",
            role="star-pointer"
        ))

        # Cyan Ether Star Bead perched on the pointer tip
        T_tip_bead = mat4_translate(sx, sy + 2.1, sz)
        objects.append(make_sdf_object(
            f"celestial-rete-star-bead-{star_n.lower()}",
            "celestial.crystal.cyan",
            [sx, sy + 2.1, sz],
            T_tip_bead,
            sdf_convex(make_polyhedron_planes("octahedron", 0.35)),
            extent=[0.6, 0.6, 0.6],
            display_name=f"Fixed Star Celestial Nodule — {star_n}",
            role="star-bead"
        ))

    # --- E. RING III (PRIME MERIDIAN): 18 DECLINATION LATITUDE INDEX TABS ---
    # Vertical Y-Z circle (R = 32.0): angle from -90 deg (south) to +90 deg (north)
    for lat_i in range(18):
        decl_ang = -math.pi / 2.0 + (lat_i + 0.5) * (math.pi / 18.0)
        my = OY + 32.0 * math.sin(decl_ang)
        mz = OZ + 32.0 * math.cos(decl_ang)
        mx = OX
        T_decl = mat4_translate(mx, my, mz)

        # Inward-pointing index tooth
        decl_shape = sdf_leaf(5, [0.32, 0.95, 0.0]) # Cone
        objects.append(make_sdf_object(
            f"celestial-meridian-decl-tab-{lat_i+1:02d}",
            "celestial.gold.electrum",
            [mx, my, mz],
            T_decl,
            decl_shape,
            extent=[0.6, 1.2, 0.6],
            display_name=f"Prime Meridian Declination Index {int(math.degrees(decl_ang))} deg",
            role="declination-tab"
        ))

    # --- F. RINGS V & VI (TROPICS): 8 RIGIDIFYING ARMILLARY STRUT BRACKETS ---
    # 4 struts on Cancer (Y = OY + 10.5), 4 struts on Capricorn (Y = OY - 10.5)
    for t_idx, (t_sign, t_name) in enumerate([(1.0, "cancer"), (-1.0, "capricorn")]):
        ty = OY + t_sign * 10.5
        for b_i, b_ang in enumerate([0.0, math.pi / 2.0, math.pi, 3.0 * math.pi / 2.0]):
            bx = OX + r_tropic * math.cos(b_ang)
            bz = OZ + r_tropic * math.sin(b_ang)
            T_tb = mat4_translate(bx, ty, bz)

            # Vertical strut bracket clamping Tropic to vertical meridian/colure
            bracket_shape = sdf_leaf(2, [0.35, 1.2, 0.35], p0=0.08)
            objects.append(make_sdf_object(
                f"celestial-tropic-{t_name}-bracket-{b_i+1}",
                "celestial.bronze.armillary",
                [bx, ty, bz],
                T_tb,
                bracket_shape,
                extent=[0.6, 1.5, 0.6],
                display_name=f"Armillary Structural Tie Bracket — {t_name.capitalize()} #{b_i+1}",
                role="tropic-bracket"
            ))

    # --- G. RING VII (PRECESSIONAL GYRO R=36): 16 INERTIAL BALANCER SPHERULES ---
    # Positioned along the 45-deg precessional ring
    for g_i in range(16):
        g_ang = g_i * (2.0 * math.pi / 16.0)
        # Point on local circle [36*cos, 36*sin, 0], rotated by rx45 and ry45
        local_v = [36.0 * math.cos(g_ang), 36.0 * math.sin(g_ang), 0.0]
        # rx45: y' = y*c - z*s, z' = y*s + z*c
        rad45 = math.radians(45.0)
        c45, s45 = math.cos(rad45), math.sin(rad45)
        rx_v = [local_v[0], local_v[1] * c45, local_v[1] * s45]
        # ry45: x'' = x'*c + z'*s, z'' = -x'*s + z'*c
        gx = OX + (rx_v[0] * c45 + rx_v[2] * s45)
        gy = OY + rx_v[1]
        gz = OZ + (-rx_v[0] * s45 + rx_v[2] * c45)
        T_gyro_spherule = mat4_translate(gx, gy, gz)

        # Quicksilver Inertial Balance Orb
        objects.append(make_sdf_object(
            f"celestial-precession-spherule-{g_i+1:02d}",
            "celestial.silver.quicksilver",
            [gx, gy, gz],
            T_gyro_spherule,
            sdf_leaf(0, [0.55, 0.55, 0.55], p0=0.55),
            extent=[0.8, 0.8, 0.8],
            display_name=f"Precessional Inertial Counter-Weight Spherule #{g_i+1}",
            role="precessional-spherule"
        ))

        # Bronze Gimbal Hoop around each spherule
        T_ghoop = mat4_mul(mat4_translate(gx, gy, gz), rx90)
        objects.append(make_sdf_object(
            f"celestial-precession-hoop-{g_i+1:02d}",
            "celestial.bronze.armillary",
            [gx, gy, gz],
            T_ghoop,
            sdf_leaf(6, [0.92, 0.08, 0.0]),
            extent=[1.2, 1.2, 0.4],
            display_name=f"Inertial Spherule Gimbal Hoop #{g_i+1}",
            role="precessional-hoop"
        ))

    # --- H. RING VIII (OUTER ZODIAC R=42): 24 ESCAPEMENT RATCHETS & 8 SIGHTING GIMBALS ---
    for r_i in range(24):
        r_ang = r_i * (2.0 * math.pi / 24.0)
        rx = OX + 42.0 * math.cos(r_ang)
        rz = OZ + 42.0 * math.sin(r_ang)
        ry = OY
        T_ratchet = mat4_mul(mat4_translate(rx, ry, rz), mat4_rotate_y(-r_ang))
        # Escapement Ratchet Cog: Chamfered tooth
        cog_shape = sdf_leaf(2, [0.45, 0.65, 0.35], p0=0.08)
        objects.append(make_sdf_object(
            f"celestial-escapement-cog-{r_i+1:02d}",
            "celestial.gold.stellar",
            [rx, ry, rz],
            T_ratchet,
            cog_shape,
            extent=[0.8, 0.9, 0.6],
            display_name=f"Zodiac Perimeter Escapement Cog #{r_i+1}",
            role="escapement-cog"
        ))

    # 8 Outrigger Astrolabe Sighting Gimbals extending into deep space
    for o_i in range(8):
        o_ang = o_i * (2.0 * math.pi / 8.0) + (math.pi / 8.0)
        ox_p = OX + 45.5 * math.cos(o_ang)
        oz_p = OZ + 45.5 * math.sin(o_ang)
        oy_p = OY
        T_outrig = mat4_mul(mat4_translate(ox_p, oy_p, oz_p), mat4_rotate_y(-o_ang))

        # Outrigger Arm: Chamfered strut extending outward from Ring VIII
        outrig_arm = sdf_leaf(2, [2.4, 0.35, 0.45], p0=0.10)
        objects.append(make_sdf_object(
            f"celestial-outrigger-arm-{o_i+1:02d}",
            "celestial.bronze.armillary",
            [ox_p, oy_p, oz_p],
            T_outrig,
            outrig_arm,
            extent=[2.8, 0.6, 0.8],
            display_name=f"Deep-Space Sighting Outrigger Arm #{o_i+1}",
            role="outrigger-arm"
        ))

        # Levitating Octahedral Sighting Lens mounted on each outrigger tip
        tip_x = OX + 47.5 * math.cos(o_ang)
        tip_z = OZ + 47.5 * math.sin(o_ang)
        T_tip_lens = mat4_translate(tip_x, oy_p + 0.85, tip_z)
        lens_shape = sdf_convex(make_polyhedron_planes("octahedron", 0.75))
        objects.append(make_sdf_object(
            f"celestial-sighting-lens-{o_i+1:02d}",
            "celestial.crystal.cyan",
            [tip_x, oy_p + 0.85, tip_z],
            T_tip_lens,
            lens_shape,
            extent=[1.1, 1.1, 1.1],
            display_name=f"Outrigger Refractive Sighting Crystal #{o_i+1}",
            role="sighting-lens"
        ))

    # =========================================================================
    # 4. 12 ZODIAC STATIONS WITH GIMBAL CLASPS & POINTERS
    # =========================================================================
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

        # Gimbal Clasp Bracket embracing each Zodiac Polyhedron
        T_zclasp = mat4_mul(mat4_translate(zx, zy, zz), mat4_rotate_y(-angle))
        clasp_ring = sdf_leaf(6, [1.65, 0.18, 0.0])
        clasp_box = sdf_leaf(1, [1.8, 1.8, 1.0], offset=[0.0, 0.9, 0.0])
        clasp_c = sdf_binary(3, clasp_ring, clasp_box)
        objects.append(make_sdf_object(
            f"celestial-zodiac-clasp-{i+1:02d}",
            "celestial.gold.stellar",
            [zx, zy, zz],
            T_zclasp,
            clasp_c,
            extent=[2.0, 2.0, 1.2],
            display_name=f"Zodiac Gimbal Mount Clasp {i+1}",
            role="zodiac-clasp"
        ))

        # Ecliptic Chrono-Gear Teeth: Chamfered RoundBox teeth on the tilted 23.4 ring
        local_v = [26.0 * math.cos(angle), 26.0 * math.sin(angle), 0.0]
        rad23 = math.radians(23.4)
        c23, s23 = math.cos(rad23), math.sin(rad23)
        ex = OX + (local_v[0] * c23 - local_v[1] * s23)
        ey = OY + (local_v[0] * s23 + local_v[1] * c23)
        ez = OZ
        T_eg = mat4_translate(ex, ey, ez)
        tooth_shape = sdf_leaf(2, [0.65, 0.45, 0.45], p0=0.15)
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
    # 5. SACRED GEOMETRIC KEPLERIAN PLANETARY SATELLITES & MOONLETS
    # =========================================================================
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
         sdf_leaf(3, [1.8, 1.3, 1.8]), 2.2),
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

        p_gimbal_r = pext * 1.35
        p_gimbal_shape = sdf_leaf(6, [p_gimbal_r, 0.12, 0.0])
        T_pgim = mat4_mul(mat4_translate(px, py, pz), mat4_mul(mat4_rotate_y(prad), rx90))
        objects.append(make_sdf_object(
            f"celestial-planet-gimbal-{pname}",
            "celestial.bronze.armillary",
            [px, py, pz],
            T_pgim,
            p_gimbal_shape,
            extent=[p_gimbal_r + 0.5, p_gimbal_r + 0.5, 0.5],
            display_name=f"Equatorial Gimbal Spherule — {pname.capitalize()}",
            role="planet-gimbal"
        ))

    # Saturn's Cassini-Division Double Ring
    saturn_rad = math.radians(335.0)
    sx = OX + 35.5 * math.cos(saturn_rad)
    sz = OZ + 35.5 * math.sin(saturn_rad)
    sy = OY + 2.5 * math.sin(saturn_rad * 2.0)
    sat_ring_main = sdf_leaf(6, [3.8, 0.35, 0.0])
    sat_ring_gap = sdf_leaf(6, [3.8, 0.08, 0.0])
    sat_ring_csg = sdf_binary(4, sat_ring_main, sat_ring_gap)
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

    # Saturn's Shepherd Moonlets (Titan & Mimas)
    for m_i, (m_name, m_dist, m_size) in enumerate([("titan", 4.8, 0.38), ("mimas", 3.2, 0.22)]):
        m_ang = saturn_rad + (m_i + 1) * 1.6
        mx = sx + m_dist * math.cos(m_ang)
        mz = sz + m_dist * math.sin(m_ang)
        my = sy + 0.2 * math.sin(m_ang)
        T_m = mat4_translate(mx, my, mz)
        objects.append(make_sdf_object(
            f"celestial-saturn-moon-{m_name}",
            "celestial.silver.quicksilver",
            [mx, my, mz],
            T_m,
            sdf_leaf(0, [m_size, m_size, m_size], p0=m_size),
            extent=[0.6, 0.6, 0.6],
            display_name=f"Saturnian Shepherd Moonlet — {m_name.capitalize()}",
            role="shepherd-moon"
        ))

    # Jupiter's 4 Galilean Moons (Io, Europa, Ganymede, Callisto)
    jup_rad = math.radians(75.0)
    jx = OX + 30.5 * math.cos(jup_rad)
    jz = OZ + 30.5 * math.sin(jup_rad)
    jy = OY + 2.5 * math.sin(jup_rad * 2.0)
    galilean = [("io", 3.2, 0.32, "celestial.ruby.nodal"),
                ("europa", 4.0, 0.28, "celestial.marble.astral"),
                ("ganymede", 5.0, 0.42, "celestial.crystal.cyan"),
                ("callisto", 6.2, 0.38, "celestial.bronze.armillary")]
    for gi, (gname, gdist, gsize, gmat) in enumerate(galilean):
        gang = jup_rad + gi * (math.pi / 2.0)
        gx = jx + gdist * math.cos(gang)
        gz = jz + gdist * math.sin(gang)
        gy = jy + 0.35 * math.sin(gang * 2.0)
        T_g = mat4_translate(gx, gy, gz)
        objects.append(make_sdf_object(
            f"celestial-jupiter-moon-{gname}",
            gmat,
            [gx, gy, gz],
            T_g,
            sdf_convex(make_polyhedron_planes("octahedron", gsize)),
            extent=[0.7, 0.7, 0.7],
            display_name=f"Galilean Harmonic Moonlet — {gname.capitalize()}",
            role="galilean-moon"
        ))

    # Sol's Coronal Rays & Flare Halo
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

    # 4 Cardinal Solar Ray Needles radiating from Sol
    for s_ri, (s_dx, s_dz) in enumerate([(3.2, 0.0), (-3.2, 0.0), (0.0, 3.2), (0.0, -3.2)]):
        s_rx, s_rz = sol_x + s_dx, sol_z + s_dz
        T_sray = mat4_translate(s_rx, sol_y, s_rz)
        objects.append(make_sdf_object(
            f"celestial-solar-ray-{s_ri+1}",
            "celestial.gold.stellar",
            [s_rx, sol_y, s_rz],
            T_sray,
            sdf_leaf(5, [0.45, 1.2, 0.0]),
            extent=[0.8, 1.5, 0.8],
            display_name=f"Solar Coronal Ray Spicule {s_ri+1}",
            role="solar-spicule"
        ))

    # 24 Accretion Infall Stream Pearls along 3 Logarithmic Spiral Arms
    for arm in range(3):
        arm_phase = arm * (2.0 * math.pi / 3.0)
        for s in range(8):
            theta = arm_phase + s * 0.42
            r_spiral = 6.2 + 1.4 * s
            sx = OX + r_spiral * math.cos(theta)
            sz = OZ + r_spiral * math.sin(theta)
            sy = OY + (s - 4) * 0.35
            T_s = mat4_translate(sx, sy, sz)
            if s % 2 == 0:
                mat_s = "celestial.crystal.cyan"
                shape_s = sdf_leaf(0, [0.42, 0.42, 0.42], p0=0.42)
            else:
                mat_s = "celestial.gold.stellar"
                shape_s = sdf_convex(make_polyhedron_planes("octahedron", 0.46))

            objects.append(make_sdf_object(
                f"celestial-accretion-arm{arm+1}-pearl-{s+1:02d}",
                mat_s,
                [sx, sy, sz],
                T_s,
                shape_s,
                extent=[0.7, 0.7, 0.7],
                display_name=f"Accretion Spiral Nodule Arm {arm+1} #{s+1}",
                role="accretion-pearl"
            ))

    # =========================================================================
    # 6. TOWERING RELATIVISTIC BIPOLAR JET SPIRE & STATOR CHOKES
    # =========================================================================
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
        for sti, st_ang in enumerate([0.0, math.pi/2.0, math.pi, 3.0*math.pi/2.0]):
            st_x = OX + cr * math.cos(st_ang)
            st_z = OZ + cr * math.sin(st_ang)
            T_st = mat4_translate(st_x, cy, st_z)
            objects.append(make_sdf_object(
                f"celestial-choke-{int(cy)}-stator-{sti+1}",
                "celestial.bronze.armillary",
                [st_x, cy, st_z],
                T_st,
                sdf_leaf(2, [0.45, 0.75, 0.45], p0=0.12),
                extent=[0.8, 1.0, 0.8],
                display_name=f"Magnetic Stator Clamp Y={int(cy)} #{sti+1}",
                role="stator-clamp"
            ))

    # Zenith Collimation Pylons
    for pi, (pdx, pdz) in enumerate([(6.5, 0.0), (-6.5, 0.0), (0.0, 6.5), (0.0, -6.5)]):
        py_center = 88.0
        T_zpylon = mat4_translate(OX + pdx, py_center, OZ + pdz)
        pylon_col = sdf_leaf(4, [0.45, 14.0, 0.0])
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

    # Zenith Apex Spire Needle + Crown Diamond at Y = 126.0m!
    T_apex = mat4_translate(OX, 126.0, OZ)
    apex_cone = sdf_leaf(5, [1.8, 8.0, 0.0])
    apex_octa = sdf_convex(make_polyhedron_planes("octahedron", 4.0))
    apex_shape = sdf_binary(1, apex_cone, apex_octa, t=0.4)
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

    T_crown = mat4_translate(OX, 135.5, OZ)
    crown_shape = sdf_convex(make_polyhedron_planes("octahedron", 1.85))
    objects.append(make_sdf_object(
        "celestial-zenith-crown-diamond",
        "celestial.ruby.nodal",
        [OX, 135.5, OZ],
        T_crown,
        crown_shape,
        extent=[2.2, 2.2, 2.2],
        display_name="Apex Starlight Crown Diamond",
        role="apex-crown"
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

    # Nadir Gravitational Crucible Anchor (Y = 8.5m)
    T_anchor = mat4_translate(OX, 8.5, OZ)
    anc_sphere = sdf_leaf(0, [3.2, 3.2, 3.2], p0=3.2)
    anc_cone = sdf_leaf(5, [2.2, 3.5, 0.0])
    anc_crucible = sdf_binary(4, anc_sphere, anc_cone)
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
    # 7. 8 RESONANT GRAVITATIONAL PYLONS & FLOATING HALO RINGS
    # =========================================================================
    for i in range(8):
        angle = i * (2.0 * math.pi / 8.0)
        px = OX + 28.0 * math.cos(angle)
        pz = OZ + 28.0 * math.sin(angle)
        py = OY
        R_yaw = mat4_rotate_y(-angle)
        T_monolith = mat4_mul(mat4_translate(px, py, pz), R_yaw)

        pylon_base = sdf_leaf(2, [4.2, 1.1, 1.1], p0=0.25)
        pylon_gem = sdf_convex(make_polyhedron_planes("octahedron", 1.4))
        pylon_shape = sdf_binary(5, pylon_base, pylon_gem, t=0.45)

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

        halo_torus = sdf_leaf(6, [1.8, 0.24, 0.0])
        halo_slot = sdf_leaf(1, [2.2, 0.15, 0.15])
        halo_shape = sdf_binary(4, halo_torus, halo_slot)
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

        T_pneedle = mat4_translate(px, py + 4.9, pz)
        objects.append(make_sdf_object(
            f"celestial-pylon-needle-{i+1:02d}",
            "celestial.gold.electrum",
            [px, py + 4.9, pz],
            T_pneedle,
            sdf_leaf(5, [0.35, 0.85, 0.0]),
            extent=[0.6, 1.1, 0.6],
            display_name=f"Pylon Harmonic Focus Needle {i+1}",
            role="pylon-needle"
        ))

    # =========================================================================
    # 8. THE FIRMAMENT DAIS & HIGH CELESTIAL PROMENADE (Y = 22.0)
    # =========================================================================
    DY = 22.0
    cardinal_decks = [
        ("north", [OX, DY, OZ + 18.0], [3.2, 0.35, 7.5], "z"),
        ("south", [OX, DY, OZ - 18.0], [3.2, 0.35, 7.5], "z"),
        ("east",  [OX + 18.0, DY, OZ], [7.5, 0.35, 3.2], "x"),
        ("west",  [OX - 18.0, DY, OZ], [7.5, 0.35, 3.2], "x"),
    ]
    for bname, bcenter, bdims, main_axis in cardinal_decks:
        Tb = mat4_translate(*bcenter)
        deck_shape = sdf_leaf(2, bdims, p0=0.12)
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

        half_w = 1.45
        half_l = 6.8
        for side_sign in [-1.0, 1.0]:
            if main_axis == "z":
                rx_rail = bcenter[0] + side_sign * half_w
                rz_rail = bcenter[2]
                T_rail = mat4_translate(rx_rail, DY + 0.95, rz_rail)
                rail_shape = sdf_leaf(2, [0.12, 0.12, half_l], p0=0.04)
            else:
                rx_rail = bcenter[0]
                rz_rail = bcenter[2] + side_sign * half_w
                T_rail = mat4_translate(rx_rail, DY + 0.95, rz_rail)
                rail_shape = sdf_leaf(2, [half_l, 0.12, 0.12], p0=0.04)

            objects.append(make_sdf_object(
                f"skybridge-handrail-{bname}-side{int(side_sign+2)}",
                "celestial.gold.electrum",
                [rx_rail, DY + 0.95, rz_rail],
                T_rail,
                rail_shape,
                extent=[half_l + 0.5, 0.5, half_l + 0.5],
                display_name=f"Balustrade Railing — {bname.capitalize()} Side",
                role="skybridge-railing"
            ))

            for post_i in [-4.5, 0.0, 4.5]:
                if main_axis == "z":
                    px_post = rx_rail
                    pz_post = rz_rail + post_i
                else:
                    px_post = rx_rail + post_i
                    pz_post = rz_rail
                T_post = mat4_translate(px_post, DY + 0.50, pz_post)
                objects.append(make_sdf_object(
                    f"skybridge-baluster-{bname}-side{int(side_sign+2)}-p{int(post_i+5)}",
                    "celestial.bronze.armillary",
                    [px_post, DY + 0.50, pz_post],
                    T_post,
                    sdf_leaf(4, [0.10, 0.45, 0.0]),
                    extent=[0.3, 0.6, 0.3],
                    display_name=f"Baluster Post — {bname.capitalize()}",
                    role="baluster-post"
                ))

        p_offset = 6.2
        if bname == "north": px, pz = OX, OZ + 18.0 + p_offset
        elif bname == "south": px, pz = OX, OZ - 18.0 - p_offset
        elif bname == "east": px, pz = OX + 18.0 + p_offset, OZ
        elif bname == "west": px, pz = OX - 18.0 - p_offset, OZ

        Tp_base = mat4_translate(px, DY + 0.15, pz)
        objects.append(make_sdf_object(
            f"skybridge-plinth-{bname}",
            "celestial.marble.astral",
            [px, DY + 0.15, pz],
            Tp_base,
            sdf_leaf(4, [1.35, 0.15, 0.0]),
            extent=[1.6, 0.4, 1.6],
            display_name=f"Observation Dais Stepped Plinth — {bname.capitalize()}",
            role="altar-plinth"
        ))

        Tp = mat4_translate(px, DY + 0.85, pz)
        pedestal_shape = sdf_leaf(4, [0.85, 0.65, 0.0])
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

        Tp_dial = mat4_translate(px, DY + 1.55, pz)
        dial_shape = sdf_leaf(4, [0.92, 0.06, 0.0])
        objects.append(make_sdf_object(
            f"skybridge-dial-{bname}",
            "celestial.bronze.armillary",
            [px, DY + 1.55, pz],
            Tp_dial,
            dial_shape,
            extent=[1.1, 0.3, 1.1],
            display_name=f"Azimuth Calibrated Dial Plate — {bname.capitalize()}",
            role="azimuth-dial"
        ))

        T_sex = mat4_mul(mat4_translate(px, DY + 2.15, pz), mat4_rotate_y(math.pi / 4.0))
        sex_torus = sdf_leaf(6, [0.85, 0.14, 0.0])
        sex_box = sdf_leaf(1, [1.0, 1.0, 0.5], offset=[0.5, 0.5, 0.0])
        sex_shape = sdf_binary(3, sex_torus, sex_box)
        objects.append(make_sdf_object(
            f"skybridge-sextant-{bname}",
            "celestial.bronze.armillary",
            [px, DY + 2.15, pz],
            T_sex,
            sex_shape,
            extent=[1.2, 1.2, 0.8],
            display_name=f"Armillary Observation Quadrant Sextant — {bname.capitalize()}",
            role="observation-sextant"
        ))

        T_diop = mat4_mul(mat4_translate(px, DY + 2.15, pz), mat4_mul(mat4_rotate_y(math.pi / 4.0), mat4_rotate_x(math.radians(35.0))))
        diop_shape = sdf_leaf(4, [0.06, 0.95, 0.0])
        objects.append(make_sdf_object(
            f"skybridge-diopter-{bname}",
            "celestial.gold.electrum",
            [px, DY + 2.15, pz],
            T_diop,
            diop_shape,
            extent=[0.4, 1.2, 0.4],
            display_name=f"Astronomical Diopter Sighting Needle — {bname.capitalize()}",
            role="sighting-diopter"
        ))

    # Intercardinal Connecting Skybridges
    intercardinals = [
        ("northeast", [OX + 12.8, DY, OZ + 12.8], math.radians(-45.0)),
        ("northwest", [OX - 12.8, DY, OZ + 12.8], math.radians(45.0)),
        ("southeast", [OX + 12.8, DY, OZ - 12.8], math.radians(45.0)),
        ("southwest", [OX - 12.8, DY, OZ - 12.8], math.radians(-45.0)),
    ]
    for iname, icenter, irot in intercardinals:
        T_ic = mat4_mul(mat4_translate(*icenter), mat4_rotate_y(irot))
        bridge_shape = sdf_leaf(2, [5.5, 0.32, 2.4], p0=0.10)
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

    # 8 Astral Brazier Beacons with Urn Calyx, Gem Cages & Fire Crystals
    octa_flame = sdf_convex(make_polyhedron_planes("octahedron", 0.55))
    for bi in range(8):
        b_angle = bi * (2.0 * math.pi / 8.0) + (math.pi / 8.0)
        bx = OX + 21.0 * math.cos(b_angle)
        bz = OZ + 21.0 * math.sin(b_angle)
        by = DY + 0.9

        T_bplinth = mat4_translate(bx, by - 0.55, bz)
        objects.append(make_sdf_object(
            f"skybridge-brazier-plinth-{bi+1:02d}",
            "celestial.marble.astral",
            [bx, by - 0.55, bz],
            T_bplinth,
            sdf_leaf(4, [0.68, 0.15, 0.0]),
            extent=[0.9, 0.3, 0.9],
            display_name=f"Brazier Stepped Plinth #{bi+1}",
            role="brazier-plinth"
        ))

        T_br = mat4_translate(bx, by, bz)
        brazier_base = sdf_leaf(4, [0.48, 0.55, 0.0])
        objects.append(make_sdf_object(
            f"skybridge-brazier-{bi+1:02d}",
            "celestial.gold.electrum",
            [bx, by, bz],
            T_br,
            brazier_base,
            extent=[0.8, 0.9, 0.8],
            display_name=f"Astral Flame Brazier Beacon {bi+1}",
            role="brazier-beacon"
        ))

        T_bowl = mat4_translate(bx, by + 0.65, bz)
        outer_cup = sdf_leaf(5, [0.75, 0.45, 0.0])
        inner_cup = sdf_leaf(5, [0.62, 0.48, 0.0])
        calyx_bowl = sdf_binary(4, outer_cup, inner_cup)
        objects.append(make_sdf_object(
            f"skybridge-brazier-bowl-{bi+1:02d}",
            "celestial.bronze.armillary",
            [bx, by + 0.65, bz],
            T_bowl,
            calyx_bowl,
            extent=[1.0, 0.8, 1.0],
            display_name=f"Brazier Crucible Calyx Bowl #{bi+1}",
            role="brazier-bowl"
        ))

        T_flame = mat4_translate(bx, by + 1.45, bz)
        mat_fl = "celestial.ruby.nodal" if (bi % 2 == 0) else "celestial.crystal.cyan"
        objects.append(make_sdf_object(
            f"skybridge-flame-{bi+1:02d}",
            mat_fl,
            [bx, by + 1.45, bz],
            T_flame,
            octa_flame,
            extent=[0.8, 0.8, 0.8],
            display_name=f"Astral Octahedral Fire Crystal {bi+1}",
            role="brazier-flame"
        ))

        T_bcage = mat4_mul(mat4_translate(bx, by + 1.45, bz), rx90)
        objects.append(make_sdf_object(
            f"skybridge-brazier-cage-{bi+1:02d}",
            "celestial.gold.stellar",
            [bx, by + 1.45, bz],
            T_bcage,
            sdf_leaf(6, [0.85, 0.08, 0.0]),
            extent=[1.1, 1.1, 0.4],
            display_name=f"Brazier Fire Containment Halo #{bi+1}",
            role="brazier-halo"
        ))

    # Firmament Dais Central Oculus Architrave Rim
    T_dais_ring = mat4_mul(mat4_translate(OX, DY - 0.2, OZ), rx90)
    dais_t1 = sdf_leaf(6, [12.5, 1.25, 0.0])
    dais_t2 = sdf_leaf(6, [11.0, 0.50, 0.0])
    dais_rim_shape = sdf_binary(2, dais_t1, dais_t2)
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

    # Ground-level Grand Celestial Oculus Architrave
    T_oculus_rim = mat4_mul(mat4_translate(OX, 0.2, OZ), rx90)
    oculus_t1 = sdf_leaf(6, [8.8, 0.90, 0.0])
    oculus_t2 = sdf_leaf(6, [7.8, 0.40, 0.0])
    oculus_rim_shape = sdf_binary(2, oculus_t1, oculus_t2)
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
    # 9. ASCENDING SPIRAL LEVITATION DISKS & BEAD HUBS (Gallery to Dais)
    # =========================================================================
    num_steps = 16
    for st in range(num_steps):
        s_prog = st / float(num_steps)
        s_ang = s_prog * (1.75 * math.pi)
        s_rad = 14.8 + 2.2 * math.sin(s_prog * math.pi)
        sx = OX + s_rad * math.cos(s_ang)
        sz = OZ + s_rad * math.sin(s_ang)
        sy = 1.5 + s_prog * (DY - 2.5)

        T_step = mat4_translate(sx, sy, sz)
        step_shape = sdf_leaf(2, [1.4, 0.15, 1.4], p0=0.08)
        objects.append(make_sdf_object(
            f"celestial-ascension-step-{st+1:02d}",
            "celestial.marble.astral",
            [sx, sy, sz],
            T_step,
            step_shape,
            extent=[1.8, 0.5, 1.8],
            display_name=f"Ascension Levitation Step #{st+1}",
            role="ascension-step"
        ))

        T_sring = mat4_mul(mat4_translate(sx, sy + 0.14, sz), rx90)
        objects.append(make_sdf_object(
            f"celestial-ascension-rune-{st+1:02d}",
            "celestial.gold.stellar",
            [sx, sy + 0.14, sz],
            T_sring,
            sdf_leaf(6, [0.95, 0.04, 0.0]),
            extent=[1.2, 1.2, 0.2],
            display_name=f"Ascension Step Inlaid Gold Rune #{st+1}",
            role="step-rune"
        ))

        T_sbead = mat4_translate(sx, sy - 0.35, sz)
        objects.append(make_sdf_object(
            f"celestial-ascension-bead-{st+1:02d}",
            "celestial.crystal.cyan",
            [sx, sy - 0.35, sz],
            T_sbead,
            sdf_convex(make_polyhedron_planes("octahedron", 0.28)),
            extent=[0.5, 0.5, 0.5],
            display_name=f"Levitation Antigravity Core Bead #{st+1}",
            role="step-bead"
        ))

    # 4 Grand Vertical Flux Colonnades
    col_coords = [
        (OX + 14.0, OZ + 14.0),
        (OX - 14.0, OZ + 14.0),
        (OX + 14.0, OZ - 14.0),
        (OX - 14.0, OZ - 14.0),
    ]
    for ci, (cx, cz) in enumerate(col_coords):
        cy = DY / 2.0
        T_col = mat4_translate(cx, cy, cz)
        col_shape = sdf_leaf(4, [0.65, cy - 0.8, 0.0])
        objects.append(make_sdf_object(
            f"celestial-flux-colonnade-{ci+1}",
            "celestial.bronze.armillary",
            [cx, cy, cz],
            T_col,
            col_shape,
            extent=[1.2, cy + 0.5, 1.2],
            display_name=f"Celestial Flux Colonnade Shaft {ci+1}",
            role="flux-colonnade"
        ))
        T_cbase = mat4_translate(cx, 0.5, cz)
        objects.append(make_sdf_object(
            f"celestial-col-base-{ci+1}",
            "celestial.gold.stellar",
            [cx, 0.5, cz],
            T_cbase,
            sdf_leaf(2, [1.2, 0.45, 1.2], p0=0.15),
            extent=[1.5, 0.7, 1.5],
            display_name=f"Colonnade Molded Base Plinth {ci+1}",
            role="col-base"
        ))
        T_ccap = mat4_translate(cx, DY - 0.5, cz)
        objects.append(make_sdf_object(
            f"celestial-col-capital-{ci+1}",
            "celestial.gold.stellar",
            [cx, DY - 0.5, cz],
            T_ccap,
            sdf_leaf(2, [1.15, 0.45, 1.15], p0=0.15),
            extent=[1.5, 0.7, 1.5],
            display_name=f"Colonnade Cornice Capital {ci+1}",
            role="col-capital"
        ))

    return objects

def create_celestial_spatial_root():
    OX, OY, OZ = 0.0, 52.0, 87.5

    # Multi-Harmonic Relativistic Quasar Jet rho(p, t):
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

    # Cosmic Chromatic Spectrum chi(p, t):
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

    # Multi-Lobe Relativistic Jet + Equatorial Flare alpha(p, omega, t):
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
    print("Generating Masterwork Sky Celestial Apparatus with Ultra-Detailed Ring Embellishments...")
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
    gallery_zone["objects"] = total_objs
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
        "cameraPos": [0.0, 24.5, 65.0],
        "cameraFront": [0.0, 0.45, 0.89],
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
        "objects": sky_objects,
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
