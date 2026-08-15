import json
import os
import glob
from datetime import datetime

save_dir = "saves/games"
save_files = glob.glob(f"{save_dir}/*.json")
if not save_files:
    exit(1)

latest_save = max(save_files, key=os.path.getmtime)
with open(latest_save, 'r') as f:
    save_data = json.load(f)

if "concepts" in save_data and "concepts" in save_data["concepts"]:
    concepts_list = save_data["concepts"]["concepts"]
    concepts_list = [c for c in concepts_list if c.get("id") != "concept-robot-arm-001"]
    save_data["concepts"]["concepts"] = concepts_list

with open(latest_save, 'w') as f:
    json.dump(save_data, f, indent=2)

print("Fixed save")
