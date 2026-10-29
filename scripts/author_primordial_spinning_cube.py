#!/usr/bin/env python3
"""
Resurrect the Spring-2025 Primordial Spinning Cube as an authored Earthcall being.

This deliberately does NOT use ShapeKind::Cube or Object::faceColors[6].

The visible form is an authored Field:
    max(|x|-h, |y|-h, |z|-h) = 0

Its six colour regions are an authored Material::colorExpr over local x/y/z.
Its rotation is an authored WhileTrue Law mapping world time and three persisted
spin-rate properties into the ordinary rotation Property.

Origination:
- Zach commissioned the original rotating, multicolour reference cube in Spring 2025
  so movement in the newborn 3D world had something visible to be relative to.
- GPT-4o authored the first implementation.
- Zach commissioned this resurrection on 2026-10-26.
- GPT-5.6 Sol authored this modern Field/Material/Law representation.
"""

from __future__ import annotations

from copy import deepcopy
import json
import os
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ZONE_ID = "Primordial Spinning Cube"
ZONE_PATH = ROOT / "saves" / "zones" / ZONE_ID / "zone.json"
WORLD_PATH = ROOT / "saves" / "worlds" / "primordial_spinning_cube.json"
LAW_ID = "law-primordial-cube-spin"
LAW_PATH = ROOT / "saves" / "laws" / LAW_ID / "law.json"
OBJECT_ID = "primordial.spinning-cube"
MATERIAL_ID = "primordial.spinning-cube.material"
AUTHOR = "Zach"
AGENT = "GPT-5.6 Sol — The Sun"
STAMP = "2026-10-26"
HALF_EXTENT = 1.5

PALETTE = {
    "posX": (0.96, 0.22, 0.20),
    "negX": (0.18, 0.72, 0.98),
    "posY": (0.98, 0.78, 0.16),
    "negY": (0.48, 0.25, 0.92),
    "posZ": (0.24, 0.88, 0.40),
    "negZ": (0.96, 0.42, 0.82),
}


def scalar(value):
    return {"op": 0, "scalarForm": {"terms": [{"c": float(value), "factors": {}}]}}


def value(name):
    return {"op": 1, "var": str(name)}


def add(a, b):
    return {"op": 4, "children": [a, b]}


def sub(a, b):
    return {"op": 5, "children": [a, b]}


def mul(a, b):
    return {"op": 6, "children": [a, b]}


def maximum(a, b):
    # MathNode::Op::Intersection is componentwise max. No new "cube" verb.
    return {"op": 21, "children": [a, b]}


def div(a, b):
    return {"op": 23, "children": [a, b]}


def absolute(a):
    return {"op": 25, "children": [a]}


def clamp(a, lo, hi):
    return {"op": 26, "children": [a, scalar(lo), scalar(hi)]}


def vec3(x, y, z):
    return {"op": 2, "children": [x, y, z]}


def sum_nodes(nodes):
    out = nodes[0]
    for node in nodes[1:]:
        out = add(out, node)
    return out


def piecewise(node):
    return {"input": "x", "pieces": [{"hasLo": False, "hasHi": False, "mathNode": node}]}


def tagged(t, v):
    if t == "vec3":
        return {"t": "vec3", "x": float(v[0]), "y": float(v[1]), "z": float(v[2])}
    return {"t": t, "v": v}


def mat4_at(x, y, z):
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        float(x), float(y), float(z), 1.0,
    ]


def cube_field_math():
    h = scalar(HALF_EXTENT)
    dx = sub(absolute(value("x")), h)
    dy = sub(absolute(value("y")), h)
    dz = sub(absolute(value("z")), h)
    return maximum(maximum(dx, dy), dz)


def face_weight(axis, positive):
    # A colour region, not a stored "face". The weight rises only as the local
    # point approaches one signed side of the authored implicit form.
    signed = value(axis) if positive else mul(scalar(-1.0), value(axis))
    onset = HALF_EXTENT * 0.72
    width = HALF_EXTENT * 0.28
    return clamp(div(sub(signed, scalar(onset)), scalar(width)), 0.0, 1.0)


def authored_colour_math():
    keys = ["posX", "negX", "posY", "negY", "posZ", "negZ"]
    weights = [
        face_weight("x", True), face_weight("x", False),
        face_weight("y", True), face_weight("y", False),
        face_weight("z", True), face_weight("z", False),
    ]
    denominator = maximum(sum_nodes(weights), scalar(1.0))

    def channel(i):
        return div(
            sum_nodes([mul(weight, scalar(PALETTE[key][i]))
                       for key, weight in zip(keys, weights)]),
            denominator,
        )

    return vec3(channel(0), channel(1), channel(2))


def relation(a, b, kind):
    return {
        "entityA": a,
        "entityB": b,
        "type": kind,
        "directed": True,
        "weight": 1.0,
        "events": [],
        "attachment": {
            "enabled": False,
            "inheritRotation": True,
            "inheritScale": True,
            "inheritTranslation": True,
            "childAnchor": [0.0, 0.0, 0.0],
            "parentAnchor": [0.0, 0.0, 0.0],
            "localOffset": [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
        },
        "authoredProperties": {"weight": tagged("float", 1.0)},
    }


def build_material():
    return {
        "name": MATERIAL_ID,
        "baseColor": [0.82, 0.82, 0.86],
        "opacity": 1.0,
        "shininess": 48.0,
        "specular": 0.45,
        "ambient": 0.28,
        "diffuse": 0.76,
        "colorExpr": piecewise(authored_colour_math()),
    }


def build_object():
    props = {
        "primordialCube": tagged("bool", True),
        "primordial.origin": tagged(
            "string", "Spring 2025: the first visible 3D reference object in Earthcall"),
        "primordial.form": tagged(
            "string", "max(abs(x)-h, abs(y)-h, abs(z)-h)"),
        "primordial.halfExtentMetres": tagged("float", HALF_EXTENT),
        "primordial.spin.xDegreesPerSecond": tagged("float", 22.0),
        "primordial.spin.yDegreesPerSecond": tagged("float", 37.0),
        "primordial.spin.zDegreesPerSecond": tagged("float", 13.0),
        "primordial.doctrine": tagged(
            "string",
            "The form exists because authored mathematics and Laws say so; "
            "Object has no privileged six-face truth."),
        "displayName": tagged("string", "The Primordial Spinning Cube — Resurrected"),
    }
    for key, rgb in PALETTE.items():
        props[f"primordial.palette.{key}"] = tagged("vec3", rgb)

    return {
        "objectID": OBJECT_ID,
        # Field is the generic mathematical manifestation substrate. The form
        # below happens to have six planar regions; no ShapeKind::Cube is used.
        "shapeKind": 10,
        "geometryType": 10,
        "shapeParams": [1.0, 1.0, 1.0, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
        "transform": mat4_at(0.0, 2.0, 0.0),
        "center": [0.0, 2.0, 0.0],
        "materialId": MATERIAL_ID,
        "renderMode": 0,
        "rotationResponsiveness": 10.0,
        "authoritativeAxis": [0.0, 1.0, 0.0],
        "targetRotation": [0.0, 0.0, 0.0],
        "x2D": 100.0,
        "y2D": 100.0,
        "zOrder2D": 0.0,
        "field": {
            "op": 0,
            "prim": 7,  # SdfPrim::Expr
            "dims": [0.5, 0.5, 0.5],
            "offset": [0.0, 0.0, 0.0],
            "p0": 0.0,
            "p1": 0.0,
            "t": 0.5,
            "mathNode": cube_field_math(),
        },
        "fieldExtent": [2.0, 2.0, 2.0],
        "fieldCellSize": 0.25,
        "authoredProperties": props,
    }


def build_law():
    # Map rotation deterministically from world time and persisted authored
    # rates. This intentionally does not use Object::angularVelocity because
    # that registered property is not yet a persisted Object state.
    component = lambda spin: {
        "op": 0,
        "scalarForm": {
            "terms": [{"c": 1.0, "factors": {"t": 1.0, spin: 1.0}}]
        },
    }
    action = {
        "kind": 8,  # ActionNode::Kind::Map
        "path": "rotation",
        "bindings": {
            "t": "time",
            "sx": "primordial.spin.xDegreesPerSecond",
            "sy": "primordial.spin.yDegreesPerSecond",
            "sz": "primordial.spin.zDegreesPerSecond",
        },
        "function": {
            "input": "t",
            "pieces": [{
                "hasLo": False,
                "hasHi": False,
                "mathNode": vec3(component("sx"), component("sy"), component("sz")),
            }],
        },
    }
    body = {
        "id": LAW_ID,
        "name": "Primordial Cube: authored rotation from world time",
        "enabled": True,
        "authority": 0,
        "activation": 1,  # WhileTrue
        "scope": 1,       # Everyone; condition narrows to the tagged being.
        "drives": False,
        "retrigger": 0,
        "conditionMode": "all",
        "authors": [AUTHOR],
        "conditionSubjects": [],
        "targets": [],
        "applicationLog": [],
        "conditionModel": {
            "kind": 0,
            "path": "primordialCube",
            "op": 0,
            "operand": tagged("bool", True),
        },
        "actionModel": action,
        "provenance": [relation(LAW_ID, AUTHOR, "authored-by")],
    }
    return {
        "identifier": LAW_ID,
        "injected_by": f"{AGENT} · commissioned by {AUTHOR} · {STAMP}",
        "authors": [AUTHOR],
        "law": body,
        "triggers": [],
    }


def build_zone(material, object_record):
    return {
        "identifier": ZONE_ID,
        "name": ZONE_ID,
        "owner": AUTHOR,
        "parentZone": "",
        "scope": "Local",
        "deletable": {AUTHOR: True},
        "authors": [AUTHOR],
        "injected_by": (
            f"{AGENT} · {STAMP}. Resurrection of the Spring 2025 reference cube "
            "as authored Field + Material + Law; no Cube primitive or faceColors "
            "source of truth."),
        "qualities": {
            "ownerKind": "person",
            "primordial.epoch": "Spring 2025",
            "primordial.lineage": (
                "Zach request -> GPT-4o first visible 3D reference -> authored resurrection"),
            "primordial.geometryAuthority": "OntoMath Field expression",
            "primordial.behaviorAuthority": LAW_ID,
        },
        "materials": [material],
        "materialRefs": [MATERIAL_ID],
        "world": {"objects": [object_record]},
        "formationRelations": [],
        "lexemes": [],
        "lawRefs": [LAW_ID],
    }


def build_world(zone, material, law):
    return {
        "identifier": "primordial_spinning_cube",
        "saveFormat": "zone-identity-v1",
        "activeZone": ZONE_ID,
        "currentZoneId": ZONE_ID,
        "currentZone": 0,
        "cameraPos": [0.0, 2.6, 8.5],
        "cameraFront": [0.0, -0.07, -0.998],
        "cameraUp": [0.0, 1.0, 0.0],
        "yaw": -90.0,
        "pitch": -4.0,
        "flying": True,
        "worldPhysics": False,
        "worldMode": True,
        "worldTime": 0.0,
        "currentColor": [1.0, 1.0, 1.0],
        "currentTool": 0,
        "zoneRefs": [{"identifier": ZONE_ID, "kind": "zone"}],
        "zones": [deepcopy(zone)],
        "materials": [deepcopy(material)],
        "authoredLaws": {
            "laws": [deepcopy(law["law"])],
            "triggers": {},
            "formationMembers": [LAW_ID],
        },
    }


def atomic_write(path: Path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w", encoding="utf-8", dir=path.parent,
        prefix="." + path.name + ".staged-", delete=False
    ) as tmp:
        json.dump(data, tmp, indent=2)
        tmp.write("\n")
        staged = Path(tmp.name)
    os.replace(staged, path)


def verify(zone, law, world):
    obj = zone["world"]["objects"][0]
    assert obj["shapeKind"] == 10 and obj["geometryType"] == 10
    assert obj["field"]["prim"] == 7 and obj["field"].get("mathNode")
    assert "faceColors" not in obj
    assert zone["lawRefs"] == [LAW_ID]
    assert law["law"]["activation"] == 1
    assert law["law"]["actionModel"]["kind"] == 8
    assert law["law"]["actionModel"]["path"] == "rotation"
    assert law["law"]["conditionModel"]["path"] == "primordialCube"
    assert obj["materialId"] == zone["materials"][0]["name"] == MATERIAL_ID
    for axis in ("x", "y", "z"):
        assert f"primordial.spin.{axis}DegreesPerSecond" in obj["authoredProperties"]
    for side in PALETTE:
        assert f"primordial.palette.{side}" in obj["authoredProperties"]
    assert world["currentZoneId"] == ZONE_ID
    assert world["zones"][0]["identifier"] == ZONE_ID


def main():
    material = build_material()
    obj = build_object()
    law = build_law()
    zone = build_zone(material, obj)
    world = build_world(zone, material, law)

    verify(zone, law, world)
    atomic_write(ZONE_PATH, zone)
    atomic_write(LAW_PATH, law)
    atomic_write(WORLD_PATH, world)

    print("Primordial Spinning Cube resurrected.")
    print("  geometry: authored Field math, not ShapeKind::Cube")
    print("  colour: authored Material colorExpr, not faceColors[6]")
    print("  motion: authored WhileTrue Law from world time + persisted spin properties")
    print(f"  zone: {ZONE_PATH}")
    print(f"  law:  {LAW_PATH}")
    print(f"  world:{WORLD_PATH}")


if __name__ == "__main__":
    main()
