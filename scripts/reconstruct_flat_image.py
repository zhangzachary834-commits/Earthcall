"""First Mover tool: opaque raster -> exact, editable Shape2D rectangle partition.

The inference/partition policy lives in this developer tool, outside the engine.
It emits existing Object/Material serialization, never a new domain type. It
does not infer semantic objects, behavior, depth, occluded surfaces or a camera.
Only first seeds are supported: existing authored output is always refused.
Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import tempfile
from PIL import Image


def partition(image, maximum_regions=4096):
    """Lossless row runs merged vertically where color and horizontal extent agree.

    This is a compact primitive decomposition, not semantic segmentation or a
    globally minimal rectangle cover. Complex images refuse the authored budget.
    """
    width, height = image.size
    if width <= 0 or height <= 0 or width * height > 16_777_216:
        raise ValueError("Image dimensions exceed the reconstruction budget")
    if maximum_regions <= 0:
        raise ValueError("maximum_regions must be positive")
    image = image.convert("RGBA")
    if image.getchannel("A").getextrema() != (255, 255):
        raise ValueError("Exact geometry reconstruction currently requires opaque pixels")
    pixels = image.load()
    active, completed = {}, []
    for y in range(height):
        current = {}
        x = 0
        while x < width:
            color = pixels[x, y]
            right = x + 1
            while right < width and pixels[right, y] == color:
                right += 1
            key = (x, right, color)
            rect = active.pop(key, [x, y, right, y, color])
            rect[3] = y + 1
            current[key] = rect
            if len(completed) + len(active) + len(current) > maximum_regions:
                raise ValueError("Exact rectangle partition exceeds maximum_regions; no approximation emitted")
            x = right
        completed.extend(active.values())
        active = current
        if len(completed) + len(active) > maximum_regions:
            raise ValueError("Exact rectangle partition exceeds maximum_regions; no approximation emitted")
    completed.extend(active.values())
    return sorted(completed, key=lambda r: (r[1], r[0], r[3], r[2]))


def build_seed(image_path, author, maximum_regions=4096):
    if not author.strip():
        raise ValueError("A commissioning Person identifier is required")
    source = Path(image_path).read_bytes()
    digest = hashlib.sha256(source).hexdigest()
    with Image.open(image_path) as loaded:
        if loaded.width * loaded.height > 16_777_216:
            raise ValueError("Image dimensions exceed the reconstruction budget")
        image = loaded.convert("RGBA")
    rects = partition(image, maximum_regions)
    slug = "reconstruction-" + digest[:16]
    objects, materials = [], {}
    identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    for index, (x, y, right, bottom, rgba) in enumerate(rects):
        color = [v / 255 for v in rgba[:3]]
        material_id = "material." + slug + "." + bytes(rgba).hex()
        # Face color carries the reconstructed flat color. Material baseColor
        # is a texture modulation: keep it neutral so later authored paint is
        # not multiplied by the source color after copy-on-write.
        materials.setdefault(material_id, {"name": material_id[9:], "baseColor": [1, 1, 1],
            "opacity": 1, "metallic": 0, "roughness": 1})
        objects.append({"objectID": f"{slug}-region-{index:06d}", "shapeKind": 12,
            "geometryType": 12, "shapeParams": [1, 1, 0, 0, 0, 0, 0, 0, 0, right-x, bottom-y],
            "x2D": x, "y2D": y, "zOrder2D": index, "transform": identity,
            "center": [0, 0, 0], "materialId": material_id,
            "faceColors": [color for _ in range(6)], "authoredProperties": {
                "border.visible": {"t": "bool", "v": False},
                "reconstruction.sourceHash": {"t": "string", "v": digest},
                "reconstruction.method": {"t": "string", "v": "exact-opaque-rectangle-partition"},
                "reconstruction.author": {"t": "string", "v": author},
                "reconstruction.regionIndex": {"t": "int", "v": index}}})
    return {"saveFormat": "zone-identity-v1",
        "injected_by": "Codex / GPT-6 / image reconstruction First Mover tool",
        "authors": [author], "currentZoneId": slug, "zoneRefs": [{"identifier": slug}],
        "categories": [], "authoredLaws": {"laws": []},
        "zones": [{"identifier": slug, "name": "Image Reconstruction", "owner": author,
            "world": {"objects": objects}, "materials": list(materials.values()),
            "formationRelations": []}],
        "reconstructionEvidence": {"sourcePath": str(Path(image_path).resolve()),
            "sourceSha256": digest, "width": image.width, "height": image.height,
            "regions": len(objects), "exactAtSourcePixelCenters": True,
            "semanticInterpretation": False, "depthRecovered": False}}


def write_first_seed(path, document):
    """Stage, validate, publish without replacing an existing file, even in a race."""
    path = Path(path)
    if path.exists():
        raise FileExistsError("Refusing existing authored output; use a targeted patch")
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(document, indent=2).encode() + b"\n"
    json.loads(payload)
    fd, staged = tempfile.mkstemp(prefix=".reconstruction-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as file:
            file.write(payload)
            file.flush()
            os.fsync(file.fileno())
        os.link(staged, path)  # atomic publication, refuses concurrent output
    finally:
        os.unlink(staged)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--author", required=True, help="The commissioning Person's identifier")
    parser.add_argument("--maximum-regions", type=int, default=4096)
    args = parser.parse_args()
    if not args.author.strip() or args.maximum_regions <= 0:
        parser.error("author and maximum-regions must be valid")
    document = build_seed(args.image, args.author, args.maximum_regions)
    write_first_seed(args.output, document)
    print(json.dumps(document["reconstructionEvidence"], indent=2))


if __name__ == "__main__":
    main()
