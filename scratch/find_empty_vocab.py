import json

with open("saves/worlds/synthesis_studio_living.json", "r") as f:
    data = json.load(f)

laws = data["authoredLaws"].get("laws", [])
for e in laws:
    if e.get("activation") == 2:
        print(f"Law {e.get('id')} is OnBecomeTrue!")
