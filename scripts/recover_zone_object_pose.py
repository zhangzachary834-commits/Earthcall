#!/usr/bin/env python3
"""Surgically recover Object pose fields from a trusted Zone-bearing save.

This is a preservation tool, not an authoring shortcut.  It matches Objects only by
their existing stable objectID inside one explicitly named Zone, refuses duplicate
or shape-mismatched identities, copies only named pose fields, and keeps the exact
pre-repair bytes before atomically replacing the target.

Origin: Zach's 2026-09-11 report that Synthesis Studio's rectangular forms had
collapsed into cubes.  Recovery mechanism: Codex, on Zach's explicit request.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import tempfile
from pathlib import Path
from typing import Any


DEFAULT_FIELDS = ("transform", "center")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def resolved_identifier(zone: dict[str, Any]) -> str:
    value = zone.get("identifier", zone.get("name", ""))
    return value if isinstance(value, str) else ""


def find_zone(document: dict[str, Any], zone_id: str) -> dict[str, Any]:
    candidates: list[dict[str, Any]] = []
    if isinstance(document.get("world"), dict):
        candidates.append(document)
    for key in ("zones",):
        value = document.get(key)
        if isinstance(value, list):
            candidates.extend(item for item in value if isinstance(item, dict))
    semantic_roots = document.get("semanticRoots")
    if isinstance(semantic_roots, dict):
        value = semantic_roots.get("zones")
        if isinstance(value, list):
            candidates.extend(item for item in value if isinstance(item, dict))

    matches = [zone for zone in candidates if resolved_identifier(zone) == zone_id]
    if len(matches) != 1:
        raise ValueError(
            f"expected exactly one Zone '{zone_id}', found {len(matches)}"
        )
    world = matches[0].get("world")
    if not isinstance(world, dict) or not isinstance(world.get("objects"), list):
        raise ValueError(f"Zone '{zone_id}' has no world.objects array")
    return matches[0]


def index_objects(zone: dict[str, Any], label: str) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for position, value in enumerate(zone["world"]["objects"]):
        if not isinstance(value, dict):
            raise ValueError(f"{label} object at index {position} is not a record")
        object_id = value.get("objectID")
        if not isinstance(object_id, str) or not object_id:
            raise ValueError(f"{label} object at index {position} has no objectID")
        if object_id in result:
            raise ValueError(f"{label} contains duplicate objectID '{object_id}'")
        result[object_id] = value
    return result


def validate_field(field: str, value: Any, object_id: str) -> None:
    lengths = {
        "transform": 16,
        "center": 3,
        "authoritativeAxis": 3,
        "targetRotation": 3,
    }
    if field in lengths:
        expected = lengths[field]
        if not isinstance(value, list) or len(value) != expected or not all(
            isinstance(component, (int, float)) for component in value
        ):
            raise ValueError(
                f"source '{object_id}' has invalid {field}; expected {expected} numbers"
            )
    elif field == "rotationResponsiveness" and not isinstance(value, (int, float)):
        raise ValueError(
            f"source '{object_id}' has invalid rotationResponsiveness"
        )


def atomic_write_json(path: Path, document: dict[str, Any]) -> None:
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", suffix=".tmp", dir=path.parent
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as output:
            json.dump(document, output, indent=2)
            output.write("\n")
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary_name, path)
    except BaseException:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--zone-id", required=True)
    parser.add_argument("--field", action="append", dest="fields")
    parser.add_argument("--expected-target-sha256", required=True)
    parser.add_argument("--expected-source-sha256", required=True)
    parser.add_argument("--backup", type=Path)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()

    target_hash = sha256(args.target)
    source_hash = sha256(args.source)
    if target_hash != args.expected_target_sha256:
        raise SystemExit(
            f"REFUSED: target changed: expected {args.expected_target_sha256}, got {target_hash}"
        )
    if source_hash != args.expected_source_sha256:
        raise SystemExit(
            f"REFUSED: source changed: expected {args.expected_source_sha256}, got {source_hash}"
        )

    target_document = json.loads(args.target.read_text(encoding="utf-8"))
    source_document = json.loads(args.source.read_text(encoding="utf-8"))
    target_zone = find_zone(target_document, args.zone_id)
    source_zone = find_zone(source_document, args.zone_id)
    target_objects = index_objects(target_zone, "target")
    source_objects = index_objects(source_zone, "source")
    fields = tuple(args.fields or DEFAULT_FIELDS)

    changes: list[tuple[str, str]] = []
    matched = sorted(set(target_objects) & set(source_objects))
    for object_id in matched:
        target = target_objects[object_id]
        source = source_objects[object_id]
        if target.get("shapeKind") != source.get("shapeKind"):
            raise SystemExit(
                f"REFUSED: shapeKind mismatch for '{object_id}': "
                f"target={target.get('shapeKind')}, source={source.get('shapeKind')}"
            )
        for field in fields:
            if field not in source:
                continue
            validate_field(field, source[field], object_id)
            if target.get(field) != source[field]:
                target[field] = source[field]
                changes.append((object_id, field))

    changed_objects = sorted({object_id for object_id, _ in changes})
    print(f"Zone: {args.zone_id}")
    print(f"Target objects: {len(target_objects)}")
    print(f"Source objects: {len(source_objects)}")
    print(f"Matched objects: {len(matched)}")
    print(f"Target-only objects preserved: {len(set(target_objects) - set(source_objects))}")
    print(f"Changed objects: {len(changed_objects)}")
    for field in fields:
        count = sum(1 for _, changed_field in changes if changed_field == field)
        print(f"  {field}: {count}")

    if not args.apply:
        print("DRY RUN: no files written")
        return 0
    if not args.backup:
        raise SystemExit("REFUSED: --apply requires --backup")
    if args.backup.exists():
        raise SystemExit(f"REFUSED: backup already exists: {args.backup}")
    args.backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.target, args.backup)
    if sha256(args.backup) != target_hash:
        raise SystemExit("REFUSED: backup hash does not match original target")
    atomic_write_json(args.target, target_document)
    print(f"Backup: {args.backup}")
    print(f"Before SHA-256: {target_hash}")
    print(f"After SHA-256:  {sha256(args.target)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
