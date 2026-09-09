#!/usr/bin/env python3
"""The living instrument: a preserving First Mover upgrade of Synthesis Studio.

Human direction: Zach's chromatic/gesture requests and his 2026-09-08 invitation
to make intention-to-act whole and human. Design and implementation: Codex
(GPT-6 Astra), session 01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44, 2026-09-08.
All runtime behavior below is authored Law/OntoMath data, not Python callbacks.
Preview by default; --apply creates a separate edition from the clean original.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import shutil

from author_synthesis_studio import (
    all_of, any_of, compare, clamp_pieces, copy_terms, map_path, mat4_translate,
    play_audio, publish, pv, seq, set_path, sin_factor, wave_term,
)
from upgrade_synthesis_studio import object2d, caption, colors, relation

REVISION = "living-instrument-1"
MOVER = "studio.author.astra"
SESSION = "01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44"
INK, DIM, GOLD = (0.91, 0.94, 0.96), (0.49, 0.60, 0.66), (0.92, 0.76, 0.45)
PANEL = (0.035, 0.055, 0.073)
# Existing diatonic IDs are preserved even when the authored octave changes.
NOTES = [
    ("c5", "C", 523.25, (0.91, 0.28, 0.37)),
    ("cs5", "C#", 554.37, (0.97, 0.39, 0.31)),
    ("d5", "D", 587.33, (0.98, 0.52, 0.28)),
    ("ds5", "D#", 622.25, (0.97, 0.66, 0.29)),
    ("e5", "E", 659.25, (0.92, 0.79, 0.34)),
    ("f5", "F", 698.46, (0.52, 0.82, 0.42)),
    ("fs5", "F#", 739.99, (0.27, 0.82, 0.57)),
    ("g5", "G", 783.99, (0.24, 0.80, 0.74)),
    ("gs5", "G#", 830.61, (0.27, 0.72, 0.89)),
    ("a5", "A", 880.00, (0.33, 0.58, 0.95)),
    ("as5", "A#", 932.33, (0.52, 0.46, 0.92)),
    ("b5", "B", 987.77, (0.72, 0.44, 0.88)),
]


def term(c, **factors):
    return {"c": c, "factors": factors}


def equal(path, value):
    return compare(path, 0, pv("bool" if isinstance(value, bool) else "string", value))


def upgrade(document):
    doc = deepcopy(document)
    zone = next(z for z in doc.get("zones", [doc])
                if z.get("identifier", z.get("name")) in ("SynthesisStudio", "SynthesisStudio.LivingInstrument"))
    objects = zone["world"]["objects"]
    by_id = {o["objectID"]: o for o in objects}
    if by_id.get(MOVER, {}).get("authoredProperties", {}).get("revision", {}).get("v") == REVISION:
        return doc
    edges = zone.setdefault("formationRelations", [])
    new_laws, new_triggers = [], {}

    def edge(a, b, kind="authored-by"):
        r = relation(a, b, kind)
        if not any(all(e.get(k) == r[k] for k in ("type", "entityA", "entityB")) for e in edges):
            edges.append(r)

    def put(obj, button=False):
        ident = obj["objectID"]
        if ident in by_id:
            old = by_id[ident]
            props = {**old.get("authoredProperties", {}), **obj.get("authoredProperties", {})}
            # Preserve material identity and painted textures on existing beings.
            material = old.get("materialId", "")
            old.update(obj)
            old["authoredProperties"] = props
            old["materialId"] = material
            edge(ident, MOVER, "revised-by")
        else:
            objects.append(obj)
            by_id[ident] = obj
            if ident != MOVER:
                edge(ident, MOVER)
        if button:
            edge(ident, "category.control.button", "instance-of")
        return by_id[ident]

    def add(slug, title, condition, action, event=None, scope=0):
        ident = "law-studio-living-" + slug
        new_laws.append({"id": ident, "name": "Studio: " + title,
            "enabled": True, "authority": 0, "activation": 0 if event else 1,
            "scope": scope, "drives": False, "retrigger": 0,
            "conditionMode": "all", "authors": [MOVER], "conditionSubjects": [],
            "targets": [], "conditionModel": condition, "actionModel": action,
            "provenance": [relation(ident, MOVER)], "applicationLog": []})
        if event:
            new_triggers[ident] = [event]

    def sphere(ident, pos, radius, rgb, props):
        return put({"objectID": ident, "shapeKind": 2, "geometryType": 2,
            "shapeParams": [radius, radius, radius, 0.5, 0, 0, 0, 0, 0],
            "transform": mat4_translate(*pos), "center": list(pos),
            "materialId": "", "faceColors": colors(rgb), "authoredProperties": props})

    sphere(MOVER, (0, -5.5, 0), 0.01, PANEL, {
        "displayName": pv("string", "Codex / GPT-6 Astra / Living instrument"),
        "onBehalfOf": pv("string", "Zach"), "session": pv("string", SESSION),
        "revision": pv("string", REVISION)})
    edge(MOVER, "Zach", "commissioned-by")
    state = by_id["state.studio"].setdefault("authoredProperties", {})
    for name, value in {"octaveFactor": 1.0, "octave": 5.0, "bloom": 0.55,
                        "motion": 0.35, "thirdRatio": 2 ** (4 / 12),
                        "lastNoteAt": -100.0}.items():
        state.setdefault(name, pv("double", value))
    state.setdefault("harmony", pv("string", "solo"))
    state.setdefault("soundInk", pv("bool", False))
    state.setdefault("lastNoteHz", pv("double", 523.25))
    # These were absent from some old snapshots although the existing draw Law reads them.
    for name, value in {"strokeSpacing": 0.09, "lastStrokeX": 0.0,
                        "lastStrokeY": 0.0, "lastStrokeZ": 0.0}.items():
        state.setdefault(name, pv("double", value))

    put(caption("hud.title", "SYNTHESIS / STUDIO", 32, 24, 28, INK))
    put(caption("hud.title.sub", "The hand can reach the law.", 34, 64, 16, GOLD))
    for i, text in enumerate([
        "PLAY  /  twelve colors, a world of intervals",
        "SHAPE /  drag the expression field; let the room open",
        "MAKE  /  choose an ink and draw on the easel",
        "Your gesture becomes motion. Your notes become light.",
        "Every response is a Law you can make your own."]):
        put(caption(f"hud.legend.{i}", text, 34, 98 + i * 19, 12, DIM))
    put(object2d("hud.dock.bg", "", 24, 566, 1232, 138, PANEL, z=10))
    put(caption("hud.resonance.dock-caption", "CREATE / KEEP EXPLORING", 42, 581, 11, GOLD))
    put(caption("hud.resonance.notes-caption", "CHROMATIC / OCTAVE 5", 340, 581, 12, DIM))
    put(caption("hud.resonance.draw-caption", "", 42, 649, 11, DIM))
    for ident, label, x, y, w, h, rgb in [
        ("hud.btn.spawn-orb", "SPAWN ORB", 42, 610, 132, 36, (0.18, 0.42, 0.48)),
        ("hud.btn.toggle-theme", "DAY / NIGHT", 184, 610, 128, 36, (0.43, 0.34, 0.22)),
        ("hud.btn.draw-stroke", "DRAW: OFF", 42, 658, 270, 30, (0.39, 0.28, 0.50))]:
        put(object2d(ident, label, x, y, w, h, rgb,
                     props={"label.size2D": pv("double", 14)}))

    # The stable note identity connects pad, meter, and spatial resonator.
    for i, (slug, note, freq, rgb) in enumerate(NOTES):
        x = 340 + i * 73
        properties = {
            "isChordPad": pv("bool", True), "noteName": pv("string", note + "5"),
            "noteLetter": pv("string", note), "noteBaseHz": pv("double", freq),
            "acoustic.frequency": pv("double", freq), "acoustic.amplitude": pv("double", 0.22),
            "harmony.thirdHz": pv("double", freq * 2 ** (4 / 12)),
            "harmony.fifthHz": pv("double", freq * 2 ** (7 / 12)),
            "harmony.amplitude": pv("double", 0.11),
            "resonanceNote": pv("string", note + "5"), "struckAt": pv("double", -100),
            "resonanceAge": pv("double", 100), "resonanceEnergy": pv("double", 0),
            "livingPad": pv("bool", True),
            **{f"pigment{c}": pv("double", v) for c, v in zip("RGB", rgb)},
        }
        put(object2d(f"hud.pad.{slug}", note + "5", x, 620, 65, 68, rgb,
                     props={**properties, "label.size2D": pv("double", 20)}), button=True)
        put({"objectID": f"studio.pad.{slug}", "shapeKind": 0, "geometryType": 0,
             "shapeParams": [0.5, 0.5, 0.5, 0.5, 0, 0, 0, 0, 0],
             "transform": mat4_translate(0.24 + i * 0.18, 0.88, -0.35, (0.15, 0.14, 0.54)),
             "center": [0.24 + i * 0.18, 0.88, -0.35], "materialId": "",
             "faceColors": colors(rgb), "authoredProperties": {
                 **properties, "restY": pv("double", 0.88),
                 "controlLabel": pv("string", note + "5")}}, button=True)
        # Same hue, slightly nearer white: the sound version of the pad.
        sound_color = tuple(v + (1 - v) * 0.09 for v in rgb)
        put(object2d(f"hud.resonance.meter.{slug}", "", x, 613, 65, 3, sound_color,
            z=25, props={"resonanceNote": pv("string", note + "5"),
                "struckAt": pv("double", -100), "resonanceAge": pv("double", 100),
                "resonanceEnergy": pv("double", 0), "resonanceMeter": pv("bool", True),
                "pickPriority": pv("double", -1)}))
        pos = ((i - 5.5) * 0.43, 3.55 + 0.6 * math.sin(math.pi * i / 11), 1.9)
        sphere(f"studio.resonance.{slug}", pos, 0.13, sound_color, {
            "displayName": pv("string", note + " / Resonator"),
            "resonanceNote": pv("string", note + "5"), "struckAt": pv("double", -100),
            "resonanceAge": pv("double", 100), "resonanceEnergy": pv("double", 0),
            "resonanceBaseY": pv("double", pos[1]), "resonanceBaseX": pv("double", pos[0]),
            "resonanceRadius": pv("double", 0.13), "livingPhase": pv("double", i * 0.52)})
        edge(f"hud.pad.{slug}", f"studio.resonance.{slug}", "sounds-through")
        edge(f"hud.pad.{slug}", f"hud.resonance.meter.{slug}", "manifests-through")
        # A fixed constellation, no birth per note/frame. Small moons share the
        # note's envelope but trace authored orbits at a hand-shaped spread.
        for satellite in range(2):
            sphere(f"studio.living.satellite.{slug}.{satellite}", pos, 0.035, sound_color, {
                "displayName": pv("string", note + " / Orbit " + str(satellite + 1)),
                "resonanceNote": pv("string", note + "5"),
                "struckAt": pv("double", -100), "resonanceAge": pv("double", 100),
                "resonanceEnergy": pv("double", 0), "livingSatellite": pv("bool", True),
                "orbitX": pv("double", pos[0]), "orbitY": pv("double", pos[1]),
                "orbitPhase": pv("double", i * 0.52 + satellite * math.pi)})

    for ident, text, y, size, rgb in [
        ("hud.resonance.note", "PLAY A NOTE", 28, 25, GOLD),
        ("hud.resonance.note-hint", "a color you can hear", 61, 12, DIM),
        ("hud.resonance.voice-caption", "VOICE / TRIANGLE", 96, 12, INK),
        ("hud.resonance.ink-caption", "INK / SOLAR", 176, 12, INK),
        ("hud.living.octave-caption", "REGISTER / OCTAVE 5", 251, 12, INK),
        ("hud.living.harmony-caption", "HARMONY / SOLO", 321, 12, INK),
        ("hud.living.expression-caption", "EXPRESSION / TOUCH & SHAPE", 394, 12, GOLD),
        ("hud.living.expression-hint", "left: intimate     right: expansive", 546, 10, DIM)]:
        put(caption(ident, text, 1000, y, size, rgb))
    for i, voice in enumerate(("triangle", "sine", "square")):
        obj = deepcopy(by_id[f"hud.resonance.voice.{voice}"])
        obj.update(x2D=1000.0 + i * 80, y2D=119.0)
        obj["authoredProperties"]["restY2D"] = pv("double", 119)
        put(obj)
    for i, ink in enumerate(("solar", "tidal", "orchid")):
        obj = deepcopy(by_id[f"hud.resonance.ink.{ink}"])
        obj.update(x2D=1000.0 + i * 80, y2D=199.0)
        obj["authoredProperties"]["restY2D"] = pv("double", 199)
        put(obj)

    for octave in (3, 4, 5, 6):
        ident = f"hud.living.octave.{octave}"
        put(object2d(ident, str(octave), 1000 + (octave - 3) * 60, 274, 52, 32,
            (0.18, 0.27, 0.31), props={"octaveChoice": pv("string", str(octave)),
                                      "label.size2D": pv("double", 18)}), button=True)
        actions = [set_path("@state.studio.octaveFactor", pv("double", 2 ** (octave - 5))),
            set_path("@state.studio.octave", pv("double", octave)),
            set_path("@hud.living.octave-caption.label2D", pv("string", f"REGISTER / OCTAVE {octave}")),
            set_path("@hud.resonance.notes-caption.label2D", pv("string", f"CHROMATIC / OCTAVE {octave}"))]
        for slug, note, _, _ in NOTES:
            for prefix in ("hud.pad", "studio.pad"):
                actions.append(set_path(f"@{prefix}.{slug}.controlLabel", pv("string", note + str(octave))))
        add(f"octave-{octave}", f"Choose octave {octave}", equal("octaveChoice", str(octave)),
            seq(*actions), "control-activated")
    for i, harmony in enumerate(("solo", "fifth", "major", "minor")):
        put(object2d(f"hud.living.harmony.{harmony}", harmony.upper(), 1000 + i * 60, 344, 52, 32,
            (0.20, 0.25, 0.33), props={"harmonyChoice": pv("string", harmony),
                                     "label.size2D": pv("double", 11)}), button=True)
        add("harmony-" + harmony, "Choose " + harmony, equal("harmonyChoice", harmony),
            seq(set_path("@state.studio.harmony", pv("string", harmony)),
                set_path("@state.studio.thirdRatio", pv("double", 2 ** ((3 if harmony == "minor" else 4) / 12))),
                set_path("@hud.living.harmony-caption.label2D", pv("string", "HARMONY / " + harmony.upper()))),
            "control-activated")

    put(object2d("hud.living.expression", "", 1000, 419, 232, 116, (0.07, 0.13, 0.17),
        props={"livingExpression": pv("bool", True)}))
    for row in range(1, 4):
        put(object2d(f"hud.living.grid.h.{row}", "", 1008, 419 + row * 29, 216, 1,
            (0.12, 0.22, 0.26), z=32, props={"pickPriority": pv("double", -1)}))
        put(object2d(f"hud.living.grid.v.{row}", "", 1000 + row * 58, 427, 1, 100,
            (0.12, 0.22, 0.26), z=32, props={"pickPriority": pv("double", -1)}))
    put(object2d("hud.living.touch", "", 1110, 480, 10, 10, GOLD, z=38,
        props={"pickPriority": pv("double", -1), "livingCursor": pv("bool", True)}))
    add("expression", "The hand shapes bloom and motion",
        all_of(equal("livingExpression", True), equal("@world.pointerPressedOn", True)),
        seq(map_path("@state.studio.bloom", {"u": "@interaction-channel.hoveredU"},
                     pieces=clamp_pieces("u", 0, 1), input_var="u"),
            map_path("@state.studio.motion", {"v": "@interaction-channel.hoveredV"},
                     pieces=clamp_pieces("v", 0, 1, scale=-1, offset=1), input_var="v")), scope=1)
    add("cursor", "The expression field shows the hand's intention", equal("livingCursor", True),
        seq(map_path("x2D", {"b": "@state.studio.bloom"}, [term(1000), term(222, b=1)]),
            map_path("y2D", {"m": "@state.studio.motion"}, [term(525), term(-106, m=1)])), scope=1)
    add("pad-light", "A pad reverberates in its own color", equal("livingPad", True),
        seq(*(map_path("color." + c.lower(), {"p": "pigment" + c, "e": "resonanceEnergy"},
                      [term(1, p=1), term(0.22, e=1), term(-0.22, p=1, e=1)]) for c in "RGB")), scope=1)
    # Immediate visual touch; audio retains the established click/release contract.
    add("touch-light", "The pressed pad receives the hand", equal("livingPad", True),
        map_path("struckAt", {"t": "time"}, copy_terms("t")), "object-pressed")
    add("orbits", "The constellation follows the expression field", equal("livingSatellite", True),
        seq(map_path("shape.r", {"e": "resonanceEnergy"}, [term(0.025), term(0.045, e=1)]),
            map_path("position.x", {"x": "orbitX", "b": "@state.studio.bloom", "t": "time", "p": "orbitPhase"},
                [term(1,x=1), wave_term(0.14, {}, [sin_factor("t",0.8)]),
                 wave_term(0.28, {"b":1}, [sin_factor("p")])]),
            map_path("position.y", {"y":"orbitY", "m":"@state.studio.motion", "t":"time", "p":"orbitPhase", "e":"resonanceEnergy"},
                [term(1,y=1), term(0.15,e=1), wave_term(0.12,{"m":1},[sin_factor("t",1.4)]),
                 wave_term(0.19,{},[sin_factor("p",1,math.pi/2)])])), scope=1)
    for family, prop, statepath in [("voice", "studioVoice", "voice"),
                                     ("harmony", "harmonyChoice", "harmony")]:
        cond = compare(prop, 0, operand_path="@state.studio." + statepath)
        add("selected-" + family, "Show the selected " + family, cond,
            set_path("color", pv("vec3", (0.38, 0.55, 0.57))), scope=1)
        add("unselected-" + family, "Rest the other " + family + " choices",
            all_of(compare(prop, 1, operand_path="@state.studio." + statepath),
                   compare(prop, 1, pv("string", ""))),
            set_path("color", pv("vec3", (0.17, 0.24, 0.29))), scope=1)

    put(object2d("hud.living.sound-ink", "SOUND INK: OFF", 34, 214, 222, 32,
        (0.20, 0.31, 0.35), props={"soundInkToggle":pv("bool",True),
                                 "label.size2D":pv("double",13)}), button=True)
    put(caption("hud.living.sound-ink-hint", "Play a color. Draw it. Hear your marks.", 34, 258, 11, DIM))
    add("sound-ink-toggle", "Choose whether notes become drawing ink", equal("soundInkToggle",True),
        map_path("@state.studio.soundInk", {"v":"@state.studio.soundInk"}, [term(1),term(-1,v=1)]),
        "control-activated")
    for enabled in (True,False):
        add("sound-ink-label-"+str(enabled).lower(), "Show sound ink state",
            all_of(equal("soundInkToggle",True),equal("@state.studio.soundInk",enabled)),
            seq(set_path("controlLabel",pv("string","SOUND INK: "+("ON" if enabled else "OFF"))),
                set_path("color",pv("vec3",GOLD if enabled else (0.20,0.31,0.35)))), scope=1)
    add("note-ink", "The played note becomes the drawing pigment",
        all_of(equal("isChordPad",True),equal("@state.studio.soundInk",True)),
        seq(*(map_path("@state.studio.ink"+c,{"p":"pigment"+c},copy_terms("p")) for c in "RGB"),
            set_path("@hud.resonance.ink-caption.label2D",pv("string","INK / LAST NOTE"))), "note-played")

    register = doc.get("authoredLaws")
    if register:
        existing = {l["id"]: l for l in register["laws"]}
        def revise(ident, action):
            existing[ident]["actionModel"] = action
            existing[ident].setdefault("provenance", []).append(relation(ident, MOVER, "revised-by"))
        for voice, ident in [("triangle", "law-studio-pad-play"),
                              ("sine", "law-studio-resonance-play-sine"),
                              ("square", "law-studio-resonance-play-square")]:
            revise(ident, seq(
                map_path("acoustic.frequency", {"f":"noteBaseHz","o":"@state.studio.octaveFactor"}, [term(1,f=1,o=1)]),
                map_path("acoustic.amplitude", {"b":"@state.studio.bloom"}, [term(0.12),term(0.18,b=1)]),
                map_path("harmony.amplitude", {"a":"acoustic.amplitude"}, [term(0.4,a=1)]),
                map_path("harmony.thirdHz", {"f":"acoustic.frequency","r":"@state.studio.thirdRatio"}, [term(1,f=1,r=1)]),
                map_path("harmony.fifthHz", {"f":"acoustic.frequency"}, [term(2**(7/12),f=1)]),
                map_path("@state.studio.lastNoteHz", {"f":"acoustic.frequency"}, copy_terms("f")),
                play_audio("acoustic.frequency", "acoustic.amplitude", voice), publish("note-played")))
            for interval in ("fifth", "third"):
                harmony_cond = (any_of(equal("@state.studio.harmony","major"),equal("@state.studio.harmony","minor"))
                    if interval == "third" else any_of(*(equal("@state.studio.harmony", h) for h in ("fifth","major","minor"))))
                add(f"{voice}-{interval}", f"Sound the {voice} {interval}",
                    all_of(equal("isChordPad",True),equal("@state.studio.voice",voice),harmony_cond),
                    play_audio("harmony." + interval + "Hz", "harmony.amplitude", voice), "note-played")
        # A single causal chain computes each response before writing geometry.
        revise("law-studio-resonance-sculpture", seq(
            map_path("shape.r", {"r":"resonanceRadius","e":"resonanceEnergy","b":"@state.studio.bloom"},
                [term(1,r=1),term(0.12,e=1),term(0.13,e=1,b=1)]),
            map_path("position.x", {"x":"resonanceBaseX","b":"@state.studio.bloom"}, [term(0.85,x=1),term(0.3,x=1,b=1)]),
            map_path("position.y", {"y":"resonanceBaseY","e":"resonanceEnergy","m":"@state.studio.motion","t":"time"},
                [term(1,y=1),term(0.12,e=1),term(0.30,e=1,m=1),wave_term(0.06,{"m":1},[sin_factor("t",1.2)])])))
        revise("law-studio-resonance-meter", seq(
            map_path("shape.height2D", {"e":"resonanceEnergy"}, [term(3),term(29,e=1)]),
            map_path("y2D", {"e":"resonanceEnergy"}, [term(613),term(-29,e=1)])))
        for slug, note, _, rgb in NOTES:
            ident = "law-studio-resonance-readout-" + slug
            action = seq(set_path("@hud.resonance.note.label2D", pv("string", note + " / RESONATING")),
                         set_path("@hud.resonance.note.color", pv("vec3", rgb)))
            if ident in existing:
                revise(ident, action)
            else:
                add("readout-" + slug, "Read the note " + note, equal("noteName",note+"5"), action, "note-played")
        draw = existing["law-art-stroke-draw"]
        def tune_marks(node):
            if node.get("kind") == 11:
                node.setdefault("children", []).append(map_path("acoustic.frequency",
                    {"f":"@state.studio.lastNoteHz","s":"@state.studio.soundInk"},
                    [term(1046.5),term(-1046.5,s=1),term(1,s=1,f=1)]))
            for child in node.get("children", []):
                tune_marks(child)
        tune_marks(draw["actionModel"])
        if draw["actionModel"].get("kind") == 11:
            draw["actionModel"] = seq(draw["actionModel"],
                *(map_path("@state.studio.lastStroke"+c,
                    {"p":"@interaction-channel.pointerWorld"+c},copy_terms("p")) for c in "XYZ"))
        draw.setdefault("provenance",[]).append(relation(draw["id"],MOVER,"revised-by"))
        ids = {l["id"] for l in new_laws}
        register["laws"] = [l for l in register["laws"] if l["id"] not in ids] + new_laws
        members = register.setdefault("formationMembers", [])
        register["formationMembers"] = [m for m in members if m not in ids] + [l["id"] for l in new_laws]
        register.setdefault("triggers", {}).update(new_triggers)
    doc.setdefault("authoringPasses", []).append({"injected_by":"Codex (GPT-6 Astra)",
        "authors":["Zach"], "mover":MOVER, "session":SESSION, "revision":REVISION,
        "at":datetime.now(timezone.utc).isoformat(),
        "scope":"Chromatic pads, octaves, harmonies, expression field, resonant constellation; existing unrelated beings preserved."})
    return doc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    source = args.root / "saves/worlds/synthesis_studio.json"
    target = args.root / "saves/worlds/synthesis_studio_living.json"
    if target.exists():
        print("Existing living edition preserved:", target)
        return
    doc = upgrade(json.loads(source.read_text()))
    zone = doc["zones"][0]
    zone["identifier"] = zone["name"] = "SynthesisStudio.LivingInstrument"
    zone.setdefault("qualities", {})["forkedFrom"] = "SynthesisStudio"
    doc["currentZoneId"] = zone["identifier"]
    doc["zoneRefs"] = [{"identifier": zone["identifier"], "kind": "studio"}]
    # Zach explicitly requested the original authored room, not his lived-in
    # canvas experiments. A fresh Zone identity prevents their hydration here.
    for edge in zone.get("formationRelations", []):
        for endpoint in ("entityA", "entityB"):
            if edge.get(endpoint) == "SynthesisStudio":
                edge[endpoint] = zone["identifier"]
    print("Clean source:", source)
    print("New edition:", target)
    print("Beings:", len(zone["world"]["objects"]), "Laws:", len(doc["authoredLaws"]["laws"]))
    if not args.apply:
        print("Preview only; --apply creates the separate edition.")
        return
    # The clean JSON has inline authored geometry. Keep the old binary companion
    # with the fork too, for any offsets a later clean source revision carries.
    companion = source.with_suffix(".ecmatter")
    if companion.exists():
        shutil.copy2(companion, target.with_suffix(".ecmatter"))
    for path in (target, target.with_suffix(".ecform")):
        with path.open("x") as f:
            json.dump(doc, f, indent=2)
            f.write("\n")
    print("Original worlds and SynthesisStudio identity store unchanged.")
    print("Authored by", MOVER, "(Codex / GPT-6 Astra), commissioned by Zach; session", SESSION)


if __name__ == "__main__":
    main()
