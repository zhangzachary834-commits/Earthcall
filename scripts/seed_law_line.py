#!/usr/bin/env python3
"""First-Mover seed for the Law Line (docs/plans/NATURAL_LANGUAGE_LAW_AUTHORING_PLAN_2026-09-25.md).

Authored under Zach's authority (authors: Zach), injected by Claude Opus 5.5.

Patch, never regenerate (FIRST_MOVER_AUTHORING.md §7 rule 8). A Law file is
only ever CREATED when missing; one that exists is left exactly as it is. The
Zone file is PATCHED: the Lexemes, Relations and lawRefs it lacks are added,
nothing is removed or rewritten, every original entry is verified still
present, the old file is copied to saves/backups/, and the new one is staged
and renamed into place atomically. Run it again and it changes nothing.

2026-09-25, second pass (Zach: "make it more intuitive to use with more laws
and singulars"): value words ("red", "on", "off"…), each a Lexeme denoting a
Set Law that holds the value, and trigger presets ("when clicked", "on
hover"…), each a Lexeme denoting a disabled Law that fixes its trigger.

What it seeds:
  saves/zones/LawLine/zone.json   the LawLine Zone: a cube to act on, the
                                   Lexemes, and Lexeme --denotes--> Law Relations
  saves/laws/law-line-*/law.json  the Laws those Lexemes denote (where the
                                   opcodes live), the presets, and the three
                                   wiring Laws that let the Mac Terminal speak
"""
import copy
import json
import os
import shutil
import sys
import time

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
# Law an opcode word. A complete action-only Law can be a composable action
# fragment; a Law that fixes trigger/scope/condition clauses is a preset.
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
    ("law-line-act-set", "means: set", None, {"kind": 0, "path": "", "operand": none()}, ["set", "make", "change", "modify property"]),
    ("law-line-act-add", "means: add", None, {"kind": 1, "path": "", "operand": none()}, ["add", "increase", "raise"]),
    ("law-line-act-scale", "means: scale", None, {"kind": 2, "path": "", "operand": none()}, ["scale", "multiply"]),
    ("law-line-act-publish", "means: publish", None, {"kind": 10, "eventType": "", "publishSubject": "", "publishObject": ""}, ["publish", "announce"]),
    ("law-line-act-destroy", "means: destroy", None, {"kind": 16, "elementToken": ""}, ["destroy", "delete", "remove"]),
    ("law-line-act-grant", "means: grant a property", None, {"kind": 12, "path": "", "propertyName": "", "operand": none()}, ["grant", "give", "add property"]),
    ("law-line-act-revoke", "means: revoke a property", None, {"kind": 14, "path": "", "propertyName": ""}, ["revoke", "remove property"]),
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

# Value words: a Set with no path but a value. The Law holds what the word
# stands for; the sentence decides where it goes ("set color red").
def vec3(x, y, z):
    return {"t": "vec3", "x": x, "y": y, "z": z}


VALUES = [
    ("law-line-value-red", "means: red (1, 0, 0)", vec3(1.0, 0.0, 0.0), ["red"]),
    ("law-line-value-green", "means: green (0, 1, 0)", vec3(0.0, 1.0, 0.0), ["green"]),
    ("law-line-value-blue", "means: blue (0, 0, 1)", vec3(0.0, 0.0, 1.0), ["blue"]),
    ("law-line-value-white", "means: white (1, 1, 1)", vec3(1.0, 1.0, 1.0), ["white"]),
    ("law-line-value-black", "means: black (0, 0, 0)", vec3(0.0, 0.0, 0.0), ["black"]),
    ("law-line-value-yellow", "means: yellow (1, 1, 0)", vec3(1.0, 1.0, 0.0), ["yellow"]),
    ("law-line-value-orange", "means: orange (1, 0.5, 0)", vec3(1.0, 0.5, 0.0), ["orange"]),
    ("law-line-value-purple", "means: purple (0.5, 0, 1)", vec3(0.5, 0.0, 1.0), ["purple", "violet"]),
    ("law-line-value-pink", "means: pink (1, 0.4, 0.7)", vec3(1.0, 0.4, 0.7), ["pink"]),
    ("law-line-value-cyan", "means: cyan (0, 1, 1)", vec3(0.0, 1.0, 1.0), ["cyan"]),
    ("law-line-value-magenta", "means: magenta (1, 0, 1)", vec3(1.0, 0.0, 1.0), ["magenta"]),
    ("law-line-value-gold", "means: gold (1, 0.84, 0)", vec3(1.0, 0.84, 0.0), ["gold"]),
    ("law-line-value-gray", "means: gray (0.5, 0.5, 0.5)", vec3(0.5, 0.5, 0.5), ["gray", "grey"]),
    # "on" is also the structural trigger word; grammar position tells them
    # apart (a value slot admits only values), so the shared spelling is legal.
    ("law-line-value-true", "means: true", {"t": "bool", "v": True}, ["on", "yes", "y"]),
    ("law-line-value-false", "means: false", {"t": "bool", "v": False}, ["off", "no", "n"]),
]

# Presets that fix the trigger: the most common things a Law waits for.
TRIGGER_PRESETS = [
    ("law-line-preset-clicked", "fires when the subject is clicked", "object-clicked",
     ["when clicked", "on click", "when it is clicked"]),
    ("law-line-preset-hovered", "fires when the pointer enters the subject", "object-hover-entered",
     ["when hovered", "on hover", "when the pointer enters"]),
    ("law-line-preset-unhovered", "fires when the pointer leaves the subject", "object-hover-exited",
     ["when the pointer leaves", "on unhover"]),
    ("law-line-preset-touched", "fires when two objects collide", "objects-collided",
     ["when touched", "on contact", "when they collide"]),
    ("law-line-preset-landed", "fires when a person lands", "landed",
     ["when i land", "on landing"]),
    ("law-line-preset-jumped", "fires when a person jumps", "jump-started",
     ["when i jump", "on jump"]),
    ("law-line-preset-zone-entered", "fires when this Zone opens", "zone-entered",
     ["when the zone opens"]),
]

# Zach, 2026-10-04: general Create initializers must compile by Metalaw.
# Codex / GPT-6.1 Sol / session 01a10992-828e-7e80-890c-c64b09141e18.
# These are ordinary authored Laws: changing or removing their templates
# changes or refuses the resulting model, without changing the terminal parser.
COMPILER_INJECTED_BY = "Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-04"


def slot(name):
    return {"$slot": "/" + name}


def compiler(identifier, conditions, template):
    predicates = [{"kind": 0, "path": "compilation.input." + key, "op": 0,
                   "operand": value} for key, value in conditions.items()]
    predicates.append({"kind": 0, "path": "compilation.input.opcode", "op": 0,
                       "operand": "action.Create"})
    result = law(identifier, "Law Line · " + identifier.replace("law-line-compile-", ""),
                 enabled=True, condition={"kind": 3, "children": predicates},
                 action={"kind": 0, "path": "compilation.template",
                         "operand": json.dumps(template)}, targets=["terminal-channel"], triggers=["terminal-invocation-compilation-requested"])
    result["injected_by"] = COMPILER_INJECTED_BY
    return result


CREATE_COMPILERS = [
    compiler("law-line-compile-object", {"slot": "invocation", "selector": "Object"},
             {"kind": 11, "shapeKind": 0, "children": slot("children")}),
    compiler("law-line-compile-prototype", {"slot": "invocation", "prototype": True},
             {"kind": 11, "path": slot("selector"), "children": slot("children")}),
    compiler("law-line-compile-registered-value",
             {"slot": "initializer", "group": "registered", "expression": False},
             {"kind": 0, "path": slot("property"), "operand": slot("operand")}),
    compiler("law-line-compile-registered-expression",
             {"slot": "initializer", "group": "registered", "expression": True},
             {"kind": 8, "path": slot("property"), "function": slot("function"), "bindings": slot("bindings")}),
    compiler("law-line-compile-authored-value",
             {"slot": "initializer", "group": "authored", "expression": False},
             {"kind": 12, "propertyName": slot("property"), "operand": slot("operand")}),
    compiler("law-line-compile-authored-expression",
             {"slot": "initializer", "group": "authored", "expression": True},
             {"kind": 5, "children": [
                 {"kind": 12, "propertyName": slot("property"), "operand": {"t": "none"}},
                 {"kind": 8, "path": slot("property"), "function": slot("function"), "bindings": slot("bindings")},
             ]}),
]

# The wiring: whether a Terminal line becomes a Law is the world's decision.
WIRING = [
    # Terminal Zones (Zach, 2026-09-30): a line is a Law sentence only while
    # the LINE is in the Law Line -- wherever the Person's body stands.
    law("law-line-hear", "Law Line · a terminal line is a spoken law sentence", enabled=True,
        condition={"kind": 0, "path": "@terminal-channel.zone", "op": 0,
                   "operand": {"t": "string", "v": "LawLine"}},
        action={"kind": 10, "eventType": "law-sentence-spoken", "publishSubject": "", "publishObject": ""},
        triggers=["terminal-line-entered"]),
    law("law-line-speak", "Law Line · a spoken law sentence asks the Terminal to author it", enabled=True,
        action={"kind": 1, "path": "@terminal-channel.speakRequests", "operand": {"t": "double", "v": 1.0}},
        triggers=["law-sentence-spoken"]),
    # Confirmed deletion (Zach, 2026-09-25: "the metalaw that does the
    # deletions should say 'are you sure you want to delete?' and then either
    # yes or no … no means no delete and requires your yes to delete").
    # The Terminal only senses the request and the answer; these two Laws ask
    # and act. The question's words are here, in Law text, to be rewritten.
    law("law-line-ask-before-deleting", "Law Line · ask before deleting", enabled=True,
        action={"kind": 0, "path": "@terminal-channel.question",
                "operand": {"t": "string", "v": "Are you sure you want to delete"}},
        triggers=["terminal-deletion-requested"]),
    law("law-line-delete-when-confirmed", "Law Line · delete only on a yes", enabled=True,
        action={"kind": 16, "elementToken": "@event.object"},
        triggers=["terminal-deletion-confirmed"]),
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
    for law_id, name, value, symbols in VALUES:
        laws.append(law(law_id, name, enabled=False,
                        action={"kind": 0, "path": "", "operand": value}))
        for s in symbols:
            lexeme_for(s, law_id)
    for law_id, name, trigger, symbols in TRIGGER_PRESETS:
        laws.append(law(law_id, name, enabled=False, triggers=[trigger]))
        for s in symbols:
            lexeme_for(s, law_id)
    # A word still denotes a Law holding its model; no special Create enum
    # or terminal-specific spawn opcode is introduced.
    create_word = law("law-line-act-create", "means: Create a Singular", enabled=False,
                      action={"kind": 11, "shapeKind": 0})
    create_word["law"]["authoredProperties"] = {
        "sentence.arguments": {"t": "string", "v": "<‹kind›, properties: {‹property›: ‹value›}>"}}
    cube_word = law("law-line-value-cube", "means: Cube geometry", enabled=False,
                    action={"kind": 0, "path": "", "operand": {"t": "int", "v": 0}})
    my_word = law("law-line-root-my", "means: the speaking author's root", enabled=False,
                  action={"kind": 0, "path": "", "operand": "$author"})
    my_word["law"]["authoredProperties"] = {"sentence.root": {"t": "bool", "v": True}}
    for doc, symbols in [(create_word, ["Create"]), (cube_word, ["Cube"]), (my_word, ["my"] )]:
        doc["injected_by"] = COMPILER_INJECTED_BY
        laws.append(doc)
        for symbol in symbols:
            lexeme_for(symbol, doc["identifier"])
    laws.extend(CREATE_COMPILERS)
    batch = law("law-line-compile-sentences", "Law Line · register sentences in source order",
                enabled=True,
                condition={"kind": 3, "children": [
                    {"kind": 0, "path": "compilation.input.slot", "op": 0, "operand": "sentences"},
                    {"kind": 0, "path": "compilation.input.opcode", "op": 0, "operand": "sentence.batch"}]},
                action={"kind": 0, "path": "compilation.template",
                        "operand": json.dumps({"sentences": slot("sentences")})},
                targets=["terminal-channel"], triggers=["terminal-invocation-compilation-requested"])
    batch["injected_by"] = COMPILER_INJECTED_BY
    laws.append(batch)
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


def write_staged(dest, doc, expected_bytes=None):
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    staged = dest + ".staged"
    with open(staged, "w") as f:
        json.dump(doc, f, indent=1, ensure_ascii=False)
        f.write("\n")
    with open(staged) as f:
        assert json.load(f) == doc, "staged copy does not round-trip: " + dest
    if expected_bytes is not None:
        with open(dest, "rb") as f:
            assert f.read() == expected_bytes, "save changed while staging; refusing: " + dest
    os.replace(staged, dest)


def relation_key(r):
    return (r.get("type"), r.get("entityA"), r.get("entityB"))


def append_zone_bytes(old_bytes, old, new):
    """Insert only added root-array entries; preserve all original bytes."""
    text = old_bytes.decode("utf-8")
    decoder = json.JSONDecoder()
    cursor = text.index("{") + 1
    edits = []
    while True:
        while cursor < len(text) and text[cursor].isspace():
            cursor += 1
        if text[cursor] == "}":
            break
        key, cursor = decoder.raw_decode(text, cursor)
        while text[cursor].isspace():
            cursor += 1
        assert text[cursor] == ":"
        cursor += 1
        while text[cursor].isspace():
            cursor += 1
        value, end = decoder.raw_decode(text, cursor)
        if key in ("lexemes", "formationRelations", "lawRefs"):
            additions = new[key][len(old[key]):]
            if additions:
                close = end - 1
                insertion = close
                while insertion > cursor + 1 and text[insertion - 1].isspace():
                    insertion -= 1
                line_start = text.rfind("\n", cursor, close) + 1
                indent = len(text[line_start:close]) if text[line_start:close].isspace() else 2
                prefix = " " * (indent + 2)
                entries = ["\n" + "\n".join(prefix + line for line in json.dumps(entry, ensure_ascii=False, indent=2).splitlines())
                           for entry in additions]
                fragment = ("," if value else "") + ",".join(entries)
                if not value:
                    fragment += "\n" + " " * indent
                edits.append((insertion, fragment))
        cursor = end
        while text[cursor].isspace():
            cursor += 1
        if text[cursor] == ",":
            cursor += 1
        else:
            assert text[cursor] == "}"
            break
    for at, fragment in sorted(edits, reverse=True):
        text = text[:at] + fragment + text[at:]
    assert json.loads(text) == new, "byte-preserving patch differs from intended Zone"
    return text.encode("utf-8")


def write_zone_append(dest, old_bytes, old, new):
    staged = dest + ".staged"
    contents = append_zone_bytes(old_bytes, old, new)
    with open(staged, "wb") as f:
        f.write(contents)
    with open(staged, "rb") as f:
        assert json.loads(f.read()) == new
    with open(dest, "rb") as f:
        assert f.read() == old_bytes, "save changed while staging; refusing: " + dest
    os.replace(staged, dest)


def patch_zone(dest, seed):
    """Add what the Zone lacks; never remove or rewrite anything it has."""
    with open(dest, "rb") as f:
        old_bytes = f.read()
    old = json.loads(old_bytes)
    new = copy.deepcopy(old)
    have_lex = {l.get("id") for l in new.get("lexemes", [])}
    have_rel = {relation_key(r) for r in new.get("formationRelations", [])}
    have_refs = set(new.get("lawRefs", []))
    added = {"lexemes": 0, "relations": 0, "lawRefs": 0}
    for l in seed["lexemes"]:
        if l["id"] not in have_lex:
            new.setdefault("lexemes", []).append(l)
            added["lexemes"] += 1
    for r in seed["formationRelations"]:
        if relation_key(r) not in have_rel:
            new.setdefault("formationRelations", []).append(r)
            added["relations"] += 1
    for ref in seed["lawRefs"]:
        if ref not in have_refs:
            new.setdefault("lawRefs", []).append(ref)
            added["lawRefs"] += 1
    if not any(added.values()):
        return None

    # Verify nothing the Person's world had was lost or changed.
    for key, value in old.items():
        if key in ("lexemes", "formationRelations", "lawRefs"):
            assert new[key][:len(value)] == value, "patch would disturb " + key
        else:
            assert new[key] == value, "patch would disturb " + key

    backups = os.path.join(ROOT, "scratch", "backups", "law-line")
    os.makedirs(backups, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    shutil.copy2(dest, os.path.join(backups, "%s-zone-before-law-line-patch-%s.json" % (ZONE_ID, stamp)))
    write_zone_append(dest, old_bytes, old, new)
    return added


def main():
    files = build()
    zone_rel = os.path.join("saves", "zones", ZONE_ID, "zone.json")
    zone_seed = files.pop(zone_rel)
    changed = False

    for rel, doc in files.items():
        dest = os.path.join(ROOT, rel)
        if os.path.exists(dest):
            with open(dest) as f:
                if json.load(f) != doc:
                    print("kept as it is (differs from the seed; it is the world's now): " + rel)
            continue
        write_staged(dest, doc)
        print("created " + rel)
        changed = True

    zone_dest = os.path.join(ROOT, zone_rel)
    if not os.path.exists(zone_dest):
        write_staged(zone_dest, zone_seed)
        print("created " + zone_rel)
        changed = True
    else:
        added = patch_zone(zone_dest, zone_seed)
        if added:
            print("patched %s: +%d Lexemes, +%d Relations, +%d lawRefs (old bytes in scratch/backups/law-line/)"
                  % (zone_rel, added["lexemes"], added["relations"], added["lawRefs"]))
            changed = True

    if not changed:
        print("Law Line seed already present; nothing written.")
    else:
        print("authors: %s   original vocabulary injected_by: %s" % (AUTHOR, INJECTED_BY))
        print("Compiler additions injected_by: " + COMPILER_INJECTED_BY)
    return 0


if __name__ == "__main__":
    sys.exit(main())
