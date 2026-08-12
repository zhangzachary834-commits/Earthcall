import os

def replace_in_file(path):
    with open(path, 'r') as f:
        content = f.read()
    if '#include "Util/' in content or '#include "../Util/' in content:
        content = content.replace('#include "Util/', '#include "Singularity/Storage/')
        content = content.replace('#include "../Util/', '#include "../Singularity/Storage/')
        with open(path, 'w') as f:
            f.write(content)
        print(f"Updated {path}")

for root, dirs, files in os.walk('src'):
    for file in files:
        if file.endswith('.cpp') or file.endswith('.hpp'):
            replace_in_file(os.path.join(root, file))
