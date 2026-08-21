import json

def main():
    with open('saves/worlds/chess.json', 'r') as f:
        data = json.load(f)
        
    for law in data['authoredLaws']['laws']:
        if law['name'] == 'capture-enemy':
            old_action = law['actionModel']
            law['actionModel'] = {
                "kind": 5,
                "children": [
                    old_action,
                    {"kind": 0, "path": "gridX", "operand": {"t": "int", "v": -99}},
                    {"kind": 0, "path": "gridY", "operand": {"t": "int", "v": -99}}
                ]
            }
            break
            
    with open('saves/worlds/chess.json', 'w') as f:
        json.dump(data, f, indent=2)

if __name__ == '__main__':
    main()
