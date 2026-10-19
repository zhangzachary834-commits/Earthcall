#!/usr/bin/env python3
"""Migrate physical matter (.ecmatter sidecars) to per-Zone content-addressed storage.

Authority: Zach, on his 2026-09-09 direction that each Zone must be independently
complete without a conglomerate World file (Per-Zone serialization pathway).
Design: docs/Agenda/Tasks/Specific Tasks/Serialization and Storage/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md
Repair Boundary Item 2:
"Give each Zone its own verified, content-addressed matter generation for genuinely
physical/heavy geometry, and load it only through that Zone's activation transaction."

For each physical Zone in saves/zones/ and saves/homes/Home:
1. Reconstructs Zone-scoped FlatBuffers entities with owner_identifier set to the Zone.
2. Recovers dense geometry (polyhedra, smooth surfaces, Bezier patches, SDF fields)
   from legacy matter sources (saves/worlds/*.ecmatter) and authored zone definitions.
3. Compiles content-addressed <stem>.<snapshotId>.ecmatter and unversioned <stem>.ecmatter
   sidecars via flatc.
4. Atomically patches zone.json and zone.ecform (and home.json) with matterGeneration
   metadata, backing up original documents to saves/backups/zone-matter-migration-2026-10-16/.

Usage:
    python3 scripts/migrate_zone_matter.py            # dry run (default)
    python3 scripts/migrate_zone_matter.py --apply
"""
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

import msgpack

AGENT = "Antigravity / Gemini 3.8 Flash, physical matter migration"
STAMP = "2026-10-16"
BACKUP = f"saves/backups/zone-matter-migration-{STAMP}"
APPLY = "--apply" in sys.argv

FLATC = os.path.abspath("./build/_deps/flatbuffers-build/flatc")
SCHEMA = os.path.abspath("src/Singularity/Storage/Schema/Earthcall.fbs")


def sanitize_label(name: str) -> str:
    cleaned = re.sub(r'[^a-zA-Z0-9_\-.]', '_', name)
    cleaned = re.sub(r'_+', '_', cleaned)
    cleaned = cleaned.strip('._')
    return cleaned if cleaned else "unnamed"


def read_ecform(path):
    with open(path, "rb") as f:
        raw = f.read()
    data = msgpack.unpackb(raw, raw=False)
    return raw, data["MigrationRoot"]


def write_ecform_text(text, original_raw):
    packed = msgpack.packb({"MigrationRoot": text}, use_bin_type=True)
    if original_raw[15:16] == b"\xdb" and packed[15:16] != b"\xdb":
        n = len(text.encode("utf-8"))
        packed = packed[:15] + b"\xdb" + n.to_bytes(4, "big") + text.encode("utf-8")
    return packed


def canonical_zone(path_dir):
    is_home = os.path.basename(path_dir.rstrip("/")) == "Home" or "home.json" in os.listdir(path_dir)
    stem_name = "home" if is_home else "zone"
    ec = os.path.join(path_dir, f"{stem_name}.ecform")
    js = os.path.join(path_dir, f"{stem_name}.json")
    if os.path.exists(ec) and os.path.getsize(ec) > 0:
        _, text = read_ecform(ec)
        return json.loads(text), ec, stem_name
    if os.path.exists(js):
        return json.load(open(js, encoding="utf-8")), js, stem_name
    return None, None, stem_name


def objects_of(doc):
    w = doc.get("world")
    if isinstance(w, dict) and isinstance(w.get("objects"), list):
        return w["objects"]
    return doc.get("objects") if isinstance(doc.get("objects"), list) else []


def load_all_legacy_entities():
    """Decode all legacy ecmatter files and index entities."""
    pool = {}  # eid -> list of (source_path, entity_dict)
    legacy_files = sorted(glob.glob("saves/worlds/*.ecmatter"))
    print(f"Decoding {len(legacy_files)} legacy ecmatter files...")
    with tempfile.TemporaryDirectory() as tmpdir:
        for f in legacy_files:
            cmd = [FLATC, "-o", tmpdir, "--json", "--strict-json", "--raw-binary", SCHEMA, "--", f]
            res = subprocess.run(cmd, capture_output=True, text=True)
            if res.returncode != 0:
                continue
            json_name = os.path.join(tmpdir, os.path.splitext(os.path.basename(f))[0] + ".json")
            if not os.path.exists(json_name):
                continue
            with open(json_name, "r", encoding="utf-8") as jf:
                chunk = json.load(jf)
            os.remove(json_name)
            for e in chunk.get("entities", []):
                eid = e.get("id")
                if not eid:
                    continue
                pool.setdefault(eid, []).append((f, e))
    print(f"Indexed {len(pool)} unique entity IDs from legacy matter sources.")
    return pool


def resolve_legacy_entity(pool, obj_id, zone_id, zone_dir):
    """Find the best matching legacy entity for an object ID."""
    cands = pool.get(obj_id, [])
    if not cands:
        return None
    # 1. Match owner_identifier == zone_id
    for src, e in cands:
        if e.get("owner_identifier") == zone_id:
            return e
    # 2. Match source filename to zone slug
    z_slug = sanitize_label(zone_id).lower()
    d_slug = sanitize_label(os.path.basename(zone_dir.rstrip("/"))).lower()
    for src, e in cands:
        s_base = os.path.basename(src).lower()
        if z_slug in s_base or d_slug in s_base:
            return e
    # 3. If all candidate instances have identical geometry, take the first
    first_geom = None
    all_same = True
    for src, e in cands:
        g = {k: e[k] for k in ('polyhedron', 'patch', 'smooth_data', 'field') if k in e}
        g_str = json.dumps(g, sort_keys=True)
        if first_geom is None:
            first_geom = g_str
        elif g_str != first_geom:
            all_same = False
            break
    if all_same:
        return cands[0][1]
    # Ambiguous conflicting candidates across different zones.  This is a
    # migration refusal, not "no legacy geometry": silently returning None here
    # makes build_entity_dict fall back to authored/default geometry and can
    # erase dense legacy matter while reporting a successful migration.
    sources = ", ".join(sorted({src for src, _ in cands}))
    raise RuntimeError(
        f"ambiguous conflicting legacy matter for object '{obj_id}' in Zone "
        f"'{zone_id}' (candidates: {sources})"
    )


def build_entity_dict(obj, zone_id, zone_dir, legacy_pool):
    """Construct a FlatBuffers Entity dictionary conforming to Earthcall.fbs."""
    oid = obj.get("id") or obj.get("objectID") or "unnamed_object"
    name = obj.get("name") or obj.get("objectType") or ""
    
    legacy = resolve_legacy_entity(legacy_pool, oid, zone_id, zone_dir)
    
    entity = {
        "id": oid,
        "name": name,
        "owner_identifier": zone_id,
        "rotation_responsiveness": float(obj.get("rotationResponsiveness", 1.0))
    }
    
    # Transform
    if "transform" in obj and isinstance(obj["transform"], list) and len(obj["transform"]) == 16:
        entity["transform"] = [float(v) for v in obj["transform"]]
    elif legacy and "transform" in legacy:
        entity["transform"] = legacy["transform"]
    else:
        entity["transform"] = [1.0, 0.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0,
                               0.0, 0.0, 1.0, 0.0,
                               0.0, 0.0, 0.0, 1.0]

    # Center
    if "center" in obj and isinstance(obj["center"], list) and len(obj["center"]) >= 3:
        entity["center"] = {"x": float(obj["center"][0]), "y": float(obj["center"][1]), "z": float(obj["center"][2])}
    elif legacy and "center" in legacy:
        entity["center"] = legacy["center"]
    else:
        entity["center"] = {"x": 0.0, "y": 0.0, "z": 0.0}

    # Authoritative Axis
    if "authoritativeAxis" in obj and isinstance(obj["authoritativeAxis"], list) and len(obj["authoritativeAxis"]) >= 3:
        entity["authoritative_axis"] = {"x": float(obj["authoritativeAxis"][0]), "y": float(obj["authoritativeAxis"][1]), "z": float(obj["authoritativeAxis"][2])}
    elif legacy and "authoritative_axis" in legacy:
        entity["authoritative_axis"] = legacy["authoritative_axis"]
    else:
        entity["authoritative_axis"] = {"x": 0.0, "y": 1.0, "z": 0.0}

    # Target Rotation
    if "targetRotation" in obj and isinstance(obj["targetRotation"], list) and len(obj["targetRotation"]) >= 3:
        entity["target_rotation"] = {"x": float(obj["targetRotation"][0]), "y": float(obj["targetRotation"][1]), "z": float(obj["targetRotation"][2])}
    elif legacy and "target_rotation" in legacy:
        entity["target_rotation"] = legacy["target_rotation"]
    else:
        entity["target_rotation"] = {"x": 0.0, "y": 0.0, "z": 0.0}

    # Dense Geometry from legacy or object definition
    # 1. Polyhedron
    if legacy and "polyhedron" in legacy:
        entity["polyhedron"] = legacy["polyhedron"]
    elif "polyhedron" in obj and isinstance(obj["polyhedron"], dict):
        poly = obj["polyhedron"]
        verts = [{"x": float(v[0]), "y": float(v[1]), "z": float(v[2])} for v in poly.get("vertices", [])]
        faces = poly.get("faces", [])
        face_data = []
        face_offsets = []
        for face in faces:
            face_offsets.append(len(face_data))
            face_data.extend(face)
        face_offsets.append(len(face_data))
        entity["polyhedron"] = {
            "vertices": verts,
            "face_data": face_data,
            "face_offsets": face_offsets
        }

    # 2. Patch
    if legacy and "patch" in legacy:
        entity["patch"] = legacy["patch"]
    elif "patch" in obj and isinstance(obj["patch"], dict):
        patch = obj["patch"]
        ctrl = [{"x": float(c[0]), "y": float(c[1]), "z": float(c[2])} for c in patch.get("ctrl", [])]
        entity["patch"] = {
            "du": patch.get("du", 0),
            "dv": patch.get("dv", 0),
            "ctrl": ctrl
        }

    # 3. Smooth Surface
    if legacy and "smooth_data" in legacy:
        entity["smooth_data"] = legacy["smooth_data"]

    # 4. Field Data
    if legacy and "field" in legacy:
        entity["field"] = legacy["field"]
    elif "field" in obj and isinstance(obj["field"], dict):
        fd = obj["field"]
        ext = obj.get("fieldExtent", [1.0, 1.0, 1.0])
        dims = fd.get("dims", [1.0, 1.0, 1.0])
        offset = fd.get("offset", [0.0, 0.0, 0.0])
        entity["field"] = {
            "extent": {"x": float(ext[0]), "y": float(ext[1]), "z": float(ext[2])},
            "root_node": {
                "type": int(fd.get("prim", 0)),
                "operation": int(fd.get("op", 0)),
                "dims": {"x": float(dims[0]), "y": float(dims[1]), "z": float(dims[2])},
                "offset": {"x": float(offset[0]), "y": float(offset[1]), "z": float(offset[2])},
                "p0": float(fd.get("p0", 0.0)),
                "p1": float(fd.get("p1", 0.0)),
                "t": float(fd.get("t", 0.0)),
                "expr": str(fd.get("expr", ""))
            }
        }

    return entity


def compile_matter_chunk(chunk_dict):
    """Compile SaveChunk dictionary to FlatBuffers binary using flatc."""
    with tempfile.TemporaryDirectory() as tmpdir:
        json_path = os.path.join(tmpdir, "chunk.json")
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(chunk_dict, f)
        cmd = [FLATC, "-b", "--strict-json", "-o", tmpdir, SCHEMA, json_path]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            raise RuntimeError(f"flatc compilation failed: {res.stderr}")
        bin_path = os.path.join(tmpdir, "chunk.bin")
        with open(bin_path, "rb") as f:
            return f.read()


def patch_document(path, new_mg):
    """Patch matterGeneration into zone.json, zone.ecform, or home.json with strict verification."""
    if path.endswith(".ecform"):
        raw, text = read_ecform(path)
        old = json.loads(text)
        if "matterGeneration" in old:
            old["matterGeneration"] = new_mg
            new_text = json.dumps(old)
        else:
            body = text.rstrip()
            assert body.endswith("}")
            new_text = body[:-1] + ',"matterGeneration":' + json.dumps(new_mg) + "}"
        new_bytes = write_ecform_text(new_text, raw)
        check = json.loads(msgpack.unpackb(new_bytes, raw=False)["MigrationRoot"])
    else:
        raw_text = open(path, encoding="utf-8").read()
        old = json.loads(raw_text)
        if "matterGeneration" in old:
            old["matterGeneration"] = new_mg
            new_text = json.dumps(old, indent=2) + "\n"
        else:
            body = raw_text.rstrip()
            tail = raw_text[len(body):]
            assert body.endswith("}")
            inner = body[:-1].rstrip()
            mg_lines = json.dumps(new_mg, indent=2).splitlines()
            mg_indented = "\n".join(("  " + line) if i > 0 else line for i, line in enumerate(mg_lines))
            new_text = inner + ',\n  "matterGeneration": ' + mg_indented + "\n}" + tail
        new_bytes = new_text.encode("utf-8")
        check = json.loads(new_text)

    expected = dict(old)
    expected["matterGeneration"] = new_mg
    if check != expected:
        raise ValueError(f"Verification failed for {path}: document altered beyond matterGeneration key")
    return new_bytes


def main():
    print(f"=== Earthcall Zone Physical Matter Migration ({STAMP}) ===")
    print(f"Target: Worktree only ({os.getcwd()})")
    print(f"Apply mode: {'ENABLED' if APPLY else 'DRY RUN'}\n")

    legacy_pool = load_all_legacy_entities()

    zones_dirs = sorted(glob.glob("saves/zones/*/")) + ["saves/homes/Home/"]
    plans = []

    for zd in zones_dirs:
        doc, src_file, stem_name = canonical_zone(zd)
        if not doc:
            continue
        zid = doc.get("identifier", os.path.basename(zd.rstrip("/")))
        objs = objects_of(doc)
        phys_objs = [o for o in objs if o.get("shapeKind") != 12 and not o.get("isExtraSpatial", False)]

        # Check existing sidecars
        existing_sidecars = glob.glob(os.path.join(zd, f"{stem_name}.*.ecmatter"))
        has_unversioned = os.path.exists(os.path.join(zd, f"{stem_name}.ecmatter"))

        if not phys_objs and not existing_sidecars:
            # Non-physical zone, skip
            continue

        plans.append({
            "zone_dir": zd,
            "zone_id": zid,
            "stem_name": stem_name,
            "phys_objs": phys_objs,
            "existing_sidecars": existing_sidecars,
            "has_unversioned": has_unversioned,
            "doc": doc,
            "src_file": src_file
        })

    print(f"Found {len(plans)} physical Zones to process.\n")

    actions = []

    for plan in plans:
        zd = plan["zone_dir"]
        zid = plan["zone_id"]
        stem_name = plan["stem_name"]
        phys_objs = plan["phys_objs"]
        existing = plan["existing_sidecars"]

        # Special case 1: Cathedral of the Living Logos
        if "Cathedral of the Living Logos" in zd:
            cathedral_gen = os.path.join(zd, "zone.a36028441cd4045c.ecmatter")
            if os.path.exists(cathedral_gen):
                matter_bytes = open(cathedral_gen, "rb").read()
                h = hashlib.sha256(matter_bytes).hexdigest()
                snap_id = h[:16]
                mg = {
                    "snapshotId": snap_id,
                    "sha256": h,
                    "byteLength": len(matter_bytes),
                    "schemaVersion": 1
                }
                actions.append({
                    "plan": plan,
                    "reuse_matter": True,
                    "matter_bytes": matter_bytes,
                    "snapshot_id": snap_id,
                    "matter_gen": mg
                })
                print(f"[REUSE] {zid}: reusing existing verified matter '{snap_id}' ({len(matter_bytes)} bytes)")
                continue

        # Special case 2: Go
        if "Go" == os.path.basename(zd.rstrip("/")):
            go_gen = os.path.join(zd, "zone.231ef0d5ef06942e.ecmatter")
            if os.path.exists(go_gen):
                matter_bytes = open(go_gen, "rb").read()
                h = hashlib.sha256(matter_bytes).hexdigest()
                snap_id = h[:16]
                mg = {
                    "snapshotId": snap_id,
                    "sha256": h,
                    "byteLength": len(matter_bytes),
                    "schemaVersion": 1
                }
                actions.append({
                    "plan": plan,
                    "reuse_matter": True,
                    "matter_bytes": matter_bytes,
                    "snapshot_id": snap_id,
                    "matter_gen": mg
                })
                print(f"[REUSE] {zid}: reusing existing verified matter '{snap_id}' ({len(matter_bytes)} bytes)")
                continue

        # General physical zone: build entities
        entities = []
        for o in phys_objs:
            e = build_entity_dict(o, zid, zd, legacy_pool)
            entities.append(e)

        chunk = {
            "chunk_id": f"matter_{sanitize_label(zid)}",
            "entities": entities
        }

        matter_bytes = compile_matter_chunk(chunk)
        h = hashlib.sha256(matter_bytes).hexdigest()
        snap_id = h[:16]
        mg = {
            "snapshotId": snap_id,
            "sha256": h,
            "byteLength": len(matter_bytes),
            "schemaVersion": 1
        }
        actions.append({
            "plan": plan,
            "reuse_matter": False,
            "matter_bytes": matter_bytes,
            "snapshot_id": snap_id,
            "matter_gen": mg
        })
        dense_count = sum(1 for e in entities if any(k in e for k in ('polyhedron', 'patch', 'smooth_data', 'field')))
        print(f"[BUILD] {zid}: {len(entities)} entities ({dense_count} dense) -> snap '{snap_id}' ({len(matter_bytes)} bytes)")

    print(f"\nTotal actions prepared: {len(actions)}")
    if not APPLY:
        print("(Dry run finished. Run with --apply to execute migration.)")
        return 0

    # APPLY PHASE
    print("\n--- Applying migration ---")
    for act in actions:
        plan = act["plan"]
        zd = plan["zone_dir"]
        stem_name = plan["stem_name"]
        snap_id = act["snapshot_id"]
        matter_bytes = act["matter_bytes"]
        mg = act["matter_gen"]

        # 1. Write matter sidecars if not reuse or missing
        gen_path = os.path.join(zd, f"{stem_name}.{snap_id}.ecmatter")
        fixed_path = os.path.join(zd, f"{stem_name}.ecmatter")

        if not os.path.exists(gen_path):
            tmp_gen = gen_path + ".staged"
            with open(tmp_gen, "wb") as f:
                f.write(matter_bytes)
            os.replace(tmp_gen, gen_path)

        if not os.path.exists(fixed_path) or open(fixed_path, "rb").read() != matter_bytes:
            tmp_fixed = fixed_path + ".staged"
            with open(tmp_fixed, "wb") as f:
                f.write(matter_bytes)
            os.replace(tmp_fixed, fixed_path)

        # 2. Patch identity documents (zone.json, zone.ecform, home.json)
        for fname in (f"{stem_name}.ecform", f"{stem_name}.json"):
            path = os.path.join(zd, fname)
            if not os.path.exists(path) or os.path.getsize(path) == 0:
                continue

            # Read current doc to see if already has this matterGeneration
            try:
                if fname.endswith(".ecform"):
                    _, txt = read_ecform(path)
                    curr = json.loads(txt)
                else:
                    curr = json.load(open(path, encoding="utf-8"))
                if curr.get("matterGeneration") == mg:
                    continue  # already matches
            except Exception:
                pass

            # Backup
            bdir = os.path.join(BACKUP, os.path.basename(zd.rstrip("/")))
            os.makedirs(bdir, exist_ok=True)
            bpath = os.path.join(bdir, fname)
            if not os.path.exists(bpath):
                shutil.copy2(path, bpath)

            # Patch
            new_bytes = patch_document(path, mg)
            staged = path + ".staged"
            with open(staged, "wb") as f:
                f.write(new_bytes)
            os.replace(staged, path)
            print(f"  [PATCHED] {path} (+matterGeneration '{snap_id}') [backup in {bpath}]")

    print("\nMigration applied successfully!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
