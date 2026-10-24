#!/usr/bin/env python3
"""Give every Zone a complete Material closure (Per-Zone serialization pathway).

Claude Code / Sonnet 5.5, session 01GxayCUN2nc7DDaeg33kXhZ, 2026-10-07.
Authority: Zach, 2026-10-07 ("do the rest of it"), on his 2026-09-09
correction that a Zone must be independently complete without a conglomerate
World file.  Design: docs/Agenda/Tasks/Specific Tasks/Serialization and
Storage/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md

ZoneManager::prepareZoneMaterialClosure now REFUSES to enter a Zone whose
Objects name a Material that its own identity does not define (embedded
`materials`), name (`materialRefs` -> saves/materials/<stem>/material.json),
or inherit from the built-in default.  This script finds each Zone's
dangling Materials, recovers their authored definitions from the legacy
sources that still hold them (saves/worlds/*.json, other Zone identities),
writes ONE shared root per Material under saves/materials/, and adds a
`materialRefs` key to the Zone.

PATCH, NEVER REGENERATE (FIRST_MOVER_AUTHORING.md section 7, rule 8): each
Zone file is edited by inserting one key at the text level, staged beside the
original, verified to equal (old document + that one key), the old bytes kept
under saves/backups/zone-material-refs-2026-10-07/, and only then renamed
into place atomically.  A Material with conflicting legacy definitions, or
none, is REPORTED and left alone -- never guessed.

    python3 scripts/migrate_zone_materials.py            # dry run (default)
    python3 scripts/migrate_zone_materials.py --apply
"""
import glob
import json
import os
import shutil
import sys

import msgpack

DEFAULT_MATERIAL = {"name": "default", "baseColor": [1.0, 1.0, 1.0], "opacity": 1.0,
                    "shininess": 32.0, "specular": 1.0, "ambient": 0.20000000298023224,
                    "diffuse": 0.800000011920929}
AGENT = "Claude Code / Claude Sonnet 5.5, session 01GxayCUN2nc7DDaeg33kXhZ"
STAMP = "2026-10-07"
BACKUP = f"saves/backups/zone-material-refs-{STAMP}"
APPLY = "--apply" in sys.argv


def stem(entry):
    ident = entry.get("id") if isinstance(entry.get("id"), str) and entry.get("id") else None
    s = ident if ident else entry.get("name", "default")
    return s[9:] if s.startswith("material.") else s


def objects_of(doc):
    w = doc.get("world")
    if isinstance(w, dict) and isinstance(w.get("objects"), list):
        return w["objects"]
    return doc.get("objects") if isinstance(doc.get("objects"), list) else []


def read_ecform(path):
    with open(path, "rb") as f:
        raw = f.read()
    data = msgpack.unpackb(raw, raw=False)
    return raw, data["MigrationRoot"]


def write_ecform_text(text, original_raw):
    # Keep the original msgpack string width unless the new text outgrows it.
    packed = msgpack.packb({"MigrationRoot": text}, use_bin_type=True)
    if original_raw[15:16] == b"\xdb" and packed[15:16] != b"\xdb":
        n = len(text.encode("utf-8"))
        packed = packed[:15] + b"\xdb" + n.to_bytes(4, "big") + text.encode("utf-8")
    return packed


def canonical(path_dir):
    ec = os.path.join(path_dir, "zone.ecform")
    js = os.path.join(path_dir, "zone.json")
    if os.path.exists(ec) and os.path.getsize(ec) > 0:
        _, text = read_ecform(ec)
        return json.loads(text), ec
    return json.load(open(js, encoding="utf-8")), js


def pool_materials():
    """stem -> {canonical-dump: (entry, [sources])} from legacy sources."""
    pool = {}

    def add(entry, source):
        if not isinstance(entry, dict):
            return
        s = stem(entry)
        entry = dict(entry)
        entry.pop("id", None)
        entry["name"] = s
        key = json.dumps(entry, sort_keys=True)
        pool.setdefault(s, {}).setdefault(key, (entry, []))[1].append(source)

    def mats_of(doc):
        m = doc.get("materials")
        if isinstance(m, dict):
            m = m.get("materials")
        return m if isinstance(m, list) else []

    for p in sorted(glob.glob("saves/worlds/*.json")):
        try:
            doc = json.load(open(p, encoding="utf-8"))
        except Exception:
            continue
        if not isinstance(doc, dict):
            continue
        for m in mats_of(doc):
            add(m, p)
        for z in doc.get("zones", []) if isinstance(doc.get("zones"), list) else []:
            if isinstance(z, dict):
                for m in mats_of(z):
                    add(m, p)
    for d in sorted(glob.glob("saves/zones/*/")):
        try:
            doc, src = canonical(d.rstrip("/"))
        except Exception:
            continue
        for m in mats_of(doc):
            add(m, src)
    return pool


def main():
    pool = pool_materials()
    plans = []   # (zone_dir, doc, unresolved stems)
    for d in sorted(glob.glob("saves/zones/*/")):
        d = d.rstrip("/")
        try:
            doc, src = canonical(d)
        except Exception as e:
            print(f"SKIP {d}: unreadable ({e})")
            continue
        if "materialRefs" in doc:
            continue
        have = {"default"}
        for m in doc.get("materials", []) if isinstance(doc.get("materials"), list) else []:
            if isinstance(m, dict):
                have.add(stem(m))
        need = []
        for o in objects_of(doc):
            mid = o.get("materialId") or ""
            s = mid[9:] if mid.startswith("material.") else mid
            if s and s not in have and s not in need:
                need.append(s)
        if need:
            plans.append((d, doc, sorted(need)))

    roots = {}      # stem -> entry
    report = []
    for d, doc, need in plans:
        refs = []
        for s in need:
            cands = pool.get(s, {})
            if len(cands) == 1:
                (entry, sources), = cands.values()
                roots[s] = (entry, sorted(set(sources)), False)
                refs.append(s)
            elif not cands:
                # No authored definition exists in ANY source, so refusing the Zone
                # would protect nothing -- the data is already gone -- and would only
                # deny the Person their room.  Such an Object renders today through
                # material.default; declare exactly that, explicitly, as a shared
                # root a Person or Law can now see and repaint.  Nothing is invented:
                # these are the engine's own default values (Material.hpp), tagged.
                roots[s] = (dict(DEFAULT_MATERIAL, name=s), [], True)
                refs.append(s)
                report.append(f"  DEFAULTED   {d}: '{s}' was never authored; declared as engine-default root")
            else:
                report.append(f"  AMBIGUOUS   {d}: '{s}' has {len(cands)} conflicting definitions "
                              f"in {sorted({x for _, srcs in cands.values() for x in srcs})}")
        print(f"{d}: dangling={len(need)} recoverable={len(refs)}")
        plans[plans.index((d, doc, need))] = (d, doc, need, refs)

    print("\nshared roots to write:", len(roots))
    for line in report:
        print(line)
    if not APPLY:
        print("\n(dry run -- pass --apply to write)")
        return 0

    for s, (entry, sources, defaulted) in sorted(roots.items()):
        path = f"saves/materials/{s}/material.json"
        if os.path.exists(path):
            print(f"  root exists, leaving untouched: {path}")
            continue
        os.makedirs(os.path.dirname(path), exist_ok=True)
        doc = {"identifier": f"material.{s}", "material": entry,
               "authors": ["Zach"],
               "injected_by": {"agent": AGENT, "date": STAMP,
                               "recovered_from": sources}}
        if defaulted:
            doc["injected_by"]["note"] = ("No authored definition existed in any source; this root "
                                          "declares the engine-default Material those Objects "
                                          "already rendered with. Repaint freely.")
        tmp = path + ".staged"
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(doc, f, indent=2)
        os.replace(tmp, path)

    for plan in plans:
        d, doc, need, refs = plan[0], plan[1], plan[2], plan[3] if len(plan) > 3 else []
        if not refs:
            continue
        ref_ids = [f"material.{s}" for s in refs]
        for fname in ("zone.ecform", "zone.json"):
            path = os.path.join(d, fname)
            if not os.path.exists(path) or os.path.getsize(path) == 0:
                continue
            if fname == "zone.ecform":
                raw, text = read_ecform(path)
                old = json.loads(text)
                body = text.rstrip()
                assert body.endswith("}")
                new_text = body[:-1] + ',"materialRefs":' + json.dumps(ref_ids) + "}"
                new_bytes = write_ecform_text(new_text, raw)
                check = json.loads(msgpack.unpackb(new_bytes, raw=False)["MigrationRoot"])
            else:
                raw_text = open(path, encoding="utf-8").read()
                old = json.loads(raw_text)
                body = raw_text.rstrip()
                tail = raw_text[len(body):]
                assert body.endswith("}")
                inner = body[:-1].rstrip()
                items = ",\n".join(f'    {json.dumps(r)}' for r in ref_ids)
                new_text = inner + ',\n  "materialRefs": [\n' + items + "\n  ]\n}" + tail
                new_bytes = new_text.encode("utf-8")
                check = json.loads(new_text)
            expected = dict(old)
            expected["materialRefs"] = ref_ids
            if check != expected:
                print(f"  REFUSED to patch {path}: verification failed, original untouched")
                continue
            bdir = os.path.join(BACKUP, os.path.basename(d))
            os.makedirs(bdir, exist_ok=True)
            bpath = os.path.join(bdir, fname)
            if not os.path.exists(bpath):
                shutil.copy2(path, bpath)
            staged = path + ".staged"
            with open(staged, "wb") as f:
                f.write(new_bytes)
            os.replace(staged, path)
            print(f"  patched {path}: +materialRefs[{len(ref_ids)}]  (old bytes kept in {bpath})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
