import sys
sys.path.insert(0, 'scripts')
import seed_law_line as seed

path = "saves/zones/Ambient Zone/zone.ecform"
new_data = {"lawRefs": ["law-ambient-pan"]}

with open(path, "rb") as f:
    old_bytes = f.read()

try:
    if old_bytes[:len(seed.ECFORM_HEAD)] == seed.ECFORM_HEAD:
        text = seed.ecform_text(old_bytes)
        import json
        data = json.loads(text)
        if "lawRefs" not in data:
            data["lawRefs"] = []
        if "law-ambient-pan" not in data["lawRefs"]:
            data["lawRefs"].append("law-ambient-pan")
        new_text = json.dumps(data, ensure_ascii=False)
        new_bytes = seed.ecform_wrap(new_text.encode("utf-8"))
        with open(path, "wb") as f:
            f.write(new_bytes)
        print("Patched MigrationRoot ECFORM successfully!")
    else:
        # Just JSON list? Let's assume it's just JSON list and we can't easily add lawRefs to it since lawRefs needs to be in a dict.
        print("Not a MigrationRoot ecform, but we can just use zone.json")
except Exception as e:
    print("Error:", e)

