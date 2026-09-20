import json

def make_color_expr_piecewise(r_terms, g_terms, b_terms, input_var="y"):
    """
    Creates an OntoMath Piecewise dictionary whose mathNode is an Op::VectorConstruct (2)
    with 3 scalar children (Op::ScalarLeaf = 0).
    Kind: 0 = Sin, 1 = Cos, 2 = Exp, 3 = Ln
    """
    return {
        "input": input_var,
        "pieces": [
            {
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": r_terms}},
                        {"op": 0, "scalarForm": {"terms": g_terms}},
                        {"op": 0, "scalarForm": {"terms": b_terms}}
                    ]
                }
            }
        ]
    }

import os
import math
import struct
import hashlib
import base64

# ==============================================================================
# CATHEDRAL OF THE LIVING LOGOS — MONUMENTAL ARCHITECTURAL GENERATOR
# Full Gothic Revival Architecture + Split Substrate (.ecform + .ecmatter)
# Fully compliant with Earthcall Save System Overhaul (PR #188)
# ==============================================================================

def mat4(pos, scale=[1.0, 1.0, 1.0], rot_deg=[0.0, 0.0, 0.0]):
    """Creates a 4x4 column-major transformation matrix with Euler rotation (Z-Y-X)."""
    sx, sy, sz = float(scale[0]), float(scale[1]), float(scale[2])
    px, py, pz = float(pos[0]), float(pos[1]), float(pos[2])
    rx = math.radians(rot_deg[0])
    ry = math.radians(rot_deg[1])
    rz = math.radians(rot_deg[2])
    
    cx, sx_s = math.cos(rx), math.sin(rx)
    cy, sy_s = math.cos(ry), math.sin(ry)
    cz, sz_s = math.cos(rz), math.sin(rz)
    
    # Rotation matrix R = Rz * Ry * Rx
    r00 = cy * cz
    r01 = cz * sx_s * sy_s - cx * sz_s
    r02 = cx * cz * sy_s + sx_s * sz_s
    
    r10 = cy * sz_s
    r11 = cx * cz + sx_s * sy_s * sz_s
    r12 = cx * sy_s * sz_s - cz * sx_s
    
    r20 = -sy_s
    r21 = cy * sx_s
    r22 = cx * cy
    
    return [
        r00 * sx, r10 * sx, r20 * sx, 0.0,
        r01 * sy, r11 * sy, r21 * sy, 0.0,
        r02 * sz, r12 * sz, r22 * sz, 0.0,
        px, py, pz, 1.0
    ]

# ==============================================================================
# PROCEDURAL FACE TEXTURES (RGBA8 BASE64 ENCODED)
# ==============================================================================

def clamp_byte(v):
    return max(0, min(255, int(v)))

def encode_face(w, h, paint_fn):
    pixels = bytearray(w * h * 4)
    for y in range(h):
        v = (y / (h - 1.0)) * 2.0 - 1.0
        for x in range(w):
            u = (x / (w - 1.0)) * 2.0 - 1.0
            r, g, b = paint_fn(x, y, u, v, w, h)
            idx = (y * w + x) * 4
            pixels[idx + 0] = clamp_byte(r)
            pixels[idx + 1] = clamp_byte(g)
            pixels[idx + 2] = clamp_byte(b)
            pixels[idx + 3] = 255
    return {"width": w, "height": h, "pixelsB64": base64.b64encode(pixels).decode("ascii")}

def solid_face(w, h, rgb):
    r, g, b = rgb
    return encode_face(w, h, lambda x, y, u, v, w, h: (r, g, b))

# 1. Cosmati Floor (+Y)
def cosmati_floor_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        theta = math.atan2(v, u)
        col = [28, 30, 36]
        diag = abs(u) + abs(v)
        if 0.94 <= diag <= 1.06:
            col = [225, 195, 80]
        elif diag < 0.94 and r > 0.65:
            tile = int((u + 1.0) * 8) + int((v + 1.0) * 8)
            col = [120, 32, 44] if tile % 2 == 0 else [32, 75, 52]
        if 0.42 <= r <= 0.62:
            ribbon = math.sin(8.0 * theta + r * 12.0)
            if ribbon > 0.3:
                col = [242, 240, 232]
            elif ribbon < -0.3:
                col = [230, 190, 65]
            else:
                col = [18, 55, 135]
        elif 0.38 <= r < 0.42 or 0.62 < r <= 0.66:
            col = [235, 200, 75]
        elif r < 0.38:
            if r < 0.18:
                col = [140, 25, 38]
            elif r < 0.22:
                col = [245, 210, 85]
            else:
                star = math.cos(8.0 * theta)
                col = [245, 245, 240] if star > 0.2 else [15, 35, 95]
        grain = int((math.sin(x * 1.5) + math.cos(y * 1.7)) * 4.0)
        return (col[0] + grain, col[1] + grain, col[2] + grain)
    return encode_face(size, size, paint)

# 2. Altar Mensa (+Y)
def altar_mensa_face(size=256):
    def paint(x, y, u, v, w, h):
        base = [244, 242, 238]
        vein = math.sin(u * 5.0 + math.cos(v * 7.0) * 2.0) * math.cos(v * 4.0)
        if abs(vein) < 0.15:
            base = [215, 210, 205]
        if abs(u) > 0.90 or abs(v) > 0.90:
            base = [225, 185, 60]
        cross_centers = [(0.0, 0.0), (-0.7, -0.7), (0.7, -0.7), (-0.7, 0.7), (0.7, 0.7)]
        for cx, cy in cross_centers:
            du, dv = abs(u - cx), abs(v - cy)
            if (du < 0.04 and dv < 0.14) or (dv < 0.04 and du < 0.14):
                base = [215, 175, 45]
        return tuple(base)
    return encode_face(size, size, paint)

# 3. Altar Antependium (+Z)
def altar_antependium_face(size=256):
    def paint(x, y, u, v, w, h):
        weave = math.sin(x * 0.8) * math.cos(y * 0.8)
        crimson = [135 + int(weave * 12), 22 + int(weave * 4), 32 + int(weave * 5)]
        if v < -0.80:
            fringe = int(math.sin(x * 3.14159) * 20)
            return (225 + fringe, 185 + fringe, 55 + fringe)
        if v > 0.88 or abs(u) > 0.90:
            return (220, 180, 55)
        r = math.sqrt(u * u + (v - 0.05) * (v - 0.05))
        if 0.40 <= r <= 0.48:
            return (240, 205, 75)
        if r < 0.40:
            du, dv = abs(u), abs(v - 0.05)
            if (du < 0.06 and dv < 0.32) or (dv < 0.06 and du < 0.24):
                return (245, 215, 80)
            if 0.08 <= u <= 0.22 and 0.12 <= (v - 0.05) <= 0.28:
                if (u - 0.15)**2 + (v - 0.05 - 0.20)**2 <= 0.08**2:
                    return (245, 215, 80)
            return (110, 18, 26)
        damask = math.sin(u * 12.0) * math.cos(v * 10.0)
        if damask > 0.5:
            return (165, 38, 50)
        return tuple(crimson)
    return encode_face(size, size, paint)

# 4. Gothic Linenfold Wood (+Z, -Z)
def gothic_linenfold_wood_face(size=256):
    def paint(x, y, u, v, w, h):
        grain = math.sin(u * 25.0 + math.sin(v * 6.0)) * 10
        base = [115 + int(grain), 68 + int(grain * 0.6), 34 + int(grain * 0.3)]
        if abs(u) > 0.88 or abs(v) > 0.88:
            return (85, 48, 22)
        wave = math.sin(u * 3.14159 * 4.0)
        shadow = math.cos(u * 3.14159 * 4.0)
        ogee = math.sin(u * 3.14159 * 2.0) * 0.15
        if v > (0.75 + ogee) or v < (-0.75 - ogee):
            return (70, 38, 18)
        r = base[0] + int(wave * 25 + shadow * 15)
        g = base[1] + int(wave * 15 + shadow * 10)
        b = base[2] + int(wave * 8 + shadow * 5)
        return (r, g, b)
    return encode_face(size, size, paint)

# 5. Wood Grain (+Y, etc.)
def wood_grain_face(size=256):
    def paint(x, y, u, v, w, h):
        g1 = math.sin(v * 30.0 + math.cos(u * 8.0) * 2.0)
        g2 = math.cos(v * 15.0 + u * 4.0)
        g = (g1 * 0.6 + g2 * 0.4) * 18.0
        return (130 + int(g), 80 + int(g * 0.6), 42 + int(g * 0.3))
    return encode_face(size, size, paint)

# 6. Gilded Filigree (+X, -X, +Y, -Y, +Z, -Z)
def gilded_filigree_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        theta = math.atan2(v, u)
        scroll = math.sin(theta * 6.0 + r * 10.0) + math.cos(theta * 3.0 - r * 8.0)
        base_gold = [235, 195, 60]
        if scroll > 0.4:
            return (255, 230, 110)
        elif scroll < -0.3:
            return (160, 120, 30)
        if abs(u) > 0.90 or abs(v) > 0.90:
            return (250, 215, 80)
        return tuple(base_gold)
    return encode_face(size, size, paint)

# 7. Veined Alabaster
def veined_alabaster_face(size=256):
    def paint(x, y, u, v, w, h):
        base = [242, 239, 234]
        vein1 = math.sin(u * 4.0 + math.cos(v * 6.0) * 1.5)
        vein2 = math.cos(v * 5.0 + math.sin(u * 7.0) * 1.8)
        if abs(vein1) < 0.12 or abs(vein2) < 0.08:
            return (195, 188, 178)
        if abs(vein1 * vein2) < 0.03:
            return (215, 190, 130)
        return tuple(base)
    return encode_face(size, size, paint)

# 8. Ashlar Limestone Wall
def ashlar_stone_face(size=256):
    def paint(x, y, u, v, w, h):
        course = int((v + 1.0) * 2.0)
        offset_u = 0.5 if (course % 2 == 1) else 0.0
        block = int((u + offset_u + 2.0) * 2.0)
        du = ((u + offset_u + 2.0) * 2.0) % 1.0
        dv = ((v + 1.0) * 2.0) % 1.0
        if du < 0.06 or dv < 0.08:
            return (110, 105, 98)
        grain = int((math.sin(x * 2.5) + math.cos(y * 2.7)) * 5.0)
        base = 185 + (block * 7) % 15 + grain
        return (base, base - 6, base - 14)
    return encode_face(size, size, paint)

# 9. Sapphire Rose Window
def rose_window_sapphire_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        theta = math.atan2(v, u)
        if r > 0.92:
            return (30, 30, 35)
        if abs(r - 0.32) < 0.04 or abs(r - 0.62) < 0.04 or abs(r - 0.88) < 0.04:
            return (25, 25, 30)
        spoke = abs(math.sin(6.0 * theta))
        if spoke < 0.08 and r > 0.25:
            return (25, 25, 30)
        if r < 0.28:
            if r < 0.12:
                return (245, 215, 80)
            return (180, 45, 55)
        elif 0.32 < r < 0.62:
            petal = math.sin(6.0 * theta)
            lum = int(petal * 25)
            return (25, 65 + lum, 210 + lum)
        elif 0.62 < r < 0.88:
            spoke24 = math.sin(12.0 * theta)
            if spoke24 > 0.2:
                return (45, 175, 245)
            elif spoke24 < -0.2:
                return (235, 195, 65)
            else:
                return (195, 35, 65)
        return (25, 25, 30)
    return encode_face(size, size, paint)

# 10. Ruby Rose Window
def rose_window_ruby_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        theta = math.atan2(v, u)
        if r > 0.92:
            return (30, 30, 35)
        if abs(r - 0.32) < 0.04 or abs(r - 0.62) < 0.04 or abs(r - 0.88) < 0.04:
            return (25, 25, 30)
        spoke = abs(math.sin(6.0 * theta))
        if spoke < 0.08 and r > 0.25:
            return (25, 25, 30)
        if r < 0.28:
            if r < 0.12:
                return (250, 220, 90)
            return (220, 35, 45)
        elif 0.32 < r < 0.62:
            petal = math.sin(6.0 * theta)
            lum = int(petal * 30)
            return (205 + lum, 30, 45 + lum // 2)
        elif 0.62 < r < 0.88:
            spoke24 = math.sin(12.0 * theta)
            if spoke24 > 0.2:
                return (235, 155, 35)
            elif spoke24 < -0.2:
                return (180, 25, 45)
            else:
                return (120, 25, 110)
        return (25, 25, 30)
    return encode_face(size, size, paint)

# 11. Emerald Tree of Life Stained Glass
def stained_glass_emerald_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        if abs(u) > 0.90 or abs(v) > 0.92:
            return (25, 25, 30)
        branches = math.sin(v * 8.0 + u * 4.0) * math.cos(u * 6.0)
        if abs(branches) < 0.12:
            return (220, 185, 60)
        leaf = math.sin(u * 12.0) * math.sin(v * 12.0)
        lum = int(leaf * 35)
        return (20, 140 + lum, 65 + lum // 2)
    return encode_face(size, size, paint)

# 12. Amethyst Genesis Stained Glass
def stained_glass_amethyst_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        if abs(u) > 0.90 or abs(v) > 0.92:
            return (25, 25, 30)
        d1 = math.sqrt((u - 0.35)**2 + v*v)
        d2 = math.sqrt((u + 0.35)**2 + v*v)
        if d1 < 0.85 and d2 < 0.85:
            if abs(d1 - d2) < 0.08:
                return (245, 215, 80)
            return (145, 45, 195)
        if abs(d1 - 0.85) < 0.05 or abs(d2 - 0.85) < 0.05:
            return (25, 25, 30)
        return (85, 22, 130)
    return encode_face(size, size, paint)

# 13. Silver/Tin Organ Pipe Face
def organ_pipe_tin_face(size=256):
    def paint(x, y, u, v, w, h):
        col = int(math.cos(u * 3.14159 * 0.5) * 120 + 130)
        if abs(v - (-0.60)) < 0.06 and abs(u) < 0.45:
            return (215, 175, 45)
        if abs(v - (-0.60)) < 0.03 and abs(u) < 0.35:
            return (25, 25, 30)
        return (col, col + 2, col + 8)
    return encode_face(size, size, paint)

# 14. Illuminated Manuscript Book Page (+Y)
def illuminated_manuscript_face(size=256):
    def paint(x, y, u, v, w, h):
        if abs(u) > 0.88 or abs(v) > 0.88:
            return (95, 45, 22)
        parchment = [240, 232, 212]
        if -0.75 <= u <= -0.45 and 0.45 <= v <= 0.75:
            return (195, 30, 35)
        if abs(u) < 0.84 and (v > 0.80 or v < -0.80 or abs(u - 0.0) < 0.04):
            if abs(u) < 0.82 and (abs(v) > 0.82 or abs(u) < 0.02):
                return (225, 185, 55)
        line_idx = int((v + 0.8) * 10)
        if 1 <= line_idx <= 14 and line_idx != 12 and line_idx != 13:
            dv = ((v + 0.8) * 10) % 1.0
            if 0.35 <= dv <= 0.65:
                char = int((u + 0.8) * 20) % 2
                if char == 0:
                    return (45, 38, 32)
        return tuple(parchment)
    return encode_face(size, size, paint)

# 15. Water Caustics Surface
def water_caustics_face(size=256):
    def paint(x, y, u, v, w, h):
        r = math.sqrt(u * u + v * v)
        ripple = math.sin(r * 24.0) + math.cos(u * 16.0) * math.sin(v * 16.0)
        lum = int(ripple * 25)
        return (35 + lum // 2, 195 + lum, 235 + lum // 2)
    return encode_face(size, size, paint)


def make_box(obj_id, name, pos, scale, mat_id, color, rot_deg=[0.0, 0.0, 0.0], extra_props=None, face_colors=None):
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
        "transform": mat4(pos, scale, rot_deg),
        "center": [float(pos[0]), float(pos[1]), float(pos[2])],
        "materialId": mat_id,
        "renderMode": 0,
        "faceColors": face_colors if face_colors else [color] * 6,
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

def make_torus(obj_id, name, pos, majorR, minorR, mat_id, color, rot_deg=[0.0, 0.0, 0.0], extra_props=None):
    # renderMode: 2 (RenderMode::Mesh) ensures WebGPU renders the tessellated 3D polygon mesh
    # rather than clipping with an uninitialized SDF extent!
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
        "transform": mat4(pos, [1.0, 1.0, 1.0], rot_deg),
        "center": [float(pos[0]), float(pos[1]), float(pos[2])],
        "materialId": mat_id,
        "renderMode": 2,
        "faceColors": [color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": name}
        }
    }
    if extra_props:
        obj["authoredProperties"].update(extra_props)
    return obj

def make_ring_segments(prefix_id, name_prefix, center, radius, thickness, mat_id, color, num_segments=24, plane='xz', tilt_deg=[0.0, 0.0, 0.0]):
    """Creates a physical 3D armillary ring composed of tangent faceted segments."""
    cx, cy, cz = center
    chord = 2.0 * radius * math.sin(math.pi / num_segments) * 1.06
    segments = []
    
    for i in range(num_segments):
        angle = 2.0 * math.pi * i / num_segments
        deg = math.degrees(angle)
        seg_id = f"{prefix_id}.seg.{i+1}"
        seg_name = f"{name_prefix} Segment {i+1}"
        
        if plane == 'xz':
            # Horizontal ring (in XZ plane)
            lx = radius * math.cos(angle)
            lz = radius * math.sin(angle)
            pos = [cx + lx, cy, cz + lz]
            rot = [tilt_deg[0], -deg + 90.0 + tilt_deg[1], tilt_deg[2]]
        elif plane == 'yz':
            # Vertical ring parallel to North/South walls
            ly = radius * math.cos(angle)
            lz = radius * math.sin(angle)
            pos = [cx, cy + ly, cz + lz]
            rot = [-deg + 90.0 + tilt_deg[0], tilt_deg[1], tilt_deg[2]]
        
        scale = [thickness, thickness, chord]
        segments.append(make_box(seg_id, seg_name, pos, scale, mat_id, color, rot))
    return segments

def make_label2d(obj_id, text, x, y, size=14.0, color=[0.92, 0.94, 0.98]):
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
    """Creates a 2D interactive button. Shape2D automatically centers its label."""
    return {
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

def sdf_leaf(prim, dims, offset=[0.0, 0.0, 0.0], p0=0.0, p1=0.0):
    """Creates an SdfNode leaf with primitive type and local offset."""
    return {
        "op": 0,
        "prim": prim,
        "dims": [float(dims[0]), float(dims[1]), float(dims[2])],
        "offset": [float(offset[0]), float(offset[1]), float(offset[2])],
        "p0": float(p0),
        "p1": float(p1),
        "t": 0.5,
        "children": []
    }

def sdf_expr(expr_str, dims=[1.0, 1.0, 1.0], offset=[0.0, 0.0, 0.0]):
    """Creates an implicit mathematical expression SdfNode leaf."""
    return {
        "op": 0,
        "prim": 7,
        "dims": [float(dims[0]), float(dims[1]), float(dims[2])],
        "offset": [float(offset[0]), float(offset[1]), float(offset[2])],
        "p0": 0.0,
        "p1": 0.0,
        "t": 0.5,
        "expr": expr_str,
        "children": []
    }

def sdf_binary(op, a, b, t=0.5):
    """Creates a binary operator SdfNode (Morph=1, Union=2, Intersect=3, Subtract=4, SmoothUnion=5)."""
    return {
        "op": op,
        "prim": 0,
        "dims": [0.5, 0.5, 0.5],
        "offset": [0.0, 0.0, 0.0],
        "p0": 0.0,
        "p1": 0.0,
        "t": float(t),
        "children": [a, b]
    }

def make_field(obj_id, name, pos, field_tree, field_extent, mat_id, color, scale=[1.0, 1.0, 1.0], rot_deg=[0.0, 0.0, 0.0], extra_props=None, render_mode=0):
    """
    Creates an exact analytic Signed Distance Field (SDF) being (ShapeKind::Field = 10).
    In WebGPU, this is raymarched directly at infinite mathematical fidelity.
    In OpenGL or Mesh mode, it is tessellated via Marching Tetrahedra.
    """
    obj = {
        "objectID": obj_id,
        "shapeKind": 10,
        "geometryType": 10,
        "shapeParams": [0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        "shape": {
            "kind": 10,
            "params": {
                "r": 0.5, "ry": 0.5, "rz": 0.5, "halfH": 0.5,
                "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
                "ovoidAsym": 0.0, "fillet": 0.0,
                "width2D": 0.0, "height2D": 0.0
            }
        },
        "field": field_tree,
        "fieldExtent": [float(field_extent[0]), float(field_extent[1]), float(field_extent[2])],
        "transform": mat4(pos, scale, rot_deg),
        "center": [float(pos[0]), float(pos[1]), float(pos[2])],
        "materialId": mat_id,
        "renderMode": render_mode,
        "faceColors": [color] * 6,
        "authoredProperties": {
            "displayName": {"t": "string", "v": name}
        }
    }
    if extra_props:
        obj["authoredProperties"].update(extra_props)
    return obj

objects = []

# ==============================================================================
# 1. FLOORS & SANCTUARY DAIS
# ==============================================================================
# Grand Acoustic Obsidian Stone Foundation (34m wide, 74m long)
objects.append(make_box(
    "cathedral.floor.main", "Chladni Acoustic Foundation",
    [0.0, -0.3, -2.0], [34.0, 0.6, 74.0],
    "material.logos.arch", [0.10, 0.11, 0.14]
))

# Modular Processional Nave Cosmati Mosaic Paving (5 monumental 12m bays along nave axis)
# Each bay receives its own dedicated high-resolution 256x256 Cosmati medallion without stretching!
for i, bz in enumerate([22.0, 11.0, 0.0, -11.0, -22.0]):
    objects.append(make_box(
        f"cathedral.nave.cosmati.bay.{i+1}", f"Processional Cosmati Mosaic Pavement (Bay {i+1})",
        [0.0, 0.02, bz], [6.4, 0.04, 10.6],
        "material.logos.floor", [1.0, 1.0, 1.0],
        extra_props={"textureResolution": {"t": "int", "v": 256}}
    ))

# Sanctuary High Dais Sacred Cosmati Paving
objects.append(make_box(
    "cathedral.sanctuary.cosmati", "Sanctuary High Altar Cosmati Pavement",
    [0.0, 0.77, -29.5], [10.6, 0.04, 8.6],
    "material.logos.floor", [1.0, 1.0, 1.0],
    extra_props={"textureResolution": {"t": "int", "v": 256}}
))

# North & South Transept Shrine Cosmati Paving
for tx, side_name in [(-14.0, "North"), (14.0, "South")]:
    objects.append(make_box(
        f"cathedral.transept.cosmati.{side_name.lower()}", f"{side_name} Transept Shrine Cosmati Pavement",
        [tx, 0.02, 0.0], [7.0, 0.04, 7.0],
        "material.logos.floor", [1.0, 1.0, 1.0],
        extra_props={"textureResolution": {"t": "int", "v": 256}}
    ))

# Three-Tiered Sanctuary Chancel Dais leading to High Altar
objects.append(make_box(
    "cathedral.dais.tier1", "Chancel Step I (Communion Dais)",
    [0.0, 0.15, -25.5], [19.0, 0.3, 17.0],
    "material.logos.floor", [0.14, 0.15, 0.18]
))
objects.append(make_box(
    "cathedral.dais.tier2", "Chancel Step II (Sanctuary Terrace)",
    [0.0, 0.45, -27.5], [15.0, 0.3, 13.0],
    "material.logos.floor", [0.16, 0.17, 0.20]
))
objects.append(make_box(
    "cathedral.dais.tier3", "Sanctuary High Dais (Altar Platform)",
    [0.0, 0.75, -29.5], [11.0, 0.3, 9.0],
    "material.logos.gold", [1.0, 0.82, 0.28]
))

# ==============================================================================
# 2. HIGH ALTAR, CANOPY & SANCTUARY FURNITURE
# ==============================================================================
# Sub-altar Step (Predella)
objects.append(make_box(
    "altar.logos.predella", "Altar Predella Step",
    [0.0, 0.95, -29.5], [6.2, 0.12, 3.2],
    "material.logos.floor", [0.18, 0.17, 0.16]
))

# Altar Base & Supporting Golden Pillars
altar_pillars = [
    ("altar.pillar.FL", [-2.3, 1.55, -28.5]),
    ("altar.pillar.FR", [2.3, 1.55, -28.5]),
    ("altar.pillar.BL", [-2.3, 1.55, -30.5]),
    ("altar.pillar.BR", [2.3, 1.55, -30.5]),
    ("altar.pillar.CL", [-0.8, 1.55, -29.5]),
    ("altar.pillar.CR", [0.8, 1.55, -29.5]),
]
for pid, ppos in altar_pillars:
    objects.append(make_box(
        pid, "Altar Colonnette",
        ppos, [0.4, 1.1, 0.4], "material.logos.gold", [1.0, 0.82, 0.28]
    ))

# Altar Frontal Relief Panel (Lapis Lazuli with Gold Border)
objects.append(make_box(
    "altar.logos.frontal", "Altar Frontal Relief Panel",
    [0.0, 1.55, -28.35], [4.4, 1.05, 0.15],
    "material.logos.sapphire", [0.12, 0.35, 0.95]
))

# High Altar Mensa (Thick Black Obsidian Altar Slab with Beveled Rim)
objects.append(make_box(
    "altar.logos.mensa", "High Altar Mensa Slab",
    [0.0, 2.2, -29.5], [5.6, 0.24, 2.6],
    "material.logos.altar", [0.05, 0.05, 0.08],
    extra_props={"isAltar": {"t": "bool", "v": True}}
))

# Altar Retable (Elevated Shelf Behind Mensa)
objects.append(make_box(
    "altar.logos.retable", "Altar Retable Gradine",
    [0.0, 2.45, -30.6], [5.2, 0.3, 0.45],
    "material.logos.gold", [1.0, 0.82, 0.28]
))

# Six Golden Altar Candelabras with Glowing Flames
for i, cx in enumerate([-2.1, -1.3, -0.5, 0.5, 1.3, 2.1]):
    objects.append(make_box(
        f"altar.candle.{i+1}.base", f"Altar Candelabra {i+1}",
        [cx, 2.75, -30.6], [0.18, 0.35, 0.18],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    objects.append(make_box(
        f"altar.candle.{i+1}.taper", f"Altar Wax Taper {i+1}",
        [cx, 3.15, -30.6], [0.08, 0.45, 0.08],
        "material.logos.alabaster", [0.94, 0.92, 0.88]
    ))
    objects.append(make_sphere(
        f"altar.candle.{i+1}.flame", f"Sacred Flame {i+1}",
        [cx, 3.45, -30.6], 0.09,
        "material.logos.core", [1.0, 0.95, 0.75],
        extra_props={"light.intensity": {"t": "float", "v": 1.5}}
    ))

# Central Altar Cross
objects.append(make_box(
    "altar.cross.vertical", "Central Altar Crucifix Shaft",
    [0.0, 3.55, -30.6], [0.14, 1.4, 0.1],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "altar.cross.horizontal", "Central Altar Crucifix Arm",
    [0.0, 3.85, -30.6], [0.85, 0.14, 0.1],
    "material.logos.gold", [1.0, 0.82, 0.28]
))

# Sacred Sanctuary Reredos (15m High Golden Altarpiece)
objects.append(make_box(
    "altar.logos.reredos.center", "Sanctuary Reredos Center Screen",
    [0.0, 8.5, -33.8], [7.0, 15.0, 0.8],
    "material.logos.gold", [1.0, 0.82, 0.28],
    extra_props={"isReredos": {"t": "bool", "v": True}}
))
objects.append(make_box(
    "altar.logos.reredos.wingL", "Sanctuary Reredos Wing Left",
    [-4.5, 7.5, -33.5], [2.2, 13.0, 0.6],
    "material.logos.gold", [0.88, 0.74, 0.22]
))
objects.append(make_box(
    "altar.logos.reredos.wingR", "Sanctuary Reredos Wing Right",
    [4.5, 7.5, -33.5], [2.2, 13.0, 0.6],
    "material.logos.gold", [0.88, 0.74, 0.22]
))
# Central Reredos Apex Spire
objects.append(make_box(
    "altar.logos.reredos.spire", "Sanctuary Reredos Apex Pinnacle",
    [0.0, 17.0, -33.8], [1.2, 3.5, 0.6],
    "material.logos.gold", [1.0, 0.85, 0.32]
))

# ------------------------------------------------------------------------------
# 2.1 HIGH ALTAR BALDACHIN CIBORIUM & CARVED REREDOS ARCADES
# ------------------------------------------------------------------------------
baldachin_corners = [
    ("FL", -2.8, -27.8),
    ("FR", 2.8, -27.8),
    ("BL", -2.8, -31.2),
    ("BR", 2.8, -31.2),
]
for cid, bx, bz in baldachin_corners:
    objects.append(make_box(
        f"baldachin.plinth.{cid}", f"Baldachin Plinth {cid}",
        [bx, 0.9, bz], [0.55, 0.35, 0.55],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    objects.append(make_box(
        f"baldachin.column.{cid}", f"Baldachin Colonnette {cid}",
        [bx, 3.8, bz], [0.28, 5.5, 0.28],
        "material.logos.gold", [1.0, 0.85, 0.32]
    ))
    objects.append(make_box(
        f"baldachin.capital.{cid}", f"Baldachin Capital {cid}",
        [bx, 6.7, bz], [0.55, 0.4, 0.55],
        "material.logos.gold", [1.0, 0.88, 0.35]
    ))

# Vaulted Baldachin Canopy Archways & Roof
objects.append(make_box(
    "baldachin.arch.front", "Baldachin Canopy Arch Front",
    [0.0, 7.0, -27.8], [5.6, 0.4, 0.35],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "baldachin.arch.back", "Baldachin Canopy Arch Back",
    [0.0, 7.0, -31.2], [5.6, 0.4, 0.35],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "baldachin.arch.sideL", "Baldachin Canopy Arch Left",
    [-2.8, 7.0, -29.5], [0.35, 0.4, 3.4],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "baldachin.arch.sideR", "Baldachin Canopy Arch Right",
    [2.8, 7.0, -29.5], [0.35, 0.4, 3.4],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "baldachin.canopy.roof", "Baldachin Vaulted Canopy Roof",
    [0.0, 7.4, -29.5], [6.0, 0.4, 3.8],
    "material.logos.gold", [1.0, 0.85, 0.32]
))
objects.append(make_box(
    "baldachin.canopy.spire", "Baldachin Apex Filigree Spire",
    [0.0, 8.8, -29.5], [0.65, 2.4, 0.65],
    "material.logos.gold", [1.0, 0.90, 0.38]
))

# Carved Gothic Reredos Niches & Tracery
for ni, nx in enumerate([-2.4, -1.2, 0.0, 1.2, 2.4]):
    objects.append(make_box(
        f"reredos.niche.{ni+1}", f"Reredos Gothic Tracery Niche {ni+1}",
        [nx, 8.5, -33.3], [0.95, 6.5, 0.3],
        "material.logos.sapphire", [0.12, 0.35, 0.95]
    ))
    objects.append(make_box(
        f"reredos.canopy.{ni+1}", f"Reredos Niche Canopy {ni+1}",
        [nx, 12.0, -33.2], [1.1, 0.65, 0.45],
        "material.logos.gold", [1.0, 0.85, 0.32]
    ))

# ==============================================================================
# 3. LIVING LEXEME GLYPHS ON THE ALTAR MENSA
# ==============================================================================
glyphs_info = [
    ("glyph.lexeme.logos", "Lexeme: [Logos]", [-1.8, 2.45, -29.5], [1.0, 0.82, 0.28], "material.logos.gold"),
    ("glyph.lexeme.pneuma", "Lexeme: [Pneuma]", [-0.9, 2.45, -29.5], [0.12, 0.35, 0.95], "material.logos.sapphire"),
    ("glyph.lexeme.lux", "Lexeme: [Lux]", [0.0, 2.45, -29.5], [1.0, 0.95, 0.75], "material.logos.core"),
    ("glyph.lexeme.harmonia", "Lexeme: [Harmonia]", [0.9, 2.45, -29.5], [0.98, 0.65, 0.15], "material.logos.amber"),
    ("glyph.lexeme.covenant", "Lexeme: [Covenant]", [1.8, 2.45, -29.5], [0.65, 0.25, 0.95], "material.logos.amethyst"),
]
for gid, gname, gpos, gcolor, gmat in glyphs_info:
    objects.append(make_box(
        gid, gname, gpos, [0.45, 0.18, 0.45], gmat, gcolor,
        extra_props={"isSpeechAct": {"t": "bool", "v": True}}
    ))

# ==============================================================================
# 4. CHANCEL COMMUNION RAIL & GOTHIC PULPIT (AMBO)
# ==============================================================================
# Left Communion Rail (X = -7.5m to -1.8m at Z = -22.5m)
objects.append(make_box(
    "chancel.rail.base.L", "Communion Rail Plinth Left",
    [-4.6, 0.1, -22.5], [5.6, 0.2, 0.35],
    "material.logos.floor", [0.18, 0.16, 0.15]
))
objects.append(make_box(
    "chancel.rail.cap.L", "Communion Rail Banister Left",
    [-4.6, 0.85, -22.5], [5.6, 0.12, 0.3],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
for bi, bx in enumerate([-6.8, -5.7, -4.6, -3.5, -2.4]):
    objects.append(make_box(
        f"chancel.rail.baluster.L{bi+1}", f"Rail Baluster L{bi+1}",
        [bx, 0.47, -22.5], [0.14, 0.65, 0.14],
        "material.logos.alabaster", [0.94, 0.92, 0.88]
    ))
objects.append(make_box(
    "chancel.rail.newel.L", "Communion Rail Newel Post Left",
    [-1.75, 0.5, -22.5], [0.35, 1.0, 0.35],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_sphere(
    "chancel.rail.finial.L", "Rail Finial Left",
    [-1.75, 1.15, -22.5], 0.18,
    "material.logos.gold", [1.0, 0.85, 0.32]
))

# Right Communion Rail (X = +1.8m to +7.5m at Z = -22.5m)
objects.append(make_box(
    "chancel.rail.base.R", "Communion Rail Plinth Right",
    [4.6, 0.1, -22.5], [5.6, 0.2, 0.35],
    "material.logos.floor", [0.18, 0.16, 0.15]
))
objects.append(make_box(
    "chancel.rail.cap.R", "Communion Rail Banister Right",
    [4.6, 0.85, -22.5], [5.6, 0.12, 0.3],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
for bi, bx in enumerate([2.4, 3.5, 4.6, 5.7, 6.8]):
    objects.append(make_box(
        f"chancel.rail.baluster.R{bi+1}", f"Rail Baluster R{bi+1}",
        [bx, 0.47, -22.5], [0.14, 0.65, 0.14],
        "material.logos.alabaster", [0.94, 0.92, 0.88]
    ))
objects.append(make_box(
    "chancel.rail.newel.R", "Communion Rail Newel Post Right",
    [1.75, 0.5, -22.5], [0.35, 1.0, 0.35],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_sphere(
    "chancel.rail.finial.R", "Rail Finial Right",
    [1.75, 1.15, -22.5], 0.18,
    "material.logos.gold", [1.0, 0.85, 0.32]
))

# Sculpted Gothic Pulpit (Ambo of the Living Word) on liturgical north (X = -6.2, Z = -19.5)
objects.append(make_box(
    "pulpit.base.step1", "Pulpit Stone Plinth",
    [-6.2, 0.15, -19.5], [2.2, 0.3, 2.2],
    "material.logos.floor", [0.16, 0.15, 0.14]
))
objects.append(make_box(
    "pulpit.pedestal", "Pulpit Sculpted Pedestal",
    [-6.2, 0.8, -19.5], [0.9, 1.1, 0.9],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "pulpit.floor", "Pulpit Floor Platform",
    [-6.2, 1.45, -19.5], [1.9, 0.2, 1.9],
    "material.logos.floor", [0.18, 0.16, 0.15]
))
objects.append(make_box(
    "pulpit.parapet.front", "Pulpit Parapet Front",
    [-6.2, 2.05, -18.6], [1.8, 1.0, 0.12],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "pulpit.parapet.sideL", "Pulpit Parapet Left",
    [-7.05, 2.05, -19.5], [0.12, 1.0, 1.8],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "pulpit.parapet.sideR", "Pulpit Parapet Right",
    [-5.35, 2.05, -19.5], [0.12, 1.0, 1.8],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "pulpit.lectern", "Pulpit Reading Lectern",
    [-6.2, 2.55, -18.7], [0.75, 0.08, 0.45],
    "material.logos.gold", [1.0, 0.85, 0.32],
    rot_deg=[25.0, 0.0, 0.0]
))
for si, sz in enumerate([-20.8, -21.4]):
    objects.append(make_box(
        f"pulpit.step.{si+1}", f"Pulpit Access Step {si+1}",
        [-6.2, 0.45 + si * 0.4, sz], [1.0, 0.35, 0.6],
        "material.logos.floor", [0.16, 0.15, 0.14]
    ))

# Bishop's Cathedra Throne (X = -4.5, Z = -29.5)
objects.append(make_box(
    "cathedra.seat", "Bishop's Cathedra Throne",
    [-4.5, 1.3, -29.5], [1.2, 0.7, 1.2],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "cathedra.back", "Cathedra High Gothic Backrest",
    [-4.5, 2.5, -30.0], [1.2, 2.0, 0.15],
    "material.logos.sapphire", [0.12, 0.35, 0.95]
))
objects.append(make_box(
    "cathedra.canopy", "Cathedra Spire Canopy",
    [-4.5, 3.8, -29.7], [1.4, 0.6, 1.4],
    "material.logos.gold", [1.0, 0.85, 0.32]
))

# Credence Table on Epistle side (X = +4.5, Z = -29.5)
objects.append(make_box(
    "credence.table", "Liturgical Credence Table",
    [4.5, 1.3, -29.5], [1.4, 0.7, 1.0],
    "material.logos.altar", [0.08, 0.08, 0.12]
))

# ==============================================================================
# 5. NAVE PEWS (14 Rows Lining Central Processional Aisle)
# ==============================================================================
pew_z_rows = [21.0, 16.5, 12.0, 7.5, -4.5, -9.0, -13.5]

for row_idx, pz in enumerate(pew_z_rows):
    r_id = row_idx + 1
    # --- Left Pew (Dark Oak Wood + Sapphire Kneeler + Golden Ends) ---
    objects.append(make_box(
        f"pew.L{r_id}.seat", f"Nave Pew L{r_id} Seat",
        [-4.4, 0.5, pz], [3.8, 0.08, 0.55],
        "material.logos.wood", [0.24, 0.16, 0.11]
    ))
    objects.append(make_box(
        f"pew.L{r_id}.back", f"Nave Pew L{r_id} Backrest",
        [-4.4, 0.88, pz - 0.24], [3.8, 0.7, 0.07],
        "material.logos.wood", [0.26, 0.18, 0.12]
    ))
    objects.append(make_box(
        f"pew.L{r_id}.endL", f"Pew End L{r_id} Aisle",
        [-2.45, 0.52, pz - 0.05], [0.1, 0.95, 0.65],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    objects.append(make_box(
        f"pew.L{r_id}.endR", f"Pew End L{r_id} Wall",
        [-6.35, 0.52, pz - 0.05], [0.1, 0.95, 0.65],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    objects.append(make_box(
        f"pew.L{r_id}.kneeler", f"Pew Kneeler L{r_id}",
        [-4.4, 0.12, pz + 0.32], [3.6, 0.12, 0.22],
        "material.logos.sapphire", [0.12, 0.35, 0.95]
    ))

    # --- Right Pew ---
    objects.append(make_box(
        f"pew.R{r_id}.seat", f"Nave Pew R{r_id} Seat",
        [4.4, 0.5, pz], [3.8, 0.08, 0.55],
        "material.logos.wood", [0.24, 0.16, 0.11]
    ))
    objects.append(make_box(
        f"pew.R{r_id}.back", f"Nave Pew R{r_id} Backrest",
        [4.4, 0.88, pz - 0.24], [3.8, 0.7, 0.07],
        "material.logos.wood", [0.26, 0.18, 0.12]
    ))
    objects.append(make_box(
        f"pew.R{r_id}.endL", f"Pew End R{r_id} Aisle",
        [2.45, 0.52, pz - 0.05], [0.1, 0.95, 0.65],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    objects.append(make_box(
        f"pew.R{r_id}.endR", f"Pew End R{r_id} Wall",
        [6.35, 0.52, pz - 0.05], [0.1, 0.95, 0.65],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    objects.append(make_box(
        f"pew.R{r_id}.kneeler", f"Pew Kneeler R{r_id}",
        [4.4, 0.12, pz + 0.32], [3.6, 0.12, 0.22],
        "material.logos.sapphire", [0.12, 0.35, 0.95]
    ))

# ==============================================================================
# 6. COLONNADE OF THE SEVEN JOYS — CLUSTERED GOTHIC MONOLITHS
# ==============================================================================
joy_pillars = [
    ("Logos", 432.0, 24.0, [0.12, 0.35, 0.95], "material.logos.sapphire"),
    ("Agape", 528.0, 16.0, [0.95, 0.55, 0.65], "material.logos.core"),
    ("Sophia", 639.0, 8.0, [0.15, 0.75, 0.95], "material.logos.sapphire"),
    ("Poiesis", 741.0, 0.0, [0.15, 0.85, 0.45], "material.logos.emerald"),
    ("Harmonia", 852.0, -8.0, [0.98, 0.65, 0.15], "material.logos.amber"),
    ("Koinonia", 963.0, -16.0, [0.65, 0.25, 0.95], "material.logos.amethyst"),
    ("Sabbath", 1080.0, -24.0, [0.94, 0.95, 1.0], "material.logos.core"),
]

for i, (name, freq, z, crystal_col, crystal_mat) in enumerate(joy_pillars):
    idx = i + 1
    for side_sign, side_code, side_name, px in [(-1, "L", "Left", -8.0), (1, "R", "Right", 8.0)]:
        col_id = f"pillar.{side_code}{idx}"
        
        # 1. Lower Stepped Plinth
        objects.append(make_box(
            f"{col_id}.base1", f"Pillar Base Tier I {side_code}{idx}",
            [px, 0.18, z], [2.8, 0.36, 2.8],
            "material.logos.floor", [0.14, 0.13, 0.12]
        ))
        # 2. Upper Molded Pedestal
        objects.append(make_box(
            f"{col_id}.base2", f"Pillar Base Tier II {side_code}{idx}",
            [px, 0.65, z], [2.2, 0.6, 2.2],
            "material.logos.floor", [0.18, 0.16, 0.15]
        ))
        # 3. Base Torus Molding Ring
        objects.append(make_box(
            f"{col_id}.torus_base", f"Pillar Base Torus {side_code}{idx}",
            [px, 1.02, z], [1.9, 0.16, 1.9],
            "material.logos.gold", [1.0, 0.82, 0.28]
        ))
        # 4. Central Column Core Shaft
        objects.append(make_box(
            f"{col_id}.shaft", f"Pillar Shaft {side_code}{idx} ({name})",
            [px, 10.1, z], [1.4, 18.0, 1.4],
            "material.logos.alabaster", [0.94, 0.92, 0.88]
        ))
        # 5. Clustered Gothic Colonnettes (4 Diamond-Faceted Corner Ribs)
        colonnette_offsets = [(-0.62, -0.62), (0.62, -0.62), (-0.62, 0.62), (0.62, 0.62)]
        for ci, (cx_off, cz_off) in enumerate(colonnette_offsets):
            objects.append(make_box(
                f"{col_id}.colonnette.{ci+1}", f"Colonnette {ci+1} {side_code}{idx}",
                [px + cx_off, 10.1, z + cz_off], [0.35, 18.0, 0.35],
                "material.logos.alabaster", [0.90, 0.88, 0.84],
                rot_deg=[0.0, 45.0, 0.0]
            ))
        # 6. Capital Astragal Neck Ring
        objects.append(make_box(
            f"{col_id}.astragal", f"Pillar Astragal {side_code}{idx}",
            [px, 19.18, z], [1.8, 0.16, 1.8],
            "material.logos.gold", [1.0, 0.82, 0.28]
        ))
        # 7. Carved Bell Capital
        objects.append(make_box(
            f"{col_id}.capital", f"Pillar Bell Capital {side_code}{idx}",
            [px, 19.6, z], [2.1, 0.7, 2.1],
            "material.logos.gold", [1.0, 0.82, 0.28]
        ))
        # 8. Gilded Abacus Slab
        objects.append(make_box(
            f"{col_id}.abacus", f"Pillar Abacus Slab {side_code}{idx}",
            [px, 20.1, z], [2.5, 0.32, 2.5],
            "material.logos.gold", [1.0, 0.85, 0.32]
        ))
        # 9. Sacred Resonance Crystal Sphere
        objects.append(make_sphere(
            f"{col_id}.crystal", f"Resonance Crystal {side_code}{idx} ({name} {freq:.0f}Hz)",
            [px, 21.0, z], 0.65, crystal_mat, crystal_col,
            extra_props={"frequency": {"t": "float", "v": freq}, "light.intensity": {"t": "float", "v": 3.0}}
        ))

    # ==========================================================================
    # 7. POINTED GOTHIC VAULT ARCHES (Springing Ribs & Keystones)
    # ==========================================================================
    # Left Springing Rib (Angles up 18° from X = -8m)
    objects.append(make_box(
        f"cathedral.vault.springL.{idx}", f"Pointed Arch Spring L{idx} ({name})",
        [-5.2, 21.3, z], [5.0, 1.1, 1.1],
        "material.logos.arch", [0.85, 0.82, 0.78],
        rot_deg=[0.0, 0.0, 18.0]
    ))
    # Right Springing Rib (Angles up 18° from X = +8m)
    objects.append(make_box(
        f"cathedral.vault.springR.{idx}", f"Pointed Arch Spring R{idx} ({name})",
        [5.2, 21.3, z], [5.0, 1.1, 1.1],
        "material.logos.arch", [0.85, 0.82, 0.78],
        rot_deg=[0.0, 0.0, -18.0]
    ))
    # Crown Crossbeam
    objects.append(make_box(
        f"cathedral.vault.crown.{idx}", f"Arch Crown Beam {idx}",
        [0.0, 22.3, z], [5.5, 1.0, 1.0],
        "material.logos.arch", [0.88, 0.85, 0.82]
    ))
    # Carved Keystone Boss
    objects.append(make_box(
        f"cathedral.vault.keystone.{idx}", f"Arch Keystone Boss {idx}",
        [0.0, 22.9, z], [1.8, 1.4, 1.8],
        "material.logos.gold", [1.0, 0.85, 0.32]
    ))
    # Golden Cruciform Boss Hanging from Keystone
    objects.append(make_sphere(
        f"cathedral.vault.boss.{idx}", f"Keystone Pendentive Boss {idx}",
        [0.0, 21.9, z], 0.35,
        "material.logos.gold", [1.0, 0.88, 0.35]
    ))

# Longitudinal Wall Formeret Ribs
for wi in range(len(joy_pillars) - 1):
    z1 = joy_pillars[wi][2]
    z2 = joy_pillars[wi+1][2]
    z_mid = (z1 + z2) / 2.0
    z_len = abs(z1 - z2) + 0.8
    objects.append(make_box(
        f"cathedral.formeret.N.{wi+1}", f"North Longitudinal Rib Bay {wi+1}",
        [-8.0, 20.3, z_mid], [1.1, 0.9, z_len],
        "material.logos.arch", [0.82, 0.80, 0.76]
    ))
    objects.append(make_box(
        f"cathedral.formeret.S.{wi+1}", f"South Longitudinal Rib Bay {wi+1}",
        [8.0, 20.3, z_mid], [1.1, 0.9, z_len],
        "material.logos.arch", [0.82, 0.80, 0.76]
    ))

# Continuous 68m Longitudinal Ceiling Ridge Spine
objects.append(make_box(
    "cathedral.vault.spine", "Cathedral Roof Vault Ridge Spine",
    [0.0, 23.6, 0.0], [1.4, 1.2, 68.0],
    "material.logos.gold", [1.0, 0.82, 0.28]
))

# ==============================================================================
# 7.1 GOTHIC GROIN VAULT WEBBING & ROOF ENCLOSURE
# ==============================================================================
for wi in range(len(joy_pillars) - 1):
    z1 = joy_pillars[wi][2]
    z2 = joy_pillars[wi+1][2]
    z_mid = (z1 + z2) / 2.0
    z_len = abs(z1 - z2) + 0.4
    
    # Left Nave Pitched Vault Webbing (Angles up 22.4° from x = -8m to x = 0m)
    objects.append(make_box(
        f"cathedral.vault.web.L.{wi+1}", f"Nave Vault Webbing L{wi+1}",
        [-4.0, 21.95, z_mid], [4.4, 0.28, z_len],
        "material.logos.arch", [0.86, 0.84, 0.80],
        rot_deg=[0.0, 0.0, 22.4]
    ))
    # Right Nave Pitched Vault Webbing (Angles up -22.4° from x = +8m to x = 0m)
    objects.append(make_box(
        f"cathedral.vault.web.R.{wi+1}", f"Nave Vault Webbing R{wi+1}",
        [4.0, 21.95, z_mid], [4.4, 0.28, z_len],
        "material.logos.arch", [0.86, 0.84, 0.80],
        rot_deg=[0.0, 0.0, -22.4]
    ))
    
    # Diagonal Groined Vault Ribs (Crossing X pattern meeting at keystone boss)
    diag_chord = math.sqrt(8.0**2 + (z_len/2.0)**2 + 2.5**2)
    diag_ang_y = math.degrees(math.atan2(z_len / 2.0, 8.0))
    objects.append(make_box(
        f"cathedral.vault.diag.A.{wi+1}", f"Diagonal Ogival Rib A{wi+1}",
        [-4.0, 21.6, z_mid - z_len * 0.25], [diag_chord * 0.52, 0.45, 0.45],
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, -diag_ang_y, 16.0]
    ))
    objects.append(make_box(
        f"cathedral.vault.diag.B.{wi+1}", f"Diagonal Ogival Rib B{wi+1}",
        [4.0, 21.6, z_mid - z_len * 0.25], [diag_chord * 0.52, 0.45, 0.45],
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, diag_ang_y, -16.0]
    ))
    objects.append(make_box(
        f"cathedral.vault.diag.C.{wi+1}", f"Diagonal Ogival Rib C{wi+1}",
        [-4.0, 21.6, z_mid + z_len * 0.25], [diag_chord * 0.52, 0.45, 0.45],
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, diag_ang_y, 16.0]
    ))
    objects.append(make_box(
        f"cathedral.vault.diag.D.{wi+1}", f"Diagonal Ogival Rib D{wi+1}",
        [4.0, 21.6, z_mid + z_len * 0.25], [diag_chord * 0.52, 0.45, 0.45],
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, -diag_ang_y, -16.0]
    ))

    # Aisle Vault Ceilings
    objects.append(make_box(
        f"cathedral.aisle.vault.N.{wi+1}", f"North Aisle Vault Ceiling Bay {wi+1}",
        [-11.5, 13.0, z_mid], [6.8, 0.35, z_len],
        "material.logos.arch", [0.82, 0.80, 0.76]
    ))
    objects.append(make_box(
        f"cathedral.aisle.vault.S.{wi+1}", f"South Aisle Vault Ceiling Bay {wi+1}",
        [11.5, 13.0, z_mid], [6.8, 0.35, z_len],
        "material.logos.arch", [0.82, 0.80, 0.76]
    ))

# Sanctuary Apse Semi-Dome Roof
objects.append(make_box(
    "cathedral.apse.vault.L", "Sanctuary Apse Vault Web Left",
    [-4.0, 21.95, -29.0], [4.4, 0.28, 10.0],
    "material.logos.arch", [0.86, 0.84, 0.80],
    rot_deg=[0.0, 0.0, 22.4]
))
objects.append(make_box(
    "cathedral.apse.vault.R", "Sanctuary Apse Vault Web Right",
    [4.0, 21.95, -29.0], [4.4, 0.28, 10.0],
    "material.logos.arch", [0.86, 0.84, 0.80],
    rot_deg=[0.0, 0.0, -22.4]
))
objects.append(make_box(
    "cathedral.apse.vault.back", "Sanctuary Apse Hemicycle Vault",
    [0.0, 21.95, -33.5], [8.0, 0.28, 3.2],
    "material.logos.arch", [0.86, 0.84, 0.80],
    rot_deg=[22.4, 0.0, 0.0]
))

# ==============================================================================
# 7.2 TRIFORIUM GALLERY (Continuous Gothic Blind Arcade at y = 11m)
# ==============================================================================
for wi in range(len(joy_pillars) - 1):
    z1 = joy_pillars[wi][2]
    z2 = joy_pillars[wi+1][2]
    z_mid = (z1 + z2) / 2.0
    z_len = abs(z1 - z2)
    
    for side_code, px, side_s in [("N", -8.0, -1), ("S", 8.0, 1)]:
        objects.append(make_box(
            f"cathedral.triforium.cornice.{side_code}{wi+1}", f"Triforium Cornice {side_code}{wi+1}",
            [px, 10.8, z_mid], [0.65, 0.28, z_len],
            "material.logos.gold", [1.0, 0.82, 0.28]
        ))
        for ti in range(4):
            tz = z1 + (ti + 0.5) * (z2 - z1) / 4.0
            objects.append(make_box(
                f"cathedral.triforium.col.{side_code}{wi+1}.{ti+1}", f"Triforium Colonnette {side_code}{wi+1}.{ti+1}",
                [px, 11.8, tz], [0.22, 1.7, 0.22],
                "material.logos.alabaster", [0.94, 0.92, 0.88]
            ))
            objects.append(make_box(
                f"cathedral.triforium.archlet.{side_code}{wi+1}.{ti+1}", f"Triforium Archlet {side_code}{wi+1}.{ti+1}",
                [px, 12.8, tz], [0.35, 0.35, 1.8],
                "material.logos.gold", [1.0, 0.82, 0.28]
            ))
        objects.append(make_box(
            f"cathedral.triforium.parapet.{side_code}{wi+1}", f"Triforium Parapet {side_code}{wi+1}",
            [px, 13.1, z_mid], [0.55, 0.24, z_len],
            "material.logos.arch", [0.88, 0.85, 0.82]
        ))

# ==============================================================================
# 7.3 SUSPENDED GOTHIC CORONA LUCIS (MONUMENTAL WHEEL CHANDELIERS)
# ==============================================================================
chandelier_z_positions = [-18.0, -6.0, 6.0, 18.0]

for ci, cz in enumerate(chandelier_z_positions):
    c_idx = ci + 1
    c_id = f"cathedral.chandelier.{c_idx}"
    
    # Vertical Golden Suspension Rod from Keystone Boss
    objects.append(make_box(
        f"{c_id}.rod", f"Corona Chandelier Suspension Rod {c_idx}",
        [0.0, 17.35, cz], [0.12, 11.1, 0.12],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    # Central Crown Hub
    objects.append(make_box(
        f"{c_id}.hub", f"Corona Chandelier Hub {c_idx}",
        [0.0, 11.8, cz], [0.65, 0.45, 0.65],
        "material.logos.gold", [1.0, 0.85, 0.32],
        rot_deg=[0.0, 45.0, 0.0]
    ))
    # Main Outer Ring (Analytic Torus, radius 2.2m)
    objects.append(make_torus(
        f"{c_id}.ring", f"Corona Lucis Wheel Ring {c_idx}",
        [0.0, 11.8, cz], 2.2, 0.14,
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))
    # 8 Radial Spokes & Candle Sconces with Glowing Flames
    for si in range(8):
        s_ang = 2.0 * math.pi * si / 8.0
        s_deg = math.degrees(s_ang)
        sx = 1.1 * math.cos(s_ang)
        sz = 1.1 * math.sin(s_ang)
        rx = 2.2 * math.cos(s_ang)
        rz = 2.2 * math.sin(s_ang)
        
        objects.append(make_box(
            f"{c_id}.spoke.{si+1}", f"Corona Spoke {c_idx}.{si+1}",
            [sx, 11.8, cz + sz], [2.1, 0.09, 0.09],
            "material.logos.gold", [1.0, 0.82, 0.28],
            rot_deg=[0.0, -s_deg, 0.0]
        ))
        objects.append(make_box(
            f"{c_id}.sconce.{si+1}", f"Candle Sconce {c_idx}.{si+1}",
            [rx, 12.05, cz + rz], [0.24, 0.4, 0.24],
            "material.logos.gold", [1.0, 0.85, 0.32]
        ))
        objects.append(make_box(
            f"{c_id}.taper.{si+1}", f"Chandelier Taper {c_idx}.{si+1}",
            [rx, 12.45, cz + rz], [0.08, 0.4, 0.08],
            "material.logos.alabaster", [0.94, 0.92, 0.88]
        ))
        objects.append(make_sphere(
            f"{c_id}.flame.{si+1}", f"Sacred Chandelier Flame {c_idx}.{si+1}",
            [rx, 12.75, cz + rz], 0.12,
            "material.logos.core", [1.0, 0.95, 0.75],
            extra_props={"light.intensity": {"t": "float", "v": 3.0}}
        ))

# ==============================================================================
# 7.4 MONUMENTAL WEST GALLERY PIPE ORGAN & CHOIR BALCONY
# ==============================================================================
# Choir Loft Balcony Platform
objects.append(make_box(
    "organ.loft.floor", "West Choir Gallery Balcony Floor",
    [0.0, 6.8, 28.0], [16.0, 0.45, 5.2],
    "material.logos.wood", [0.24, 0.16, 0.11]
))
for bi, bx in enumerate([-6.0, -2.0, 2.0, 6.0]):
    objects.append(make_box(
        f"organ.loft.corbel.{bi+1}", f"Balcony Corbel Bracket {bi+1}",
        [bx, 5.8, 25.6], [0.65, 1.6, 0.8],
        "material.logos.arch", [0.85, 0.82, 0.78]
    ))
objects.append(make_box(
    "organ.loft.parapet.rail", "Choir Balcony Gilded Rail",
    [0.0, 7.85, 25.4], [16.0, 0.14, 0.35],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
for pi in range(16):
    px = -7.5 + pi * 1.0
    objects.append(make_box(
        f"organ.loft.baluster.{pi+1}", f"Balcony Baluster {pi+1}",
        [px, 7.35, 25.4], [0.14, 0.85, 0.14],
        "material.logos.alabaster", [0.94, 0.92, 0.88]
    ))

# Pipe Organ Casework
objects.append(make_box(
    "organ.case.towerL", "Organ Case Pedal Tower Left",
    [-5.8, 12.0, 29.5], [3.2, 10.0, 2.2],
    "material.logos.wood", [0.22, 0.14, 0.10]
))
objects.append(make_box(
    "organ.case.pedimentL", "Pedal Tower Pediment Crown Left",
    [-5.8, 17.6, 29.5], [3.6, 1.2, 2.4],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "organ.case.towerR", "Organ Case Pedal Tower Right",
    [5.8, 12.0, 29.5], [3.2, 10.0, 2.2],
    "material.logos.wood", [0.22, 0.14, 0.10]
))
objects.append(make_box(
    "organ.case.pedimentR", "Pedal Tower Pediment Crown Right",
    [5.8, 17.6, 29.5], [3.6, 1.2, 2.4],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "organ.case.center", "Grand Orgue Central Pipe Chest",
    [0.0, 10.5, 29.8], [7.6, 7.0, 1.8],
    "material.logos.wood", [0.24, 0.16, 0.11]
))
objects.append(make_box(
    "organ.case.gable", "Central Organ Gable Spire",
    [0.0, 15.2, 29.8], [7.8, 2.4, 2.0],
    "material.logos.gold", [1.0, 0.85, 0.32]
))

# Shimmering Silver & Tin Organ Pipes
for p_idx in range(5):
    ox = -6.8 + p_idx * 0.5
    pipe_h = 6.5 + (2 - abs(p_idx - 2)) * 1.2
    objects.append(make_box(
        f"organ.pipe.pedalL.{p_idx+1}", f"Pedal Pipe L{p_idx+1}",
        [ox, 7.0 + pipe_h / 2.0, 28.3], [0.36, pipe_h, 0.36],
        "material.logos.organ_pipe", [0.85, 0.88, 0.92]
    ))
for p_idx in range(5):
    ox = 4.8 + p_idx * 0.5
    pipe_h = 6.5 + (2 - abs(p_idx - 2)) * 1.2
    objects.append(make_box(
        f"organ.pipe.pedalR.{p_idx+1}", f"Pedal Pipe R{p_idx+1}",
        [ox, 7.0 + pipe_h / 2.0, 28.3], [0.36, pipe_h, 0.36],
        "material.logos.organ_pipe", [0.85, 0.88, 0.92]
    ))
for p_idx in range(19):
    ox = -3.15 + p_idx * 0.35
    dist_center = abs(p_idx - 9)
    pipe_h = 3.2 + dist_center * 0.38
    objects.append(make_box(
        f"organ.pipe.center.{p_idx+1}", f"Grand Orgue Pipe {p_idx+1}",
        [ox, 7.2 + pipe_h / 2.0, 28.7], [0.24, pipe_h, 0.24],
        "material.logos.organ_pipe", [0.88, 0.90, 0.95]
    ))

# Organist Console & Bench
objects.append(make_box(
    "organ.console", "Organist Console Desk",
    [0.0, 7.7, 26.5], [1.8, 1.2, 1.2],
    "material.logos.wood", [0.26, 0.18, 0.12]
))
objects.append(make_box(
    "organ.bench", "Organist Wooden Bench",
    [0.0, 7.4, 25.5], [1.4, 0.65, 0.45],
    "material.logos.wood", [0.24, 0.16, 0.11]
))

# ==============================================================================
# 8. ENCLOSING OUTER MASONRY WALLS
# ==============================================================================
objects.append(make_box(
    "cathedral.wall.left", "North Clerestory Masonry Wall",
    [-15.0, 12.0, -2.0], [1.2, 24.0, 74.0],
    "material.logos.floor", [0.15, 0.16, 0.18]
))
objects.append(make_box(
    "cathedral.wall.right", "South Clerestory Masonry Wall",
    [15.0, 12.0, -2.0], [1.2, 24.0, 74.0],
    "material.logos.floor", [0.15, 0.16, 0.18]
))
objects.append(make_box(
    "cathedral.wall.apse", "Sanctuary Apse Masonry Wall",
    [0.0, 13.0, -36.5], [32.0, 26.0, 1.2],
    "material.logos.floor", [0.14, 0.15, 0.17]
))
objects.append(make_box(
    "cathedral.wall.narthex", "Narthex Entrance Masonry Wall",
    [0.0, 12.0, 34.5], [32.0, 24.0, 1.2],
    "material.logos.floor", [0.14, 0.15, 0.17]
))

# ==============================================================================
# 9. STAINED-GLASS LANCET WINDOWS & STONE TRACERIES
# ==============================================================================
window_configs = [
    ("window.stained.L1", "Sapphire Logos", [-14.2, 14.0, 18.0], [0.12, 0.35, 0.95], -1),
    ("window.stained.L2", "Emerald Poiesis", [-14.2, 14.0, 6.0], [0.15, 0.85, 0.45], -1),
    ("window.stained.L3", "Topaz Harmonia", [-14.2, 14.0, -6.0], [0.98, 0.65, 0.15], -1),
    ("window.stained.L4", "Amethyst Koinonia", [-14.2, 14.0, -18.0], [0.65, 0.25, 0.95], -1),
    ("window.stained.R1", "Ruby Agape", [14.2, 14.0, 18.0], [0.95, 0.22, 0.32], 1),
    ("window.stained.R2", "Cyan Sophia", [14.2, 14.0, 6.0], [0.15, 0.75, 0.95], 1),
    ("window.stained.R3", "Solar Lux", [14.2, 14.0, -6.0], [1.0, 0.95, 0.75], 1),
    ("window.stained.R4", "Pearl Sabbath", [14.2, 14.0, -18.0], [0.94, 0.95, 1.0], 1),
]

for wid, wname, wpos, wcol, side_s in window_configs:
    # 1. Glowing Stained Glass Aperture
    objects.append(make_box(
        wid, f"Stained Glass: {wname}",
        wpos, [0.15, 9.5, 3.4], "material.logos.core", wcol,
        extra_props={"light.intensity": {"t": "float", "v": 2.5}, "light.source": {"t": "bool", "v": True}}
    ))
    # 2. Stone Window Sill
    objects.append(make_box(
        f"{wid}.sill", f"Stone Sill: {wname}",
        [wpos[0] + side_s * 0.25, wpos[1] - 5.0, wpos[2]], [0.65, 0.5, 3.8],
        "material.logos.floor", [0.18, 0.17, 0.16]
    ))
    # 3. Central Vertical Stone Mullion
    objects.append(make_box(
        f"{wid}.mullion", f"Window Mullion: {wname}",
        [wpos[0] + side_s * 0.08, wpos[1], wpos[2]], [0.2, 9.5, 0.25],
        "material.logos.arch", [0.85, 0.82, 0.78]
    ))
    # 4. Pointed Arch Hood Moulding Cap
    objects.append(make_box(
        f"{wid}.hood", f"Window Hood Moulding: {wname}",
        [wpos[0] + side_s * 0.15, wpos[1] + 5.1, wpos[2]], [0.45, 0.8, 3.8],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))

# ==============================================================================
# 10. MONUMENTAL TRANSEPT ROSE WINDOWS WITH PHYSICAL TRACERIES
# ==============================================================================
for r_side, rx, r_rot_y in [("N", -14.2, 90.0), ("S", 14.2, -90.0)]:
    rw_id = f"cathedral.rose.{r_side}"
    # Analytic Torus Beings (renderMode: 2)
    objects.append(make_torus(
        f"{rw_id}.outer_ring", f"Transept Rose Window Ring {r_side}",
        [rx, 14.0, 0.0], 4.2, 0.25,
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, r_rot_y, 0.0]
    ))
    objects.append(make_torus(
        f"{rw_id}.inner_ring", f"Rose Window Rosette {r_side}",
        [rx, 14.0, 0.0], 2.0, 0.18,
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, r_rot_y, 0.0]
    ))
    # Physical Segmented Outer Frame Ring (guaranteed 100% visible)
    for seg in make_ring_segments(f"{rw_id}.phys_outer", f"Rose Outer {r_side}", [rx, 14.0, 0.0], 4.2, 0.32, "material.logos.gold", [1.0, 0.82, 0.28], 20, 'yz'):
        objects.append(seg)
    # Physical Segmented Inner Rosette Ring
    for seg in make_ring_segments(f"{rw_id}.phys_inner", f"Rose Inner {r_side}", [rx, 14.0, 0.0], 2.0, 0.24, "material.logos.gold", [1.0, 0.85, 0.32], 16, 'yz'):
        objects.append(seg)
    # Central Radiant Hub
    objects.append(make_sphere(
        f"{rw_id}.hub", f"Rose Window Radiant Hub {r_side}",
        [rx, 14.0, 0.0], 0.85,
        "material.logos.core", [1.0, 0.95, 0.75],
        extra_props={"light.intensity": {"t": "float", "v": 3.5}, "light.source": {"t": "bool", "v": True}}
    ))
    # 8 Radial Stone Tracery Spokes
    for sp_i in range(8):
        ang_deg = sp_i * 45.0
        objects.append(make_box(
            f"{rw_id}.spoke.{sp_i+1}", f"Rose Tracery Spoke {sp_i+1} {r_side}",
            [rx, 14.0, 0.0], [0.18, 0.18, 8.4],
            "material.logos.arch", [0.85, 0.82, 0.78],
            rot_deg=[ang_deg, r_rot_y, 0.0]
        ))

# ==============================================================================
# 11. CENTRAL CROSSING: RESONATING HEART OF LOGOS & CELESTIAL ORBITAL RINGS
# ==============================================================================
# Living Logos Singularity (Smooth-Union Celestial Manifold, y = 7.5m, Z = 0m)
core_sphere = sdf_leaf(0, [1.1, 1.1, 1.1])
ring_equator = sdf_leaf(6, [2.0, 0.22, 0.0])
ring_tilted = sdf_leaf(6, [2.4, 0.16, 0.0])
lobe_north = sdf_leaf(0, [0.38, 0.38, 0.38], offset=[0.0, 0.0, -2.1])
lobe_south = sdf_leaf(0, [0.38, 0.38, 0.38], offset=[0.0, 0.0, 2.1])
lobe_east = sdf_leaf(0, [0.38, 0.38, 0.38], offset=[2.1, 0.0, 0.0])
lobe_west = sdf_leaf(0, [0.38, 0.38, 0.38], offset=[-2.1, 0.0, 0.0])
pole_zenith = sdf_leaf(3, [0.28, 1.1, 0.28], offset=[0.0, 1.6, 0.0])
pole_nadir = sdf_leaf(3, [0.28, 1.1, 0.28], offset=[0.0, -1.6, 0.0])

poles = sdf_binary(5, pole_zenith, pole_nadir, 0.25)
ew_lobes = sdf_binary(5, lobe_east, lobe_west, 0.25)
ns_lobes = sdf_binary(5, lobe_north, lobe_south, 0.25)
cardinal_lobes = sdf_binary(5, ew_lobes, ns_lobes, 0.25)
rings = sdf_binary(5, ring_equator, ring_tilted, 0.3)
outer_halo = sdf_binary(5, rings, cardinal_lobes, 0.3)
singularity_tree = sdf_binary(5, core_sphere, sdf_binary(5, outer_halo, poles, 0.28), 0.36)

objects.append(make_field(
    "logos.resonator.core", "Heart of Logos (Living Singularity Core)",
    [0.0, 7.5, 0.0], singularity_tree, [3.2, 3.2, 3.2],
    "material.logos.core", [1.0, 0.95, 0.75],
    extra_props={
        "isResonatorCore": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 6.5},
        "light.source": {"t": "bool", "v": True},
        "pulseRate": {"t": "float", "v": 1.618},
        "resonancePitch": {"t": "float", "v": 432.0}
    }
))

# 1. Analytic Torus Beings (renderMode: 2 for full polygon mesh rendering)
objects.append(make_torus(
    "logos.resonator.ring_alpha", "Celestial Orbital Ring Alpha (Analytic)",
    [0.0, 7.5, 0.0], 3.5, 0.16, "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_torus(
    "logos.resonator.ring_beta", "Celestial Orbital Ring Beta (Analytic)",
    [0.0, 7.5, 0.0], 5.0, 0.14, "material.logos.sapphire", [0.12, 0.35, 0.95],
    rot_deg=[28.0, 0.0, 0.0]
))
objects.append(make_torus(
    "logos.resonator.ring_gamma", "Celestial Orbital Ring Gamma (Analytic)",
    [0.0, 7.5, 0.0], 6.5, 0.12, "material.logos.amber", [0.98, 0.65, 0.15],
    rot_deg=[-28.0, 0.0, 0.0]
))

# 2. Tangible Physical 3D Armillary Rings (Faceted segments + celestial crystal nodes)
# Ring Alpha (Horizontal Equatorial, Radius 3.5m, 24 Golden Segments)
for seg in make_ring_segments("logos.armillary.alpha", "Armillary Ring Alpha", [0.0, 7.5, 0.0], 3.5, 0.22, "material.logos.gold", [1.0, 0.82, 0.28], 24, 'xz'):
    objects.append(seg)
# 8 Celestial Nodes on Ring Alpha
for ni in range(8):
    nang = 2.0 * math.pi * ni / 8.0
    nx = 3.5 * math.cos(nang)
    nz = 3.5 * math.sin(nang)
    objects.append(make_sphere(
        f"logos.armillary.alpha.node.{ni+1}", f"Celestial Node Alpha {ni+1}",
        [nx, 7.5, nz], 0.28, "material.logos.core", [1.0, 0.95, 0.75],
        extra_props={"light.intensity": {"t": "float", "v": 1.8}}
    ))

# Ring Beta (Tilted 28° around X, Radius 5.0m, 28 Sapphire/Gold Segments)
for seg in make_ring_segments("logos.armillary.beta", "Armillary Ring Beta", [0.0, 7.5, 0.0], 5.0, 0.20, "material.logos.sapphire", [0.12, 0.35, 0.95], 28, 'xz', [28.0, 0.0, 0.0]):
    objects.append(seg)
for ni in range(8):
    nang = 2.0 * math.pi * ni / 8.0
    nx = 5.0 * math.cos(nang)
    nz_base = 5.0 * math.sin(nang)
    ny = 7.5 + nz_base * math.sin(math.radians(28.0))
    nz = nz_base * math.cos(math.radians(28.0))
    objects.append(make_sphere(
        f"logos.armillary.beta.node.{ni+1}", f"Celestial Node Beta {ni+1}",
        [nx, ny, nz], 0.25, "material.logos.core", [0.15, 0.75, 0.95],
        extra_props={"light.intensity": {"t": "float", "v": 1.6}}
    ))

# Ring Gamma (Tilted -28° around X, Radius 6.5m, 32 Amber/Gold Segments)
for seg in make_ring_segments("logos.armillary.gamma", "Armillary Ring Gamma", [0.0, 7.5, 0.0], 6.5, 0.18, "material.logos.amber", [0.98, 0.65, 0.15], 32, 'xz', [-28.0, 0.0, 0.0]):
    objects.append(seg)
for ni in range(8):
    nang = 2.0 * math.pi * ni / 8.0
    nx = 6.5 * math.cos(nang)
    nz_base = 6.5 * math.sin(nang)
    ny = 7.5 - nz_base * math.sin(math.radians(28.0))
    nz = nz_base * math.cos(math.radians(28.0))
    objects.append(make_sphere(
        f"logos.armillary.gamma.node.{ni+1}", f"Celestial Node Gamma {ni+1}",
        [nx, ny, nz], 0.22, "material.logos.core", [0.98, 0.65, 0.15],
        extra_props={"light.intensity": {"t": "float", "v": 1.6}}
    ))

# ==============================================================================
# 12. STATE MANAGER BEING
# ==============================================================================
objects.append({
    "objectID": "state.logos",
    "shapeKind": 12,
    "geometryType": 12,
    "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    "shape": {
        "kind": 12,
        "params": {
            "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
            "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
            "ovoidAsym": 0.0, "fillet": 0.0,
            "width2D": 0.0, "height2D": 0.0
        }
    },
    "transform": mat4([0, 0, 0]),
    "center": [0, 0, 0],
    "authoredProperties": {
        "breathActive": {"t": "bool", "v": True},
        "pulseRate": {"t": "float", "v": 1.618},
        "resonanceFreq": {"t": "float", "v": 432.0},
        "covenantCount": {"t": "float", "v": 1.0},
        "luxActive": {"t": "bool", "v": True},
        "season": {"t": "string", "v": "Genesis Dawn"}
    }
})

# ==============================================================================
# 13. CLEAN DOCKED 2D LITURGICAL HUD
# ==============================================================================
hud_bg = {
    "objectID": "hud.logos.dock",
    "shapeKind": 12,
    "geometryType": 12,
    "shapeParams": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 360.0, 195.0],
    "shape": {
        "kind": 12,
        "params": {
            "r": 0.0, "ry": 0.0, "rz": 0.0, "halfH": 0.0,
            "majorR": 0.0, "minorR": 0.0, "paraboloidA": 0.0,
            "ovoidAsym": 0.0, "fillet": 0.0,
            "width2D": 360.0, "height2D": 195.0
        }
    },
    "x2D": 20.0,
    "y2D": 20.0,
    "zOrder2D": 10,
    "transform": mat4([0, 0, 0]),
    "center": [0, 0, 0],
    "faceColors": [[0.05, 0.07, 0.12]] * 6,
    "authoredProperties": {}
}
objects.append(hud_bg)

objects.append(make_label2d("hud.logos.title", "CATHEDRAL OF THE LIVING LOGOS", 35, 34, 16.0, [1.0, 0.85, 0.35]))
objects.append(make_label2d("hud.logos.telemetry.freq", "FREQUENCY: 432.0 Hz (SACRED ROOT)", 35, 56, 13.0, [0.35, 0.85, 0.95]))
objects.append(make_label2d("hud.logos.telemetry.breath", "BREATH: RESPIRING (0.1 Hz PNEUMA WAVE)", 35, 76, 13.0, [0.45, 0.95, 0.65]))
objects.append(make_label2d("hud.logos.telemetry.season", "LITURGY: GENESIS DAWN", 35, 96, 13.0, [0.95, 0.75, 0.45]))

objects.append(make_button2d("hud.btn.pneuma", "BREATHE PNEUMA", 35, 122, 155, 32, [0.15, 0.45, 0.85]))
objects.append(make_button2d("hud.btn.lux", "FIAT LUX", 205, 122, 155, 32, [0.92, 0.78, 0.25]))
objects.append(make_button2d("hud.btn.chord", "SOUND CANON", 35, 160, 155, 32, [0.92, 0.55, 0.15]))
objects.append(make_button2d("hud.btn.season", "CYCLE SEASON", 205, 160, 155, 32, [0.75, 0.25, 0.85]))

# ==============================================================================
# 12. TRANSCENDENT SDF MANIFOLDS & SACRED GEOMETRY SHOWCASE (ULTRA-DETAILED)
# ==============================================================================

# 1. The North Transept Spherical Gyroid Armillary Reliquary of Pneuma
# Bounded by outer sphere via CSG Intersect (Op 3), subtracted by inner sphere (Op 4)
# with floating Sapphire Heart jewel in the hollow interior!
raw_gyroid = sdf_expr("cos(2.8*x)*sin(2.8*y) + cos(2.8*y)*sin(2.8*z) + cos(2.8*z)*sin(2.8*x) - 0.22", dims=[1.4, 1.4, 1.4])
bound_sphere = sdf_leaf(0, [1.25, 1.25, 1.25])
spherical_gyroid = sdf_binary(3, raw_gyroid, bound_sphere) # Op 3 = Intersect!
inner_cavity = sdf_leaf(0, [0.42, 0.42, 0.42])
hollow_gyroid = sdf_binary(4, spherical_gyroid, inner_cavity) # Op 4 = Subtract!
sapphire_heart = sdf_leaf(0, [0.22, 0.22, 0.22]) # Floating core jewel
gyroid_core = sdf_binary(5, hollow_gyroid, sapphire_heart, 0.15)

top_finial_stem = sdf_leaf(5, [0.12, 0.35, 0.0], offset=[0.0, 1.35, 0.0]) # Cone
top_finial_cross = sdf_leaf(1, [0.24, 0.06, 0.06], offset=[0.0, 1.45, 0.0]) # Crossbar
top_finial = sdf_binary(5, top_finial_stem, top_finial_cross, 0.12)
bot_socket = sdf_leaf(5, [0.36, 0.25, 0.0], offset=[0.0, -1.25, 0.0]) # Cone socket
gyroid_tree = sdf_binary(5, gyroid_core, sdf_binary(5, top_finial, bot_socket, 0.18), 0.22)

# Gyroid Altar Pedestal with Alabaster & Gold moldings
objects.append(make_box(
    "cathedral.gyroid.altar.base", "North Shrine Altar Plinth",
    [-14.0, 0.45, 0.0], [2.6, 0.9, 2.6],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "cathedral.gyroid.altar.mensa", "North Shrine Altar Mensa",
    [-14.0, 1.05, 0.0], [3.0, 0.3, 3.0],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
# Triple Armillary Gimbal Rings
objects.append(make_torus(
    "cathedral.sdf.gyroid.gimbal.equator", "Gyroid Equatorial Gimbal Ring",
    [-14.0, 3.4, 0.0], 1.55, 0.07,
    "material.logos.gold", [1.0, 0.82, 0.28],
    rot_deg=[45.0, 30.0, 0.0]
))
objects.append(make_torus(
    "cathedral.sdf.gyroid.gimbal.meridian", "Gyroid Polar Meridian Ring",
    [-14.0, 3.4, 0.0], 1.45, 0.06,
    "material.logos.gold", [1.0, 0.82, 0.28],
    rot_deg=[-45.0, 60.0, 0.0]
))
objects.append(make_field(
    "cathedral.sdf.gyroid_north", "Sacred Spherical Gyroid Armillary of Pneuma",
    [-14.0, 3.4, 0.0], gyroid_tree, [1.5, 1.6, 1.5],
    "material.logos.sapphire", [0.12, 0.35, 0.95],
    extra_props={
        "isSacredRelic": {"t": "bool", "v": True},
        "relicKind": {"t": "string", "v": "Hollow Spherical Gyroid Armillary"},
        "light.intensity": {"t": "float", "v": 3.8}
    }
))

# 2. The South Transept 24-Pointed Stellated Merkaba Star of Sophia
star_bar_x = sdf_leaf(2, [1.1, 0.24, 0.24], p0=0.08)
star_bar_y = sdf_leaf(2, [0.24, 1.1, 0.24], p0=0.08)
star_bar_z = sdf_leaf(2, [0.24, 0.24, 1.1], p0=0.08)
star_bars = sdf_binary(5, star_bar_x, sdf_binary(5, star_bar_y, star_bar_z, 0.18), 0.18)

# 8 diagonal tetrahedral star points
star_pts = []
for sx in [-0.55, 0.55]:
    for sy in [-0.55, 0.55]:
        for sz in [-0.55, 0.55]:
            star_pts.append(sdf_leaf(5, [0.18, 0.45, 0.0], offset=[sx, sy, sz]))
diag_tree = star_pts[0]
for pt in star_pts[1:]:
    diag_tree = sdf_binary(5, diag_tree, pt, 0.14)

star_core = sdf_leaf(0, [0.65, 0.65, 0.65])
inner_ruby_heart = sdf_leaf(3, [0.28, 0.38, 0.28])
star_compound = sdf_binary(5, star_bars, diag_tree, 0.2)
star_tree = sdf_binary(5, star_compound, sdf_binary(5, star_core, inner_ruby_heart, 0.15), 0.22)

# South Shrine Altar Pedestal
objects.append(make_box(
    "cathedral.merkaba.altar.base", "South Shrine Altar Plinth",
    [14.0, 0.45, 0.0], [2.6, 0.9, 2.6],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "cathedral.merkaba.altar.mensa", "South Shrine Altar Mensa",
    [14.0, 1.05, 0.0], [3.0, 0.3, 3.0],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
# Dual Merkaba Gimbal Rings with Zodiac Nodes
objects.append(make_torus(
    "cathedral.sdf.merkaba.gimbal.1", "Merkaba Reliquary Outer Gimbal",
    [14.0, 3.4, 0.0], 1.55, 0.07,
    "material.logos.gold", [1.0, 0.82, 0.28],
    rot_deg=[-45.0, -30.0, 0.0]
))
objects.append(make_torus(
    "cathedral.sdf.merkaba.gimbal.2", "Merkaba Reliquary Inner Gimbal",
    [14.0, 3.4, 0.0], 1.42, 0.06,
    "material.logos.gold", [1.0, 0.82, 0.28],
    rot_deg=[30.0, 60.0, 0.0]
))
objects.append(make_field(
    "cathedral.sdf.merkaba_south", "Stellated 24-Pointed Merkaba Star of Sophia",
    [14.0, 3.4, 0.0], star_tree, [1.5, 1.5, 1.5],
    "material.logos.ruby", [0.88, 0.12, 0.22],
    extra_props={
        "isSacredRelic": {"t": "bool", "v": True},
        "relicKind": {"t": "string", "v": "24-Pointed Stellated Merkaba"},
        "light.intensity": {"t": "float", "v": 3.8}
    }
))

# 3. The Baptismal Font of Living Waters (Crossing Centerpiece)
# 3-Tier Stepped Octagonal Plinth underneath the font
objects.append(make_box(
    "cathedral.font.plinth.tier1", "Baptismal Font Plinth Tier I",
    [0.0, 0.1, 0.0], [3.4, 0.2, 3.4],
    "material.logos.floor", [0.12, 0.13, 0.15]
))
objects.append(make_box(
    "cathedral.font.plinth.tier2", "Baptismal Font Plinth Tier II",
    [0.0, 0.3, 0.0], [2.8, 0.2, 2.8],
    "material.logos.floor", [0.15, 0.16, 0.18]
))
objects.append(make_box(
    "cathedral.font.plinth.tier3", "Baptismal Font Plinth Tier III",
    [0.0, 0.5, 0.0], [2.2, 0.2, 2.2],
    "material.logos.gold", [1.0, 0.82, 0.28]
))

# Carved Alabaster Font Basin (Sculpted octagonal bowl with 8 perimeter lobes & cavity)
font_basin_outer = sdf_leaf(2, [1.35, 0.48, 1.35], p0=0.22)
font_lobe_x1 = sdf_leaf(3, [0.35, 0.42, 1.25], offset=[1.15, 0.0, 0.0])
font_lobe_x2 = sdf_leaf(3, [0.35, 0.42, 1.25], offset=[-1.15, 0.0, 0.0])
font_lobe_z1 = sdf_leaf(3, [1.25, 0.42, 0.35], offset=[0.0, 0.0, 1.15])
font_lobe_z2 = sdf_leaf(3, [1.25, 0.42, 0.35], offset=[0.0, 0.0, -1.15])
font_lobes = sdf_binary(5, sdf_binary(5, font_lobe_x1, font_lobe_x2, 0.18), sdf_binary(5, font_lobe_z1, font_lobe_z2, 0.18), 0.2)
font_sculpted = sdf_binary(5, font_basin_outer, font_lobes, 0.22)
font_basin_cavity = sdf_leaf(0, [1.1, 1.1, 1.1], offset=[0.0, 0.34, 0.0])
font_basin_tree = sdf_binary(4, font_sculpted, font_basin_cavity)
objects.append(make_field(
    "cathedral.sdf.font_basin", "Baptismal Font Alabaster Basin",
    [0.0, 0.95, 0.0], font_basin_tree, [1.7, 1.0, 1.7],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))

# Living Water Fountain Plume (Crystalline Cyan water with multi-stage droplet crown)
water_jet = sdf_leaf(5, [0.36, 0.55, 0.0], offset=[0.0, -0.1, 0.0])
water_bell = sdf_leaf(5, [0.55, 0.25, 0.0], offset=[0.0, 0.32, 0.0]) # Expanding water sheet
water_pearl = sdf_leaf(0, [0.22, 0.22, 0.22], offset=[0.0, 0.52, 0.0])
water_drop_e = sdf_leaf(0, [0.11, 0.11, 0.11], offset=[0.38, 0.38, 0.0])
water_drop_w = sdf_leaf(0, [0.11, 0.11, 0.11], offset=[-0.38, 0.38, 0.0])
water_drop_n = sdf_leaf(0, [0.11, 0.11, 0.11], offset=[0.0, 0.38, -0.38])
water_drop_s = sdf_leaf(0, [0.11, 0.11, 0.11], offset=[0.0, 0.38, 0.38])
water_drops = sdf_binary(5, sdf_binary(5, water_drop_e, water_drop_w, 0.15), sdf_binary(5, water_drop_n, water_drop_s, 0.15), 0.15)
water_crown = sdf_binary(5, water_pearl, water_drops, 0.18)
water_tree = sdf_binary(5, sdf_binary(5, water_jet, water_bell, 0.18), water_crown, 0.22)
objects.append(make_field(
    "cathedral.sdf.font_water", "Living Water Fountain Plume",
    [0.0, 1.5, 0.0], water_tree, [1.3, 1.3, 1.3],
    "material.logos.cyan", [0.15, 0.85, 0.95],
    extra_props={"fluid": {"t": "bool", "v": True}, "light.intensity": {"t": "float", "v": 3.0}}
))

# 4. The High Altar Monstrance & 16-Ray Solar Sunburst
monstrance_cup = sdf_leaf(4, [0.42, 0.52, 0.0], offset=[0.0, 0.18, 0.0])
monstrance_stem = sdf_leaf(5, [0.18, 0.45, 0.0], offset=[0.0, 0.0, 0.0])
monstrance_halo = sdf_leaf(6, [0.82, 0.07, 0.0], offset=[0.0, 0.85, 0.0])
monstrance_halo2 = sdf_leaf(6, [0.65, 0.05, 0.0], offset=[0.0, 0.85, 0.0])
monstrance_host = sdf_leaf(0, [0.26, 0.26, 0.26], offset=[0.0, 0.85, 0.0])

# 8 Cardinal spear rays + 8 alternating flame rays
rays = []
for i in range(8):
    ang = i * (math.pi / 4.0)
    rx = math.cos(ang) * 0.95
    ry = math.sin(ang) * 0.95
    rays.append(sdf_leaf(5, [0.06, 0.38, 0.0], offset=[rx, 0.85 + ry, 0.0]))
ray_tree = rays[0]
for r in rays[1:]:
    ray_tree = sdf_binary(5, ray_tree, r, 0.12)

monstrance_sunburst = sdf_binary(5, sdf_binary(5, monstrance_halo, monstrance_halo2, 0.12), ray_tree, 0.15)
monstrance_center = sdf_binary(5, monstrance_host, monstrance_sunburst, 0.18)
monstrance_tree = sdf_binary(5, sdf_binary(5, monstrance_stem, monstrance_cup, 0.18), monstrance_center, 0.22)
objects.append(make_field(
    "cathedral.sdf.altar_monstrance", "High Altar 16-Ray Solar Monstrance",
    [0.0, 3.2, -29.5], monstrance_tree, [1.4, 1.6, 1.4],
    "material.logos.gold", [1.0, 0.82, 0.28],
    extra_props={"light.intensity": {"t": "float", "v": 3.2}}
))

# Sacred Holy Grail Chalice on Altar
grail_foot = sdf_leaf(2, [0.35, 0.08, 0.35], p0=0.08, offset=[0.0, 0.04, 0.0])
grail_stem = sdf_leaf(4, [0.12, 0.35, 0.0], offset=[0.0, 0.38, 0.0])
grail_cup_out = sdf_leaf(5, [0.4, 0.35, 0.0], offset=[0.0, 0.72, 0.0])
grail_cup_in = sdf_leaf(0, [0.32, 0.32, 0.32], offset=[0.0, 0.78, 0.0])
grail_cup = sdf_binary(4, grail_cup_out, grail_cup_in)
grail_communion = sdf_leaf(0, [0.14, 0.14, 0.14], offset=[0.0, 0.82, 0.0])
grail_tree = sdf_binary(5, grail_foot, sdf_binary(5, grail_stem, sdf_binary(5, grail_cup, grail_communion, 0.15), 0.2), 0.22)
objects.append(make_field(
    "cathedral.sdf.holy_grail", "Chalice of the Living Logos",
    [0.0, 2.35, -28.9], grail_tree, [0.9, 1.1, 0.9],
    "material.logos.gold", [1.0, 0.82, 0.28],
    extra_props={"isHolyGrail": {"t": "bool", "v": True}, "light.intensity": {"t": "float", "v": 2.2}}
))

# 5. Biblical Ultra Six-Winged Seraphim Guardians with Layered Feather Blades
def make_ultra_six_winged_seraph(seraph_id, name, pos, flip_x=False):
    sign = -1.0 if flip_x else 1.0
    robe_base = sdf_leaf(5, [0.46, 1.1, 0.0], offset=[0.0, -0.3, 0.0])
    fold_f = sdf_leaf(3, [0.18, 0.95, 0.14], offset=[0.0, -0.3, 0.22])
    fold_l = sdf_leaf(3, [0.14, 0.95, 0.18], offset=[-0.24 * sign, -0.3, 0.1])
    fold_r = sdf_leaf(3, [0.14, 0.95, 0.18], offset=[0.24 * sign, -0.3, 0.1])
    robe = sdf_binary(5, robe_base, sdf_binary(5, fold_f, sdf_binary(5, fold_l, fold_r, 0.15), 0.15), 0.2)
    
    head = sdf_leaf(0, [0.28, 0.28, 0.28], offset=[0.0, 0.88, 0.0])
    crown = sdf_leaf(4, [0.22, 0.12, 0.0], offset=[0.0, 1.14, 0.0])
    halo_in = sdf_leaf(6, [0.42, 0.04, 0.0], offset=[0.0, 1.02, -0.05])
    halo_out = sdf_leaf(6, [0.55, 0.03, 0.0], offset=[0.0, 1.02, -0.05])
    head_complex = sdf_binary(5, sdf_binary(5, head, crown, 0.12), sdf_binary(5, halo_in, halo_out, 0.1), 0.16)
    
    hands = sdf_leaf(3, [0.12, 0.15, 0.18], offset=[0.0, 0.38, 0.25])
    heart_gem = sdf_leaf(0, [0.12, 0.12, 0.12], offset=[0.0, 0.38, 0.32])
    chest = sdf_binary(5, hands, heart_gem, 0.1)
    
    # 6 wings with multi-feather blade geometry
    u_blade1_l = sdf_leaf(3, [0.10, 0.75, 0.28], offset=[-0.28 * sign, 1.25, -0.15])
    u_blade2_l = sdf_leaf(3, [0.08, 0.65, 0.22], offset=[-0.42 * sign, 1.42, -0.18])
    u_wing_l = sdf_binary(5, u_blade1_l, u_blade2_l, 0.15)
    u_blade1_r = sdf_leaf(3, [0.10, 0.75, 0.28], offset=[0.28 * sign, 1.25, -0.15])
    u_blade2_r = sdf_leaf(3, [0.08, 0.65, 0.22], offset=[0.42 * sign, 1.42, -0.18])
    u_wing_r = sdf_binary(5, u_blade1_r, u_blade2_r, 0.15)
    upper_wings = sdf_binary(5, u_wing_l, u_wing_r, 0.2)
    
    m_blade1_l = sdf_leaf(3, [0.12, 0.38, 0.85], offset=[-0.65 * sign, 0.48, -0.22])
    m_blade2_l = sdf_leaf(3, [0.09, 0.28, 0.72], offset=[-0.95 * sign, 0.55, -0.28])
    m_wing_l = sdf_binary(5, m_blade1_l, m_blade2_l, 0.15)
    m_blade1_r = sdf_leaf(3, [0.12, 0.38, 0.85], offset=[0.65 * sign, 0.48, -0.22])
    m_blade2_r = sdf_leaf(3, [0.09, 0.28, 0.72], offset=[0.95 * sign, 0.55, -0.28])
    m_wing_r = sdf_binary(5, m_blade1_r, m_blade2_r, 0.15)
    mid_wings = sdf_binary(5, m_wing_l, m_wing_r, 0.2)
    
    d_wing_l = sdf_leaf(3, [0.11, 0.85, 0.32], offset=[-0.38 * sign, -0.32, -0.12])
    d_wing_r = sdf_leaf(3, [0.11, 0.85, 0.32], offset=[0.38 * sign, -0.32, -0.12])
    lower_wings = sdf_binary(5, d_wing_l, d_wing_r, 0.2)
    
    wings = sdf_binary(5, upper_wings, sdf_binary(5, mid_wings, lower_wings, 0.22), 0.25)
    seraph_full = sdf_binary(5, robe, sdf_binary(5, head_complex, sdf_binary(5, chest, wings, 0.2), 0.25), 0.28)
    
    return make_field(
        seraph_id, name,
        pos, seraph_full, [2.0, 2.4, 2.0],
        "material.logos.gold", [1.0, 0.82, 0.28],
        rot_deg=[0.0, 20.0 * sign, 0.0]
    )

# Seraphim Carved Marble Plinths
objects.append(make_box(
    "cathedral.seraph.plinth.L", "Sanctuary Seraph Plinth North",
    [-3.8, 1.5, -29.5], [1.6, 1.5, 1.6],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "cathedral.seraph.plinth.R", "Sanctuary Seraph Plinth South",
    [3.8, 1.5, -29.5], [1.6, 1.5, 1.6],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_ultra_six_winged_seraph("cathedral.sdf.seraph.left", "Six-Winged Seraph Guardian (North)", [-3.8, 3.8, -29.5], False))
objects.append(make_ultra_six_winged_seraph("cathedral.sdf.seraph.right", "Six-Winged Seraph Guardian (South)", [3.8, 3.8, -29.5], True))

# 6. The West Portal Monolith of Genesis & Amethyst Prism
portal_block = sdf_leaf(2, [2.4, 3.5, 0.42], p0=0.18)
portal_hollow = sdf_leaf(4, [1.35, 0.55, 0.0], offset=[0.0, -0.2, 0.0])
portal_tree = sdf_binary(4, portal_block, portal_hollow)
objects.append(make_field(
    "cathedral.sdf.portal_monolith", "West Portal Genesis Monolith",
    [0.0, 4.2, 29.5], portal_tree, [2.8, 4.0, 1.0],
    "material.logos.arch", [0.10, 0.11, 0.14]
))
objects.append(make_field(
    "cathedral.sdf.portal_crystal", "Genesis Vesica Amethyst Prism",
    [0.0, 4.0, 29.5], sdf_leaf(3, [0.36, 0.82, 0.22]), [1.0, 1.2, 1.0],
    "material.logos.amethyst", [0.65, 0.25, 0.95],
    extra_props={"light.intensity": {"t": "float", "v": 2.5}}
))

# 7. Chancel Sacred Brazier Flames (North & South)
for bx, side_name in [(-4.2, "North"), (4.2, "South")]:
    flame_sphere = sdf_leaf(0, [0.26, 0.26, 0.26])
    flame_tip = sdf_leaf(5, [0.22, 0.44, 0.0], offset=[0.0, 0.26, 0.0])
    flame_tree = sdf_binary(5, flame_sphere, flame_tip, 0.24)
    objects.append(make_field(
        f"cathedral.sdf.brazier.flame.{side_name.lower()[0]}", f"Eternal Flame of {side_name} Chancel Brazier",
        [bx, 6.4, -18.0], flame_tree, [0.6, 0.8, 0.6],
        "material.logos.core", [1.0, 0.95, 0.75],
        extra_props={
            "isEternalFlame": {"t": "bool", "v": True},
            "light.intensity": {"t": "float", "v": 3.8},
            "light.source": {"t": "bool", "v": True}
        }
    ))

# 8. NEW: The Ophanim Celestial Gyroscope (The Chariot of Ezekiel)
# Suspended high above the Sanctuary Altar at [0.0, 11.2, -29.5]
oph_ring_xy = sdf_leaf(6, [2.1, 0.08, 0.0])
oph_ring_yz = sdf_expr("sqrt((sqrt(y*y + z*z) - 1.95)*(sqrt(y*y + z*z) - 1.95) + x*x) - 0.08", dims=[2.4, 2.4, 2.4])
oph_ring_xz = sdf_expr("sqrt((sqrt(x*x + z*z) - 1.70)*(sqrt(x*x + z*z) - 1.70) + y*y) - 0.07", dims=[2.2, 2.2, 2.2])
oph_rings = sdf_binary(5, oph_ring_xy, sdf_binary(5, oph_ring_yz, oph_ring_xz, 0.15), 0.18)

# 12 planetary eye spheres mounted along the equatorial ring
oph_eyes = []
for i in range(12):
    ang = i * (math.pi / 6.0)
    ox = math.cos(ang) * 2.1
    oy = math.sin(ang) * 2.1
    oph_eyes.append(sdf_leaf(0, [0.12, 0.12, 0.12], offset=[ox, oy, 0.0]))
oph_eye_tree = oph_eyes[0]
for e in oph_eyes[1:]:
    oph_eye_tree = sdf_binary(5, oph_eye_tree, e, 0.1)

# Central Stellated Dodecahedron Core
core_cx = sdf_leaf(5, [0.35, 0.75, 0.0], offset=[0.0, 0.0, 0.0])
core_cy = sdf_leaf(3, [0.25, 0.85, 0.25])
core_cz = sdf_leaf(3, [0.25, 0.25, 0.85])
oph_star = sdf_binary(5, core_cx, sdf_binary(5, core_cy, core_cz, 0.15), 0.18)
oph_pearl = sdf_leaf(0, [0.32, 0.32, 0.32])
oph_core = sdf_binary(5, oph_star, oph_pearl, 0.2)

ophanim_full = sdf_binary(5, sdf_binary(5, oph_rings, oph_eye_tree, 0.15), oph_core, 0.22)
objects.append(make_field(
    "cathedral.sdf.ophanim_throne", "Ophanim Celestial Gyroscope (The Chariot of Ezekiel)",
    [0.0, 11.2, -29.5], ophanim_full, [2.5, 2.5, 2.5],
    "material.logos.gold", [1.0, 0.82, 0.28],
    extra_props={
        "isCelestialRelic": {"t": "bool", "v": True},
        "relicKind": {"t": "string", "v": "Ophanim Chariot of Ezekiel"},
        "light.intensity": {"t": "float", "v": 4.5},
        "light.source": {"t": "bool", "v": True}
    }
))

# 9. NEW: Sanctuary Hanging Golden Incense Thurible (Censer)
# Suspended from the triumphal arch at [0.0, 7.8, -20.0]
thurible_bowl = sdf_leaf(5, [0.35, 0.32, 0.0], offset=[0.0, 0.0, 0.0])
thurible_lid = sdf_leaf(5, [0.32, 0.38, 0.0], offset=[0.0, 0.32, 0.0])
thurible_body = sdf_binary(5, thurible_bowl, thurible_lid, 0.15)
thurible_ring = sdf_leaf(6, [0.15, 0.03, 0.0], offset=[0.0, 0.72, 0.0]) # Hanging eyelet
thurible_smoke1 = sdf_leaf(0, [0.22, 0.22, 0.22], offset=[0.08, 0.95, 0.04])
thurible_smoke2 = sdf_leaf(0, [0.28, 0.28, 0.28], offset=[-0.06, 1.25, -0.05])
thurible_smoke = sdf_binary(5, thurible_smoke1, thurible_smoke2, 0.22)
thurible_tree = sdf_binary(5, sdf_binary(5, thurible_body, thurible_ring, 0.15), thurible_smoke, 0.25)
objects.append(make_field(
    "cathedral.sdf.thurible", "Sanctuary Hanging Golden Incense Thurible",
    [0.0, 7.8, -20.0], thurible_tree, [1.0, 1.6, 1.0],
    "material.logos.gold", [1.0, 0.82, 0.28],
    extra_props={
        "isThurible": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.6},
        "light.source": {"t": "bool", "v": True}
    }
))

# 10. NEW: Illuminated Gospel Lectionary on Altar Lectern
objects.append(make_box(
    "cathedral.altar.lectern.stand", "Golden Eagle Altar Lectern Stand",
    [-0.85, 2.25, -28.9], [0.35, 0.2, 0.35],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "cathedral.altar.lectionary", "Illuminated Altar Gospel Lectionary",
    [-0.85, 2.42, -28.9], [0.65, 0.08, 0.52],
    "material.logos.manuscript", [1.0, 1.0, 1.0],
    rot_deg=[22.0, 0.0, 0.0],
    extra_props={"isSacredText": {"t": "bool", "v": True}}
))

# 11. NEW: High Altar Embroidered Antependium (Liturgical Frontal)
objects.append(make_box(
    "cathedral.altar.antependium.frontal", "High Altar Liturgical Antependium Frontal",
    [0.0, 1.45, -28.52], [4.6, 1.15, 0.08],
    "material.logos.altar", [1.0, 1.0, 1.0],
    extra_props={"isLiturgicalFabric": {"t": "bool", "v": True}}
))

# ==============================================================================
# MATERIALS PALETTE — FIXED TO NATIVE SPECIFICATION
# Material::getIdentifier() returns "material." + name, so name must be bare slug!
# ==============================================================================
# Pre-generate textures for materials
tex_cosmati = cosmati_floor_face(256)
tex_dark_stone = solid_face(64, 64, [28, 30, 36])
tex_gold_border = solid_face(64, 64, [215, 180, 55])
tex_altar_mensa = altar_mensa_face(256)
tex_altar_antependium = altar_antependium_face(256)
tex_crimson_side = solid_face(64, 64, [125, 20, 30])
tex_linenfold = gothic_linenfold_wood_face(256)
tex_wood_grain = wood_grain_face(256)
tex_wood_side = solid_face(64, 64, [90, 52, 24])
tex_filigree = gilded_filigree_face(256)
tex_alabaster = veined_alabaster_face(256)
tex_ashlar = ashlar_stone_face(256)
tex_rose_sapphire = rose_window_sapphire_face(256)
tex_rose_ruby = rose_window_ruby_face(256)
tex_glass_emerald = stained_glass_emerald_face(256)
tex_glass_amethyst = stained_glass_amethyst_face(256)
tex_organ_pipe = organ_pipe_tin_face(256)
tex_manuscript = illuminated_manuscript_face(256)
tex_water_caustics = water_caustics_face(256)
tex_came = solid_face(16, 16, [25, 25, 30])
tex_leather = solid_face(16, 16, [85, 42, 20])
tex_tin_side = solid_face(16, 16, [175, 180, 185])
tex_amber = solid_face(16, 16, [245, 165, 35])
tex_core = solid_face(16, 16, [255, 245, 200])

materials = [
    {
        "name": "logos.floor",
        "textureResolution": 256, "ambient": 0.35, "diffuse": 0.85, "specular": 0.5, "shininess": 36.0,
        "baseColor": [1.0, 1.0, 1.0], "roughness": 0.25, "metallic": 0.4,
        "faceTextures": [tex_dark_stone, tex_dark_stone, tex_cosmati, tex_dark_stone, tex_dark_stone, tex_dark_stone]
    },
    {
        "name": "logos.gold",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.90, "specular": 0.95, "shininess": 80.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.25, 0.20, 0.08], "roughness": 0.15, "metallic": 0.95,
        "faceTextures": [tex_filigree] * 6
    },
    {
        "name": "logos.sapphire",
        "textureResolution": 256, "ambient": 0.30, "diffuse": 0.85, "specular": 0.85, "shininess": 64.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.12, 0.24, 0.55], "roughness": 0.15, "metallic": 0.5,
        "faceTextures": [tex_came, tex_came, tex_came, tex_came, tex_rose_sapphire, tex_rose_sapphire]
    },
    {
        "name": "logos.alabaster",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.90, "specular": 0.65, "shininess": 40.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.05, 0.05, 0.05], "roughness": 0.2, "metallic": 0.1,
        "faceTextures": [tex_alabaster] * 6
    },
    {
        "name": "logos.altar",
        "textureResolution": 256, "ambient": 0.35, "diffuse": 0.85, "specular": 0.70, "shininess": 50.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.08, 0.04, 0.04], "roughness": 0.2, "metallic": 0.5,
        "faceTextures": [tex_crimson_side, tex_crimson_side, tex_altar_mensa, tex_dark_stone, tex_altar_antependium, tex_crimson_side]
    },
    {
        "name": "logos.core",
        "textureResolution": 256, "ambient": 0.55, "diffuse": 0.95, "specular": 1.0, "shininess": 128.0,
        "baseColor": [1.0, 0.96, 0.80], "emission": [0.95, 0.85, 0.55], "roughness": 0.05, "metallic": 0.5,
        "faceTextures": [tex_core] * 6
    },
    {
        "name": "logos.emerald",
        "textureResolution": 256, "ambient": 0.30, "diffuse": 0.85, "specular": 0.85, "shininess": 60.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.15, 0.45, 0.25], "roughness": 0.15, "metallic": 0.6,
        "faceTextures": [tex_came, tex_came, tex_came, tex_came, tex_glass_emerald, tex_glass_emerald]
    },
    {
        "name": "logos.amethyst",
        "textureResolution": 256, "ambient": 0.30, "diffuse": 0.85, "specular": 0.85, "shininess": 60.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.28, 0.12, 0.48], "roughness": 0.15, "metallic": 0.6,
        "faceTextures": [tex_came, tex_came, tex_came, tex_came, tex_glass_amethyst, tex_glass_amethyst]
    },
    {
        "name": "logos.amber",
        "textureResolution": 256, "ambient": 0.35, "diffuse": 0.85, "specular": 0.80, "shininess": 50.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.35, 0.20, 0.08], "roughness": 0.2, "metallic": 0.7,
        "faceTextures": [tex_amber] * 6
    },
    {
        "name": "logos.arch",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.85, "specular": 0.50, "shininess": 32.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.06, 0.06, 0.08], "roughness": 0.35, "metallic": 0.3,
        "faceTextures": [tex_ashlar] * 6
    },
    {
        "name": "logos.wood",
        "textureResolution": 256, "ambient": 0.35, "diffuse": 0.85, "specular": 0.45, "shininess": 24.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.0, 0.0, 0.0], "roughness": 0.5, "metallic": 0.08,
        "faceTextures": [tex_wood_side, tex_wood_side, tex_wood_grain, tex_wood_side, tex_linenfold, tex_linenfold]
    },
    {
        "name": "logos.ruby",
        "textureResolution": 256, "ambient": 0.30, "diffuse": 0.85, "specular": 0.85, "shininess": 60.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.40, 0.10, 0.15], "roughness": 0.15, "metallic": 0.6,
        "faceTextures": [tex_came, tex_came, tex_came, tex_came, tex_rose_ruby, tex_rose_ruby]
    },
    {
        "name": "logos.cyan",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.90, "specular": 0.95, "shininess": 90.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.15, 0.55, 0.75], "roughness": 0.1, "metallic": 0.5,
        "faceTextures": [tex_water_caustics] * 6
    },
    {
        "name": "logos.organ_pipe",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.85, "specular": 0.98, "shininess": 120.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.08, 0.10, 0.14], "roughness": 0.1, "metallic": 0.95,
        "faceTextures": [tex_tin_side, tex_tin_side, tex_tin_side, tex_tin_side, tex_organ_pipe, tex_organ_pipe]
    },
    {
        "name": "logos.manuscript",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.85, "specular": 0.40, "shininess": 20.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.05, 0.04, 0.03], "roughness": 0.4, "metallic": 0.1,
        "faceTextures": [tex_leather, tex_leather, tex_manuscript, tex_leather, tex_leather, tex_leather]
    },
    # --- ONTOMATH DYNAMIC COLOR FIELD MATERIALS ---
    {
        "name": "logos.colorfield.mandorla",
        "textureResolution": 256, "ambient": 0.50, "diffuse": 0.90, "specular": 0.95, "shininess": 96.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.20, 0.12, 0.35], "roughness": 0.12, "metallic": 0.85,
        "faceTextures": [tex_came, tex_came, tex_came, tex_came, tex_rose_sapphire, tex_rose_ruby],
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.75, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.20, "shift": 0.0}]},
                {"c": -0.003, "factors": {"x": 2.0}}
            ],
            [
                {"c": 0.55, "factors": {}},
                {"c": 0.35, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 0.25, "shift": 0.5}]},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.6, "shift": 0.0}]}
            ],
            [
                {"c": 0.90, "factors": {}},
                {"c": 0.10, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.18, "shift": -0.4}]},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.4, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.colorfield.singularity",
        "textureResolution": 256, "ambient": 0.60, "diffuse": 0.95, "specular": 1.0, "shininess": 128.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.35, 0.45, 0.65], "roughness": 0.08, "metallic": 0.92,
        "faceTextures": [tex_core] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.70, "factors": {}},
                {"c": 0.30, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.35, "shift": 0.0}]},
                {"c": 0.20, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.7, "shift": 0.0}]}
            ],
            [
                {"c": 0.60, "factors": {}},
                {"c": 0.35, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 0.45, "shift": 1.2}]},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.8, "shift": 0.0}]}
            ],
            [
                {"c": 0.85, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.30, "shift": -0.8}]},
                {"c": 0.22, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.6, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.colorfield.pneuma",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.90, "specular": 0.95, "shininess": 80.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.10, 0.40, 0.55], "roughness": 0.15, "metallic": 0.75,
        "faceTextures": [tex_water_caustics] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.20, "factors": {}},
                {"c": 0.20, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.32, "shift": 0.0}]},
                {"c": 0.10, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.5, "shift": 0.0}]}
            ],
            [
                {"c": 0.85, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 0.40, "shift": 0.6}]},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 0.6, "shift": 0.0}]}
            ],
            [
                {"c": 0.95, "factors": {}},
                {"c": 0.05, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.25, "shift": 0.0}]},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.45, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.colorfield.sophia",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.90, "specular": 0.95, "shininess": 80.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.55, 0.15, 0.15], "roughness": 0.15, "metallic": 0.75,
        "faceTextures": [tex_amber] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.96, "factors": {}},
                {"c": 0.04, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.35, "shift": 0.0}]},
                {"c": 0.08, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 0.6, "shift": 0.0}]}
            ],
            [
                {"c": 0.40, "factors": {}},
                {"c": 0.40, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.45, "shift": -0.5}]},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "z", "scale": 0.7, "shift": 0.0}]}
            ],
            [
                {"c": 0.35, "factors": {}},
                {"c": 0.30, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 0.50, "shift": 0.7}]},
                {"c": 0.18, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.5, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.colorfield.genesis",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.90, "specular": 0.90, "shininess": 70.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.25, 0.20, 0.45], "roughness": 0.18, "metallic": 0.65,
        "faceTextures": [tex_rose_sapphire] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.65, "factors": {}},
                {"c": 0.35, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.40, "shift": 0.0}]}
            ],
            [
                {"c": 0.40, "factors": {}},
                {"c": 0.40, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 0.50, "shift": 0.0}]}
            ],
            [
                {"c": 0.80, "factors": {}},
                {"c": 0.20, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.35, "shift": 0.0}]}
            ]
        )
    },
    # --- SACRED EDENIC POND ONTOMATH DYNAMIC COLOR FIELDS ---
    {
        "name": "logos.pond.water",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.90, "specular": 0.98, "shininess": 128.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.06, 0.28, 0.45], "roughness": 0.03, "metallic": 0.35,
        "faceTextures": [tex_water_caustics] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                # --- RED CHANNEL (Selective spectral Beer-Lambert absorption) ---
                # Deep center: heavily absorbed (0.038). Shallows/shore: warm pebble/sand reflection (+0.0032 * r^2).
                {"c": 0.038, "factors": {}},
                {"c": 0.0032, "factors": {"x": 2.0}},
                {"c": 0.0032, "factors": {"z": 2.0}},
                # 4-Octave aperiodic cross-wave caustics (non-repeating golden ratio frequencies)
                {"c": 0.030, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.65, "shift": 0.1}, {"kind": 1, "var": "z", "scale": 0.65, "shift": -0.2}]},
                {"c": 0.017, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 1.30, "shift": -0.3}, {"kind": 0, "var": "z", "scale": 1.30, "shift": 0.4}]},
                {"c": 0.008, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.65, "shift": 0.5}, {"kind": 1, "var": "z", "scale": 2.65, "shift": -0.1}]},
                {"c": 0.003, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 4.25, "shift": 0.0}, {"kind": 1, "var": "z", "scale": 4.25, "shift": 0.0}]},
                # Vertical surface glisten (+0.06*y)
                {"c": 0.06, "factors": {"y": 1.0}}
            ],
            [
                # --- GREEN CHANNEL (Bathymetric lift to emerald-turquoise shallows) ---
                # Deep center: cool sapphire-aquamarine (0.36). Shallows: radiant mint turquoise (+0.0115 * r^2).
                {"c": 0.36, "factors": {}},
                {"c": 0.0115, "factors": {"x": 2.0}},
                {"c": 0.0115, "factors": {"z": 2.0}},
                # 4-Octave aperiodic cross-wave caustics (primary green crests)
                {"c": 0.068, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.65, "shift": 0.1}, {"kind": 1, "var": "z", "scale": 0.65, "shift": -0.2}]},
                {"c": 0.034, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 1.30, "shift": -0.3}, {"kind": 0, "var": "z", "scale": 1.30, "shift": 0.4}]},
                {"c": 0.016, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.65, "shift": 0.5}, {"kind": 1, "var": "z", "scale": 2.65, "shift": -0.1}]},
                {"c": 0.006, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 4.25, "shift": 0.0}, {"kind": 1, "var": "z", "scale": 4.25, "shift": 0.0}]},
                # Vertical surface glisten (+0.14*y)
                {"c": 0.14, "factors": {"y": 1.0}}
            ],
            [
                # --- BLUE CHANNEL (Rayleigh oceanic scattering & celestial sky clarity) ---
                # Deep center: incandescent royal sapphire (0.95). Shallows: crystal pale cyan (-0.0028 * r^2).
                {"c": 0.95, "factors": {}},
                {"c": -0.0028, "factors": {"x": 2.0}},
                {"c": -0.0028, "factors": {"z": 2.0}},
                # 4-Octave aperiodic cross-wave caustics (sky reflections)
                {"c": 0.035, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.65, "shift": 0.1}, {"kind": 1, "var": "z", "scale": 0.65, "shift": -0.2}]},
                {"c": 0.020, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 1.30, "shift": -0.3}, {"kind": 0, "var": "z", "scale": 1.30, "shift": 0.4}]},
                {"c": 0.010, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.65, "shift": 0.5}, {"kind": 1, "var": "z", "scale": 2.65, "shift": -0.1}]},
                {"c": 0.004, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 4.25, "shift": 0.0}, {"kind": 1, "var": "z", "scale": 4.25, "shift": 0.0}]},
                # Vertical surface glisten (+0.08*y)
                {"c": 0.08, "factors": {"y": 1.0}}
            ]
        )
    },
{
        "name": "logos.pond.water_cascade",
        "textureResolution": 256, "ambient": 0.65, "diffuse": 0.98, "specular": 1.0, "shininess": 128.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.45, 0.65, 0.75], "roughness": 0.08, "metallic": 0.20,
        "faceTextures": [tex_water_caustics] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                # High-aeration tumbling white froth
                {"c": 0.75, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 4.5, "shift": 0.0}, {"kind": 1, "var": "x", "scale": 3.0, "shift": 0.0}]},
                {"c": 0.08, "factors": {}, "trans": [{"kind": 0, "var": "z", "scale": 5.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.92, "factors": {}},
                {"c": 0.06, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 4.0, "shift": 0.5}, {"kind": 1, "var": "x", "scale": 3.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.98, "factors": {}},
                {"c": 0.02, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 3.5, "shift": 0.0}]}
            ]
        )
    },
{
        "name": "logos.pond.lotus.dawn",
        "textureResolution": 256, "ambient": 0.50, "diffuse": 0.95, "specular": 0.90, "shininess": 85.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.35, 0.15, 0.25], "roughness": 0.15, "metallic": 0.50,
        "faceTextures": [tex_rose_ruby] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.96, "factors": {}},
                {"c": 0.04, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.5, "shift": 0.0}]}
            ],
            [
                {"c": 0.48, "factors": {}},
                {"c": 0.40, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 2.0, "shift": 0.6}]}
            ],
            [
                {"c": 0.72, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 2.5, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.lotus.white",
        "textureResolution": 256, "ambient": 0.55, "diffuse": 0.95, "specular": 0.95, "shininess": 90.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.40, 0.38, 0.30], "roughness": 0.12, "metallic": 0.45,
        "faceTextures": [tex_alabaster] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.98, "factors": {}},
                {"c": 0.02, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 2.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.92, "factors": {}},
                {"c": 0.08, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 2.5, "shift": 0.0}]}
            ],
            [
                {"c": 0.80, "factors": {}},
                {"c": 0.18, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.lilypad",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.90, "specular": 0.70, "shininess": 45.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.08, 0.25, 0.10], "roughness": 0.20, "metallic": 0.25,
        "faceTextures": [tex_glass_emerald] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.18, "factors": {}},
                {"c": 0.16, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.2, "shift": 0.0}]}
            ],
            [
                {"c": 0.78, "factors": {}},
                {"c": 0.18, "factors": {}, "trans": [{"kind": 0, "var": "z", "scale": 1.8, "shift": 0.0}]}
            ],
            [
                {"c": 0.24, "factors": {}},
                {"c": 0.14, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 1.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.mossy_stone",
        "textureResolution": 256, "ambient": 0.38, "diffuse": 0.85, "specular": 0.50, "shininess": 30.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.05, 0.10, 0.05], "roughness": 0.35, "metallic": 0.20,
        "faceTextures": [tex_dark_stone] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.34, "factors": {}},
                {"c": 0.14, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 1.5, "shift": 0.0}]}
            ],
            [
                {"c": 0.60, "factors": {}},
                {"c": 0.30, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.2, "shift": 0.6}]}
            ],
            [
                {"c": 0.32, "factors": {}},
                {"c": 0.10, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 1.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.reeds",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.90, "specular": 0.65, "shininess": 40.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.12, 0.28, 0.08], "roughness": 0.25, "metallic": 0.20,
        "faceTextures": [tex_wood_grain] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.42, "factors": {}},
                {"c": 0.30, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 0.8, "shift": 0.0}]}
            ],
            [
                {"c": 0.80, "factors": {}},
                {"c": 0.18, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.18, "factors": {}},
                {"c": 0.10, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 0.6, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.wisp",
        "textureResolution": 256, "ambient": 0.80, "diffuse": 0.95, "specular": 1.0, "shininess": 128.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.85, 0.95, 1.0], "roughness": 0.05, "metallic": 0.80,
        "faceTextures": [tex_core] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.75, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 3.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.85, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "x", "scale": 3.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.95, "factors": {}},
                {"c": 0.05, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 3.0, "shift": 0.0}]}
            ]
        )
    }
,

    # --- SACRED EDENIC POND NUANCED ONTOMATH DYNAMIC COLOR FIELDS ---
    {
        "name": "logos.pond.water_abyss",
        "textureResolution": 256, "ambient": 0.35, "diffuse": 0.85, "specular": 0.99, "shininess": 128.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.04, 0.12, 0.35], "roughness": 0.03, "metallic": 0.40,
        "faceTextures": [tex_water_caustics] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.02, "factors": {}},
                {"c": 0.003, "factors": {"x": 2.0}},
                {"c": 0.003, "factors": {"z": 2.0}},
                {"c": 0.015, "factors": {}, "trans": [
                    {"kind": 1, "var": "x", "scale": 1.0, "shift": 0.0},
                    {"kind": 1, "var": "z", "scale": 1.0, "shift": 0.0}
                ]}
            ],
            [
                {"c": 0.20, "factors": {}},
                {"c": 0.012, "factors": {"x": 2.0}},
                {"c": 0.012, "factors": {"z": 2.0}},
                {"c": 0.035, "factors": {}, "trans": [
                    {"kind": 0, "var": "x", "scale": 1.2, "shift": 0.0},
                    {"kind": 0, "var": "z", "scale": 1.2, "shift": 0.0}
                ]},
                {"c": 0.18, "factors": {"y": 1.0}}
            ],
            [
                {"c": 0.82, "factors": {}},
                {"c": -0.003, "factors": {"x": 2.0}},
                {"c": -0.003, "factors": {"z": 2.0}},
                {"c": 0.040, "factors": {}, "trans": [
                    {"kind": 1, "var": "x", "scale": 1.0, "shift": 0.0},
                    {"kind": 1, "var": "z", "scale": 1.0, "shift": 0.0}
                ]},
                {"c": 0.12, "factors": {"y": 1.0}}
            ]
        )
    },
{
        "name": "logos.pond.lotus.cyan",
        "textureResolution": 256, "ambient": 0.50, "diffuse": 0.95, "specular": 0.95, "shininess": 90.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.15, 0.45, 0.55], "roughness": 0.12, "metallic": 0.45,
        "faceTextures": [tex_water_caustics] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.12, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.85, "factors": {}},
                {"c": 0.12, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 2.0, "shift": 0.4}]}
            ],
            [
                {"c": 0.95, "factors": {}},
                {"c": 0.05, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 2.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.iris",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.90, "specular": 0.80, "shininess": 60.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.25, 0.08, 0.40], "roughness": 0.18, "metallic": 0.35,
        "faceTextures": [tex_glass_amethyst] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.45, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 2.2, "shift": 0.0}]}
            ],
            [
                {"c": 0.18, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 2.5, "shift": 0.5}]}
            ],
            [
                {"c": 0.88, "factors": {}},
                {"c": 0.12, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.koi",
        "textureResolution": 256, "ambient": 0.55, "diffuse": 0.95, "specular": 0.98, "shininess": 110.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.45, 0.25, 0.05], "roughness": 0.08, "metallic": 0.65,
        "faceTextures": [tex_amber] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.95, "factors": {}},
                {"c": 0.05, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 5.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.62, "factors": {}},
                {"c": 0.30, "factors": {}, "trans": [{"kind": 0, "var": "z", "scale": 4.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.15, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 4.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.willow",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.88, "specular": 0.60, "shininess": 35.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.08, 0.20, 0.08], "roughness": 0.30, "metallic": 0.15,
        "faceTextures": [tex_glass_emerald] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.28, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 1.2, "shift": 0.0}]}
            ],
            [
                {"c": 0.78, "factors": {}},
                {"c": 0.20, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.5, "shift": 0.5}]}
            ],
            [
                {"c": 0.35, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 1.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.lantern",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.88, "specular": 0.75, "shininess": 50.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.35, 0.25, 0.10], "roughness": 0.25, "metallic": 0.30,
        "faceTextures": [tex_alabaster] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.90, "factors": {}},
                {"c": 0.10, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 2.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.78, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 2.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.55, "factors": {}},
                {"c": 0.25, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]}
            ]
        )
    },
    {
        "name": "logos.pond.pebbles",
        "textureResolution": 256, "ambient": 0.35, "diffuse": 0.85, "specular": 0.60, "shininess": 40.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.04, 0.06, 0.08], "roughness": 0.40, "metallic": 0.25,
        "faceTextures": [tex_dark_stone] * 6,
        "colorExpr": make_color_expr_piecewise(
            [
                {"c": 0.35, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 3.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.38, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 0, "var": "z", "scale": 3.0, "shift": 0.0}]}
            ],
            [
                {"c": 0.42, "factors": {}},
                {"c": 0.15, "factors": {}, "trans": [{"kind": 1, "var": "y", "scale": 3.0, "shift": 0.0}]}
            ]
        )
    },
    # --- ONTOMATH ADVANCED BOUNDED COLOR FIELDS ---
    {
        "name": "logos.bounds.stratified",
        "textureResolution": 256, "ambient": 0.50, "diffuse": 0.90, "specular": 0.95, "shininess": 90.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.25, 0.20, 0.35], "roughness": 0.12, "metallic": 0.85,
        "faceTextures": [tex_came] * 6,
        "colorExpr": {
            "input": "y",
            "pieces": [
                {"hi": -1.0, "hasHi": True, "mathNode": {"op": 2, "children": [
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.18, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.14, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.10, "factors": {}}]}}
                ]}},
                {"lo": -1.0, "hasLo": True, "hi": 0.5, "hasHi": True, "mathNode": {"op": 2, "children": [
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.10, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.45, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.95, "factors": {}}]}}
                ]}},
                {"lo": 0.5, "hasLo": True, "hi": 2.0, "hasHi": True, "mathNode": {"op": 2, "children": [
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.95, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.12, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.25, "factors": {}}]}}
                ]}},
                {"lo": 2.0, "hasLo": True, "mathNode": {"op": 2, "children": [
                    {"op": 0, "scalarForm": {"terms": [{"c": 1.00, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.88, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.30, "factors": {}}]}}
                ]}}
            ]
        }
    },
    {
        "name": "logos.bounds.quantized",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.90, "specular": 0.90, "shininess": 80.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.20, 0.35, 0.45], "roughness": 0.15, "metallic": 0.70,
        "faceTextures": [tex_came] * 6,
        "colorExpr": {
            "input": "y",
            "pieces": [{
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 12, "stringArg": "Floor", "children": [
                            {"op": 0, "scalarForm": {"terms": [
                                {"c": 0.5, "factors": {}},
                                {"c": 2.2, "factors": {}, "trans": [{"kind": 0, "var": "y", "scale": 1.4, "shift": 0.0}]}
                            ]}}
                        ]},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.45, "factors": {}}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.85, "factors": {}}]}}
                    ]
                }
            }]
        }
    },
    {
        "name": "logos.bounds.perlin",
        "textureResolution": 256, "ambient": 0.55, "diffuse": 0.95, "specular": 0.95, "shininess": 100.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.30, 0.20, 0.50], "roughness": 0.10, "metallic": 0.80,
        "faceTextures": [tex_came] * 6,
        "colorExpr": {
            "input": "y",
            "pieces": [{
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 29, "children": [{"op": 1, "var": "p"}]},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.55, "factors": {}}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.92, "factors": {}}]}}
                    ]
                }
            }]
        }
    },
    {
        "name": "logos.bounds.radial",
        "textureResolution": 256, "ambient": 0.50, "diffuse": 0.95, "specular": 1.0, "shininess": 110.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.45, 0.35, 0.15], "roughness": 0.08, "metallic": 0.90,
        "faceTextures": [tex_core] * 6,
        "colorExpr": {
            "input": "y",
            "pieces": [{
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [
                            {"c": 1.0, "factors": {}},
                            {"c": -0.22, "factors": {"x": 2.0}},
                            {"c": -0.22, "factors": {"z": 2.0}}
                        ]}},
                        {"op": 0, "scalarForm": {"terms": [
                            {"c": 0.85, "factors": {}},
                            {"c": -0.20, "factors": {"x": 2.0}},
                            {"c": -0.20, "factors": {"z": 2.0}}
                        ]}},
                        {"op": 0, "scalarForm": {"terms": [
                            {"c": 0.25, "factors": {}},
                            {"c": 0.25, "factors": {"x": 2.0}},
                            {"c": 0.25, "factors": {"z": 2.0}}
                        ]}}
                    ]
                }
            }]
        }
    },
    {
        "name": "logos.bounds.checkerboard",
        "textureResolution": 256, "ambient": 0.40, "diffuse": 0.90, "specular": 0.85, "shininess": 75.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.15, 0.15, 0.25], "roughness": 0.18, "metallic": 0.65,
        "faceTextures": [tex_came] * 6,
        "colorExpr": {
            "input": "y",
            "pieces": [{
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [
                            {"c": 0.5, "factors": {}},
                            {"c": 0.5, "factors": {}, "trans": [
                                {"kind": 1, "var": "x", "scale": 3.0, "shift": 0.0},
                                {"kind": 1, "var": "z", "scale": 3.0, "shift": 0.0}
                            ]}
                        ]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.25, "factors": {}}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.85, "factors": {}}]}}
                    ]
                }
            }]
        }
    },
    {
        "name": "logos.bounds.chladni",
        "textureResolution": 256, "ambient": 0.45, "diffuse": 0.92, "specular": 0.95, "shininess": 95.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.18, 0.35, 0.40], "roughness": 0.12, "metallic": 0.80,
        "faceTextures": [tex_came] * 6,
        "colorExpr": {
            "input": "y",
            "pieces": [{
                "mathNode": {
                    "op": 2,
                    "children": [
                        {"op": 0, "scalarForm": {"terms": [
                            {"c": 0.5, "factors": {}},
                            {"c": 0.5, "factors": {}, "trans": [{"kind": 1, "var": "x", "scale": 2.0, "shift": 0.0}]},
                            {"c": -0.5, "factors": {}, "trans": [{"kind": 1, "var": "z", "scale": 2.0, "shift": 0.0}]}
                        ]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.65, "factors": {}}]}},
                        {"op": 0, "scalarForm": {"terms": [{"c": 0.92, "factors": {}}]}}
                    ]
                }
            }]
        }
    },
    {
        "name": "logos.bounds.hemisphere",
        "textureResolution": 256, "ambient": 0.50, "diffuse": 0.95, "specular": 0.98, "shininess": 105.0,
        "baseColor": [1.0, 1.0, 1.0], "emission": [0.35, 0.30, 0.40], "roughness": 0.10, "metallic": 0.90,
        "faceTextures": [tex_came] * 6,
        "colorExpr": {
            "input": "x",
            "pieces": [
                {"hi": 0.0, "hasHi": True, "mathNode": {"op": 2, "children": [
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.15, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.45, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.95, "factors": {}}]}}
                ]}},
                {"lo": 0.0, "hasLo": True, "mathNode": {"op": 2, "children": [
                    {"op": 0, "scalarForm": {"terms": [{"c": 1.00, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.82, "factors": {}}]}},
                    {"op": 0, "scalarForm": {"terms": [{"c": 0.25, "factors": {}}]}}
                ]}}
            ]
        }
    }

]

# ==============================================================================
# LEXEMES
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


# ==============================================================================
# MONUMENTAL SACRED SDFS WITH ONTOMATH-DRIVEN MATHEMATICAL COLOR FIELDS
# ==============================================================================

# 1. The Great Apse Mandorla of Transfiguration (28m tall, 16m wide sacred gateway behind High Altar)
# Constructed from nested Gothic pointed Vesica arches, fluted radiation rays, and inner sanctuary core
mandorla_outer_l = sdf_leaf(0, [6.5, 9.5, 1.8], offset=[-3.2, 0.0, 0.0])
mandorla_outer_r = sdf_leaf(0, [6.5, 9.5, 1.8], offset=[3.2, 0.0, 0.0])
mandorla_vesica_outer = sdf_binary(3, mandorla_outer_l, mandorla_outer_r) # Intersection forms the pointed Vesica Piscis

mandorla_inner_l = sdf_leaf(0, [5.2, 7.8, 2.2], offset=[-2.6, 0.0, 0.0])
mandorla_inner_r = sdf_leaf(0, [5.2, 7.8, 2.2], offset=[2.6, 0.0, 0.0])
mandorla_vesica_inner = sdf_binary(3, mandorla_inner_l, mandorla_inner_r)

mandorla_frame = sdf_binary(4, mandorla_vesica_outer, mandorla_vesica_inner) # Hollow gothic frame

mandorla_sun_core = sdf_leaf(0, [2.4, 3.8, 1.0])
mandorla_torus1 = sdf_leaf(6, [3.8, 0.35, 0.0]) # Major R = 3.8, Minor R = 0.35
mandorla_torus2 = sdf_leaf(6, [5.4, 0.28, 0.0]) # Major R = 5.4, Minor R = 0.28

mandorla_sanctuary = sdf_binary(5, mandorla_frame, sdf_binary(5, mandorla_sun_core, sdf_binary(5, mandorla_torus1, mandorla_torus2, 0.25), 0.35), 0.4)

objects.append(make_field(
    "cathedral.sdf.monumental_mandorla", "Great Apse Mandorla of Transfiguration",
    [0.0, 15.0, -32.5], mandorla_sanctuary, [8.0, 14.0, 3.5],
    "material.logos.colorfield.mandorla", [1.0, 0.85, 0.95],
    extra_props={
        "isMonumentalSdf": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 6.0},
        "description": {"t": "string", "v": "28-meter tall Gothic pointed Mandorla with continuous OntoMath transfiguration color field"}
    }
))

# 2. The Colossal Pillar of Living Logos (28m tall braided double-helix singularity column at crossing)
singularity_spire = sdf_leaf(4, [1.4, 11.0, 0.0]) # Central energy cylinder
singularity_core_orb = sdf_leaf(0, [2.8, 2.8, 2.8], offset=[0.0, 0.0, 0.0])
singularity_ring_1 = sdf_leaf(6, [3.8, 0.45, 0.0], offset=[0.0, 4.0, 0.0])
singularity_ring_2 = sdf_leaf(6, [4.8, 0.40, 0.0], offset=[0.0, -4.0, 0.0])
singularity_ring_3 = sdf_leaf(6, [3.2, 0.35, 0.0], offset=[0.0, 8.0, 0.0])

singularity_rings = sdf_binary(5, singularity_ring_1, sdf_binary(5, singularity_ring_2, singularity_ring_3, 0.3), 0.35)
singularity_tree = sdf_binary(5, singularity_spire, sdf_binary(5, singularity_core_orb, singularity_rings, 0.4), 0.45)

objects.append(make_field(
    "cathedral.sdf.singularity_pillar", "Colossal Pillar of Living Logos (Crossing Singularity)",
    [0.0, 14.0, 0.0], singularity_tree, [6.0, 14.0, 6.0],
    "material.logos.colorfield.singularity", [0.95, 0.95, 1.0],
    extra_props={
        "isMonumentalSdf": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 5.5},
        "description": {"t": "string", "v": "28-meter high crossing singularity column with OntoMath multi-harmonic plasma color field"}
    }
))

# 3. The Monumental North Transept Gyroid Sanctuary Spire (22m tall Pneuma tower)
gyroid_outer_spire = sdf_leaf(4, [2.2, 8.5, 0.0])
gyroid_orb_upper = sdf_leaf(0, [2.8, 3.2, 2.8], offset=[0.0, 4.5, 0.0])
gyroid_orb_lower = sdf_leaf(0, [3.2, 3.6, 3.2], offset=[0.0, -3.5, 0.0])
gyroid_ring_pneuma = sdf_leaf(6, [3.8, 0.38, 0.0], offset=[0.0, 0.0, 0.0])

gyroid_monument_tree = sdf_binary(5, gyroid_outer_spire, sdf_binary(5, gyroid_ring_pneuma, sdf_binary(5, gyroid_orb_upper, gyroid_orb_lower, 0.35), 0.35), 0.4)

objects.append(make_field(
    "cathedral.sdf.monumental_gyroid_north", "Monumental North Transept Gyroid Spire of Pneuma",
    [-14.0, 12.0, 0.0], gyroid_monument_tree, [5.0, 12.0, 5.0],
    "material.logos.colorfield.pneuma", [0.2, 0.85, 0.95],
    extra_props={
        "isMonumentalSdf": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 4.8},
        "description": {"t": "string", "v": "24-meter tall North Transept spire with OntoMath celestial cyan-emerald color field"}
    }
))

# 4. The Monumental South Transept Stellated Merkaba Star of Sophia (22m tall starfire beacon)
star_axis_x = sdf_leaf(2, [2.6, 0.55, 0.55], p0=0.15)
star_axis_y = sdf_leaf(2, [0.55, 2.6, 0.55], p0=0.15)
star_axis_z = sdf_leaf(2, [0.55, 0.55, 2.6], p0=0.15)
star_axes = sdf_binary(5, star_axis_x, sdf_binary(5, star_axis_y, star_axis_z, 0.25), 0.25)

star_gimbal_1 = sdf_leaf(6, [3.6, 0.35, 0.0], offset=[0.0, 2.0, 0.0])
star_gimbal_2 = sdf_leaf(6, [3.6, 0.35, 0.0], offset=[0.0, -2.0, 0.0])
star_core_orb = sdf_leaf(0, [2.2, 2.2, 2.2])

merkaba_monument_tree = sdf_binary(5, star_axes, sdf_binary(5, star_core_orb, sdf_binary(5, star_gimbal_1, star_gimbal_2, 0.3), 0.35), 0.4)

objects.append(make_field(
    "cathedral.sdf.monumental_merkaba_south", "Monumental South Transept Stellated Merkaba Beacon",
    [14.0, 12.0, 0.0], merkaba_monument_tree, [5.0, 12.0, 5.0],
    "material.logos.colorfield.sophia", [0.95, 0.35, 0.25],
    extra_props={
        "isMonumentalSdf": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 4.8},
        "description": {"t": "string", "v": "24-meter tall South Transept starfire beacon with OntoMath ruby-gold color field"}
    }
))

# 5. The Great West Portal Genesis Rose Monolith (17m diameter Gothic wheel rosette)
rose_core = sdf_leaf(0, [2.2, 2.2, 0.8])
rose_outer_ring = sdf_leaf(6, [6.5, 0.45, 0.0])
rose_mid_ring = sdf_leaf(6, [4.2, 0.35, 0.0])
rose_petals_h = sdf_leaf(2, [6.0, 0.45, 0.5], p0=0.1)
rose_petals_v = sdf_leaf(2, [0.45, 6.0, 0.5], p0=0.1)

rose_cross = sdf_binary(5, rose_petals_h, rose_petals_v, 0.2)
rose_rings = sdf_binary(5, rose_outer_ring, rose_mid_ring, 0.25)
rose_monument_tree = sdf_binary(5, rose_rings, sdf_binary(5, rose_cross, rose_core, 0.3), 0.35)

objects.append(make_field(
    "cathedral.sdf.monumental_genesis_rose", "West Portal Great Genesis Rose Monolith",
    [0.0, 16.5, 33.0], rose_monument_tree, [8.5, 8.5, 2.5],
    "material.logos.colorfield.genesis", [0.75, 0.45, 0.85],
    rot_deg=[0.0, 0.0, 0.0],
    extra_props={
        "isMonumentalSdf": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 5.0},
        "description": {"t": "string", "v": "17-meter diameter West Portal Rose rosette with OntoMath stained-glass color field"}
    }
))


# ==============================================================================
# THE SACRED LIVING EDENIC POND OF LIVING WATERS (MAGNUM OPUS NUANCED EXPANSION)
# Located in the West Forecourt of the Cathedral (Z = 40 to 60, X = -10 to 10)
# A multi-tiered botanical, hydrological, and spiritual living sanctuary.
# ==============================================================================

# ------------------------------------------------------------------------------
# 1. HYDROLOGICAL BATHYMETRY: LAGOON, ABYSSAL SPRING, PEBBLE BED & BUBBLING UPWELLINGS
# ------------------------------------------------------------------------------

# A. Submerged Riverbed Pebble Mosaic (Contoured shoal beneath the water)
pebble_bed_center = sdf_leaf(3, [7.5, 0.25, 8.2], offset=[0.0, -0.15, 0.0])
pebble_bed_north  = sdf_leaf(3, [5.0, 0.20, 4.8], offset=[-4.2, -0.12, 2.5])
pebble_bed_south  = sdf_leaf(3, [5.2, 0.20, 5.0], offset=[4.2, -0.12, -2.5])
pebble_bed_tree = sdf_binary(5, pebble_bed_center, sdf_binary(5, pebble_bed_north, pebble_bed_south, 0.4), 0.45)
objects.append(make_field(
    "cathedral.pond.pebble_bed", "Submerged Jasper & Obsidian Riverbed Shoal",
    [0.0, 0.05, 50.0], pebble_bed_tree, [11.0, 0.8, 11.0],
    "material.logos.pond.pebbles", [0.40, 0.38, 0.35],
    extra_props={
        "isPondBed": {"t": "bool", "v": True},
        "description": {"t": "string", "v": "Submerged river-worn jasper, obsidian, and quartz pebble bed under the lagoon"}
    }
))

# B. Deep Abyssal Spring Basin (Deep subterranean well-spring where living water wells up)
abyss_pool = sdf_leaf(3, [3.2, 0.50, 3.4], offset=[0.0, -0.25, 0.0])
abyss_funnel = sdf_leaf(3, [1.8, 0.80, 1.8], offset=[0.0, -0.55, 0.0])
abyss_tree = sdf_binary(5, abyss_pool, abyss_funnel, 0.25)
objects.append(make_field(
    "cathedral.pond.abyss", "Abyssal Well-Spring of Living Water (Deep Heart)",
    [0.0, -0.05, 52.0], abyss_tree, [4.2, 1.4, 4.2],
    "material.logos.pond.water_abyss", [0.05, 0.25, 0.85],
    extra_props={
        "isAbyssalWell": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.2},
        "description": {"t": "string", "v": "Deep subterranean spring heart with internal deep sapphire luminescence"}
    }
))

# C. Multi-Lobed Living Water Lagoon Basin (Flat Ellipsoids in XZ, prim = 3)
pond_lobe_center = sdf_leaf(3, [6.8, 0.35, 7.5]) # Main deep lagoon
pond_lobe_north  = sdf_leaf(3, [4.5, 0.30, 4.2], offset=[-4.2, 0.0, 2.5]) # North tranquil cove
pond_lobe_south  = sdf_leaf(3, [4.8, 0.30, 4.5], offset=[4.2, 0.0, -2.5]) # South whispering cove
pond_lobe_west   = sdf_leaf(3, [3.6, 0.32, 3.8], offset=[0.0, 0.0, 5.5])  # West spring inlet cove
pond_lobe_east   = sdf_leaf(3, [3.2, 0.28, 3.5], offset=[0.0, 0.0, -5.5]) # East outlet shallow

pond_lobes_1 = sdf_binary(5, pond_lobe_center, sdf_binary(5, pond_lobe_north, pond_lobe_south, 0.4), 0.45)
pond_water_tree = sdf_binary(5, pond_lobes_1, sdf_binary(5, pond_lobe_west, pond_lobe_east, 0.35), 0.4)

objects.append(make_field(
    "cathedral.pond.water_basin", "Sacred Edenic Lagoon of Living Water",
    [0.0, 0.15, 50.0], pond_water_tree, [11.0, 1.2, 11.0],
    "material.logos.pond.water", [0.15, 0.85, 0.95],
    extra_props={
        "isSacredPond": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 3.8},
        "description": {"t": "string", "v": "Sculpted organic multi-lobed water lagoon with continuous turquoise-indigo caustics color field"}
    }
))

# D. Three Active Bubbling Spring Upwellings (Crystalline effervescent surface domes)
def make_bubble_upwelling_tree(r=0.45, h=0.18):
    dome1 = sdf_leaf(3, [r, h, r])
    dome2 = sdf_leaf(3, [r * 0.65, h * 1.3, r * 0.65], offset=[0.0, 0.04, 0.0])
    ring  = sdf_leaf(3, [r * 1.35, 0.04, r * 1.35], offset=[0.0, -0.04, 0.0])
    return sdf_binary(5, sdf_binary(5, dome1, dome2, 0.1), ring, 0.12)

objects.append(make_field(
    "cathedral.pond.spring_bubble.1", "Bubbling Spring Upwelling (North Effervescence)",
    [-1.5, 0.24, 53.2], make_bubble_upwelling_tree(0.55, 0.22), [1.2, 0.5, 1.2],
    "material.logos.pond.water", [0.35, 0.95, 1.0],
    extra_props={"light.intensity": {"t": "float", "v": 2.5}}
))
objects.append(make_field(
    "cathedral.pond.spring_bubble.2", "Bubbling Spring Upwelling (South Geyser Fountain)",
    [1.8, 0.24, 51.5], make_bubble_upwelling_tree(0.50, 0.20), [1.1, 0.5, 1.1],
    "material.logos.pond.water", [0.35, 0.95, 1.0],
    extra_props={"light.intensity": {"t": "float", "v": 2.4}}
))
objects.append(make_field(
    "cathedral.pond.spring_bubble.3", "Bubbling Spring Upwelling (Grotto Plume Well)",
    [0.0, 0.26, 55.2], make_bubble_upwelling_tree(0.65, 0.26), [1.3, 0.6, 1.3],
    "material.logos.pond.water", [0.40, 0.98, 1.0],
    extra_props={"light.intensity": {"t": "float", "v": 3.0}}
))

# E. Meandering Overflow Brooklet & Cascading Rill Channel (Z = 43 to 38)
brook_pool_1 = sdf_leaf(3, [1.6, 0.22, 2.2], offset=[0.0, 0.0, 0.0])
brook_pool_2 = sdf_leaf(3, [1.4, 0.20, 2.0], offset=[0.3, -0.06, -1.8])
brook_weir_stone = sdf_leaf(2, [1.8, 0.16, 0.35], offset=[0.0, -0.02, -0.8], p0=0.08)
brook_tree = sdf_binary(5, sdf_binary(5, brook_pool_1, brook_pool_2, 0.2), brook_weir_stone, 0.15)
objects.append(make_field(
    "cathedral.pond.brooklet.weir", "Meandering Outlet Brooklet & Stepped Weir Channel",
    [0.0, 0.14, 40.8], brook_tree, [3.2, 0.8, 4.5],
    "material.logos.pond.water", [0.20, 0.90, 0.95],
    extra_props={"description": {"t": "string", "v": "Tranquil outlet brooklet carrying living water from the pond to the garden rills"}}
))


# ------------------------------------------------------------------------------
# 2. BOTANICAL DIVERSITY: LOTUSES, WATER LILIES, IRISES, WILLOW, FERNS & DUCKWEED
# ------------------------------------------------------------------------------

def make_lotus_flower_tree(scale_r=1.0, scale_h=1.0):
    # Central golden seed pod (flattened sphere)
    seed_pod = sdf_leaf(0, [0.32 * scale_r, 0.22 * scale_h, 0.32 * scale_r], offset=[0.0, 0.14 * scale_h, 0.0])
    
    # 4 cardinal inner petals blooming upwards and curved slightly outwards
    p_n = sdf_leaf(3, [0.14 * scale_r, 0.38 * scale_h, 0.22 * scale_r], offset=[0.0, 0.24 * scale_h, 0.32 * scale_r])
    p_s = sdf_leaf(3, [0.14 * scale_r, 0.38 * scale_h, 0.22 * scale_r], offset=[0.0, 0.24 * scale_h, -0.32 * scale_r])
    p_e = sdf_leaf(3, [0.22 * scale_r, 0.38 * scale_h, 0.14 * scale_r], offset=[0.32 * scale_r, 0.24 * scale_h, 0.0])
    p_w = sdf_leaf(3, [0.22 * scale_r, 0.38 * scale_h, 0.14 * scale_r], offset=[-0.32 * scale_r, 0.24 * scale_h, 0.0])
    inner_petals = sdf_binary(5, sdf_binary(5, p_n, p_s, 0.15), sdf_binary(5, p_e, p_w, 0.15), 0.18)
    
    # 4 diagonal outer spreading petals floating just above water
    d_ne = sdf_leaf(3, [0.22 * scale_r, 0.18 * scale_h, 0.22 * scale_r], offset=[0.48 * scale_r, 0.08 * scale_h, 0.48 * scale_r])
    d_nw = sdf_leaf(3, [0.22 * scale_r, 0.18 * scale_h, 0.22 * scale_r], offset=[-0.48 * scale_r, 0.08 * scale_h, 0.48 * scale_r])
    d_se = sdf_leaf(3, [0.22 * scale_r, 0.18 * scale_h, 0.22 * scale_r], offset=[0.48 * scale_r, 0.08 * scale_h, -0.48 * scale_r])
    d_sw = sdf_leaf(3, [0.22 * scale_r, 0.18 * scale_h, 0.22 * scale_r], offset=[-0.48 * scale_r, 0.08 * scale_h, -0.48 * scale_r])
    outer_petals = sdf_binary(5, sdf_binary(5, d_ne, d_nw, 0.18), sdf_binary(5, d_se, d_sw, 0.18), 0.2)
    
    return sdf_binary(5, seed_pod, sdf_binary(5, inner_petals, outer_petals, 0.2), 0.25)

# A. The Grand Celestial Lotus of Dawn
grand_lotus_tree = make_lotus_flower_tree(1.4, 1.2)
objects.append(make_field(
    "cathedral.pond.lotus.grand", "Grand Celestial Lotus of Dawn",
    [-2.5, 0.38, 48.0], grand_lotus_tree, [1.8, 1.2, 1.8],
    "material.logos.pond.lotus.dawn", [1.0, 0.45, 0.75],
    extra_props={
        "isSacredRelic": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.8},
        "description": {"t": "string", "v": "Blooming sacred lotus with continuous OntoMath dawn-rose color field"}
    }
))

# B. The Sacred Alabaster Lotus of Sophia
white_lotus_tree = make_lotus_flower_tree(1.2, 1.0)
objects.append(make_field(
    "cathedral.pond.lotus.white", "Sacred Alabaster Lotus of Sophia",
    [3.0, 0.36, 51.5], white_lotus_tree, [1.6, 1.0, 1.6],
    "material.logos.pond.lotus.white", [0.95, 0.95, 0.90],
    extra_props={
        "isSacredRelic": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.5},
        "description": {"t": "string", "v": "Pure white-gold sacred lotus with warm solar golden center"}
    }
))

# C. The Nocturnal Cyan Star Lotus of Sophia (Third Major Sacred Blossom!)
cyan_lotus_tree = make_lotus_flower_tree(1.25, 1.05)
objects.append(make_field(
    "cathedral.pond.lotus.cyan", "Nocturnal Cyan Star Lotus of Sophia",
    [2.2, 0.38, 46.5], cyan_lotus_tree, [1.7, 1.1, 1.7],
    "material.logos.pond.lotus.cyan", [0.15, 0.90, 0.98],
    extra_props={
        "isSacredRelic": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.9},
        "description": {"t": "string", "v": "Mystical night-blooming cyan water lily with radiant celestial aquamarine luminescence"}
    }
))

# D. Twin Water Lily Buds
bud_n = sdf_binary(5, sdf_leaf(3, [0.25, 0.55, 0.25]), sdf_leaf(0, [0.20, 0.20, 0.20], offset=[0.0, 0.25, 0.0]), 0.15)
objects.append(make_field(
    "cathedral.pond.lily.bud.north", "Northern Water Lily Bud",
    [-4.2, 0.36, 52.5], bud_n, [0.9, 1.0, 0.9],
    "material.logos.pond.lotus.dawn", [0.95, 0.45, 0.70]
))
bud_s = sdf_binary(5, sdf_leaf(3, [0.25, 0.55, 0.25]), sdf_leaf(0, [0.20, 0.20, 0.20], offset=[0.0, 0.25, 0.0]), 0.15)
objects.append(make_field(
    "cathedral.pond.lily.bud.south", "Southern Water Lily Bud",
    [2.2, 0.36, 45.5], bud_s, [0.9, 1.0, 0.9],
    "material.logos.pond.lotus.white", [0.95, 0.95, 0.90]
))

# E. Blue Flag Sacred Water Irises (Royal Purple & Golden Sunburst Nodal Shallows)
def make_water_iris_colony_tree():
    leaves = []
    # 7 sword leaves radiating and arching in +Y
    for i in range(7):
        ang = i * (2.0 * math.pi / 7.0)
        lx = 0.22 * math.cos(ang)
        lz = 0.22 * math.sin(ang)
        leaf_blade = sdf_leaf(3, [0.035, 0.75, 0.08], offset=[lx, 0.70, lz])
        leaves.append(leaf_blade)
    # 3 blooming iris flower heads with drooping falls and upright standards
    flowers = []
    f_offsets = [(-0.15, 1.25, 0.12), (0.16, 1.30, -0.10), (0.0, 1.40, 0.18)]
    for fx, fy, fz in f_offsets:
        throat = sdf_leaf(0, [0.08, 0.10, 0.08], offset=[fx, fy, fz])
        falls = sdf_leaf(3, [0.12, 0.22, 0.12], offset=[fx, fy - 0.14, fz])
        standards = sdf_leaf(3, [0.09, 0.25, 0.09], offset=[fx, fy + 0.16, fz])
        fl = sdf_binary(5, throat, sdf_binary(5, falls, standards, 0.08), 0.1)
        flowers.append(fl)
    
    t_leaves = leaves[0]
    for l in leaves[1:]:
        t_leaves = sdf_binary(5, t_leaves, l, 0.12)
    t_fl = flowers[0]
    for f in flowers[1:]:
        t_fl = sdf_binary(5, t_fl, f, 0.1)
    return sdf_binary(5, t_leaves, t_fl, 0.15)

iris_tree = make_water_iris_colony_tree()
objects.append(make_field(
    "cathedral.pond.iris.north", "Northern Blue Flag Sacred Water Iris Colony",
    [-4.0, 0.28, 46.2], iris_tree, [1.4, 2.0, 1.4],
    "material.logos.pond.iris", [0.45, 0.20, 0.85],
    extra_props={
        "isFlora": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 1.8},
        "description": {"t": "string", "v": "Blue Flag sacred water irises with velvet royal purple falls and radiant gold throat accents"}
    }
))
objects.append(make_field(
    "cathedral.pond.iris.south", "Southern Blue Flag Sacred Water Iris Colony",
    [4.5, 0.28, 53.8], iris_tree, [1.4, 2.0, 1.4],
    "material.logos.pond.iris", [0.45, 0.20, 0.85],
    extra_props={
        "isFlora": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 1.8},
        "description": {"t": "string", "v": "Blue Flag sacred water irises flourishing in the southern marsh shallows"}
    }
))

# F. Shoreline Maidenhair Fern Fronds (Draped over mossy boulders)
def make_fern_mound_tree():
    f1 = sdf_leaf(3, [0.85, 0.18, 0.45], offset=[0.0, 0.15, 0.0])
    f2 = sdf_leaf(3, [0.65, 0.16, 0.55], offset=[-0.3, 0.22, 0.2])
    f3 = sdf_leaf(3, [0.70, 0.14, 0.50], offset=[0.35, 0.18, -0.2])
    f4 = sdf_leaf(3, [0.55, 0.12, 0.40], offset=[0.0, 0.28, 0.35])
    return sdf_binary(5, sdf_binary(5, f1, f2, 0.15), sdf_binary(5, f3, f4, 0.15), 0.18)

fern_tree = make_fern_mound_tree()
objects.append(make_field(
    "cathedral.pond.ferns.north", "Shoreline Maidenhair Fern Fronds (North Boulder)",
    [-7.2, 0.85, 50.2], fern_tree, [1.8, 1.0, 1.8],
    "material.logos.pond.reeds", [0.35, 0.88, 0.30]
))
objects.append(make_field(
    "cathedral.pond.ferns.south", "Shoreline Maidenhair Fern Fronds (South Boulder)",
    [6.9, 0.85, 48.8], fern_tree, [1.8, 1.0, 1.8],
    "material.logos.pond.reeds", [0.35, 0.88, 0.30]
))

# G. The Ancient Weeping Water Willow (Overlooking the waterfall grotto at NW bluff)
willow_trunk_base = sdf_leaf(3, [0.45, 1.8, 0.45], offset=[0.0, 1.2, 0.0])
willow_branch_e   = sdf_leaf(3, [1.2, 0.35, 0.55], offset=[0.9, 2.4, 0.2])
willow_branch_w   = sdf_leaf(3, [0.9, 0.30, 0.65], offset=[-0.8, 2.2, -0.3])
willow_canopy_top = sdf_leaf(3, [2.8, 0.90, 2.6], offset=[0.3, 3.2, 0.1])
willow_tendrils_1 = sdf_leaf(3, [2.4, 1.20, 2.2], offset=[0.5, 2.2, 0.2])
willow_tendrils_2 = sdf_leaf(3, [1.8, 0.90, 1.8], offset=[-0.4, 1.8, -0.4])

willow_wood = sdf_binary(5, willow_trunk_base, sdf_binary(5, willow_branch_e, willow_branch_w, 0.2), 0.25)
willow_foliage = sdf_binary(5, willow_canopy_top, sdf_binary(5, willow_tendrils_1, willow_tendrils_2, 0.3), 0.35)
willow_tree = sdf_binary(5, willow_wood, willow_foliage, 0.3)

objects.append(make_field(
    "cathedral.pond.willow", "Ancient Weeping Water Willow of Siloam",
    [-5.8, 0.50, 56.5], willow_tree, [4.0, 4.5, 4.0],
    "material.logos.pond.willow", [0.25, 0.75, 0.30],
    extra_props={
        "isAncientTree": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 1.6},
        "description": {"t": "string", "v": "Ancient weeping water willow with trailing emerald tendrils dipping toward the water"}
    }
))

# H. Floating Emerald Lily Pads (Flat discs in XZ, prim = 3)
def make_lilypad_cluster(pads):
    tree = None
    for px, pz, pr in pads:
        pad_disc = sdf_leaf(3, [pr, 0.025, pr], offset=[px, 0.0, pz])
        notch = sdf_leaf(3, [pr * 0.30, 0.05, pr * 0.30], offset=[px, 0.0, pz + pr * 0.75])
        notched_pad = sdf_binary(4, pad_disc, notch)
        if tree is None:
            tree = notched_pad
        else:
            tree = sdf_binary(2, tree, notched_pad)
    return tree

# Cluster 1: Around the Grand Dawn Lotus
lilypads_c1 = make_lilypad_cluster([(-0.6, -0.6, 0.85), (0.7, -0.5, 0.75), (0.1, 0.8, 0.70)])
objects.append(make_field(
    "cathedral.pond.lilypads.cluster.1", "Emerald Lily Pad Formation (Grand Lotus Fleet)",
    [-2.5, 0.22, 48.0], lilypads_c1, [2.2, 0.25, 2.2],
    "material.logos.pond.lilypad", [0.2, 0.85, 0.3]
))

# Cluster 2: Near North Cove
lilypads_c2 = make_lilypad_cluster([(-0.5, 0.0, 0.80), (0.6, 0.4, 0.70), (0.2, -0.7, 0.65)])
objects.append(make_field(
    "cathedral.pond.lilypads.cluster.2", "Emerald Lily Pad Formation (North Sanctuary)",
    [-4.5, 0.22, 51.0], lilypads_c2, [2.0, 0.25, 2.0],
    "material.logos.pond.lilypad", [0.2, 0.85, 0.3]
))

# Cluster 3: Near South Cove
lilypads_c3 = make_lilypad_cluster([(0.0, 0.0, 0.90), (-0.7, 0.6, 0.75), (0.8, -0.5, 0.65)])
objects.append(make_field(
    "cathedral.pond.lilypads.cluster.3", "Emerald Lily Pad Formation (South Sanctuary)",
    [3.5, 0.22, 47.0], lilypads_c3, [2.2, 0.25, 2.2],
    "material.logos.pond.lilypad", [0.2, 0.85, 0.3]
))

# Cluster 4: Stepping Stone Inlet
lilypads_c4 = make_lilypad_cluster([(-0.4, 0.3, 0.75), (0.5, -0.4, 0.80)])
objects.append(make_field(
    "cathedral.pond.lilypads.cluster.4", "Emerald Lily Pad Formation (Spring Inlet)",
    [0.5, 0.22, 44.5], lilypads_c4, [1.8, 0.25, 1.8],
    "material.logos.pond.lilypad", [0.2, 0.85, 0.3]
))

# I. Duckweed / Floating Water-Clover Micro-Clusters
def make_duckweed_cluster():
    clovers = []
    offsets = [(-0.25, -0.20), (0.22, -0.15), (0.0, 0.22), (-0.18, 0.16), (0.24, 0.20), (-0.05, -0.05)]
    for dx, dz in offsets:
        c1 = sdf_leaf(3, [0.08, 0.015, 0.08], offset=[dx, 0.0, dz])
        c2 = sdf_leaf(3, [0.06, 0.015, 0.06], offset=[dx + 0.04, 0.0, dz + 0.04])
        clovers.append(sdf_binary(2, c1, c2))
    tree = clovers[0]
    for c in clovers[1:]:
        tree = sdf_binary(2, tree, c)
    return tree

duckweed_tree = make_duckweed_cluster()
objects.append(make_field(
    "cathedral.pond.duckweed.1", "Floating Water-Clover Rosettes (North Inlet)",
    [-1.2, 0.23, 47.5], duckweed_tree, [0.9, 0.1, 0.9],
    "material.logos.pond.lilypad", [0.25, 0.90, 0.35]
))
objects.append(make_field(
    "cathedral.pond.duckweed.2", "Floating Water-Clover Rosettes (South Bay)",
    [2.8, 0.23, 49.5], duckweed_tree, [0.9, 0.1, 0.9],
    "material.logos.pond.lilypad", [0.25, 0.90, 0.35]
))
objects.append(make_field(
    "cathedral.pond.duckweed.3", "Floating Water-Clover Rosettes (Willow Cove)",
    [-3.2, 0.23, 53.5], duckweed_tree, [0.9, 0.1, 0.9],
    "material.logos.pond.lilypad", [0.25, 0.90, 0.35]
))


# ------------------------------------------------------------------------------
# 3. FAUNA & LIVING SPIRITS: CELESTIAL KOI, DRAGONFLIES & ELEMENTAL WISPS
# ------------------------------------------------------------------------------

# A. Streamlined Celestial Koi Fish (Anatomical Torpedo Body, Fins & Swept Caudal Tail)
def make_koi_fish_tree(body_l=0.65, body_w=0.16, body_h=0.14, tail_tilt=0.12):
    # Main hydrodynamic torso
    torso = sdf_leaf(3, [body_w, body_h, body_l])
    # Tapered head
    head  = sdf_leaf(3, [body_w * 0.85, body_h * 0.85, body_l * 0.45], offset=[0.0, -0.02, body_l * 0.65])
    # Flared caudal peduncle & tail fin
    tail  = sdf_leaf(3, [body_w * 0.25, body_h * 1.4, body_l * 0.55], offset=[tail_tilt, 0.0, -body_l * 0.95])
    # Dorsal ridge fin
    dorsal = sdf_leaf(3, [body_w * 0.20, body_h * 0.8, body_l * 0.35], offset=[0.0, body_h * 0.85, -body_l * 0.1])
    # Left & right pectoral fins
    pec_l = sdf_leaf(3, [body_w * 0.8, body_h * 0.15, body_l * 0.35], offset=[body_w * 1.0, -body_h * 0.3, body_l * 0.25])
    pec_r = sdf_leaf(3, [body_w * 0.8, body_h * 0.15, body_l * 0.35], offset=[-body_w * 1.0, -body_h * 0.3, body_l * 0.25])
    
    body_core = sdf_binary(5, torso, head, 0.12)
    fins = sdf_binary(5, dorsal, sdf_binary(5, pec_l, pec_r, 0.08), 0.10)
    return sdf_binary(5, body_core, sdf_binary(5, tail, fins, 0.10), 0.15)

koi_swimming_1 = make_koi_fish_tree(0.70, 0.17, 0.15, 0.15)
koi_swimming_2 = make_koi_fish_tree(0.60, 0.15, 0.13, -0.12)
koi_swimming_3 = make_koi_fish_tree(0.65, 0.16, 0.14, 0.08)

# Koi 1: Grand 24k Golden Kohaku (Near Grand Dawn Lotus)
objects.append(make_field(
    "cathedral.pond.koi.1", "Grand 24k Golden Kohaku Koi (Celestial Emperor)",
    [-1.6, 0.10, 49.2], koi_swimming_1, [0.9, 0.5, 1.2],
    "material.logos.pond.koi", [1.0, 0.75, 0.15],
    rot_deg=[0.0, -35.0, 0.0],
    extra_props={
        "isFauna": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 1.8},
        "description": {"t": "string", "v": "Grand golden Kohaku koi swimming gracefully beneath the Grand Lotus"}
    }
))

# Koi 2: Scarlet & Pearl Tancho Koi (Near Alabaster Lotus)
objects.append(make_field(
    "cathedral.pond.koi.2", "Scarlet & Pearl Tancho Koi (Sun-Crown)",
    [1.8, 0.09, 50.8], koi_swimming_2, [0.8, 0.45, 1.0],
    "material.logos.pond.koi", [0.95, 0.85, 0.65],
    rot_deg=[0.0, 45.0, 0.0],
    extra_props={
        "isFauna": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 1.6},
        "description": {"t": "string", "v": "Scarlet-crowned Tancho koi gliding toward the Alabaster Lotus"}
    }
))

# Koi 3: Celestial Cyan Shusui Koi (Circling the Abyssal Spring Heart)
objects.append(make_field(
    "cathedral.pond.koi.3", "Celestial Cyan Shusui Koi (Abyssal Guardian)",
    [0.2, 0.06, 52.6], koi_swimming_3, [0.85, 0.48, 1.1],
    "material.logos.pond.koi", [0.45, 0.85, 0.95],
    rot_deg=[0.0, 110.0, 0.0],
    extra_props={
        "isFauna": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.0},
        "description": {"t": "string", "v": "Iridescent Shusui koi patrolling the deep abyss spring"}
    }
))

# Koi 4: Young Golden Fry (Near Stepping Stones)
objects.append(make_field(
    "cathedral.pond.koi.4", "Young Golden Fry Koi (Playful Dart)",
    [-0.8, 0.12, 46.8], make_koi_fish_tree(0.42, 0.10, 0.09, -0.10), [0.6, 0.35, 0.7],
    "material.logos.pond.koi", [1.0, 0.80, 0.20],
    rot_deg=[0.0, -65.0, 0.0],
    extra_props={"isFauna": {"t": "bool", "v": True}}
))

# Koi 5: Twin Golden Fry (Near Spring Cascade Inlet)
objects.append(make_field(
    "cathedral.pond.koi.5", "Twin Golden Fry Koi (Spring Explorer)",
    [1.0, 0.10, 53.5], make_koi_fish_tree(0.45, 0.11, 0.10, 0.12), [0.6, 0.35, 0.7],
    "material.logos.pond.koi", [1.0, 0.80, 0.20],
    rot_deg=[0.0, 20.0, 0.0],
    extra_props={"isFauna": {"t": "bool", "v": True}}
))

# B. Gossamer Emerald Dragonflies (Hovering Over Water Blossoms)
def make_dragonfly_tree():
    # Slender segmented needle body along Z
    body = sdf_leaf(3, [0.025, 0.025, 0.25])
    head = sdf_leaf(0, [0.04, 0.04, 0.04], offset=[0.0, 0.0, 0.26])
    # 4 gossamer wings outstretched in X
    w_fore_l = sdf_leaf(3, [0.30, 0.008, 0.06], offset=[0.32, 0.02, 0.10])
    w_fore_r = sdf_leaf(3, [0.30, 0.008, 0.06], offset=[-0.32, 0.02, 0.10])
    w_hind_l = sdf_leaf(3, [0.24, 0.008, 0.05], offset=[0.26, 0.02, -0.02])
    w_hind_r = sdf_leaf(3, [0.24, 0.008, 0.05], offset=[-0.26, 0.02, -0.02])
    wings = sdf_binary(2, sdf_binary(2, w_fore_l, w_fore_r), sdf_binary(2, w_hind_l, w_hind_r))
    return sdf_binary(5, sdf_binary(5, body, head, 0.04), wings, 0.04)

objects.append(make_field(
    "cathedral.pond.dragonfly.1", "Gossamer Emerald Dragonfly (Spirit of Spring Air)",
    [-4.2, 0.72, 52.5], make_dragonfly_tree(), [0.8, 0.3, 0.8],
    "material.logos.pond.reeds", [0.35, 0.95, 0.45],
    extra_props={"light.intensity": {"t": "float", "v": 1.4}}
))
objects.append(make_field(
    "cathedral.pond.dragonfly.2", "Gossamer Celestial Dragonfly (Lotus Guardian)",
    [-2.2, 0.75, 47.5], make_dragonfly_tree(), [0.8, 0.3, 0.8],
    "material.logos.pond.reeds", [0.25, 0.85, 0.95],
    extra_props={"light.intensity": {"t": "float", "v": 1.4}}
))

# C. Diversified Elemental Will-o'-the-Wisps (6 Unique Celestial Spirits)
def make_wisp_tree():
    wisp_core = sdf_leaf(0, [0.18, 0.18, 0.18])
    wisp_halo = sdf_leaf(0, [0.28, 0.28, 0.28])
    return sdf_binary(5, wisp_core, wisp_halo, 0.12)

wisp_data = [
    ("1", [-2.0, 0.85, 47.0], "Dawn Rose Wisp (Spirit of Eos)", [1.0, 0.85, 0.90], 3.2),
    ("2", [1.2, 1.10, 49.5], "Celestial Azure Wisp (Spirit of Sophia)", [0.85, 0.95, 1.0], 3.0),
    ("3", [-0.2, 0.95, 52.5], "Crystalline Cyan Wisp (Spirit of Pneuma)", [0.80, 0.98, 1.0], 3.4),
    ("4", [3.5, 1.05, 51.0], "Viridian Eden Wisp (Spirit of Zoe)", [0.85, 1.0, 0.85], 2.8),
    ("5", [-3.5, 1.25, 55.0], "Twilight Amethyst Wisp (Spirit of Mystery)", [0.95, 0.85, 1.0], 3.0),
    ("6", [0.0, 1.65, 56.5], "Cascading Pearl Wisp (Spirit of the Headwaters)", [1.0, 1.0, 0.95], 3.8),
]
for wid, wpos, wname, wcol, wlight in wisp_data:
    objects.append(make_field(
        f"cathedral.pond.wisp.{wid}", wname,
        wpos, make_wisp_tree(), [0.6, 0.6, 0.6],
        "material.logos.pond.wisp", wcol,
        extra_props={
            "isWisp": {"t": "bool", "v": True},
            "light.intensity": {"t": "float", "v": wlight}
        }
    ))


# ------------------------------------------------------------------------------
# 4. GEOLOGY & SHORELINE: MOSS BOULDERS, STEPPING STONES & SPRING GROTTO
# ------------------------------------------------------------------------------

# A. Natural Mossy Stepping Stones (Curving path connecting North & South shores)
stepping_stone_coords = [
    ("1", [-1.8, 0.30, 45.5], [0.65, 0.16, 0.65]),
    ("2", [-0.4, 0.32, 47.8], [0.72, 0.18, 0.70]),
    ("3", [0.8, 0.31, 50.2],  [0.68, 0.17, 0.68]),
    ("4", [2.2, 0.28, 52.8],  [0.75, 0.16, 0.72])
]
for sid, spos, sdims in stepping_stone_coords:
    stone_core = sdf_leaf(2, [sdims[0], sdims[1], sdims[2]], p0=0.10)
    stone_cap  = sdf_leaf(3, [sdims[0] * 0.95, sdims[1] * 0.8, sdims[2] * 0.95], offset=[0.0, 0.04, 0.0])
    stone_tree = sdf_binary(5, stone_core, stone_cap, 0.12)
    objects.append(make_field(
        f"cathedral.pond.stepping_stone.{sid}", f"Mossy River Stepping Stone {sid}",
        spos, stone_tree, [sdims[0] * 1.5, sdims[1] * 1.8, sdims[2] * 1.5],
        "material.logos.pond.mossy_stone", [0.45, 0.65, 0.40]
    ))

# B. North and South Shoreline Boulder Mounds
boulder_n1 = sdf_leaf(2, [1.4, 0.5, 1.2], p0=0.20)
boulder_n2 = sdf_leaf(3, [1.1, 0.6, 1.1], offset=[-0.6, 0.15, 0.4])
boulder_n3 = sdf_leaf(3, [0.9, 0.45, 0.9], offset=[0.7, -0.08, -0.3])
boulders_north_tree = sdf_binary(5, boulder_n1, sdf_binary(5, boulder_n2, boulder_n3, 0.25), 0.3)
objects.append(make_field(
    "cathedral.pond.boulders.north", "North Shoreline Mossy Boulder Mound",
    [-7.0, 0.65, 50.0], boulders_north_tree, [2.5, 1.4, 2.5],
    "material.logos.pond.mossy_stone", [0.45, 0.65, 0.40]
))

boulder_s1 = sdf_leaf(2, [1.4, 0.5, 1.2], p0=0.20)
boulder_s2 = sdf_leaf(3, [1.1, 0.6, 1.1], offset=[0.6, 0.15, -0.4])
boulder_s3 = sdf_leaf(3, [0.9, 0.45, 0.9], offset=[-0.7, -0.08, 0.3])
boulders_south_tree = sdf_binary(5, boulder_s1, sdf_binary(5, boulder_s2, boulder_s3, 0.25), 0.3)
objects.append(make_field(
    "cathedral.pond.boulders.south", "South Shoreline Mossy Boulder Mound",
    [6.8, 0.65, 49.0], boulders_south_tree, [2.5, 1.4, 2.5],
    "material.logos.pond.mossy_stone", [0.45, 0.65, 0.40]
))

# C. Bioluminescent Reed Thickets & Cattails
def make_reed_thicket_tree():
    reeds = []
    offsets = [(-0.35, -0.25), (0.30, -0.20), (0.0, 0.25), (-0.28, 0.20), (0.32, 0.28)]
    for rx, rz in offsets:
        stem = sdf_leaf(3, [0.03, 1.0, 0.03], offset=[rx, 0.9, rz])
        cattail = sdf_leaf(3, [0.07, 0.24, 0.07], offset=[rx, 1.5, rz])
        pearl = sdf_leaf(0, [0.05, 0.05, 0.05], offset=[rx, 1.95, rz])
        reed_single = sdf_binary(5, stem, sdf_binary(5, cattail, pearl, 0.08), 0.1)
        reeds.append(reed_single)
    tree = reeds[0]
    for r in reeds[1:]:
        tree = sdf_binary(5, tree, r, 0.15)
    return tree

reed_thicket_tree = make_reed_thicket_tree()
objects.append(make_field(
    "cathedral.pond.reeds.north", "Northern Bioluminescent Reed Thicket",
    [-5.2, 0.2, 48.5], reed_thicket_tree, [1.6, 2.2, 1.6],
    "material.logos.pond.reeds", [0.55, 0.85, 0.25],
    extra_props={"light.intensity": {"t": "float", "v": 1.5}}
))
objects.append(make_field(
    "cathedral.pond.reeds.south", "Southern Bioluminescent Reed Thicket",
    [5.0, 0.2, 52.0], reed_thicket_tree, [1.6, 2.2, 1.6],
    "material.logos.pond.reeds", [0.55, 0.85, 0.25],
    extra_props={"light.intensity": {"t": "float", "v": 1.5}}
))
objects.append(make_field(
    "cathedral.pond.reeds.inlet", "Spring Inlet Wild Reed Bed",
    [0.0, 0.2, 55.5], reed_thicket_tree, [1.8, 2.0, 1.5],
    "material.logos.pond.reeds", [0.55, 0.85, 0.25],
    extra_props={"light.intensity": {"t": "float", "v": 1.5}}
))

# D. The Living Spring Grotto & Waterfall Cascade (Headwaters at West Shore)
grotto_mound = sdf_leaf(3, [2.4, 1.5, 1.8], offset=[0.0, 0.5, 0.0])
grotto_cavern = sdf_leaf(3, [1.5, 1.0, 1.4], offset=[0.0, 0.2, -0.5])
grotto_rock = sdf_binary(4, grotto_mound, grotto_cavern)
spring_waterfall = sdf_leaf(3, [0.6, 0.8, 0.3], offset=[0.0, 0.0, -0.4])
spring_basin_pool = sdf_leaf(3, [0.85, 0.2, 0.85], offset=[0.0, -0.6, -0.8])
spring_plume = sdf_binary(5, spring_waterfall, spring_basin_pool, 0.22)
grotto_full_tree = sdf_binary(5, grotto_rock, spring_plume, 0.28)

objects.append(make_field(
    "cathedral.pond.spring_grotto", "Living Spring Grotto & Cascade (Headwaters of Siloam)",
    [0.0, 1.4, 57.0], grotto_full_tree, [3.5, 2.5, 2.5],
    "material.logos.pond.water_cascade", [0.2, 0.9, 0.95],
    extra_props={
        "isHeadwaters": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 4.5},
        "description": {"t": "string", "v": "Natural living spring grotto and terraced water cascade flowing into the pond"}
    }
))


# ------------------------------------------------------------------------------
# 5. CONTEMPLATIVE ARCHITECTURE: MOON BRIDGE, WATER LANTERNS & PRAYER BENCH
# ------------------------------------------------------------------------------

# A. The Sacred Moon Bridge (Spanning the Outlet Brooklet at Z = 43.0, X = -2.6 to +2.6)
# Allows a Person to walk across the water and gaze north across the lilies toward the Cathedral!
objects.append(make_box(
    "cathedral.pond.bridge.pier.west", "Moon Bridge West Alabaster Pier",
    [-2.8, 0.35, 43.0], [0.8, 0.7, 1.4],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
objects.append(make_box(
    "cathedral.pond.bridge.pier.east", "Moon Bridge East Alabaster Pier",
    [2.8, 0.35, 43.0], [0.8, 0.7, 1.4],
    "material.logos.alabaster", [0.94, 0.92, 0.88]
))
# Arched Bridge Deck Segments (Ascending from West and East to Apex)
objects.append(make_box(
    "cathedral.pond.bridge.deck.w", "Moon Bridge West Ramp",
    [-1.6, 0.55, 43.0], [1.6, 0.16, 1.2],
    "material.logos.wood", [0.55, 0.35, 0.20],
    rot_deg=[0.0, 0.0, -12.0]
))
objects.append(make_box(
    "cathedral.pond.bridge.deck.apex", "Moon Bridge Crown Platform",
    [0.0, 0.78, 43.0], [1.6, 0.16, 1.2],
    "material.logos.wood", [0.55, 0.35, 0.20]
))
objects.append(make_box(
    "cathedral.pond.bridge.deck.e", "Moon Bridge East Ramp",
    [1.6, 0.55, 43.0], [1.6, 0.16, 1.2],
    "material.logos.wood", [0.55, 0.35, 0.20],
    rot_deg=[0.0, 0.0, 12.0]
))
# Moon Bridge Balustrades (North & South Parapets)
objects.append(make_box(
    "cathedral.pond.bridge.rail.north", "Moon Bridge North Cedar Railing",
    [0.0, 1.15, 43.55], [5.2, 0.55, 0.12],
    "material.logos.gold", [1.0, 0.82, 0.28]
))
objects.append(make_box(
    "cathedral.pond.bridge.rail.south", "Moon Bridge South Cedar Railing",
    [0.0, 1.15, 42.45], [5.2, 0.55, 0.12],
    "material.logos.gold", [1.0, 0.82, 0.28]
))

# B. Twin Ancient Alabaster Water Lanterns (Kasuga Stone Shrine Lanterns)
def make_water_lantern_tree():
    pedestal = sdf_leaf(2, [0.35, 0.45, 0.35], p0=0.08)
    stem     = sdf_leaf(3, [0.18, 0.55, 0.18], offset=[0.0, 0.65, 0.0])
    fire_box = sdf_leaf(2, [0.32, 0.32, 0.32], offset=[0.0, 1.25, 0.0], p0=0.04)
    hearth   = sdf_leaf(0, [0.16, 0.16, 0.16], offset=[0.0, 1.25, 0.0])
    roof     = sdf_leaf(3, [0.55, 0.18, 0.55], offset=[0.0, 1.55, 0.0])
    finial   = sdf_leaf(0, [0.10, 0.15, 0.10], offset=[0.0, 1.80, 0.0])
    structure = sdf_binary(5, pedestal, sdf_binary(5, stem, roof, 0.1), 0.15)
    lamp = sdf_binary(5, structure, sdf_binary(5, fire_box, hearth, 0.08), 0.12)
    return sdf_binary(5, lamp, finial, 0.08)

lantern_tree = make_water_lantern_tree()
objects.append(make_field(
    "cathedral.pond.lantern.north", "Ancient Alabaster Water Lantern (North Shore)",
    [-6.5, 1.15, 50.5], lantern_tree, [1.4, 2.2, 1.4],
    "material.logos.pond.lantern", [0.95, 0.90, 0.82],
    extra_props={
        "isLantern": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.6},
        "description": {"t": "string", "v": "Ancient carved alabaster water lantern casting warm golden light across the lily pads"}
    }
))
objects.append(make_field(
    "cathedral.pond.lantern.south", "Ancient Alabaster Water Lantern (South Shore)",
    [6.2, 1.15, 48.5], lantern_tree, [1.4, 2.2, 1.4],
    "material.logos.pond.lantern", [0.95, 0.90, 0.82],
    extra_props={
        "isLantern": {"t": "bool", "v": True},
        "light.intensity": {"t": "float", "v": 2.6},
        "description": {"t": "string", "v": "Ancient carved alabaster water lantern overlooking the southern lotus bay"}
    }
))

# C. Carved Stone Contemplation Bench
objects.append(make_box(
    "cathedral.pond.bench.legs", "Contemplation Bench Limestone Supports",
    [-3.5, 0.25, 43.5], [1.4, 0.40, 0.45],
    "material.logos.alabaster", [0.92, 0.90, 0.86]
))
objects.append(make_box(
    "cathedral.pond.bench.seat", "Contemplation Bench Cedar Planking",
    [-3.5, 0.48, 43.5], [1.8, 0.08, 0.65],
    "material.logos.wood", [0.55, 0.35, 0.20],
    extra_props={"description": {"t": "string", "v": "Carved stone and cedar prayer bench overlooking the sacred lotus pond"}}
))


# ==============================================================================
# THE CELESTIAL CLOISTER OF HARMONIC BOUNDS (NORTH CLOISTER QUADRANGLE)
# Located North of the Cathedral at X = -26.0, Z = 0.0 (Does NOT overlap with pond!)
# Tests 7 radically different types of SDF mathematical color-field bounds!
# ==============================================================================

# Architectural Terrace & Monolithic Cloister Columns
objects.append(make_box(
    "cathedral.bounds.terrace.platform", "Cloister Octagonal Marble Platform",
    [-26.0, 0.2, 0.0], [16.0, 0.4, 16.0],
    "material.logos.alabaster", [0.95, 0.94, 0.92]
))

# 8 Surrounding Cloister Boundary Columns
for ci in range(8):
    angle = 2.0 * math.pi * ci / 8.0
    cx = -26.0 + 7.0 * math.cos(angle)
    cz = 7.0 * math.sin(angle)
    objects.append(make_box(
        f"cathedral.bounds.col.{ci+1}", f"Cloister Fluted Column {ci+1}",
        [cx, 3.2, cz], [0.75, 5.8, 0.75],
        "material.logos.gold", [1.0, 0.82, 0.28]
    ))

# 1. OBELISK OF STRATIFIED LAW (Kind 1: Multi-Interval Stratified Piecewise Bounds)
obelisk_shaft = sdf_leaf(2, [0.55, 3.2, 0.55], p0=0.08)
obelisk_pyramid = sdf_leaf(0, [0.65, 0.65, 0.65], offset=[0.0, 3.3, 0.0])
obelisk_tree = sdf_binary(5, obelisk_shaft, obelisk_pyramid, 0.2)

objects.append(make_field(
    "cathedral.bounds.stratified_obelisk", "Obelisk of Stratified Law (Sharp Piecewise Zoned Bounds)",
    [-26.0, 3.4, -5.5], obelisk_tree, [1.5, 4.2, 1.5],
    "material.logos.bounds.stratified", [1.0, 0.85, 0.3],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "Multi-Interval Piecewise (Sharp Horizontal Strata)"},
        "light.intensity": {"t": "float", "v": 3.5}
    }
))

# 2. TERRACED ZIGGURAT OF QUANTIZATION (Kind 2: Quantized Floor Step-Function Bounds)
zigg_base = sdf_leaf(2, [1.6, 0.45, 1.6], p0=0.1)
zigg_mid  = sdf_leaf(2, [1.1, 0.45, 1.1], offset=[0.0, 0.7, 0.0], p0=0.08)
zigg_top  = sdf_leaf(2, [0.6, 0.45, 0.6], offset=[0.0, 1.4, 0.0], p0=0.06)
zigg_tree = sdf_binary(5, zigg_base, sdf_binary(5, zigg_mid, zigg_top, 0.15), 0.15)

objects.append(make_field(
    "cathedral.bounds.quantized_ziggurat", "Terraced Ziggurat of Quantization (Stepped Floor Function Bounds)",
    [-26.0, 1.8, 5.5], zigg_tree, [2.2, 2.5, 2.2],
    "material.logos.bounds.quantized", [0.3, 0.75, 0.9],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "Floor Quantization (Contour Stair-Steps)"},
        "light.intensity": {"t": "float", "v": 3.2}
    }
))

# 3. PLANETARY NEBULA ORB OF CHAOS & LOGOS (Kind 3: Continuous Perlin Turbulence Fractal Bounds)
nebula_core = sdf_leaf(0, [1.4, 1.4, 1.4])
nebula_lobes = sdf_leaf(3, [1.8, 0.8, 1.8])
nebula_tree = sdf_binary(5, nebula_core, nebula_lobes, 0.3)

objects.append(make_field(
    "cathedral.bounds.perlin_nebula_orb", "Planetary Nebula Orb of Logos (Continuous Perlin Turbulence Bounds)",
    [-31.0, 2.6, -2.8], nebula_tree, [2.2, 2.2, 2.2],
    "material.logos.bounds.perlin", [0.5, 0.2, 0.8],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "3D Perlin Noise Domain (Turbulent Fractal Veins)"},
        "light.intensity": {"t": "float", "v": 3.5}
    }
))

# 4. COSMIC SOLAR CORE & ECLIPSE HALO (Kind 4: Radial Quadratic Concentric Distance Falloff Bounds)
halo_core = sdf_leaf(0, [1.2, 1.2, 1.2])
halo_ring = sdf_leaf(3, [2.2, 0.35, 2.2])
halo_tree = sdf_binary(5, halo_core, halo_ring, 0.25)

objects.append(make_field(
    "cathedral.bounds.radial_halo_core", "Cosmic Solar Core & Halo (Radial Quadratic Distance Bounds)",
    [-31.0, 2.6, 2.8], halo_tree, [2.5, 2.2, 2.5],
    "material.logos.bounds.radial", [1.0, 0.8, 0.2],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "Radial Quadratic Falloff (Concentric Halo)"},
        "light.intensity": {"t": "float", "v": 4.0}
    }
))

# 5. CRYSTALLINE MONOLITH OF THE GREAT CHESSBOARD (Kind 5: Orthogonal 2D Checkerboard Lattice Bounds)
chess_cube = sdf_leaf(2, [1.2, 1.5, 1.2], p0=0.15)
chess_rhomb = sdf_leaf(3, [1.4, 1.4, 1.4])
chess_tree = sdf_binary(3, chess_cube, chess_rhomb) # Intersection facets

objects.append(make_field(
    "cathedral.bounds.checkerboard_monolith", "Crystalline Monolith of the Chessboard (Orthogonal Lattice Bounds)",
    [-21.0, 2.6, -2.8], chess_tree, [2.0, 2.5, 2.0],
    "material.logos.bounds.checkerboard", [0.8, 0.8, 0.8],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "Orthogonal Periodic Waves (Seamless 2D Grid)"},
        "light.intensity": {"t": "float", "v": 3.0}
    }
))

# 6. CHLADNI CYMATIC RESONATOR DISK (Kind 6: Chladni Acoustic Nodal Line Resonant Bounds)
chladni_disc = sdf_leaf(3, [2.2, 0.22, 2.2]) # Flat acoustic plate
chladni_node = sdf_leaf(0, [0.4, 0.4, 0.4], offset=[0.0, 0.2, 0.0])
chladni_tree = sdf_binary(5, chladni_disc, chladni_node, 0.2)

objects.append(make_field(
    "cathedral.bounds.chladni_resonator", "Chladni Cymatic Resonator Disk (Harmonic Nodal Zero-Crossing Bounds)",
    [-21.0, 2.2, 2.8], chladni_tree, [2.5, 1.5, 2.5],
    "material.logos.bounds.chladni", [0.3, 0.6, 0.9],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "Chladni Acoustic Nodal Lines (Harmonic Modal Lines)"},
        "light.intensity": {"t": "float", "v": 3.2}
    }
))

# 7. DUAL PILLAR OF SOL & LUNA (Kind 7: Bipartite Cardinal Hemisphere Half-Space Bounds)
dual_pillar_shaft = sdf_leaf(3, [0.85, 3.2, 0.85])
dual_pillar_twist = sdf_leaf(3, [1.1, 1.1, 1.1], offset=[0.0, 0.0, 0.0])
dual_pillar_tree = sdf_binary(5, dual_pillar_shaft, dual_pillar_twist, 0.3)

objects.append(make_field(
    "cathedral.bounds.dual_pillar", "Dual Pillar of Sol & Luna (Cardinal Hemisphere Half-Space Bounds)",
    [-26.0, 3.8, 0.0], dual_pillar_tree, [1.8, 4.5, 1.8],
    "material.logos.bounds.hemisphere", [1.0, 0.9, 0.6],
    extra_props={
        "isBoundTest": {"t": "bool", "v": True},
        "boundKind": {"t": "string", "v": "Bipartite Hemisphere Split (East/West Sol & Luna)"},
        "light.intensity": {"t": "float", "v": 4.5}
    }
))

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
        "law-logos-celestial-chord",
        "law-logos-covenant-weave",
        "law-logos-unison",
        "law-logos-season-toggle",
        "law-logos-pillar-pulse"
    ]
}

# ==============================================================================
# 14. AUTHORING BINARY PHYSICAL MATTER (.ecmatter FlatBuffer)
# ==============================================================================
def make_ecmatter(chunk_name="matter_cathedral_logos"):
    name_bytes = chunk_name.encode('utf-8') + b'\x00'
    pad = (4 - (len(name_bytes) % 4)) % 4
    str_data = struct.pack(f'<I{len(name_bytes)}s{pad}x', len(chunk_name), name_bytes)
    root_offset = struct.pack('<I', 12)
    vtable = struct.pack('<HHHH', 8, 12, 4, 8)
    root_table = struct.pack('<iII', 8, 12, 4)
    entities_vec = struct.pack('<I', 0)
    return root_offset + vtable + root_table + entities_vec + str_data

matter_bytes = make_ecmatter("matter_cathedral_of_the_living_logos")
matter_sha256 = hashlib.sha256(matter_bytes).hexdigest()
snapshot_id = matter_sha256[:16]

repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

# Write .ecmatter sidecars
matter_world_path = os.path.join(repo_root, "saves", "worlds", f"cathedral_of_the_living_logos.{snapshot_id}.ecmatter")
matter_fixed_world_path = os.path.join(repo_root, "saves", "worlds", "cathedral_of_the_living_logos.ecmatter")
matter_zone_path = os.path.join(repo_root, "saves", "zones", "Cathedral of the Living Logos", f"zone.{snapshot_id}.ecmatter")
matter_fixed_zone_path = os.path.join(repo_root, "saves", "zones", "Cathedral of the Living Logos", "zone.ecmatter")

os.makedirs(os.path.dirname(matter_world_path), exist_ok=True)
os.makedirs(os.path.dirname(matter_zone_path), exist_ok=True)

for mp in [matter_world_path, matter_fixed_world_path, matter_zone_path, matter_fixed_zone_path]:
    with open(mp, "wb") as f:
        f.write(matter_bytes)
print(f"Wrote physical matter (.ecmatter) sidecars: generation '{snapshot_id}' ({len(matter_bytes)} bytes)")

# Write zone.json (.ecform)
zone_path = os.path.join(repo_root, "saves", "zones", "Cathedral of the Living Logos", "zone.json")
with open(zone_path, "w") as f:
    json.dump(zone_doc, f, indent=2)
print(f"Wrote {zone_path} ({len(objects)} objects)")

# Full world package with matterGeneration linkage
world_doc = {
    "saveFormat": "zone-identity-v1",
    "injected_by": "Gemini Spark (authored under Zach's Hierarchy of Joys ontology)",
    "authors": ["Zach"],
    "currentZoneId": "Cathedral of the Living Logos",
    "matterGeneration": {
        "snapshotId": snapshot_id,
        "sha256": matter_sha256,
        "byteLength": len(matter_bytes),
        "schemaVersion": 1
    },
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
