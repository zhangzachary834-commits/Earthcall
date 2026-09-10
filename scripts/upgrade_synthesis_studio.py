#!/usr/bin/env python3
"""Upgrade the lived-in Studio with authored instruments, preserving other beings.

Zach asked: "make the synthesis studio way cooler". Music/light direction and
implementation: Codex, session synthesis-studio-20260904, 2026-09-04.
This is a First Mover authoring tool; all live behavior is serialized Law text.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import tempfile

from author_synthesis_studio import (
    all_of, any_of, compare, clamp_pieces, copy_terms, map_path, mat4_translate,
    offset_terms, play_audio, publish, pv, seq, set_path, sin_factor, wave_term,
)

REVISION = "resonance-3"
MOVER = "studio.author.codex"
INK = (0.88, 0.93, 0.96)
DIM = (0.46, 0.57, 0.64)
TEAL = (0.30, 0.91, 0.78)
PANEL = (0.035, 0.055, 0.073)
NOTES = [
    ("c5", "C5", (0.94, 0.32, 0.43)),
    ("d5", "D5", (1.00, 0.53, 0.30)),
    ("e5", "E5", (0.98, 0.78, 0.34)),
    ("f5", "F5", (0.51, 0.86, 0.44)),
    ("g5", "G5", (0.27, 0.85, 0.70)),
    ("a5", "A5", (0.32, 0.69, 0.98)),
    ("b5", "B5", (0.66, 0.54, 0.97)),
]


def colors(rgb):
    return [list(rgb) for _ in range(6)]


def relation(a, b, kind="authored-by"):
    return {"type": kind, "entityA": a, "entityB": b,
            "directed": True, "weight": 1.0, "events": []}


def object2d(identifier, label, x, y, w, h, rgb, text=False, z=30, props=None):
    return {
        "objectID": identifier, "shapeKind": 13 if text else 12,
        "geometryType": 13 if text else 12,
        "shapeParams": [0.0] * 9 + [float(w), float(h)],
        "transform": mat4_translate(0, 0, 0), "center": [0, 0, 0],
        "x2D": float(x), "y2D": float(y), "zOrder2D": z,
        "materialId": "", "faceColors": colors(rgb),
        "authoredProperties": {
            "displayName": pv("string", label),
            "label2D" if text else "controlLabel": pv("string", label),
            "shape.width2D": pv("double", float(w)),
            "shape.height2D": pv("double", float(h)),
            "restY2D": pv("double", float(y)),
            **({"pickPriority": pv("double", -1)} if text else {}),
            **(props or {}),
        },
    }


def caption(identifier, label, x, y, size=14, rgb=DIM):
    return object2d(identifier, label, x, y, 0, size, rgb, text=True, z=40)


def law(identifier, name, condition, action, event=None, scope=0):
    return {"id": identifier, "name": name, "enabled": True, "authority": 0,
            "activation": 0 if event else 1, "scope": scope, "drives": False,
            "retrigger": 0, "conditionMode": "all", "authors": [MOVER],
            "conditionSubjects": [], "targets": [], "conditionModel": condition,
            "actionModel": action, "provenance": [relation(identifier, MOVER)],
            "applicationLog": []}


def upgrade(document):
    """Return a surgical copy. A completed revision never reapplies over later edits."""
    doc = deepcopy(document)
    if doc.get("studioUpgrade") == REVISION:
        return doc
    zones = doc.get("zones", [doc])
    zone = next(z for z in zones if z.get("identifier", z.get("name")) == "SynthesisStudio")
    objects = zone["world"]["objects"]
    by_id = {o["objectID"]: o for o in objects}
    # Top-level annotations are not guaranteed to survive an engine save.
    # The authoring marker's registered revision does, so a later Person edit
    # must remain protected even after those annotations have been dropped.
    if by_id.get(MOVER, {}).get("authoredProperties", {}).get("revision", {}).get("v") == REVISION:
        return doc
    edges = zone.setdefault("formationRelations", [])

    def put(obj, button=False):
        identifier = obj["objectID"]
        if identifier in by_id:
            # Keep unmentioned properties, attachments, textures, and metadata.
            old = by_id[identifier]
            props = {**old.get("authoredProperties", {}), **obj["authoredProperties"]}
            old.update(obj)
            old["authoredProperties"] = props
        else:
            objects.append(obj)
            by_id[identifier] = obj
        for edge in [relation(identifier, MOVER)] + (
                [relation(identifier, "category.control.button", "instance-of")] if button else []):
            if not any(all(e.get(k) == edge[k] for k in ("type", "entityA", "entityB")) for e in edges):
                edges.append(edge)

    # A model's authoring presence is an Object, never a counterfeit Person.
    put({"objectID": MOVER, "shapeKind": 0, "geometryType": 0,
         "shapeParams": [0.01] * 9, "transform": mat4_translate(0, -5, 0),
         "center": [0, -5, 0], "materialId": "", "faceColors": colors(PANEL),
         "authoredProperties": {"displayName": pv("string", "Codex / Studio authoring"),
                                "onBehalfOf": pv("string", "Zach"),
                                "revision": pv("string", REVISION)}})
    # No self-authorship relation: provenance points to the requesting human's
    # existing authorial marker; the model's declaration remains readable.
    edges[:] = [e for e in edges if not (e.get("entityA") == MOVER and e.get("entityB") == MOVER)]
    edges.append(relation(MOVER, "Zach", "commissioned-by"))

    state = by_id["state.studio"]["authoredProperties"]
    for name, value in {"voice": pv("string", "triangle"),
                        "inkR": pv("double", 1.0), "inkG": pv("double", 0.85),
                        "inkB": pv("double", 0.15),
                        "strokeSpacing": pv("double", 0.09),
                        "lastStrokeX": pv("double", 0.0),
                        "lastStrokeY": pv("double", 0.0),
                        "lastStrokeZ": pv("double", 0.0)}.items():
        state.setdefault(name, value)

    put(caption("hud.title", "SYNTHESIS / STUDIO", 32, 26, 28, INK))
    put(caption("hud.title.sub", "A room you can play.", 34, 64, 16, TEAL))
    instructions = ["01  PLAY the spectrum. Watch the room answer.",
                    "02  SPAWN an orb. Explore its sound with your pointer.",
                    "03  Choose an ink, enable DRAW, then drag on the easel.",
                    "Drag the desk slider to deepen the crystal's pulse.",
                    "Your notes, your color, your composition."]
    for i, label in enumerate(instructions):
        put(caption(f"hud.legend.{i}", label, 34, 98 + i * 19, 13, DIM))

    put(object2d("hud.dock.bg", "", 24, 584, 1232, 120, PANEL, z=10))
    put(caption("hud.resonance.dock-caption", "PLAY / CREATE", 42, 599, 12, TEAL))
    put(caption("hud.resonance.notes-caption", "THE SPECTRUM   /   C MAJOR", 380, 599, 12, DIM))
    put(caption("hud.resonance.draw-caption", "MARK MAKING", 1008, 599, 12, DIM))
    for identifier, label, x, w, rgb in [
        ("hud.btn.spawn-orb", "SPAWN ORB", 42, 150, (0.19, 0.50, 0.59)),
        ("hud.btn.toggle-theme", "DAY / NIGHT", 204, 150, (0.62, 0.48, 0.26)),
        ("hud.btn.draw-stroke", "DRAW: OFF", 1008, 228, (0.49, 0.35, 0.68)),
    ]:
        put(object2d(identifier, label, x, 624, w, 64, rgb, props={"label.size2D": pv("double", 16)}))

    for i, (slug, note, rgb) in enumerate(NOTES):
        x = 380 + i * 86
        put(object2d(f"hud.pad.{slug}", note, x, 624, 76, 64, rgb,
                     props={"label.size2D": pv("double", 23)}))
        # Seven fixed resonators. Playing never allocates more objects.
        pos = ((i - 3) * 0.57, 3.3 + 0.16 * (3 - abs(i - 3)), 1.9)
        put({"objectID": f"studio.resonance.{slug}", "shapeKind": 2, "geometryType": 2,
             "shapeParams": [0.13, 0.13, 0.13, 0.5, 0, 0, 0, 0, 0],
             "transform": mat4_translate(*pos), "center": list(pos), "materialId": "",
             "faceColors": colors(rgb), "authoredProperties": {
                 "displayName": pv("string", f"{note} / Resonator"),
                 "resonanceNote": pv("string", note), "struckAt": pv("double", -100),
                 "resonanceAge": pv("double", 100), "resonanceEnergy": pv("double", 0),
                 "resonanceBaseY": pv("double", pos[1]), "resonanceRadius": pv("double", 0.13),
             }})
        put(object2d(f"hud.resonance.meter.{slug}", "", x, 617, 76, 3, rgb, z=25,
                     props={"resonanceNote": pv("string", note), "struckAt": pv("double", -100),
                            "resonanceAge": pv("double", 100), "resonanceEnergy": pv("double", 0),
                            "resonanceMeter": pv("bool", True)}))

    put(caption("hud.resonance.note", "PLAY A NOTE", 1000, 34, 26, TEAL))
    put(caption("hud.resonance.note-hint", "and watch it become light", 1000, 69, 12))
    put(caption("hud.resonance.voice-caption", "VOICE / TRIANGLE", 1000, 109, 13, INK))
    put(caption("hud.resonance.ink-caption", "INK / SOLAR", 1000, 205, 13, INK))

    authored_laws = []
    triggers = {}

    def add(identifier, name, condition, action, event=None, scope=0):
        authored_laws.append(law(identifier, name, condition, action, event, scope))
        if event:
            triggers[identifier] = [event]

    # The shipped save had release Laws but no press Laws (15 baseline test
    # failures). Each space gets its own defined binding, with no failed sibling.
    for suffix, rest, target, offset in [("2d", "restY2D", "y2D", 3),
                                         ("3d", "restY", "position.y", -0.04)]:
        add(f"law-studio-resonance-press-{suffix}", f"Studio: Press feedback in {suffix}",
            compare(rest, 4, pv("double", -100)),
            map_path(target, {"r": rest}, offset_terms("r", offset)), "object-pressed")

    for i, (name, label) in enumerate([("triangle", "TRI"), ("sine", "SINE"), ("square", "SQR")]):
        put(object2d(f"hud.resonance.voice.{name}", label, 1000 + i * 80, 137, 72, 40,
                     (0.17, 0.27, 0.32), props={"studioVoice": pv("string", name)}), button=True)
        add(f"law-studio-resonance-voice-{name}", f"Studio: Select {name} voice",
            compare("studioVoice", 0, pv("string", name)),
            seq(set_path("@state.studio.voice", pv("string", name)),
                set_path("@hud.resonance.voice-caption.label2D", pv("string", f"VOICE / {name.upper()}"))),
            "control-activated")
        if name != "triangle":
            add(f"law-studio-resonance-play-{name}", f"Studio: Play {name} note",
                all_of(compare("isChordPad", 0, pv("bool", True)),
                       compare("@state.studio.voice", 0, pv("string", name))),
                seq(play_audio("acoustic.frequency", "acoustic.amplitude", name), publish("note-played")),
                "control-activated")

    for i, (name, rgb) in enumerate([("SOLAR", (1.0, 0.85, 0.15)),
                                     ("TIDAL", (0.22, 0.88, 0.82)),
                                     ("ORCHID", (0.85, 0.40, 0.95))]):
        put(object2d(f"hud.resonance.ink.{name.lower()}", name, 1000 + i * 80, 233, 72, 40,
                     rgb, props={"studioInk": pv("string", name), "label.size2D": pv("double", 13)}), button=True)
        add(f"law-studio-resonance-ink-{name.lower()}", f"Studio: Select {name.lower()} ink",
            compare("studioInk", 0, pv("string", name)),
            seq(*(set_path(f"@state.studio.ink{c}", pv("double", v)) for c, v in zip("RGB", rgb)),
                set_path("@hud.resonance.ink-caption.label2D", pv("string", f"INK / {name}"))),
            "control-activated")

    # Reaction is an edge; recovery is a continuous expression over world time.
    add("law-studio-resonance-struck", "Studio: A played note strikes its light",
        compare("resonanceNote", 0, operand_path="@event.subject.noteName"),
        map_path("struckAt", {"t": "time"}, copy_terms("t")), "note-played", scope=1)
    for slug, note, rgb in NOTES:
        add(f"law-studio-resonance-readout-{slug}", f"Studio: Show the last note {note}",
            compare("noteName", 0, pv("string", note)),
            seq(set_path("@hud.resonance.note.label2D", pv("string", f"{note} / PLAYED")),
                set_path("@hud.resonance.note.color", pv("vec3", rgb))), "note-played")
    add("law-studio-resonance-envelope", "Studio: Light relaxes after a note",
        compare("resonanceAge", 5, pv("double", 0)),
        seq(map_path("resonanceAge", {"t": "time", "s": "struckAt"},
                     [{"c": 1, "factors": {"t": 1}}, {"c": -1, "factors": {"s": 1}}]),
            map_path("resonanceAge", {"a": "resonanceAge"},
                     pieces=clamp_pieces("a", 0, 3600), input_var="a"),
            map_path("resonanceEnergy", {"a": "resonanceAge"},
                     [wave_term(1, {}, [{"kind": 2, "var": "a", "scale": -3.8, "shift": 0}])])), scope=1)
    add("law-studio-resonance-sculpture", "Studio: Resonators rise and swell with the note",
        compare("resonanceRadius", 4, pv("double", 0)),
        seq(map_path("shape.r", {"r": "resonanceRadius", "e": "resonanceEnergy"},
                     [{"c": 1, "factors": {"r": 1}}, {"c": 0.15, "factors": {"e": 1}}]),
            map_path("position.y", {"y": "resonanceBaseY", "e": "resonanceEnergy", "t": "time"},
                     [{"c": 1, "factors": {"y": 1}}, {"c": 0.22, "factors": {"e": 1}},
                      wave_term(0.025, {}, [sin_factor("t", 1.2)])])), scope=1)
    add("law-studio-resonance-meter", "Studio: The spectrum shows each note's decay",
        compare("resonanceMeter", 0, pv("bool", True)),
        seq(map_path("shape.height2D", {"e": "resonanceEnergy"},
                     [{"c": 3, "factors": {}}, {"c": 29, "factors": {"e": 1}}]),
            map_path("y2D", {"e": "resonanceEnergy"},
                     [{"c": 617, "factors": {}}, {"c": -29, "factors": {"e": 1}}])), scope=1)

    if "authoredLaws" in doc:
        register = doc["authoredLaws"]
        existing = {l["id"]: l for l in register["laws"]}
        # Preserve the original pad Law's identity and original attribution.
        original = existing["law-studio-pad-play"]
        original["conditionModel"] = all_of(original["conditionModel"],
            compare("@state.studio.voice", 0, pv("string", "triangle")))
        revision_edge = relation(original["id"], MOVER, "revised-by")
        if not any(all(edge.get(k) == revision_edge[k]
                       for k in ("type", "entityA", "entityB"))
                   for edge in original.setdefault("provenance", [])):
            original["provenance"].append(revision_edge)
        # Drawing is measured from the last dab in world space, not from a
        # one-frame cursor delta: slow real drawing must remain expressive.
        draw_law = existing["law-art-stroke-draw"]
        draw_law["conditionModel"] = all_of(
            compare("@interaction-channel.leftDown", 0, pv("bool", True)),
            compare("@interaction-channel.dragging", 0, pv("bool", True)),
            compare("@state.studio.drawMode", 0, pv("bool", True)),
            compare("isCanvas", 0, pv("bool", True)),
            compare("@world.pointerOver", 0, pv("bool", True)),
            any_of(compare("@interaction-channel.dragX", 1, pv("double", 0.0)),
                   compare("@interaction-channel.dragY", 1, pv("double", 0.0))),
            {"kind": 6, "function": {"input": "px", "pieces": [{"expr": {"terms": [
                {"c": 1.0, "factors": {"px": 2.0}}, {"c": -2.0, "factors": {"px": 1.0, "lx": 1.0}}, {"c": 1.0, "factors": {"lx": 2.0}},
                {"c": 1.0, "factors": {"py": 2.0}}, {"c": -2.0, "factors": {"py": 1.0, "ly": 1.0}}, {"c": 1.0, "factors": {"ly": 2.0}},
                {"c": 1.0, "factors": {"pz": 2.0}}, {"c": -2.0, "factors": {"pz": 1.0, "lz": 1.0}}, {"c": 1.0, "factors": {"lz": 2.0}},
                {"c": -1.0, "factors": {"s": 2.0}},
            ]}}]}, "bindings": {"px": "@interaction-channel.pointerWorldX", "py": "@interaction-channel.pointerWorldY", "pz": "@interaction-channel.pointerWorldZ", "lx": "@state.studio.lastStrokeX", "ly": "@state.studio.lastStrokeY", "lz": "@state.studio.lastStrokeZ", "s": "@state.studio.strokeSpacing"}, "lo": pv("double", 0.0)},
        )
        create = draw_law["actionModel"]
        if create.get("kind") != 5:
            draw_law["actionModel"] = seq(
                create,
                map_path("@state.studio.lastStrokeX", {"x": "@interaction-channel.pointerWorldX"}, copy_terms("x")),
                map_path("@state.studio.lastStrokeY", {"y": "@interaction-channel.pointerWorldY"}, copy_terms("y")),
                map_path("@state.studio.lastStrokeZ", {"z": "@interaction-channel.pointerWorldZ"}, copy_terms("z")),
            )
        # A new stroke receives the selected ink through ordinary property paths.
        draw = draw_law["actionModel"]
        def tint_births(node):
            if node.get("kind") == 11:
                node.setdefault("children", []).extend(
                    map_path(f"color.{c}", {"v": f"@state.studio.ink{c.upper()}"}, copy_terms("v"))
                    for c in "rgb")
            else:
                for child in node.get("children", []):
                    tint_births(child)
        tint_births(draw)
        # An upgrade revises these authored laws; it never means "add a second
        # identical orchestra." This also repairs the resonance-2 pass, whose
        # revision bump re-appended the 21 existing laws before its marker was
        # written. The IDs are stable, so replacing only these Law records is
        # surgical and leaves Person-authored Laws untouched.
        authored_ids = {entry["id"] for entry in authored_laws}
        register["laws"] = [entry for entry in register["laws"]
                            if entry.get("id") not in authored_ids]
        register["laws"].extend(authored_laws)
        register["formationMembers"] = [identifier for identifier in register["formationMembers"]
                                        if identifier not in authored_ids]
        register["formationMembers"].extend(entry["id"] for entry in authored_laws)
        register.setdefault("triggers", {}).update(triggers)
    doc["studioUpgrade"] = REVISION
    # Retain earlier attribution verbatim and add this intervention explicitly.
    doc["injected_by"] = str(doc.get("injected_by", "")) + " / Codex (resonance studio, 2026-09-04)"
    doc.setdefault("authors", ["Zach"])
    return doc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--apply", action="store_true", help="apply after snapshotting every affected save")
    args = parser.parse_args()
    paths = [args.root / "saves/worlds/synthesis_studio.json",
             args.root / "saves/worlds/synthesis_studio.ecform",
             args.root / "saves/zones/SynthesisStudio/zone.json"]
    changes = []
    for path in paths:
        if not path.exists():
            if path.suffix == ".ecform":
                continue
            raise SystemExit(f"REFUSED: missing Studio source {path}")
        before = json.loads(path.read_text())
        after = upgrade(before)
        if before != after:
            changes.append((path, after))
            print(f"Upgrade: {path}")
    if not changes or not args.apply:
        print("Already upgraded." if not changes else "Preview only. Use --apply to write with a complete backup.")
        return
    backup_root = args.root / "saves/backups"
    backup_root.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    backup = Path(tempfile.mkdtemp(prefix=f"synthesis-studio-{stamp}-", dir=backup_root))
    # Back up both readable manifests AND the binary companion, without deleting
    # or rewriting the latter. Existing external geometry offsets remain valid.
    for path in paths + [args.root / "saves/worlds/synthesis_studio.ecmatter"]:
        if path.exists():
            dest = backup / path.relative_to(args.root / "saves")
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
    for path, after in changes:
        path.write_text(json.dumps(after, indent=2) + "\n")
    print(f"Preserved originals: {backup}")
    print(f"Revised the Studio's authored draw spacing and reconciled its 21 resonance Laws; mover {MOVER}, requested by Zach.")


if __name__ == "__main__":
    main()
