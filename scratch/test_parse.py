import json
import glob
import os

for f in glob.glob("saves/zones/*/zone.json"):
    try:
        with open(f, 'r') as fp:
            data = json.load(fp)
            if 'objects' in data:
                for obj in data['objects']:
                    if 'shapeKind' in obj and obj['shapeKind'] == 'Field':
                        print("Found Field in", f)
                        print("ID:", obj.get("objectID"))
                        print("Material:", obj.get("materialId"))
                        print("FaceColors:", obj.get("faceColors"))
                        mid = obj.get("materialId")
                        if 'materials' in data:
                            for m in data['materials']:
                                if m.get("name") == mid.replace("material.", ""):
                                    print("  Material baseColor:", m.get("baseColor"))
                                    fts = m.get("faceTextures", [])
                                    print("  FaceTextures count:", len(fts))
                        print("---")
    except Exception as e:
        pass
