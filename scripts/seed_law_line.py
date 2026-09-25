#!/usr/bin/env python3
"""First-Mover seed for the Law Line (docs/plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md).

Authored under Zach's authority (authors: Zach), injected by Claude Opus 5.5.

This is a FIRST SEED (FIRST_MOVER_AUTHORING.md §7): every file it writes is
new. It refuses to touch a file that already exists, stages each file beside
its destination, re-reads and verifies the staged copy, then renames it into
place atomically. Run it again and it changes nothing.

What it seeds:
  saves/zones/LawLine/zone.json   the LawLine Zone: a cube to act on, the
                                   Lexemes, and Lexeme --denotes--> Law Relations
  saves/laws/law-line-*/law.json  the Laws those Lexemes denote (where the
                                   opcodes live), the presets, and the three
                                   wiring Laws that let the Mac Terminal speak
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AUTHOR = "Zach"
INJECTED_BY = "Claude Opus 5.5 (Claude Code), Law Line seed, 2026-09-25"
ZONE_ID = "LawLine"


def none():
    return {"t": "none"}


def compare(op):
    return {"kind": 0, "path": "", "op": op, "operand": none()}


def law(identifier, name, *, enabled, activation=0, scope=0, condition=None, action=None,
        triggers=(), targets=()):
    body = {
        "activation": activation,
        "applicationLog": [],
        "authors": [AUTHOR],
        "authority": 0,
        "conditionMode": "all",
        "conditionSubjects": [],
        "drives": False,
        "enabled": enabled,
        "id": identifier,
        "name": name,
        "provenance": [{
            "directed": True, "entityA": identifier, "entityB": AUTHOR, "events": [],
            "type": "authored-by", "weight": 1.0,
        }],
        "retrigger": 0,
        "scope": scope,
        "targets": list(targets),
    }
    if condition is not None:
        body["conditionModel"] = condition
    if action is not None:
        body["actionModel"] = action
    return {
        "authors": [AUTHOR],
        "identifier": identifier,
        "injected_by": INJECTED_BY,
        "law": body,
        "triggers": list(triggers),
    }


# ---------------------------------------------------------------------------
# The Laws Lexemes denote. An OPEN slot (empty path, empty event…) makes the
# Law an opcode word; a Law with no open slot is a preset that fixes clauses.
# All are disabled: they are meanings, not actors.
# ---------------------------------------------------------------------------
OPCODES = [
    # (law id, law name, condition, action, lexeme symbols)
    ("law-line-op-eq", "means: equals", compare(0), None, ["equals", "is", "is equal to"]),
    ("law-line-op-ne", "means: differs from", compare(1), None, ["is not", "isn't", "does not equal"]),
    ("law-line-op-lt", "means: less than", compare(2), None, ["less than", "is less than", "below", "is below", "fewer than"]),
    ("law-line-op-le", "means: at most", compare(3), None, ["at most", "is at most"]),
    ("law-line-op-gt", "means: greater than", compare(4), None, ["greater than", "is greater than", "above", "is above", "more than"]),
    ("law-line-op-ge", "means: at least", compare(5), None, ["at least", "is at least"]),
    ("law-line-cond-iskind", "means: is a kind of", {"kind": 7, "beingKind": 0}, None, ["is a", "is an"]),
    ("law-line-cond-related", "means: related", {"kind": 2, "relationType": "", "otherId": ""}, None, ["related", "is related"]),
    ("law-line-cond-overlaps", "means: touching", {"kind": 11, "otherId": ""}, None, ["touching", "touches"]),
    ("law-line-act-set", "means: set", None, {"kind": 0, "path": "", "operand": none()}, ["set", "make", "change"]),
    ("law-line-act-add", "means: add", None, {"kind": 1, "path": "", "operand": none()}, ["add", "increase", "raise"]),
    ("law-line-act-scale", "means: scale", None, {"kind": 2, "path": "", "operand": none()}, ["scale", "multiply"]),
    ("law-line-act-publish", "means: publish", None, {"kind": 10, "eventType": "", "publishSubject": "", "publishObject": ""}, ["publish", "announce"]),
    ("law-line-act-destroy", "means: destroy", None, {"kind": 16, "elementToken": ""}, ["destroy", "delete"]),
    ("law-line-act-grant", "means: grant a property", None, {"kind": 12, "path": "", "propertyName": "", "operand": none()}, ["grant", "give"]),
    ("law-line-act-revoke", "means: revoke a property", None, {"kind": 14, "path": "", "propertyName": ""}, ["revoke"]),
    ("law-line-act-relate", "means: relate", None, {"kind": 20}, ["relate"]),
]

# Presets (Zach's list in "Natural Language Law Authoring.md").
PRESETS = [
    ("law-line-preset-event", "my event-triggered law", 0, 0, None,
     ["my event-triggered law", "my event-triggered"]),
    ("law-line-preset-constant", "my constantly-applied law", 1, 1, None,
     ["my constantly-applied law", "always", "constantly", "every moment"]),
    ("law-line-preset-becomes", "my law that fires when it becomes true", 2, 1, None,
     ["my law that fires when it becomes true", "whenever it becomes true", "becomes true"]),
    ("law-line-preset-unconditioned", "my law with no condition", 0, 0, {"kind": 3, "children": []},
     ["my law with no condition", "unconditionally"]),
]

# The wiring: whether a Terminal line becomes a Law is the world's decision.
WIRING = [
    law("law-line-hear", "Law Line · a terminal line is a spoken law sentence", enabled=True,
        action={"kind": 10, "eventType": "law-sentence-spoken", "publishSubject": "", "publishObject": ""},
        triggers=["terminal-line-entered"]),
    law("law-line-speak", "Law Line · a spoken law sentence asks the Terminal to author it", enabled=True,
        action={"kind": 1, "path": "@terminal-channel.speakRequests", "operand": {"t": "double", "v": 1.0}},
        triggers=["law-sentence-spoken"]),
    law("law-line-scope", "Law Line · bare paths complete against the Law Line cube", enabled=True,
        action={"kind": 0, "path": "@terminal-channel.scopeBeing", "operand": {"t": "string", "v": "law-line-cube"}},
        triggers=["zone-entered"]),
]


def slug(symbol):
    out = "".join(c if c.isalnum() else "-" for c in symbol.lower()).strip("-")
    while "--" in out:
        out = out.replace("--", "-")
    return out


def relation(a, b):
    return {"type": "denotes", "entityA": a, "entityB": b, "directed": True, "weight": 1.0,
            "events": [{"description": "denotes", "deltaWeight": 1.0, "timestamp": 1790380800}]}


def build():
    laws = []
    lexemes = []
    relations = []
    seen_ids = set()

    def lexeme_for(symbol, law_id):
        lex_id = "lexeme.law-line." + slug(symbol) + "." + law_id.replace("law-line-", "")
        assert lex_id not in seen_ids, lex_id
        seen_ids.add(lex_id)
        lexemes.append({"id": lex_id, "symbol": symbol})
        relations.append(relation(lex_id, law_id))

    for law_id, name, cond, act, symbols in OPCODES:
        laws.append(law(law_id, name, enabled=False, condition=cond, action=act))
        for s in symbols:
            lexeme_for(s, law_id)
    for law_id, name, activation, scope, cond, symbols in PRESETS:
        laws.append(law(law_id, name, enabled=False, activation=activation, scope=scope, condition=cond))
        for s in symbols:
            lexeme_for(s, law_id)
    laws.extend(WIRING)

    cube = {
        "authoritativeAxis": [0.0, 1.0, 0.0], "center": [0.0, 0.0, 0.0],
        "faceColors": [[0.85, 0.85, 0.82]] * 6, "geometryType": 0,
        "materialId": "material.default", "objectID": "law-line-cube", "renderMode": 0,
        "rotationResponsiveness": 10.0, "shapeKind": 0,
        "shapeParams": [0.5, 0.32, 0.5, 0.5, 0.35, 0.15, 2.0, 0.25, 0.12, 100.0, 100.0],
        "targetRotation": [0.0, 0.0, 0.0],
        "transform": [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.5, -3.0, 1.0],
        "x2D": 100.0, "y2D": 100.0, "zOrder2D": 0,
        "authoredProperties": {
            "displayName": {"t": "string", "v": "Law Line Cube"},
            "hp": {"t": "double", "v": 3.0},
            "glow": {"t": "double", "v": 0.0},
        },
    }
    zone = {
        "deletable": {AUTHOR: True},
        "formationRelations": relations,
        "identifier": ZONE_ID,
        "lawRefs": [l["identifier"] for l in laws],
        "lexemes": lexemes,
        "materials": [{"ambient": 0.2, "baseColor": [1, 1, 1], "diffuse": 0.8, "name": "default",
                       "opacity": 1.0, "shininess": 32.0, "specular": 1.0}],
        "name": "Law Line",
        "owner": AUTHOR,
        "parentZone": "",
        "qualities": {"ownerKind": "person", "purpose": "law-line-terminal-authoring",
                      "injected_by": INJECTED_BY},
        "scope": "Local",
        "spatialRoot": {
            "field": {"amplitude": 1.0, "baseDensity": 1.0, "frequency": 1.0, "mode": "Procedural"},
            "id": ZONE_ID + "_spatialRoot", "origin": [0.0, 0.0, 0.0], "scale": [1.0, 1.0, 1.0],
            "vectorField": {"amplitude": 1.0, "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
                            "frequency": 1.0, "mode": "Procedural"},
        },
        "world": {"objects": [cube]},
    }
    files = {os.path.join("saves", "zones", ZONE_ID, "zone.json"): zone}
    for l in laws:
        files[os.path.join("saves", "laws", l["identifier"], "law.json")] = l
    return files


def main():
    files = build()
    existing = [p for p in files if os.path.exists(os.path.join(ROOT, p))]
    if existing:
        if len(existing) == len(files):
            print("Law Line seed already present; nothing written.")
            return 0
        print("REFUSED: part of the Law Line seed already exists; a first seed never overwrites:")
        for p in existing:
            print("  " + p)
        return 1
    for rel, doc in files.items():
        dest = os.path.join(ROOT, rel)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        staged = dest + ".staged"
        with open(staged, "w") as f:
            json.dump(doc, f, indent=1, ensure_ascii=False)
            f.write("\n")
        with open(staged) as f:
            assert json.load(f) == doc, "staged copy does not round-trip: " + rel
        os.replace(staged, dest)
        print("wrote " + rel)
    print("authors: %s   injected_by: %s" % (AUTHOR, INJECTED_BY))
    return 0


if __name__ == "__main__":
    sys.exit(main())
