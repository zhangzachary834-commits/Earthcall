import os
import re

src_dir = '/Users/zacharyzhang/documents/github/earthcall/src'

def process_file(path, func):
    with open(path, 'r') as f:
        content = f.read()
    new_content = func(content)
    if content != new_content:
        with open(path, 'w') as f:
            f.write(new_content)
        print(f"Updated {path}")

def fix_relation_manager_cpp(content):
    content = content.replace('r.weight', 'r.getWeight()')
    content = content.replace('it->weight +=', 'it->setWeight(it->getWeight() +')
    content = content.replace('input.weight', 'input.getWeight()')
    # Fix the `it->setWeight(it->getWeight() + r.getWeight()` syntax correctly if it broke
    # Wait, in RelationManager.cpp line 41: `it->weight += r.weight;`
    # We replaced `r.weight` so it became `it->weight += r.getWeight();`
    # Then `it->weight +=` becomes `it->setWeight(it->getWeight() +`
    # This might result in `it->setWeight(it->getWeight() + r.getWeight();` which is missing a closing paren.
    # Let's just do regex instead.
    return content

# Read RelationManager.cpp
rm_path = os.path.join(src_dir, 'Relation/RelationManager.cpp')
with open(rm_path, 'r') as f:
    rm_content = f.read()
rm_content = rm_content.replace('r->weight', 'r->getWeight()')
rm_content = rm_content.replace('r.weight', 'r.getWeight()')
rm_content = re.sub(r'it->weight \+= (.*?);', r'it->setWeight(it->getWeight() + \1);', rm_content)
rm_content = rm_content.replace('input.weight', 'input.getWeight()')
with open(rm_path, 'w') as f:
    f.write(rm_content)
print("Updated RelationManager.cpp")

# Read ObjectConcept.cpp
oc_path = os.path.join(src_dir, 'ConstructedBeing/Object/Creation/ObjectConcept.cpp')
with open(oc_path, 'r') as f:
    oc_content = f.read()
oc_content = oc_content.replace('rel->weight', 'rel->getWeight()')
with open(oc_path, 'w') as f:
    f.write(oc_content)
print("Updated ObjectConcept.cpp")

# Read Relationship.hpp
rel_path = os.path.join(src_dir, 'Person/Relationship/Relationship.hpp')
with open(rel_path, 'r') as f:
    rel_content = f.read()
rel_content = rel_content.replace('float weight = 1.0f', 'float initialWeight = -1.0f')
with open(rel_path, 'w') as f:
    f.write(rel_content)
print("Updated Relationship.hpp")

