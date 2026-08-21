import json

def fix():
    with open("saves/worlds/chess.json", "r") as f:
        data = json.load(f)

    # 1. Base world leftover: keep only zone.chess
    zone = data["zones"][0]
    zone["id"] = "zone.chess"
    zone["name"] = "Chess Board"
    data["zones"] = [zone]

    # 2. Fix the Author to "Gemini" globally
    def set_author(node):
        if isinstance(node, dict):
            if "authors" in node:
                node["authors"] = ["Gemini"]
            for k, v in node.items():
                set_author(v)
        elif isinstance(node, list):
            for v in node:
                set_author(v)
    set_author(data)

    # 3. Fix category.chess.piece (make it extra-spatial)
    for obj in data.get("objects", []):
        if obj.get("objectID") == "category.chess.piece":
            obj["shapeKind"] = 0
            if "position" in obj: del obj["position"]
            if "extents" in obj: del obj["extents"]
            if "faceColors" in obj: del obj["faceColors"]
            # It's a category Formation root
        if obj.get("objectID") == "object.chess.board":
            # 7. isBoard -> bool
            if "authoredProperties" in obj and "isBoard" in obj["authoredProperties"]:
                obj["authoredProperties"]["isBoard"]["t"] = "bool"
                obj["authoredProperties"]["isBoard"]["v"] = True
    
    # Remove any stray squares if they exist
    data["objects"] = [o for o in data.get("objects", []) if not o.get("objectID", "").startswith("square_")]
    
    # Also remove "object-45" etc. (base world leftovers)
    data["objects"] = [o for o in data["objects"] if "object-" not in o.get("objectID", "")]

    # 4. Law IDs -> stable slugs.
    law_map = {
        "law-100": "law-chess-click",
        "law-101": "law-chess-select",
        "law-102": "law-chess-deselect",
        "law-103": "law-chess-validate",
        "law-104": "law-chess-execute",
        "law-105": "law-chess-capture",
        "law-106": "law-chess-update",
        "law-107": "law-chess-hl-sel",
        "law-108": "law-chess-hl-w",
        "law-109": "law-chess-hl-b"
    }

    new_laws = []
    for law in data["authoredLaws"]["laws"]:
        old_id = law["id"]
        if old_id in law_map:
            law["id"] = law_map[old_id]
        
        # 5. Event names to past-tense
        def fix_events(node):
            if isinstance(node, dict):
                if node.get("eventType") == "process-click":
                    node["eventType"] = "board-clicked"
                elif node.get("eventType") == "execute-move":
                    node["eventType"] = "move-executed"
                elif node.get("eventType") == "capture-enemy":
                    node["eventType"] = "enemy-captured"
                elif node.get("eventType") == "update-positions":
                    node["eventType"] = "positions-updated"
                for k, v in node.items():
                    fix_events(v)
            elif isinstance(node, list):
                for v in node:
                    fix_events(v)
        fix_events(law)
        
        # 6. Capture corpse under map -> Destroy action
        if law["id"] == "law-chess-capture":
            law["actionModel"] = {
                "kind": 16, # Destroy
                "elementToken": "@event.subject"
            }
        
        # 7. handle-click isBoard comparison should use bool
        if law["id"] == "law-chess-click":
            for child in law["conditionModel"].get("children", []):
                if child.get("path") == "isBoard":
                    child["operand"]["t"] = "bool"
                    child["operand"]["v"] = True
        
        new_laws.append(law)
    
    data["authoredLaws"]["laws"] = new_laws

    # Fix triggers
    new_triggers = {}
    for old_id, evts in data["authoredLaws"]["triggers"].items():
        if old_id in law_map:
            mapped_evts = []
            for ev in evts:
                if ev == "process-click": mapped_evts.append("board-clicked")
                elif ev == "execute-move": mapped_evts.append("move-executed")
                elif ev == "capture-enemy": mapped_evts.append("enemy-captured")
                elif ev == "update-positions": mapped_evts.append("positions-updated")
                else: mapped_evts.append(ev)
            new_triggers[law_map[old_id]] = mapped_evts
        else:
            new_triggers[old_id] = evts
    data["authoredLaws"]["triggers"] = new_triggers

    # Fix formationMembers
    data["authoredLaws"]["formationMembers"] = [
        law_map.get(m, m) for m in data["authoredLaws"]["formationMembers"]
    ]

    with open("saves/worlds/chess.json", "w") as f:
        json.dump(data, f, indent=2)

if __name__ == "__main__":
    fix()
