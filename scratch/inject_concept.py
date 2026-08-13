import json
import os
import glob
from datetime import datetime
import copy

save_dir = "saves/games"
save_files = glob.glob(f"{save_dir}/*.json")
if not save_files:
    print("No save files found.")
    exit(1)

# Find the latest save file based on modification time
latest_save = max(save_files, key=os.path.getmtime)
print(f"Injecting into latest save: {latest_save}")

with open(latest_save, 'r') as f:
    save_data = json.load(f)

# Define the RobotArm concept
robot_arm_concept = {
    "id": "concept-robot-arm-001",
    "name": "Robot Arm Base",
    "slug": "robot-arm-base",
    "transform": {
        "position": [0.0, 0.0, 0.0],
        "rotation": [0.0, 0.0, 0.0, 1.0],
        "scale": [1.0, 1.0, 1.0]
    },
    "objects": [
        {
            "id": "robot-base",
            "name": "Base",
            "slug": "robot-base",
            "form": "Cylinder",
            "color": [0.3, 0.3, 0.3, 1.0],
            "transform": {
                "position": [0.0, 0.0, 0.0],
                "rotation": [0.0, 0.0, 0.0, 1.0],
                "scale": [1.0, 0.2, 1.0]
            }
        },
        {
            "id": "robot-link1",
            "name": "Link 1",
            "slug": "robot-link1",
            "form": "Cylinder",
            "color": [0.8, 0.4, 0.1, 1.0],
            "transform": {
                "position": [0.0, 0.6, 0.0],
                "rotation": [0.0, 0.0, 0.0, 1.0],
                "scale": [0.4, 1.0, 0.4]
            }
        },
        {
            "id": "robot-link2",
            "name": "Link 2",
            "slug": "robot-link2",
            "form": "Cylinder",
            "color": [0.8, 0.4, 0.1, 1.0],
            "transform": {
                "position": [0.0, 1.6, 0.0],
                "rotation": [0.0, 0.0, 0.0, 1.0],
                "scale": [0.3, 1.0, 0.3]
            }
        }
    ],
    "relations": [
        {
            "id": "joint-1",
            "type": "attachment",
            "source": "robot-base",
            "target": "robot-link1",
            "properties": {
                "actuated-by": "physical-channel",
                "axis": "y",
                "joint_type": "revolute"
            }
        },
        {
            "id": "joint-2",
            "type": "attachment",
            "source": "robot-link1",
            "target": "robot-link2",
            "properties": {
                "actuated-by": "physical-channel",
                "axis": "x",
                "joint_type": "revolute"
            }
        }
    ]
}

if "concepts" not in save_data:
    save_data["concepts"] = {"concepts": []}
if "concepts" not in save_data["concepts"]:
    save_data["concepts"]["concepts"] = []

# Replace if exists, or append
concepts_list = save_data["concepts"]["concepts"]
concepts_list = [c for c in concepts_list if c.get("id") != "concept-robot-arm-001"]
concepts_list.append(robot_arm_concept)
save_data["concepts"]["concepts"] = concepts_list

with open(latest_save, 'w') as f:
    json.dump(save_data, f, indent=2)

print(f"Successfully injected RobotArm concept into {latest_save}")
