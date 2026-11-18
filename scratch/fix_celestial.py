import json
import os

path = "saves/zones/Celestial Zenith/zone.json"
with open(path, "r") as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if '"name": "Chess"' in line or '"identifier": "Chess"' in line:
        continue
    new_lines.append(line)

with open(path, "w") as f:
    f.writelines(new_lines)

print("Fixed JSON!")
