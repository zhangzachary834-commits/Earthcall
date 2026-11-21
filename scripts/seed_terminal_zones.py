#!/usr/bin/env python3
"""First-Mover seed for Terminal Zones (docs/Agenda/Tasks/Specific Tasks/
Interaction and Interface/Terminal_Zones/Terminal_Zones.md).

Zach, 2026-09-30: "WE NEED TO TREAT THIS LIKE ZONES (which terminal mode im in)
AND MAKE AN OPCODE TO SWITCH BETWEEN ZONES". Decisions: a terminal zone is a
real Earthcall Zone; `enter <zone>` moves only the terminal line; first zones
Identity, Quiet, World. And: do not build more systems on top of this until
the opcodes are verified as minimal-maximal invariants.

Authored under Zach's authority (authors: Zach), injected by Claude Opus 5.5.

Patch, never regenerate (FIRST_MOVER_AUTHORING.md §7 rule 8):
  * a Zone or Law file is only CREATED when missing; one that exists is left
    exactly as it is;
  * the one existing file touched, saves/laws/law-line-hear/law.json, gains a
    single condition and nothing else. Every other key is verified unchanged,
    the old file is copied to saves/backups/, and the new one is staged and
    renamed into place atomically. Run it again and it changes nothing.

What it seeds:
  saves/zones/Identity/zone.json    the line's Identity Zone. Its one Law asks
                                    the Terminal for a hidden passphrase line
  saves/zones/Quiet/zone.json       no Law hears a typed line here
  saves/zones/World/zone.json       GATED: exists, hears nothing yet (see task doc)
  saves/laws/law-terminal-identity-unlock/law.json
  law-line-hear gains  if @terminal-channel.zone == "LawLine", so a line is a
                       Law sentence only when the LINE is in the Law Line,
                       wherever the Person's body stands.
"""
import copy
import json
import os
import shutil
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AUTHOR = "Zach"
INJECTED_BY = "Claude Opus 5.5 (Claude Code), Terminal Zones seed, 2026-09-30"


def line_is_in(zone_id):
    return {"kind": 0, "path": "@terminal-channel.zone", "op": 0,
            "operand": {"t": "string", "v": zone_id}}


def law(identifier, name, *, condition=None, action=None, triggers=()):
    body = {
        "activation": 0, "applicationLog": [], "authors": [AUTHOR], "authority": 0,
        "conditionMode": "all", "conditionSubjects": [], "drives": False, "enabled": True,
        "id": identifier, "name": name,
        "provenance": [{"directed": True, "entityA": identifier, "entityB": AUTHOR, "events": [],
                        "type": "authored-by", "weight": 1.0}],
        "retrigger": 0, "scope": 0, "targets": [],
    }
    if condition is not None:
        body["conditionModel"] = condition
    if action is not None:
        body["actionModel"] = action
    return {"authors": [AUTHOR], "identifier": identifier, "injected_by": INJECTED_BY,
            "law": body, "triggers": list(triggers)}


def zone(identifier, name, purpose, law_refs):
    return {
        "deletable": {AUTHOR: True},
        "formationRelations": [],
        "identifier": identifier,
        "lawRefs": list(law_refs),
        "lexemes": [],
        "materials": [{"ambient": 0.2, "baseColor": [1, 1, 1], "diffuse": 0.8, "name": "default",
                       "opacity": 1.0, "shininess": 32.0, "specular": 1.0}],
        "name": name,
        "owner": AUTHOR,
        "parentZone": "",
        "qualities": {"ownerKind": "person", "purpose": purpose, "injected_by": INJECTED_BY},
        "scope": "Local",
        "spatialRoot": {
            "field": {"amplitude": 1.0, "baseDensity": 1.0, "frequency": 1.0, "mode": "Procedural"},
            "id": identifier + "_spatialRoot", "origin": [0.0, 0.0, 0.0], "scale": [1.0, 1.0, 1.0],
            "vectorField": {"amplitude": 1.0, "baseFlowX": 0.0, "baseFlowY": 0.0, "baseFlowZ": 0.0,
                            "frequency": 1.0, "mode": "Procedural"},
        },
        "world": {"objects": []},
    }


IDENTITY_LAW = law(
    "law-terminal-identity-unlock",
    "Identity · when the line enters, ask the Terminal for a hidden passphrase",
    condition=line_is_in("Identity"),
    action={"kind": 1, "path": "@terminal-channel.unlockRequests", "operand": {"t": "double", "v": 1.0}},
    triggers=["terminal-zone-entered"],
)

CREATE = {
    os.path.join("saves", "laws", IDENTITY_LAW["identifier"], "law.json"): IDENTITY_LAW,
    os.path.join("saves", "zones", "Identity", "zone.json"):
        zone("Identity", "Identity", "terminal-identity: become present by key",
             [IDENTITY_LAW["identifier"]]),
    os.path.join("saves", "zones", "Quiet", "zone.json"):
        zone("Quiet", "Quiet", "terminal-quiet: no Law hears a typed line", []),
    # GATED (Terminal_Zones.md): World's opcodes must be verified as
    # minimal-maximal invariants before any Law here hears a line.
    os.path.join("saves", "zones", "World", "zone.json"):
        zone("World", "World", "terminal-world: GATED until its opcodes are verified", []),
}

HEAR = os.path.join("saves", "laws", "law-line-hear", "law.json")


def indent_of(path):
    """Keep a patched file's own formatting, so its diff shows only the edit."""
    with open(path) as f:
        for line in f:
            stripped = line.lstrip(" ")
            if stripped and stripped != line:
                return len(line) - len(stripped)
    return 1


def write_staged(dest, doc, indent=1):
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    staged = dest + ".staged"
    with open(staged, "w") as f:
        json.dump(doc, f, indent=indent, ensure_ascii=False)
        f.write("\n")
    with open(staged) as f:
        assert json.load(f) == doc, "staged copy does not round-trip: " + dest
    os.replace(staged, dest)


def patch_hear():
    dest = os.path.join(ROOT, HEAR)
    if not os.path.exists(dest):
        print("absent (seed the Law Line first: scripts/seed_law_line.py): " + HEAR)
        return False
    with open(dest) as f:
        old = json.load(f)
    if "conditionModel" in old.get("law", {}):
        if old["law"]["conditionModel"] != line_is_in("LawLine"):
            print("kept as it is (already carries an authored condition): " + HEAR)
        return False
    new = copy.deepcopy(old)
    new["law"]["conditionModel"] = line_is_in("LawLine")
    # Nothing but the one condition may differ.
    for key, value in old.items():
        if key != "law":
            assert new[key] == value, "patch would disturb " + key
    for key, value in old["law"].items():
        assert new["law"][key] == value, "patch would disturb law." + key
    assert set(new["law"]) - set(old["law"]) == {"conditionModel"}
    backups = os.path.join(ROOT, "saves", "backups")
    os.makedirs(backups, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    shutil.copy2(dest, os.path.join(backups, "law-line-hear-before-terminal-zones-%s.json" % stamp))
    write_staged(dest, new, indent=indent_of(dest))
    print("patched %s: + condition @terminal-channel.zone == \"LawLine\" (old copy in saves/backups/)" % HEAR)
    return True


def main():
    changed = False
    for rel, doc in CREATE.items():
        dest = os.path.join(ROOT, rel)
        if os.path.exists(dest):
            print("kept as it is (it is the world's now): " + rel)
            continue
        write_staged(dest, doc)
        print("created " + rel)
        changed = True
    changed = patch_hear() or changed
    print("authors: %s   injected_by: %s" % (AUTHOR, INJECTED_BY) if changed
          else "Terminal Zones seed already present; nothing written.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
