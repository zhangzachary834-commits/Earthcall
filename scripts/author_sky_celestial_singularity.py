#!/usr/bin/env python3
"""
author_sky_celestial_singularity.py

Authors the Masterwork Celestial Radiance Engine & Astral Orrery in the sky
above the 28-Room Radiance Gallery.
"""

import json, math, os

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

def make_sdf_object(obj_id, material_id, center, transform, prim, dims, extent, display_name=None, role=None):
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
        "field": {
            "op": 0,
            "prim": prim,
            "dims": dims,
            "offset": [0, 0, 0],
            "p0": dims[0] if prim == 0 else (dims[2] if prim == 2 else 0),
            "p1": 0,
            "t": 0.5
        },
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
    # 1. CORE ASTRAL SINGULARITY & ERGOSPHERE
    # =========================================================================
    T_core = mat4_translate(OX, OY, OZ)
    objects.append(make_sdf_object(
        "celestial-nucleus-core",
        "celestial.obsidian.core",
        [OX, OY, OZ],
        T_core,
        prim=0, # Sphere
        dims=[4.5, 4.5, 4.5],
        extent=[5.5, 5.5, 5.5],
        display_name="Astral Singularity Nucleus",
        role="singularity-core"
    ))

    # Inner Ergosurface Torus (Frame-dragging boundary)
    T_ergo = mat4_mul(mat4_translate(OX, OY, OZ), mat4_mul(mat4_rotate_x(0.21), rx90))
    objects.append(make_sdf_object(
        "celestial-nucleus-ergosphere",
        "celestial.amethyst.resonance",
        [OX, OY, OZ],
        T_ergo,
        prim=6, # Torus
        dims=[5.8, 0.45, 0.0],
        extent=[6.8, 6.8, 1.8],
        display_name="Ergosurface Frame-Dragging Torus",
        role="ergosphere"
    ))

    # Coronal Stellar Focus Shell
    objects.append(make_sdf_object(
        "celestial-nucleus-corona",
        "celestial.gold.electrum",
        [OX, OY, OZ],
        T_core,
        prim=0, # Sphere
        dims=[6.4, 6.4, 6.4],
        extent=[7.5, 7.5, 7.5],
        display_name="Radiant Coronal Focus Sphere",
        role="singularity-corona"
    ))

    # Quadrupole Quantum Flux Nodes orbiting the event horizon
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
            prim=1, # Box
            dims=[0.65, 0.65, 0.65],
            extent=[1.1, 1.1, 1.1],
            display_name=f"Quantum Flux Quadrupole Node {qname.capitalize()}",
            role="quantum-node"
        ))

    # =========================================================================
    # 2. THE GRAND KEPLERIAN ARMILLARY ORRERY (7 Concentric Gyro Rings)
    # =========================================================================
    # Ring 1: Equatorial Accretion Ring (R = 20.0)
    T_ring1 = mat4_mul(mat4_translate(OX, OY, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-ring-01-equatorial",
        "celestial.gold.stellar",
        [OX, OY, OZ],
        T_ring1,
        prim=6, # Torus
        dims=[20.0, 0.85, 0.0],
        extent=[22.0, 22.0, 2.5],
        display_name="Armillary Ring I — Celestial Equator",
        role="armillary-ring"
    ))

    # Ring 2: Ecliptic Zodiac Ring (R = 26.0, tilted at 23.4 degrees)
    T_rot_ecliptic = mat4_mul(mat4_rotate_z(math.radians(23.4)), rx90)
    T_ring2 = mat4_mul(mat4_translate(OX, OY, OZ), T_rot_ecliptic)
    objects.append(make_sdf_object(
        "celestial-ring-02-ecliptic",
        "celestial.bronze.armillary",
        [OX, OY, OZ],
        T_ring2,
        prim=6, # Torus
        dims=[26.0, 0.75, 0.0],
        extent=[28.5, 28.5, 2.5],
        display_name="Armillary Ring II — Ecliptic Chrono-Path",
        role="armillary-ring"
    ))

    # Ring 3: Prime Celestial Meridian (R = 32.0, vertical Y-Z)
    T_ring3 = mat4_mul(mat4_translate(OX, OY, OZ), ry90)
    objects.append(make_sdf_object(
        "celestial-ring-03-meridian",
        "celestial.gold.electrum",
        [OX, OY, OZ],
        T_ring3,
        prim=6, # Torus
        dims=[32.0, 0.70, 0.0],
        extent=[34.5, 34.5, 2.5],
        display_name="Armillary Ring III — Celestial Prime Meridian",
        role="armillary-ring"
    ))

    # Ring 4: Equinoctial Colure (R = 32.0, vertical X-Y)
    T_ring4 = mat4_translate(OX, OY, OZ)
    objects.append(make_sdf_object(
        "celestial-ring-04-colure",
        "celestial.silver.quicksilver",
        [OX, OY, OZ],
        T_ring4,
        prim=6, # Torus
        dims=[32.0, 0.65, 0.0],
        extent=[34.5, 34.5, 2.5],
        display_name="Armillary Ring IV — Equinoctial Colure",
        role="armillary-ring"
    ))

    # Ring 5 & 6: Tropics of Cancer & Capricorn (Parallel circles at Y = OY +/- 10.5)
    r_tropic = math.sqrt(max(1.0, 32.0**2 - 10.5**2)) # ~30.23
    for t_sign, t_name in [(1.0, "cancer"), (-1.0, "capricorn")]:
        ty = OY + t_sign * 10.5
        T_tropic = mat4_mul(mat4_translate(OX, ty, OZ), rx90)
        objects.append(make_sdf_object(
            f"celestial-ring-tropic-{t_name}",
            "celestial.bronze.armillary",
            [OX, ty, OZ],
            T_tropic,
            prim=6, # Torus
            dims=[r_tropic, 0.55, 0.0],
            extent=[r_tropic + 2.0, r_tropic + 2.0, 2.0],
            display_name=f"Armillary Tropic Ring — {t_name.capitalize()}",
            role="armillary-ring"
        ))

    # Ring 7: Precessional Polar Gyro (R = 36.0, 45 degree tilt)
    T_rot_prec = mat4_mul(mat4_rotate_x(math.radians(45.0)), mat4_rotate_y(math.radians(45.0)))
    T_ring7 = mat4_mul(mat4_translate(OX, OY, OZ), T_rot_prec)
    objects.append(make_sdf_object(
        "celestial-ring-07-precessional",
        "celestial.crystal.sapphire",
        [OX, OY, OZ],
        T_ring7,
        prim=6, # Torus
        dims=[36.0, 0.70, 0.0],
        extent=[39.0, 39.0, 2.5],
        display_name="Armillary Ring VII — Precessional Polar Gyro",
        role="armillary-ring"
    ))

    # Ring 8: Great Outer Zodiac Framework (R = 42.0)
    T_ring8 = mat4_mul(mat4_translate(OX, OY, OZ), rx90)
    objects.append(make_sdf_object(
        "celestial-ring-08-zodiac",
        "celestial.marble.astral",
        [OX, OY, OZ],
        T_ring8,
        prim=6, # Torus
        dims=[42.0, 1.05, 0.0],
        extent=[45.5, 45.5, 3.2],
        display_name="Armillary Ring VIII — Great Outer Zodiac Framework",
        role="armillary-ring"
    ))

    # =========================================================================
    # 3. 12 ZODIAC NODES & 12 ECLIPTIC CHRONO-GEAR TEETH
    # =========================================================================
    for i in range(12):
        angle = i * (2.0 * math.pi / 12.0)
        # Outer Zodiac Node on Ring 8 (horizontal R=42.0)
        zx = OX + 42.0 * math.cos(angle)
        zz = OZ + 42.0 * math.sin(angle)
        zy = OY
        T_znode = mat4_translate(zx, zy, zz)
        mat_z = "celestial.ruby.nodal" if (i % 3 == 0) else "celestial.crystal.sapphire"
        objects.append(make_sdf_object(
            f"celestial-zodiac-node-{i+1:02d}",
            mat_z,
            [zx, zy, zz],
            T_znode,
            prim=1, # Box
            dims=[1.1, 1.1, 1.1],
            extent=[1.6, 1.6, 1.6],
            display_name=f"Zodiac Constellation Nodal Prism {i+1}",
            role="zodiac-node"
        ))

        # Ecliptic Chrono-Gear Marker on Ring 2 (tilted R=26.0)
        # Point on tilted circle:
        # circle in local space: [26*cos, 0, 26*sin]
        # rotate around X by 90 deg -> [26*cos, 26*sin, 0]
        # rotate around Z by 23.4 deg ->
        ecl_x = 26.0 * math.cos(angle) * math.cos(math.radians(23.4)) - 26.0 * math.sin(angle) * math.sin(math.radians(23.4))
        ecl_y = 26.0 * math.cos(angle) * math.sin(math.radians(23.4)) + 26.0 * math.sin(angle) * math.cos(math.radians(23.4))
        # Wait, local plane was x-z, rotated by rx90 and rz23.4.
        # Let's directly calculate from matrix multiplication:
        local_v = [26.0 * math.cos(angle), 26.0 * math.sin(angle), 0.0] # rx90 puts z into y
        rad23 = math.radians(23.4)
        c23, s23 = math.cos(rad23), math.sin(rad23)
        ex = OX + (local_v[0] * c23 - local_v[1] * s23)
        ey = OY + (local_v[0] * s23 + local_v[1] * c23)
        ez = OZ
        T_eg = mat4_translate(ex, ey, ez)
        objects.append(make_sdf_object(
            f"celestial-ecliptic-tooth-{i+1:02d}",
            "celestial.bronze.armillary",
            [ex, ey, ez],
            T_eg,
            prim=4, # Cylinder
            dims=[0.55, 0.45, 0.0],
            extent=[1.0, 1.0, 1.0],
            display_name=f"Ecliptic Chrono-Gear Tooth {i+1}",
            role="ecliptic-tooth"
        ))

    # =========================================================================
    # 4. CLASSICAL PLANETARY EPICYCLE SATELLITES & SECONDARY RINGS
    # =========================================================================
    planets = [
        # (name, mat, orbit_r, orbit_deg, size, has_ring)
        ("mercury", "celestial.silver.quicksilver", 13.5,  30.0, 0.85, False),
        ("venus",   "celestial.gold.electrum",     16.5, 110.0, 1.10, False),
        ("sol",     "celestial.gold.stellar",      21.5, 195.0, 1.75, True),
        ("mars",    "celestial.ruby.nodal",        25.0, 275.0, 0.95, False),
        ("jupiter", "celestial.crystal.sapphire",  30.5,  75.0, 2.20, True),
        ("saturn",  "celestial.bronze.armillary",   35.5, 335.0, 1.80, True),
    ]
    for pname, pmat, porbit, pdeg, psize, phsring in planets:
        prad = math.radians(pdeg)
        px = OX + porbit * math.cos(prad)
        pz = OZ + porbit * math.sin(prad)
        py = OY + 2.5 * math.sin(prad * 2.0) # slight inclination wave
        T_p = mat4_translate(px, py, pz)
        objects.append(make_sdf_object(
            f"celestial-planet-{pname}",
            pmat,
            [px, py, pz],
            T_p,
            prim=0, # Sphere
            dims=[psize, psize, psize],
            extent=[psize + 0.8, psize + 0.8, psize + 0.8],
            display_name=f"Epicyclic Satellite — {pname.capitalize()}",
            role="celestial-planet"
        ))
        if phsring:
            T_pring = mat4_mul(mat4_translate(px, py, pz), mat4_mul(mat4_rotate_x(0.35), rx90))
            objects.append(make_sdf_object(
                f"celestial-planet-ring-{pname}",
                "celestial.gold.electrum",
                [px, py, pz],
                T_pring,
                prim=6, # Torus
                dims=[psize * 2.1, 0.22, 0.0],
                extent=[psize * 2.1 + 0.8, psize * 2.1 + 0.8, 0.8],
                display_name=f"Satellite Halo — {pname.capitalize()} Ring",
                role="planet-ring"
            ))

    # Accretion Infall Stream Pearls (Logarithmic Spiral of Astral Nodules)
    for s in range(16):
        theta = s * 0.45
        r_spiral = 6.0 + 1.2 * theta
        sx = OX + r_spiral * math.cos(theta)
        sz = OZ + r_spiral * math.sin(theta)
        sy = OY + (s - 8) * 0.25
        T_s = mat4_translate(sx, sy, sz)
        mat_s = "celestial.crystal.cyan" if (s % 2 == 0) else "celestial.gold.stellar"
        objects.append(make_sdf_object(
            f"celestial-accretion-pearl-{s+1:02d}",
            mat_s,
            [sx, sy, sz],
            T_s,
            prim=0, # Sphere
            dims=[0.48, 0.48, 0.48],
            extent=[0.8, 0.8, 0.8],
            display_name=f"Accretion Infall Flux Nodule {s+1}",
            role="accretion-pearl"
        ))

    # =========================================================================
    # 5. TOWERING RELATIVISTIC BIPOLAR JET SPIRE (Zenith & Nadir)
    # =========================================================================
    # Zenith Magnetic Collimation Choke Coils (Y = 62 to 118)
    zenith_chokes = [
        (62.0, 11.5, 0.65),
        (72.0,  9.5, 0.58),
        (82.0,  7.5, 0.52),
        (94.0,  5.8, 0.46),
        (106.0, 4.2, 0.40),
        (118.0, 2.8, 0.35)
    ]
    for cy, cr, cw in zenith_chokes:
        T_zc = mat4_mul(mat4_translate(OX, cy, OZ), rx90)
        objects.append(make_sdf_object(
            f"celestial-jet-choke-zenith-{int(cy)}",
            "celestial.gold.stellar",
            [OX, cy, OZ],
            T_zc,
            prim=6, # Torus
            dims=[cr, cw, 0.0],
            extent=[cr + 1.5, cr + 1.5, 1.5],
            display_name=f"Zenith Magnetic Collimation Choke Y={int(cy)}",
            role="jet-choke"
        ))

    # Zenith Collimation Pylons (4 vertical crystal needles flanking the jet at Y = 88)
    for pi, (pdx, pdz) in enumerate([(6.5, 0.0), (-6.5, 0.0), (0.0, 6.5), (0.0, -6.5)]):
        py_center = 88.0
        T_zpylon = mat4_translate(OX + pdx, py_center, OZ + pdz)
        objects.append(make_sdf_object(
            f"celestial-zenith-pylon-{pi+1}",
            "celestial.crystal.cyan",
            [OX + pdx, py_center, OZ + pdz],
            T_zpylon,
            prim=4, # Cylinder
            dims=[0.45, 14.0, 0.0],
            extent=[1.0, 15.0, 1.0],
            display_name=f"Zenith Jet Guide Needle {pi+1}",
            role="jet-guide"
        ))

    # Zenith Apex Celestial Pinnacle Needle (Y = 126.0)
    T_apex = mat4_translate(OX, 126.0, OZ)
    objects.append(make_sdf_object(
        "celestial-zenith-apex-spire",
        "celestial.crystal.cyan",
        [OX, 126.0, OZ],
        T_apex,
        prim=5, # Cone
        dims=[1.8, 8.0, 0.0],
        extent=[2.5, 9.0, 2.5],
        display_name="Zenith Astral Spire Apex Needle",
        role="zenith-apex"
    ))

    # Nadir Collimation Choke Coils (Y = 44 down to 18)
    nadir_chokes = [
        (44.0, 11.5, 0.65),
        (36.0,  9.5, 0.58),
        (28.0,  7.8, 0.52),
        (18.0,  6.2, 0.46)
    ]
    for cy, cr, cw in nadir_chokes:
        T_nc = mat4_mul(mat4_translate(OX, cy, OZ), rx90)
        objects.append(make_sdf_object(
            f"celestial-jet-choke-nadir-{int(cy)}",
            "celestial.bronze.armillary",
            [OX, cy, OZ],
            T_nc,
            prim=6, # Torus
            dims=[cr, cw, 0.0],
            extent=[cr + 1.5, cr + 1.5, 1.5],
            display_name=f"Nadir Gravitational Choke Y={int(cy)}",
            role="jet-choke"
        ))

    # Nadir Gravitational Anchor Counter-Weight (Y = 8.5)
    T_anchor = mat4_translate(OX, 8.5, OZ)
    objects.append(make_sdf_object(
        "celestial-nadir-gravity-anchor",
        "celestial.obsidian.core",
        [OX, 8.5, OZ],
        T_anchor,
        prim=0, # Sphere
        dims=[3.2, 3.2, 3.2],
        extent=[4.0, 4.0, 4.0],
        display_name="Nadir Gravitational Counter-Weight Core",
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
        objects.append(make_sdf_object(
            f"celestial-pylon-{i+1:02d}",
            "celestial.crystal.sapphire",
            [px, py, pz],
            T_monolith,
            prim=2, # RoundBox
            dims=[4.2, 1.1, 1.1],
            extent=[5.2, 1.8, 1.8],
            display_name=f"Resonant Gravitational Pylon {i+1}",
            role="gravitational-pylon"
        ))

        # Levitating Toroidal Halo above each Pylon
        T_halo = mat4_mul(mat4_translate(px, py + 3.8, pz), rx90)
        objects.append(make_sdf_object(
            f"celestial-pylon-halo-{i+1:02d}",
            "celestial.gold.stellar",
            [px, py + 3.8, pz],
            T_halo,
            prim=6, # Torus
            dims=[1.8, 0.22, 0.0],
            extent=[2.5, 2.5, 0.8],
            display_name=f"Pylon Levitation Halo Ring {i+1}",
            role="pylon-halo"
        ))

    # =========================================================================
    # 7. THE FIRMAMENT DAIS & HIGH CELESTIAL PROMENADE (Y = 22.0)
    # =========================================================================
    DY = 22.0
    # Cardinal Observation Decks (North, South, East, West)
    cardinal_decks = [
        ("north", [OX, DY, OZ + 18.0], [3.2, 0.35, 7.5]),
        ("south", [OX, DY, OZ - 18.0], [3.2, 0.35, 7.5]),
        ("east",  [OX + 18.0, DY, OZ], [7.5, 0.35, 3.2]),
        ("west",  [OX - 18.0, DY, OZ], [7.5, 0.35, 3.2]),
    ]
    for bname, bcenter, bdims in cardinal_decks:
        Tb = mat4_translate(*bcenter)
        objects.append(make_sdf_object(
            f"skybridge-deck-{bname}",
            "celestial.marble.astral",
            bcenter,
            Tb,
            prim=1, # Box
            dims=bdims,
            extent=[bdims[0] + 0.6, bdims[1] + 0.6, bdims[2] + 0.6],
            display_name=f"Firmament Dais Deck — {bname.capitalize()}",
            role="skybridge-deck"
        ))
        # Pedestal & Armillary Sextant on Cardinal Wings
        p_offset = 6.2
        if bname == "north": px, pz = OX, OZ + 18.0 + p_offset
        elif bname == "south": px, pz = OX, OZ - 18.0 - p_offset
        elif bname == "east": px, pz = OX + 18.0 + p_offset, OZ
        elif bname == "west": px, pz = OX - 18.0 - p_offset, OZ
        Tp = mat4_translate(px, DY + 0.85, pz)
        objects.append(make_sdf_object(
            f"skybridge-pedestal-{bname}",
            "celestial.gold.stellar",
            [px, DY + 0.85, pz],
            Tp,
            prim=4, # Cylinder
            dims=[0.9, 0.85, 0.0],
            extent=[1.4, 1.4, 1.4],
            display_name=f"Celestial Observation Dais — {bname.capitalize()}",
            role="skybridge-pedestal"
        ))
        # Sextant Astrolabe Ring mounted on pedestal
        T_sex = mat4_mul(mat4_translate(px, DY + 2.1, pz), mat4_rotate_y(math.pi / 4.0))
        objects.append(make_sdf_object(
            f"skybridge-sextant-{bname}",
            "celestial.bronze.armillary",
            [px, DY + 2.1, pz],
            T_sex,
            prim=6, # Torus
            dims=[0.75, 0.12, 0.0],
            extent=[1.2, 1.2, 0.5],
            display_name=f"Armillary Observation Sextant — {bname.capitalize()}",
            role="observation-sextant"
        ))

    # Intercardinal Connecting Skybridges (NE, NW, SE, SW) forming an Octagonal Observatory
    intercardinals = [
        ("northeast", [OX + 12.8, DY, OZ + 12.8], math.radians(-45.0)),
        ("northwest", [OX - 12.8, DY, OZ + 12.8], math.radians(45.0)),
        ("southeast", [OX + 12.8, DY, OZ - 12.8], math.radians(45.0)),
        ("southwest", [OX - 12.8, DY, OZ - 12.8], math.radians(-45.0)),
    ]
    for iname, icenter, irot in intercardinals:
        T_ic = mat4_mul(mat4_translate(*icenter), mat4_rotate_y(irot))
        objects.append(make_sdf_object(
            f"skybridge-octagonal-{iname}",
            "celestial.marble.astral",
            icenter,
            T_ic,
            prim=1, # Box
            dims=[5.5, 0.32, 2.4],
            extent=[6.5, 0.8, 3.2],
            display_name=f"Firmament Octagonal Cloister Bridge — {iname.capitalize()}",
            role="octagonal-bridge"
        ))

    # 8 Astral Brazier Beacons at the Octagonal Dais Corners
    for bi in range(8):
        b_angle = bi * (2.0 * math.pi / 8.0) + (math.pi / 8.0)
        bx = OX + 21.0 * math.cos(b_angle)
        bz = OZ + 21.0 * math.sin(b_angle)
        by = DY + 0.9
        T_br = mat4_translate(bx, by, bz)
        objects.append(make_sdf_object(
            f"skybridge-brazier-{bi+1:02d}",
            "celestial.gold.electrum",
            [bx, by, bz],
            T_br,
            prim=4, # Cylinder
            dims=[0.45, 0.9, 0.0],
            extent=[0.8, 1.4, 0.8],
            display_name=f"Astral Flame Brazier Beacon {bi+1}",
            role="brazier-beacon"
        ))
        # Flaming Crystal on each brazier
        T_flame = mat4_translate(bx, by + 1.25, bz)
        mat_fl = "celestial.ruby.nodal" if (bi % 2 == 0) else "celestial.crystal.cyan"
        objects.append(make_sdf_object(
            f"skybridge-flame-{bi+1:02d}",
            mat_fl,
            [bx, by + 1.25, bz],
            T_flame,
            prim=5, # Cone
            dims=[0.35, 0.65, 0.0],
            extent=[0.6, 0.9, 0.6],
            display_name=f"Astral Ether Fire Prism {bi+1}",
            role="brazier-flame"
        ))

    # Firmament Dais Central Oculus Architrave Rim
    T_dais_ring = mat4_mul(mat4_translate(OX, DY - 0.2, OZ), rx90)
    objects.append(make_sdf_object(
        "skybridge-central-aperture-ring",
        "celestial.bronze.armillary",
        [OX, DY - 0.2, OZ],
        T_dais_ring,
        prim=6, # Torus
        dims=[12.5, 1.25, 0.0],
        extent=[15.0, 15.0, 2.5],
        display_name="Firmament Dais Central Oculus Rim",
        role="skybridge-rim"
    ))

    # Ground-level Grand Celestial Oculus Architrave (Roof of 28-Room Central Gallery)
    T_oculus_rim = mat4_mul(mat4_translate(OX, 0.2, OZ), rx90)
    objects.append(make_sdf_object(
        "gallery-sky-oculus-architrave",
        "celestial.gold.stellar",
        [OX, 0.2, OZ],
        T_oculus_rim,
        prim=6, # Torus
        dims=[8.8, 0.90, 0.0],
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
        sy = 1.5 + s_prog * (DY - 2.5) # from Y = 1.5m to Y = 21.0m!
        T_step = mat4_translate(sx, sy, sz)
        objects.append(make_sdf_object(
            f"celestial-ascension-step-{st+1:02d}",
            "celestial.marble.astral",
            [sx, sy, sz],
            T_step,
            prim=4, # Cylinder
            dims=[1.4, 0.15, 0.0],
            extent=[1.8, 0.5, 1.8],
            display_name=f"Ascension Levitation Disk {st+1}",
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
        objects.append(make_sdf_object(
            f"celestial-flux-colonnade-{ci+1}",
            "celestial.bronze.armillary",
            [cx, cy, cz],
            T_col,
            prim=4, # Cylinder
            dims=[0.65, cy, 0.0],
            extent=[1.2, cy + 0.5, 1.2],
            display_name=f"Celestial Flux Colonnade Pillar {ci+1}",
            role="flux-colonnade"
        ))

    return objects

def create_celestial_spatial_root():
    OX, OY, OZ = 0.0, 52.0, 87.5

    # 1. Advanced Multi-Harmonic Relativistic Quasar Jet rho(p, t):
    # Anisotropic falloff with vertical beaming (0.0008*y^2 vs 0.004*(x^2+z^2))
    # Pulsated by dual-frequency celestial heartbeat: 1 + 0.28*sin(1.8*t) + 0.14*cos(3.2*t + 0.785)
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
                                            {"kind": 0, "scale": 1.8, "var": "t"} # Sin(1.8*t)
                                        ]
                                    },
                                    {
                                        "c": 0.14,
                                        "factors": {},
                                        "trans": [
                                            {"kind": 1, "scale": 3.2, "shift": 0.7854, "var": "t"} # Cos(3.2*t + pi/4)
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
    # Shifting from Solar Gold to Astral Sapphire, Rose, and Violet
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
    # base 0.60 + 4.50*omega.y^4 (super-collimated polar jet) + 1.80*omega.y^2 + 0.85*omega.x^2 (equatorial flare)
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
    print("Generating Masterwork Sky Celestial Apparatus...")
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
