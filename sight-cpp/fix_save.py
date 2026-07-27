import json

def main():
    try:
        with open('saves/games/20260724_132741.json', 'r') as f:
            data = json.load(f)
    except FileNotFoundError:
        print("Could not find the original save file.")
        return

    if 'authoredLaws' in data:
        data['authoredLaws']['triggers'] = {
            "law-shape-3d": ["onMouseClicked"]
        }
        
    has_concept = False
    if 'concepts' in data and 'concepts' in data['concepts']:
        for c in data['concepts']['concepts']:
            if c.get('id') == 'concept-shape-3d':
                has_concept = True
                
    if not has_concept:
        try:
            with open('saves/games/ToolMigration.json', 'r') as f2:
                tool_data = json.load(f2)
                if 'concepts' in tool_data and 'concepts' in tool_data['concepts']:
                    for c in tool_data['concepts']['concepts']:
                        if c.get('id') == 'concept-shape-3d':
                            if 'concepts' not in data:
                                data['concepts'] = {'concepts': []}
                            if 'concepts' not in data['concepts']:
                                data['concepts']['concepts'] = []
                            data['concepts']['concepts'].append(c)
        except FileNotFoundError:
            pass
            
    # Also inject the law itself just in case it wasn't saved in the main save
    has_law = False
    if 'authoredLaws' in data and 'laws' in data['authoredLaws']:
        for law in data['authoredLaws']['laws']:
            if law.get('id') == 'law-shape-3d':
                has_law = True
    if not has_law and 'authoredLaws' in data:
        try:
            with open('saves/games/ToolMigration.json', 'r') as f2:
                tool_data = json.load(f2)
                if 'authoredLaws' in tool_data and 'laws' in tool_data['authoredLaws']:
                    for law in tool_data['authoredLaws']['laws']:
                        if law.get('id') == 'law-shape-3d':
                            if 'laws' not in data['authoredLaws']:
                                data['authoredLaws']['laws'] = []
                            data['authoredLaws']['laws'].append(law)
                            if 'formationMembers' not in data['authoredLaws']:
                                data['authoredLaws']['formationMembers'] = []
                            if 'law-shape-3d' not in data['authoredLaws']['formationMembers']:
                                data['authoredLaws']['formationMembers'].append('law-shape-3d')
        except FileNotFoundError:
            pass

    with open('saves/games/20260724_132741_fixed.json', 'w') as f:
        json.dump(data, f, indent=2)

if __name__ == "__main__":
    main()
