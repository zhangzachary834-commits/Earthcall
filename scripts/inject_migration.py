import json
import sys

migration_path = 'saves/games/ToolMigration.json'
target_save_path = 'saves/games/20260713_191903.json'
output_save_path = 'saves/games/Migrated_Save.json'

with open(migration_path, 'r') as f:
    migration_data = json.load(f)

with open(target_save_path, 'r') as f:
    save_data = json.load(f)

# Initialize if not present
if 'concepts' not in save_data:
    save_data['concepts'] = {'concepts': []}
if 'authoredLaws' not in save_data:
    save_data['authoredLaws'] = {'laws': [], 'formationMembers': []}

# Overwrite or append concepts
for new_concept in migration_data.get('concepts', {}).get('concepts', []):
    replaced = False
    for i, c in enumerate(save_data['concepts']['concepts']):
        if c.get('id') == new_concept.get('id'):
            save_data['concepts']['concepts'][i] = new_concept
            replaced = True
            break
    if not replaced:
        save_data['concepts']['concepts'].append(new_concept)

# Overwrite or append laws
for new_law in migration_data.get('authoredLaws', {}).get('laws', []):
    replaced = False
    for i, l in enumerate(save_data['authoredLaws']['laws']):
        if l.get('id') == new_law.get('id'):
            save_data['authoredLaws']['laws'][i] = new_law
            replaced = True
            break
    if not replaced:
        save_data['authoredLaws']['laws'].append(new_law)

# Add to formationMembers if needed
for member in migration_data.get('authoredLaws', {}).get('formationMembers', []):
    if member not in save_data['authoredLaws']['formationMembers']:
        save_data['authoredLaws']['formationMembers'].append(member)

with open(output_save_path, 'w') as f:
    json.dump(save_data, f, indent=4)

print(f"Successfully injected migration data into {output_save_path}")
