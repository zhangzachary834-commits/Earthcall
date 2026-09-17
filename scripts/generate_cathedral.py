import json
import os

# ==============================================================================
# CATHEDRAL OF THE LIVING LOGOS — GENERATOR
# Fully aligned with Earthcall Save System Overhaul (PR #188)
#
# Rule 1: Form determines what the being is.
# Rule 2: Writes canonical self-describing 'shape' arm (kind + params) alongside
#         legacy fields (shapeKind, geometryType, shapeParams) for full integrity.
# Rule 3: Analytic shapes (Sphere, Torus) carry size in shape.params; transform
#         carries pose/rotation only (never multiply scale twice).
# Rule 4: Cube (ShapeKind 0) has unit shape params [0.5, 0.5, 0.5, 0.5] and
#         scale in transform matrix diagonal.
# Rule 5: 2D HUD uses Shape2D (12) and Text2D (13) with width2D/height2D,
#         screen coordinates x2D/y2D, zOrder2D, faceColors, textString, and label2D.
# ==============================================================================

def mat4(pos, scale=[1.0, 1.0, 1.0]):
    sx, sy, sz = scale
    px, py, pz = pos
    return [
        float(sx), 0.0, 0.0, 0.0,
        0.0, float(sy), 0.0, 0.0,
        0.0, 0.0, float(sz), 0.0,
        float(px), float(py), float(pz), 1.0
    ]

def make_box(obj_id, name, pos, scale, mat_id, color, extra_props=None):
    obj = {
        "objectID": obj_id,
        "shapeKind": 0,
        "geometryType": 0,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "shape": {
            "kind": 0,
            "params": {
                "r": 0.5, "ry": 0.5, "rz": 0.5, "halfH": 0.5,
                "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
                "ovoidAsym": 0.0, "fillet": 0.0,
                "width2D": 0.0, "height2D": 0.0
            }
        },
        "transform": mat4(pos, scale),
        "center": [float(pos[0]), float(pos[1]), float(pos[2])],
        "materialId": mat_id,
        "renderMode": 0,
        "faceColors": [color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": name}
        }
    }
    if extra_props:
        obj["authoredProperties"].update(extra_props)
    return obj

def make_sphere(obj_id, name, pos, radius, mat_id, color, extra_props=None):
    obj = {
        "objectID": obj_id,
        "shapeKind": 2,
        "geometryType": 2,
        "shapeParams": [float(radius), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "shape": {
            "kind": 2,
            "params": {
                "r": float(radius), "ry": 0.0, "rz": 0.0, "halfH": 0.0,
                "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
                "ovoidAsym": 0.0, "fillet": 0.0,
                "width2D": 0.0, "height2D": 0.0
            }
        },
        # Analytic sphere carries radius in shape.params.r. Transform is pose.
        "transform": mat4(pos),
        "center": [float(pos[0]), float(pos[1]), float(pos[2])],
        "materialId": mat_id,
        "renderMode": 0,
        "faceColors": [color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": name}
        }
    }
    if extra_props:
        obj["authoredProperties"].update(extra_props)
    return obj

def make_torus(obj_id, name, pos, majorR, minorR, mat_id, color, extra_props=None):
    obj = {
        "objectID": obj_id,
        "shapeKind": 8,
        "geometryType": 8,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, float(majorR), float(minorR), 0.0, 0.0, 0.0, 0.0, 0.0],
        "shape": {
            "kind": 8,
            "params": {
                "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
                "majorR": float(majorR), "minorR": float(minorR),
                "paraboloidA": 0.0, "ovoidAsym": 0.0, "fillet": 0.0,
                "width2D": 0.0, "height2D": 0.0
            }
        },
        # Analytic torus carries majorR/minorR in shape.params. Transform is pose.
        "transform": mat4(pos),
        "center": [float(pos[0]), float(pos[1]), float(pos[2])],
        "materialId": mat_id,
        "renderMode": 0,
        "faceColors": [color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": name}
        }
    }
    if extra_props:
        obj["authoredProperties"].update(extra_props)
    return obj

def make_label2d(obj_id, text, x, y, size=16.0, color=[0.9, 0.92, 0.95]):
    return {
        "objectID": obj_id,
        "shapeKind": 13,
        "geometryType": 13,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, float(size)],
        "shape": {
            "kind": 13,
            "params": {
                "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
                "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
                "ovoidAsym": 0.0, "fillet": 0.0,
                "width2D": 0.0, "height2D": float(size)
            }
        },
        "x2D": float(x),
        "y2D": float(y),
        "zOrder2D": 25,
        "textString": text,
        "transform": mat4([0, 0, 0]),
        "center": [0, 0, 0],
        "faceColors": [color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": text},
            "label2D": {"t": "string", "v": text}
        }
    }

def make_button2d(btn_id, label, x, y, w, h, bg_color):
    btn = {
        "objectID": btn_id,
        "shapeKind": 12,
        "geometryType": 12,
        "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, float(w), float(h)],
        "shape": {
            "kind": 12,
            "params": {
                "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
                "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
                "ovoidAsym": 0.0, "fillet": 0.0,
                "width2D": float(w), "height2D": float(h)
            }
        },
        "x2D": float(x),
        "y2D": float(y),
        "zOrder2D": 20,
        "transform": mat4([0, 0, 0]),
        "center": [0, 0, 0],
        "faceColors": [bg_color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": label},
            "label2D": {"t": "string", "v": label},
            "controlLabel": {"t": "string", "v": label},
            "isButton": {"t": "bool", "v": True}
        }
    }
    lbl = make_label2d(f"{btn_id}.text", label, x + 12, y + 8, 13.0, [1.0, 1.0, 1.0])
    return [btn, lbl]

objects = []

# ==============================================================================
# 1. FLOORS & SANCTUARY DAIS
# ==============================================================================
# Grand Acoustic Stone Floor (32m wide, 72m long)
objects.append(make_box(
    "cathedral.floor.main", "Chladni Acoustic Floor",
    [0.0, -0.3, -2.0], [32.0, 0.6, 72.0],
    "material.logos.floor", [0.08, 0.09, 0.12]
))

# Lapis Lazuli Processional Runner (6m wide, 68m long)
objects.append(make_box(
    "cathedral.floor.runner", "Sacred Lapis Processional Runner",
    [0.0, 0.03, -2.0], [6.0, 0.06, 68.0],
    "material.logos.sapphire", [0.12, 0.22, 0.55]
))

# Three-Tiered Sanctuary Chancel Dais leading to the High Altar
objects.append(make_box(
    "cathedral.dais.tier1", "Chancel Step I",
    [0.0, 0.15, -25.0], [18.0, 0.3, 16.0],
    "material.logos.floor", [0.12, 0.13, 0.16]
))
objects.append(make_box(
    "cathedral.dais.tier2", "Chancel Step II",
    [0.0, 0.45, -27.0], [14.0, 0.3, 13.0],
    "material.logos.floor", [0.15, 0.16, 0.20]
))
objects.append(make_box(
    "cathedral.dais.tier3", "Sanctuary High Dais",
    [0.0, 0.75, -29.0], [10.0, 0.3, 10.0],
    "material.logos.gold", [0.85, 0.72, 0.22]
))

# ==============================================================================
# 2. HIGH ALTAR & REREDOS
# ==============================================================================
objects.append(make_box(
    "altar.logos.mensa", "High Altar of the Spoken Word",
    [0.0, 1.6, -29.0], [5.2, 1.4, 2.2],
    "material.logos.altar", [0.05, 0.05, 0.08],
    extra_props={"isAltar": {"t": "bool", "v": True}}
))

objects.append(make_box(
    "altar.logos.reredos", "Sacred Sanctuary Reredos",
    [0.0, 8.0, -33.5], [8.0, 14.0, 0.8],
    "material.logos.gold", [0.92, 0.78, 0.25],
    extra_props={"isReredos": {"t": "bool", "v": True}}
))

# ==============================================================================
# 3. LIVING LEXEME GLYPHS ON THE ALTAR MENSA
# ==============================================================================
glyphs_info = [
    ("glyph.lexeme.logos", "Lexeme: [Logos]", [-1.8, 2.45, -29.0], [0.95, 0.82, 0.25], "material.logos.gold"),
    ("glyph.lexeme.pneuma", "Lexeme: [Pneuma]", [-0.9, 2.45, -29.0], [0.15, 0.45, 0.95], "material.logos.sapphire"),
    ("glyph.lexeme.lux", "Lexeme: [Lux]", [0.0, 2.45, -29.0], [1.0, 0.98, 0.85], "material.logos.core"),
    ("glyph.lexeme.harmonia", "Lexeme: [Harmonia]", [0.9, 2.45, -29.0], [0.98, 0.65, 0.15], "material.logos.amber"),
    ("glyph.lexeme.covenant", "Lexeme: [Covenant]", [1.8, 2.45, -29.0], [0.75, 0.25, 0.95], "material.logos.amethyst"),
]
for gid, gname, gpos, gcolor, gmat in glyphs_info:
    objects.append(make_box(
        gid, gname, gpos, [0.4, 0.2, 0.4], gmat, gcolor,
        extra_props={"isSpeechAct": {"t": "bool", "v": True}}
    ))

# ==============================================================================
# 4. COLONNADE OF THE SEVEN JOYS (Left X = -8m, Right X = +8m, 7 Bays along Z)
# ==============================================================================
joy_pillars = [
    ("Logos", 432.0, 24.0, [0.12, 0.35, 0.95], "material.logos.sapphire"),
    ("Agape", 528.0, 16.0, [0.95, 0.55, 0.65], "material.logos.core"),
    ("Sophia", 639.0, 8.0, [0.15, 0.75, 0.95], "material.logos.sapphire"),
    ("Poiesis", 741.0, 0.0, [0.15, 0.85, 0.45], "material.logos.emerald"),
    ("Harmonia", 852.0, -8.0, [0.98, 0.65, 0.15], "material.logos.amber"),
    ("Koinonia", 963.0, -16.0, [0.75, 0.25, 0.95], "material.logos.amethyst"),
    ("Sabbath", 1080.0, -24.0, [0.94, 0.95, 1.0], "material.logos.core"),
]

for i, (name, freq, z, crystal_col, crystal_mat) in enumerate(joy_pillars):
    idx = i + 1
    # --- Left Pillar (X = -8m) ---
    col_l = f"pillar.L{idx}"
    objects.append(make_box(f"{col_l}.base", f"Pillar Base L{idx}", [-8.0, 0.6, z], [2.2, 1.2, 2.2], "material.logos.floor", [0.18, 0.16, 0.15]))
    objects.append(make_box(f"{col_l}.shaft", f"Pillar Shaft L{idx} ({name})", [-8.0, 10.2, z], [1.4, 18.0, 1.4], "material.logos.alabaster", [0.92, 0.90, 0.86]))
    objects.append(make_box(f"{col_l}.capital", f"Pillar Capital L{idx}", [-8.0, 19.8, z], [2.2, 1.2, 2.2], "material.logos.gold", [0.95, 0.78, 0.22]))
    objects.append(make_sphere(f"{col_l}.crystal", f"Resonance Crystal L{idx} ({name} {freq:.0f}Hz)", [-8.0, 21.0, z], 0.65, crystal_mat, crystal_col,
        extra_props={"frequency": {"t": "float", "v": freq}, "light.intensity": {"t": "float", "v": 2.5}}
    ))

    # --- Right Pillar (X = +8m) ---
    col_r = f"pillar.R{idx}"
    objects.append(make_box(f"{col_r}.base", f"Pillar Base R{idx}", [8.0, 0.6, z], [2.2, 1.2, 2.2], "material.logos.floor", [0.18, 0.16, 0.15]))
    objects.append(make_box(f"{col_r}.shaft", f"Pillar Shaft R{idx} ({name})", [8.0, 10.2, z], [1.4, 18.0, 1.4], "material.logos.alabaster", [0.92, 0.90, 0.86]))
    objects.append(make_box(f"{col_r}.capital", f"Pillar Capital R{idx}", [8.0, 19.8, z], [2.2, 1.2, 2.2], "material.logos.gold", [0.95, 0.78, 0.22]))
    objects.append(make_sphere(f"{col_r}.crystal", f"Resonance Crystal R{idx} ({name} {freq:.0f}Hz)", [8.0, 21.0, z], 0.65, crystal_mat, crystal_col,
        extra_props={"frequency": {"t": "float", "v": freq}, "light.intensity": {"t": "float", "v": 2.5}}
    ))

    # --- Transverse Vault Arch Beam Spanning Nave (Span 16m) ---
    objects.append(make_box(
        f"cathedral.vault.arch{idx}", f"Vault Arch Bay {idx} ({name})",
        [0.0, 21.5, z], [17.6, 1.2, 1.2],
        "material.logos.arch", [0.85, 0.82, 0.78]
    ))
    objects.append(make_box(
        f"cathedral.vault.keystone{idx}", f"Arch Keystone {idx}",
        [0.0, 22.8, z], [2.2, 1.4, 1.8],
        "material.logos.gold", [0.95, 0.82, 0.28]
    ))

# ==============================================================================
# 5. LONGITUDINAL ROOF RIDGE SPINE (64m Long at y = 23.6m)
# ==============================================================================
objects.append(make_box(
    "cathedral.vault.spine", "Cathedral Roof Vault Spine",
    [0.0, 23.6, 0.0], [1.2, 1.2, 64.0],
    "material.logos.gold", [0.88, 0.75, 0.22]
))

# ==============================================================================
# 6. ENCLOSING OUTER WALLS
# ==============================================================================
objects.append(make_box(
    "cathedral.wall.left", "North Clerestory Wall",
    [-15.0, 12.0, -2.0], [1.2, 24.0, 72.0],
    "material.logos.floor", [0.15, 0.16, 0.18]
))
objects.append(make_box(
    "cathedral.wall.right", "South Clerestory Wall",
    [15.0, 12.0, -2.0], [1.2, 24.0, 72.0],
    "material.logos.floor", [0.15, 0.16, 0.18]
))
objects.append(make_box(
    "cathedral.wall.apse", "Sanctuary Apse Wall",
    [0.0, 13.0, -35.5], [30.0, 26.0, 1.2],
    "material.logos.floor", [0.14, 0.15, 0.17]
))
objects.append(make_box(
    "cathedral.wall.narthex", "Narthex Entrance Wall",
    [0.0, 12.0, 33.5], [30.0, 24.0, 1.2],
    "material.logos.floor", [0.14, 0.15, 0.17]
))

# ==============================================================================
# 7. GLOWING STAINED-GLASS LANCET WINDOWS (Emissive Light Sources)
# ==============================================================================
window_colors_left = [
    ("window.stained.L1", "Stained Glass: Sapphire Logos", [-14.3, 14.0, 18.0], [0.12, 0.35, 0.95]),
    ("window.stained.L2", "Stained Glass: Emerald Poiesis", [-14.3, 14.0, 6.0], [0.15, 0.85, 0.35]),
    ("window.stained.L3", "Stained Glass: Topaz Harmonia", [-14.3, 14.0, -6.0], [0.98, 0.65, 0.15]),
    ("window.stained.L4", "Stained Glass: Amethyst Koinonia", [-14.3, 14.0, -18.0], [0.75, 0.25, 0.95]),
]
window_colors_right = [
    ("window.stained.R1", "Stained Glass: Ruby Agape", [14.3, 14.0, 18.0], [0.95, 0.22, 0.35]),
    ("window.stained.R2", "Stained Glass: Cyan Sophia", [14.3, 14.0, 6.0], [0.15, 0.75, 0.95]),
    ("window.stained.R3", "Stained Glass: Solar Lux", [14.3, 14.0, -6.0], [1.0, 0.85, 0.25]),
    ("window.stained.R4", "Stained Glass: Pearl Sabbath", [14.3, 14.0, -18.0], [0.92, 0.95, 1.0]),
]
for wid, wname, wpos, wcol in window_colors_left + window_colors_right:
    objects.append(make_box(
        wid, wname, wpos, [0.2, 9.0, 3.2], "material.logos.core", wcol,
        extra_props={"light.intensity": {"t": "float", "v": 2.0}, "light.source": {"t": "bool", "v": True}}
    ))

# ==============================================================================
# 8. CENTRAL CROSSING: RESONATING HEART OF LOGOS & CELESTIAL RINGS
# ==============================================================================
objects.append(make_sphere(
    "logos.resonator.core", "Heart of Logos (432 Hz Radiant Core)",
    [0.0, 7.5, 0.0], 2.0, "material.logos.core", [1.0, 0.92, 0.65],
    extra_props={
        "isResonatorCore": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 6.0},
        "light.source": {"t": "bool", "v": True},
        "pulseRate": {"t": "float", "v": 1.618},
        "resonancePitch": {"t": "float", "v": 432.0}
    }
))
objects.append(make_torus(
    "logos.resonator.ring_alpha", "Celestial Orbital Ring Alpha",
    [0.0, 7.5, 0.0], 3.5, 0.15, "material.logos.gold", [0.98, 0.82, 0.25]
))
objects.append(make_torus(
    "logos.resonator.ring_beta", "Celestial Orbital Ring Beta",
    [0.0, 7.5, 0.0], 5.0, 0.12, "material.logos.sapphire", [0.15, 0.45, 0.95]
))
objects.append(make_torus(
    "logos.resonator.ring_gamma", "Celestial Orbital Ring Gamma",
    [0.0, 7.5, 0.0], 6.5, 0.10, "material.logos.amber", [0.98, 0.65, 0.15]
))

# ==============================================================================
# 9. STATE MANAGER BEING
# ==============================================================================
objects.append({
    "objectID": "state.logos",    "shapeKind": 12,    "geometryType": 12,    "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],    "shape": {        "kind": 12,        "params": {            "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,            "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,            "ovoidAsym": 0.0, "fillet": 0.0,            "width2D": 0.0, "height2D": 0.0        }    },    "transform": mat4([0, 0, 0]),    "center": [0, 0, 0],    "authoredProperties": {        "breathActive": {"t": "bool", "v": True},        "pulseRate": {"t": "float", "v": 1.618},        "resonanceFreq": {"t": "float", "v": 432.0},        "covenantCount": {"t": "float", "v": 1.0},        "luxActive": {"t": "bool", "v": True},        "season": {"t": "string", "v": "Genesis Dawn"}    }})

# ==============================================================================
# 10. CLEAN DOCKED 2D LITURGICAL HUD (Docked in top-left)
# ==============================================================================
hud_bg = {
    "objectID": "hud.logos.dock",
    "shapeKind": 12,
    "geometryType": 12,
    "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 360.0, 190.0],
    "shape": {
        "kind": 12,
        "params": {
            "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
            "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
            "ovoidAsym": 0.0, "fillet": 0.0,
            "width2D": 360.0, "height2D": 190.0
        }
    },
    "x2D": 20.0,
    "y2D": 20.0,
    "zOrder2D": 10,
    "transform": mat4([0, 0, 0]),
    "center": [0, 0, 0],
    "faceColors": [[0.05, 0.07, 0.12]] * 6,
    "authoredProperties": {"displayName": {"t": "string", "v": "Liturgical Console Dock"}}
}
objects.append(hud_bg)

objects.append(make_label2d("hud.logos.title", "CATHEDRAL OF THE LIVING LOGOS", 35, 35, 18.0, [1.0, 0.85, 0.35]))
objects.append(make_label2d("hud.logos.telemetry.freq", "FREQUENCY: 432.0 Hz (SACRED ROOT)", 35, 58, 14.0, [0.35, 0.85, 0.95]))
objects.append(make_label2d("hud.logos.telemetry.breath", "BREATH: RESPIRING (0.1 Hz PNEUMA WAVE)", 35, 78, 14.0, [0.45, 0.95, 0.65]))
objects.append(make_label2d("hud.logos.telemetry.season", "LITURGY: GENESIS DAWN", 35, 98, 14.0, [0.95, 0.75, 0.45]))

for item in make_button2d("hud.btn.pneuma", "BREATHE PNEUMA", 35, 125, 155, 32, [0.15, 0.45, 0.85]):
    objects.append(item)
for item in make_button2d("hud.btn.lux", "FIAT LUX", 205, 125, 155, 32, [0.92, 0.78, 0.25]):
    objects.append(item)
for item in make_button2d("hud.btn.chord", "SOUND CANON", 35, 165, 155, 32, [0.92, 0.55, 0.15]):
    objects.append(item)
for item in make_button2d("hud.btn.season", "CYCLE SEASON", 205, 165, 155, 32, [0.75, 0.25, 0.85]):
    objects.append(item)

# ==============================================================================
# MATERIALS PALETTE
# ==============================================================================
materials = [
    {"name": "material.logos.floor", "ambient": 0.2, "diffuse": 0.75, "specular": 0.6, "shininess": 40.0, "baseColor": [0.08, 0.09, 0.12], "roughness": 0.2, "metallic": 0.8},
    {"name": "material.logos.gold", "ambient": 0.35, "diffuse": 0.85, "specular": 0.9, "shininess": 64.0, "baseColor": [1.0, 0.82, 0.28], "emission": [0.3, 0.24, 0.08], "roughness": 0.15, "metallic": 0.95},
    {"name": "material.logos.sapphire", "ambient": 0.2, "diffuse": 0.75, "specular": 0.8, "shininess": 48.0, "baseColor": [0.12, 0.35, 0.95], "emission": [0.05, 0.15, 0.4], "roughness": 0.2, "metallic": 0.6},
    {"name": "material.logos.alabaster", "ambient": 0.4, "diffuse": 0.9, "specular": 0.6, "shininess": 32.0, "baseColor": [0.94, 0.92, 0.88], "emission": [0.05, 0.05, 0.05], "roughness": 0.25, "metallic": 0.1},
    {"name": "material.logos.altar", "ambient": 0.15, "diffuse": 0.6, "specular": 0.8, "shininess": 50.0, "baseColor": [0.05, 0.05, 0.08], "emission": [0.0, 0.0, 0.0], "roughness": 0.1, "metallic": 0.8},
    {"name": "material.logos.core", "ambient": 0.5, "diffuse": 0.9, "specular": 1.0, "shininess": 128.0, "baseColor": [1.0, 0.95, 0.75], "emission": [0.9, 0.8, 0.5], "roughness": 0.05, "metallic": 0.5},
    {"name": "material.logos.emerald", "ambient": 0.25, "diffuse": 0.8, "specular": 0.85, "shininess": 50.0, "baseColor": [0.15, 0.85, 0.45], "emission": [0.1, 0.4, 0.2], "roughness": 0.2, "metallic": 0.7},
    {"name": "material.logos.amethyst", "ambient": 0.25, "diffuse": 0.8, "specular": 0.85, "shininess": 50.0, "baseColor": [0.65, 0.25, 0.95], "emission": [0.25, 0.1, 0.45], "roughness": 0.2, "metallic": 0.7},
    {"name": "material.logos.amber", "ambient": 0.3, "diffuse": 0.85, "specular": 0.8, "shininess": 45.0, "baseColor": [0.98, 0.65, 0.15], "emission": [0.3, 0.18, 0.05], "roughness": 0.2, "metallic": 0.8},
    {"name": "material.logos.arch", "ambient": 0.3, "diffuse": 0.8, "specular": 0.7, "shininess": 40.0, "baseColor": [0.85, 0.82, 0.78], "emission": [0.08, 0.08, 0.12], "roughness": 0.3, "metallic": 0.4}
]

# ==============================================================================
# LEXEMES (First-Class Singular Beings)
# ==============================================================================
lexemes = [
    {"id": "lexeme.logos", "symbol": "Logos"},
    {"id": "lexeme.pneuma", "symbol": "Pneuma"},
    {"id": "lexeme.lux", "symbol": "Lux"},
    {"id": "lexeme.harmonia", "symbol": "Harmonia"},
    {"id": "lexeme.covenant", "symbol": "Covenant"},
    {"id": "lexeme.agape", "symbol": "Agape"},
    {"id": "lexeme.sophia", "symbol": "Sophia"},
    {"id": "lexeme.poiesis", "symbol": "Poiesis"},
    {"id": "lexeme.koinonia", "symbol": "Koinonia"},
    {"id": "lexeme.sabbath", "symbol": "Sabbath"}
]

# ==============================================================================
# FORMATION RELATIONS
# ==============================================================================
formationRelations = [
    {"directed": True, "entityA": "altar.logos.mensa", "entityB": "logos.resonator.core", "type": "acoustic-resonance", "weight": 1.0, "events": [{"deltaWeight": 1.0, "description": "acoustic-resonance", "timestamp": 1787395000}]},
    {"directed": True, "entityA": "glyph.lexeme.logos", "entityB": "lexeme.logos", "type": "speech-act", "weight": 1.0, "events": [{"deltaWeight": 1.0, "description": "speech-act", "timestamp": 1787395000}]},
    {"directed": True, "entityA": "glyph.lexeme.pneuma", "entityB": "lexeme.pneuma", "type": "speech-act", "weight": 1.0, "events": [{"deltaWeight": 1.0, "description": "speech-act", "timestamp": 1787395000}]},
    {"directed": True, "entityA": "glyph.lexeme.lux", "entityB": "lexeme.lux", "type": "speech-act", "weight": 1.0, "events": [{"deltaWeight": 1.0, "description": "speech-act", "timestamp": 1787395000}]},
    {"directed": True, "entityA": "glyph.lexeme.harmonia", "entityB": "lexeme.harmonia", "type": "speech-act", "weight": 1.0, "events": [{"deltaWeight": 1.0, "description": "speech-act", "timestamp": 1787395000}]},
    {"directed": True, "entityA": "glyph.lexeme.covenant", "entityB": "lexeme.covenant", "type": "speech-act", "weight": 1.0, "events": [{"deltaWeight": 1.0, "description": "speech-act", "timestamp": 1787395000}]},
]

zone_doc = {
    "identifier": "Cathedral of the Living Logos",
    "name": "Cathedral of the Living Logos",
    "owner": "Zach",
    "parentZone": "",
    "scope": "Local",
    "qualities": {
        "kind": "cathedral",
        "telos": "Acoustic-Visual Unity in Christ",
        "foundation": "Hierarchy of Joys"
    },
    "deletable": {"Zach": True},
    "spatialRoot": {
        "id": "Cathedral of the Living Logos_spatialRoot",
        "origin": [0.0, 0.0, 0.0],
        "scale": [120.0, 80.0, 120.0],
        "field": {
            "amplitude": 1.2,
            "baseDensity": 0.88,
            "frequency": 1.618,
            "mode": "Procedural"
        },
        "vectorField": {
            "amplitude": 0.6,
            "baseFlowX": 0.0,
            "baseFlowY": 0.25,
            "baseFlowZ": 0.0,
            "frequency": 1.0,
            "mode": "Procedural"
        }
    },
    "materials": materials,
    "world": {
        "objects": objects
    },
    "lexemes": lexemes,
    "formationRelations": formationRelations,
    "lawRefs": [
        "law-logos-breath",
        "law-logos-fiat-lux",
        "law-logos-celestial-chord",        "law-logos-covenant-weave",
        "law-logos-unison",
        "law-logos-season-toggle",
        "law-logos-pillar-pulse"
    ]
}

# Repository-relative output paths
repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
zone_path = os.path.join(repo_root, "saves", "zones", "Cathedral of the Living Logos", "zone.json")
os.makedirs(os.path.dirname(zone_path), exist_ok=True)
with open(zone_path, "w") as f:
    json.dump(zone_doc, f, indent=2)
print(f"Wrote {zone_path} ({len(objects)} objects)")

# Full world package with grounded player camera viewpoint
world_doc = {
    "saveFormat": "zone-identity-v1",
    "injected_by": "Gemini Spark (authored under Zach's Hierarchy of Joys ontology)",
    "authors": ["Zach"],
    "currentZoneId": "Cathedral of the Living Logos",
    "flying": False,
    "cameraPos": [0.0, 2.8, 26.0],
    "cameraFront": [0.0, -0.02, -1.0],
    "cameraUp": [0.0, 1.0, 0.0],
    "yaw": -90.0,
    "pitch": 0.0,
    "zoneRefs": [{"identifier": "Cathedral of the Living Logos"}],
    "categories": [
        {
            "objectID": "Zach",
            "shapeKind": 12,
            "geometryType": 12,
            "shapeParams": [0,0,0,0,0,0,0,0,0,0,0],
            "shape": {
                "kind": 12,
                "params": {
                    "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
                    "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
                    "ovoidAsym": 0.0, "fillet": 0.0,
                    "width2D": 0.0, "height2D": 0.0
                }
            },
            "transform": mat4([0,0,0]),
            "center": [0,0,0],
            "authoredProperties": {"displayName": {"t": "string", "v": "Zachary Zhang"}}
        }
    ],
    "zones": [zone_doc],
    "authoredLaws": {
        "laws": [
            {
                "id": "law-logos-breath",
                "name": "Logos: Breath of Life (Pneuma Respiration)",
                "enabled": True, "authority": 0, "activation": 0, "scope": 1, "drives": False, "retrigger": 0, "conditionMode": "any",
                "authors": ["Zach"], "conditionSubjects": [], "targets": [], "applicationLog": [],
                "conditionModel": {
                    "kind": 4,
                    "children": [{"kind": 8, "otherId": "hud.btn.pneuma"}, {"kind": 8, "otherId": "glyph.lexeme.pneuma"}]
                },
                "actionModel": {
                    "kind": 5,
                    "children": [
                        {"kind": 0, "path": "@state.logos.breathActive", "operand": {"t": "bool", "v": True}},
                        {"kind": 0, "path": "@state.logos.pulseRate", "operand": {"t": "float", "v": 2.4}},
                        {"kind": 0, "path": "@hud.logos.telemetry.breath.label2D", "operand": {"t": "string", "v": "BREATH: RESPIRING (PNEUMA HARMONIC ACTIVE)"}},
                        {"kind": 0, "path": "@logos.resonator.core.light.intensity", "operand": {"t": "float", "v": 6.0}}
                    ]
                },
                "provenance": [{"entityA": "law-logos-breath", "entityB": "Zach", "directed": True, "weight": 1.0, "events": [], "type": "authored-by"}]
            },
            {
                "id": "law-logos-fiat-lux",
                "name": "Logos: Fiat Lux (Transfiguring Illumination)",
                "enabled": True, "authority": 0, "activation": 0, "scope": 1, "drives": False, "retrigger": 0, "conditionMode": "any",
                "authors": ["Zach"], "conditionSubjects": [], "targets": [], "applicationLog": [],
                "conditionModel": {
                    "kind": 4,
                    "children": [{"kind": 8, "otherId": "hud.btn.lux"}, {"kind": 8, "otherId": "glyph.lexeme.lux"}]
                },
                "actionModel": {
                    "kind": 5,
                    "children": [
                        {"kind": 0, "path": "@state.logos.luxActive", "operand": {"t": "bool", "v": True}},
                        {"kind": 0, "path": "@logos.resonator.core.light.intensity", "operand": {"t": "float", "v": 9.0}},
                        {"kind": 0, "path": "@hud.logos.telemetry.freq.label2D", "operand": {"t": "string", "v": "FIAT LUX: TRANSFIGURED RADIANCE (528 Hz SOLFEGGIO)"}}
                    ]
                },
                "provenance": [{"entityA": "law-logos-fiat-lux", "entityB": "Zach", "directed": True, "weight": 1.0, "events": [], "type": "authored-by"}]
            },
            {
                "id": "law-logos-celestial-chord",
                "name": "Logos: Sound Polyphonic Celestial Canon",
                "enabled": True, "authority": 0, "activation": 0, "scope": 1, "drives": False, "retrigger": 0, "conditionMode": "any",
                "authors": ["Zach"], "conditionSubjects": [], "targets": [], "applicationLog": [],
                "conditionModel": {
                    "kind": 4,
                    "children": [{"kind": 8, "otherId": "hud.btn.chord"}, {"kind": 8, "otherId": "glyph.lexeme.harmonia"}]
                },
                "actionModel": {
                    "kind": 5,
                    "children": [
                        {"kind": 0, "path": "@state.logos.resonanceFreq", "operand": {"t": "float", "v": 432.0}},
                        {"kind": 0, "path": "@logos.resonator.core.light.intensity", "operand": {"t": "float", "v": 7.5}},
                        {"kind": 0, "path": "@hud.logos.telemetry.freq.label2D", "operand": {"t": "string", "v": "CHORD: 432 Hz PYTHAGOREAN CELESTIAL CANON (ACTIVE)"}},
                        {"kind": 18, "path": "acoustic.frequency", "input": "acoustic.amplitude", "propertyName": "sine"}
                    ]
                },
                "provenance": [{"entityA": "law-logos-celestial-chord", "entityB": "Zach", "directed": True, "weight": 1.0, "events": [], "type": "authored-by"}]
            },
            {
                "id": "law-logos-season-toggle",
                "name": "Logos: Cycle Liturgical Season",
                "enabled": True, "authority": 0, "activation": 0, "scope": 1, "drives": False, "retrigger": 0, "conditionMode": "any",
                "authors": ["Zach"], "conditionSubjects": [], "targets": [], "applicationLog": [],
                "conditionModel": {
                    "kind": 4,
                    "children": [{"kind": 8, "otherId": "hud.btn.season"}]
                },
                "actionModel": {
                    "kind": 5,
                    "children": [
                        {"kind": 0, "path": "@state.logos.season", "operand": {"t": "string", "v": "Solar Transfiguration"}},
                        {"kind": 0, "path": "@hud.logos.telemetry.season.label2D", "operand": {"t": "string", "v": "LITURGY: SOLAR TRANSFIGURATION (NOON)"}},
                        {"kind": 0, "path": "@logos.resonator.core.light.intensity", "operand": {"t": "float", "v": 8.5}}
                    ]
                },
                "provenance": [{"entityA": "law-logos-season-toggle", "entityB": "Zach", "directed": True, "weight": 1.0, "events": [], "type": "authored-by"}]
            }
        ],
        "triggers": {
            "law-logos-breath": ["object-clicked"],
            "law-logos-fiat-lux": ["object-clicked"],
            "law-logos-celestial-chord": ["object-clicked"],
            "law-logos-season-toggle": ["object-clicked"]
        },
        "formationMembers": [
            "law-logos-breath",
            "law-logos-fiat-lux",
            "law-logos-celestial-chord",
            "law-logos-season-toggle"
        ],
        "rete": {"alphaNodes": [], "betaNodes": [], "facts": [], "agenda": []}
    }
}

world_path = os.path.join(repo_root, "saves", "worlds", "cathedral_of_the_living_logos.json")
with open(world_path, "w") as f:
    json.dump(world_doc, f, indent=2)
print(f"Wrote {world_path}")
