import re
import glob

files = [
    'tests/law/basic_cube_law_test.cpp',
    'tests/law/bezier_patch_law_test.cpp',
    'tests/person/person_serialization_test.cpp',
    'tests/singularity/serialization_compat_test.cpp'
]

pattern = re.compile(r'struct TempSaveRoot \{.*?\};\n', re.DOTALL)

for fname in files:
    with open(fname, 'r') as f:
        content = f.read()
    
    # Remove the struct
    new_content = pattern.sub('', content)
    
    # Also remove extra empty lines if desired, but subbing is enough
    
    with open(fname, 'w') as f:
        f.write(new_content)

