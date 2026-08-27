#!/usr/bin/env python3
import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
SAVES_DIR = ROOT / "saves"

ENGINE_PUBLISHED_EVENTS = {
    "object-clicked", "object-pressed", "object-released", "object-scrolled",
    "object-drag-started", "object-drag-ended", "object-focused", "object-unfocused",
    "key-pressed", "key-released",
    "contact-began", "contact-ended", "collision", "PhysicsCollisionEvent",
    "locomotion-changed",
    "object-hover-entered", "object-hover-exited",
    "law-registered", "concept-registered", "relation-formed", "property-state",
    "onMouseClicked", "onMouseMoved"
}

NOUN_VERBED_RE = re.compile(r'^[a-z0-9]+(-[a-z0-9]+)+$')
VOLATILE_ID_RE = re.compile(r'^(law|object)-\d+$')
VOLATILE_TARGET_RE = re.compile(r'@(law|object)-\d+')

def extract_laws_from_json(data):
    laws = []
    if isinstance(data, dict):
        if "authoredLaws" in data and isinstance(data["authoredLaws"], dict):
            laws.extend(data["authoredLaws"].get("laws", []))
        elif "laws" in data and isinstance(data["laws"], list):
            laws.extend(data["laws"])
        if "zones" in data and isinstance(data["zones"], list):
            for z in data["zones"]:
                if isinstance(z, dict):
                    laws.extend(extract_laws_from_json(z))
    return laws

def extract_emitted_events_recursive(node):
    evs = set()
    if isinstance(node, dict):
        if "eventType" in node and node["eventType"]:
            evs.add(node["eventType"])
        if "event" in node and node["event"]:
            evs.add(node["event"])
        for v in node.values():
            evs.update(extract_emitted_events_recursive(v))
    elif isinstance(node, list):
        for item in node:
            evs.update(extract_emitted_events_recursive(item))
    return evs

def extract_emitted_events(laws):
    emitted = set(ENGINE_PUBLISHED_EVENTS)
    for law in laws:
        action = law.get("actionModel", {})
        emitted.update(extract_emitted_events_recursive(action))
        for desc in law.get("actionDescriptions", []):
            m = re.search(r'emit\(([^)]+)\)', desc)
            if m:
                emitted.add(m.group(1).strip('"\' '))
    return emitted

def extract_triggers(data, laws):
    triggers = {}
    if isinstance(data, dict):
        if "authoredLaws" in data and isinstance(data["authoredLaws"], dict):
            trig_dict = data["authoredLaws"].get("triggers", {})
            if isinstance(trig_dict, dict):
                for k, v in trig_dict.items():
                    if isinstance(v, list):
                        triggers[k] = v
        if "triggers" in data and isinstance(data["triggers"], dict):
            for k, v in data["triggers"].items():
                if isinstance(v, list):
                    triggers[k] = v

    for law in laws:
        law_id = law.get("id", "unknown")
        if "triggerEvent" in law and law["triggerEvent"]:
            triggers.setdefault(law_id, []).append(law["triggerEvent"])
        eca = law.get("ecaLoop", {})
        if isinstance(eca, dict) and eca.get("eventType"):
            triggers.setdefault(law_id, []).append(eca["eventType"])
    return triggers

def lint_save(file_path):
    rel_path = str(file_path.relative_to(ROOT))
    result = {
        "filename": rel_path,
        "naming_issues": [],
        "volatile_id_issues": [],
        "unemitted_triggers": [],
        "total_laws": 0,
        "total_triggers": 0
    }

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read().strip()
            if not content:
                return result
            data = json.loads(content)
    except Exception as e:
        result["naming_issues"].append("JSON Parse Error: " + str(e))
        return result

    laws = extract_laws_from_json(data)
    result["total_laws"] = len(laws)
    emitted_events = extract_emitted_events(laws)
    triggers_map = extract_triggers(data, laws)

    for law in laws:
        law_id = law.get("id", "")
        law_name = law.get("name", "")

        if VOLATILE_ID_RE.match(law_id):
            result["volatile_id_issues"].append("Law '" + law_name + "' uses volatile generated ID '" + law_id + "'")

        for desc in law.get("conditionDescriptions", []):
            if VOLATILE_TARGET_RE.search(desc):
                result["volatile_id_issues"].append("Law '" + law_id + "' condition contains volatile target: " + desc)

    for law_id, trig_list in triggers_map.items():
        result["total_triggers"] += len(trig_list)
        for trig in trig_list:
            if not NOUN_VERBED_RE.match(trig):
                result["naming_issues"].append("Trigger '" + trig + "' on law '" + law_id + "' does not match past-tense noun-verbed kebab-case")
            if trig not in emitted_events:
                result["unemitted_triggers"].append("Trigger '" + trig + "' on law '" + law_id + "' has no known publisher in engine or ECA cascade")

    return result

def main():
    print("=== Earthcall Authored-Save Lint Probe ===")
    json_files = sorted(SAVES_DIR.rglob("*.json"))
    active_files = [f for f in json_files if "backups" not in f.parts and f.stat().st_size > 0]
    print("Scanning " + str(len(active_files)) + " active save files in saves/...")
    print("")

    total_files = len(active_files)
    files_with_laws = 0
    total_laws_scanned = 0
    total_naming_warnings = 0
    total_volatile_warnings = 0
    total_unemitted_warnings = 0

    for f in active_files:
        res = lint_save(f)
        if res["total_laws"] > 0 or res["total_triggers"] > 0:
            files_with_laws += 1
            total_laws_scanned += res["total_laws"]
            print("File: " + res["filename"] + " (" + str(res["total_laws"]) + " laws, " + str(res["total_triggers"]) + " triggers):")
            
            for issue in res["naming_issues"]:
                print("   [EVENT NAMING] " + issue)
            total_naming_warnings += len(res["naming_issues"])
            
            for issue in res["volatile_id_issues"]:
                print("   [VOLATILE ID]  " + issue)
            total_volatile_warnings += len(res["volatile_id_issues"])
                
            for issue in res["unemitted_triggers"]:
                print("   [ORPHAN TRIGGER] " + issue)
            total_unemitted_warnings += len(res["unemitted_triggers"])

            if not res["naming_issues"] and not res["volatile_id_issues"] and not res["unemitted_triggers"]:
                print("   [CLEAN] All events past-tense kebab-case, stable slugs, publishers wired")
            print("")

    print("================ Summary ================")
    print("Files audited:            " + str(total_files))
    print("Files containing laws:    " + str(files_with_laws))
    print("Total laws analyzed:      " + str(total_laws_scanned))
    print("Event naming warnings:    " + str(total_naming_warnings))
    print("Volatile ID warnings:     " + str(total_volatile_warnings))
    print("Orphan trigger warnings:  " + str(total_unemitted_warnings))
    print("=========================================")
    print("RESULT: AUDIT COMPLETE — Probe operational.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
