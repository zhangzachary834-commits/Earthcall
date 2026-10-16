#!/usr/bin/env python3
"""Pass #031: append an isolated compound-SDF Solar Reliquary to an explicit Sun save.

Dry-run by default. Use --write only after reviewing the inspected save.
Never modifies existing object JSON, materials, spatial fields or radiance witnesses.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import tempfile

PREFIX = "sun.reliquary.031."
MATERIALS = {"ivory": "sun.reset.ivory", "gold": "sun.reset.gold",
             "stone": "sun.reset.darkstone"}
X, Z = 22.5, -11.0


def leaf(prim, dims, offset=(0, 0, 0), p0=0):
    return {"op": 0, "prim": prim, "dims": list(dims), "offset": list(offset),
            "p0": p0, "p1": 0, "t": 0.5}


def binary(op, a, b, t=0.5):
    return {"op": op, "prim": 0, "dims": [0.5] * 3,
            "offset": [0, 0, 0], "p0": 0, "p1": 0, "t": t,
            "children": [a, b]}


def union(a, b): return binary(2, a, b)
def intersect(a, b): return binary(3, a, b)
def subtract(a, b): return binary(4, a, b)
def smooth(a, b, k=0.16): return binary(5, a, b, k)
def box(dims, at, radius=0): return leaf(2 if radius else 1, dims, at, radius)
def cylinder(radius, half_depth, at): return leaf(4, [radius, half_depth, 0.5], at)
def ellipsoid(dims, at): return leaf(3, dims, at)
def convex(planes, at): return {**leaf(8, [0.5] * 3, at), "planes": planes}


def shaft(x):
    # Convex tapered vertical column: narrower at its top, Z remains thickness.
    return convex([[.995, .1, 0, .56], [-.995, .1, 0, .56],
                   [0, 0, 1, .46], [0, 0, -1, .46],
                   [0, 1, 0, 1.6], [0, -1, 0, 1.6]], [x, 1.6, 0])


def capital(x):
    return convex([[.96, -.28, 0, .64], [-.96, -.28, 0, .64],
                   [0, 0, 1, .57], [0, 0, -1, .57],
                   [0, 1, 0, .30], [0, -1, 0, .30]], [x, 3.05, 0])


def flame(x, y, rx, ry, shift):
    # Swept crescent: subtract offset ellipsoid to carve the inside of a lobe.
    return subtract(ellipsoid([rx, ry, .37], [x, y, 0]),
                    ellipsoid([rx * .85, ry * .82, .55],
                              [x + shift, y - .18, .04]))


def sculpted_portal():
    # SdfPrim::Cylinder is Z-axis. XY arch is therefore correctly oriented.
    arch = intersect(
        subtract(cylinder(3.13, .53, [0, 3.25, 0]),
                 cylinder(2.35, .85, [0, 3.25, 0])),
        box([4.2, 2, 1.1], [0, 5.25, 0]))
    for x in (-2.7, 2.7):
        arch = smooth(arch, shaft(x))
        arch = smooth(arch, capital(x))
    # Incised continuous archivolt on the forward-facing Z surface.
    groove = intersect(
        subtract(cylinder(2.98, .15, [0, 3.25, .48]),
                 cylinder(2.88, .30, [0, 3.25, .48])),
        box([4, 2, .35], [0, 5.25, .48]))
    arch = subtract(arch, groove)
    for x in (-2.7, 2.7):
        for dx in (-.24, 0, .24):
            arch = subtract(arch, box([.065, 1.04, .16],
                                      [x + dx, 1.52, .48], .035))
    for args in ((0, 6.72, .68, 1.18, .26),
                 (-.93, 6.33, .44, .79, .19),
                 (.93, 6.33, .44, .79, -.19)):
        arch = smooth(arch, flame(*args), .14)
    return arch


def carved_relief():
    panel = box([1.60, 1.60, .26], [0, 0, 0], .17)
    ring = subtract(cylinder(1.20, .17, [0, 0, .23]),
                    cylinder(1.12, .32, [0, 0, .23]))
    panel = subtract(panel, ring)
    panel = subtract(panel, cylinder(.57, .19, [0, 0, .24]))
    for i in range(12):
        a = i * math.pi / 6
        c, s = math.cos(a), math.sin(a)
        # Twelve NEGATIVE radial incisions, not twelve added decorations.
        cut = convex([[c, s, 0, .30], [-c, -s, 0, .30],
                      [-s, c, 0, .055], [s, -c, 0, .055],
                      [0, 0, 1, .18], [0, 0, -1, .18]],
                     [c * .91, s * .91, .23])
        panel = subtract(panel, cut)
    return panel


def gold_boss():
    planes = [[math.cos(i * math.pi / 6), math.sin(i * math.pi / 6), 0, .46]
              for i in range(12)]
    return convex(planes + [[0, 0, 1, .11], [0, 0, -1, .11]], [0, 0, 0])


def footing():
    base = box([4.08, .29, 1.43], [0, 0, 0], .11)
    for z in (-1.37, 1.37):
        base = subtract(base, box([3.82, .045, .15], [0, .23, z], .02))
    return base


def properties(name, role):
    return {"displayName": {"t": "string", "v": name},
            "construction.feature": {"t": "string", "v": "sun-solar-reliquary-parallel-031"},
            "construction.pass": {"t": "float", "v": 31},
            "visual.role": {"t": "string", "v": role},
            "construction.provenance": {"t": "string", "v": "parallel study; independent of Sun #030 render gate"}}


def obj(suffix, name, role, center, extent, field, material, color):
    return {"objectID": PREFIX + suffix, "center": center,
            "authoritativeAxis": [0, 1, 0], "geometryType": 10, "shapeKind": 10,
            "materialId": material, "renderMode": 0, "rotationResponsiveness": 10,
            "shapeParams": [1, 1, 1, .5, .35, .15, 2, .25, .12, 100, 100],
            "targetRotation": [0, 0, 0],
            "transform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, *center, 1],
            "x2D": 100, "y2D": 100, "zOrder2D": 0,
            "faceColors": [list(color) for _ in range(6)],
            "field": field, "fieldExtent": extent,
            "authoredProperties": properties(name, role)}


def additions():
    ivory, gold, stone = (.96, .94, .86), (.95, .58, .14), (.18, .15, .13)
    return [
        obj("sculpted-portal", "Solar Reliquary — carved archivolt and fluted tapering supports",
            "continuous compound arch, shafts, capitals, negative grooves and flame crown",
            [X, 0, Z], [3.65, 8.1, .95], sculpted_portal(), MATERIALS["ivory"], ivory),
        obj("recessed-relief", "Solar Reliquary — twelve-ray carved solar relief",
            "recessed concentric and radial chisel-work", [X, 1.76, Z - 1.02],
            [1.85, 1.85, .45], carved_relief(), MATERIALS["ivory"], ivory),
        obj("gold-boss", "Solar Reliquary — faceted gold solar boss",
            "gilded twelve-sided boss seated flush in carved recess", [X, 1.76, Z - .86],
            [.55, .55, .18], gold_boss(), MATERIALS["gold"], gold),
        obj("plinth", "Solar Reliquary — independent footing",
            "negative-profile footing, mechanically engaged with supports", [X, -.24, Z], [4.30, .49, 1.65],
            footing(), MATERIALS["stone"], stone),
    ]


def bounds(o):
    center, ext = o["center"], o.get("fieldExtent", [1, 1, 1])
    return ([center[i] - ext[i] for i in range(3)],
            [center[i] + ext[i] for i in range(3)])


def clearance(a, b):
    alo, ahi = bounds(a)
    blo, bhi = bounds(b)
    return math.sqrt(sum(max(0, blo[i] - ahi[i], alo[i] - bhi[i]) ** 2
                         for i in range(3)))


def find_objects_array_end(raw):
    # Scope to world.objects; never accidentally edit a nested 'objects' key.
    start = raw.find('"world": {')
    if start < 0:
        raise ValueError("Expected pretty-printed world object")
    match = re.search(r'"objects"\s*:\s*\[', raw[start:])
    if not match:
        raise ValueError("Cannot find world.objects")
    opening = start + match.end() - 1
    depth, quoted, escaped = 0, False, False
    for i in range(opening, len(raw)):
        ch = raw[i]
        if quoted:
            if escaped: escaped = False
            elif ch == "\\": escaped = True
            elif ch == '"': quoted = False
        elif ch == '"': quoted = True
        elif ch == "[": depth += 1
        elif ch == "]":
            depth -= 1
            if depth == 0: return opening, i
    raise ValueError("Unterminated world.objects")


def append_preserving_existing_bytes(raw, new_objects):
    opening, end = find_objects_array_end(raw)
    tail = end - 1
    while raw[tail].isspace(): tail -= 1
    old_empty = raw[opening + 1: end].strip() == ""
    lines = json.dumps(new_objects, indent=2, ensure_ascii=False).splitlines()[1:-1]
    indented = "\n".join("    " + line for line in lines)
    insert = ("" if old_empty else ",") + "\n" + indented + "\n    "
    return raw[:tail + 1] + insert + raw[end:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zone", type=Path, required=True, help="Explicit Sun/zone.json path")
    parser.add_argument("--write", action="store_true", help="Opt in to atomic modification")
    parser.add_argument("--expect-sha256", help="Optional exact input-save digest; refuse stale snapshots")
    args = parser.parse_args()
    path = args.zone.expanduser().resolve()
    if path.name != "zone.json" or path.parent.name != "Sun":
        raise SystemExit("Refusing non-Sun zone path")
    if (path.parent / "zone.ecform").exists():
        raise SystemExit("REFUSED: zone.ecform may shadow zone.json; resolve save precedence first")
    raw = path.read_text(encoding="utf-8")
    digest = hashlib.sha256(raw.encode()).hexdigest()
    if args.expect_sha256 and digest.lower() != args.expect_sha256.lower():
        raise SystemExit("REFUSED: input save SHA-256 differs from inspected snapshot")
    zone = json.loads(raw)
    if zone.get("identifier") != "Sun":
        raise SystemExit("Refusing wrong Zone")
    before = zone["world"]["objects"]
    if any(o["objectID"].startswith(PREFIX) for o in before):
        raise SystemExit("REFUSED: Pass #031 already present; never duplicate")
    missing = set(MATERIALS.values()) - {m["name"] for m in zone["materials"]}
    if missing:
        raise SystemExit("Missing existing materials: " + repr(sorted(missing)))
    new = additions()
    existing_ids = {o["objectID"] for o in before}
    if any(o["objectID"] in existing_ids for o in new):
        raise SystemExit("Duplicate object ID")
    gap = min(clearance(n, old) for n in new for old in before)
    if gap < 1.0:
        raise SystemExit(f"REFUSED: only {gap:.3f}m clearance from existing geometry")
    output = append_preserving_existing_bytes(raw, new)
    after = json.loads(output)
    if (after["world"]["objects"][:len(before)] != before or
        after["materials"] != zone["materials"] or
        after["spatialRoot"] != zone["spatialRoot"] or
        after["spatialFields"] != zone["spatialFields"]):
        raise SystemExit("REFUSED: original authored data changed")
    if len(after["world"]["objects"]) != len(before) + len(new):
        raise SystemExit("REFUSED: wrong object count")
    print(f"PASS: {len(before)} original objects + {len(new)} Reliquary objects; "
          f"clearance {gap:.3f}m; source sha256 {digest}")
    print("New IDs:", ", ".join(o["objectID"] for o in new))
    if not args.write:
        print("DRY RUN ONLY. Re-run with --write to install in this explicit save.")
        return
    backup = path.with_name("zone.json.pre-reliquary-031." + digest[:12] + ".bak")
    shutil.copy2(path, backup)
    fd, temp = tempfile.mkstemp(prefix=".sun-031-", suffix=".json", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            stream.write(output)
            stream.flush()
            os.fsync(stream.fileno())
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise RuntimeError("Save changed during authoring; refusing replacement")
        os.replace(temp, path)
    finally:
        if os.path.exists(temp): os.unlink(temp)
    print("INSTALLED:", path, "BACKUP:", backup)


if __name__ == "__main__":
    main()
