import sys

with open('tests/zones/zone_manager_test.cpp', 'r') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if '#include "Identity/PrivateKey.hpp"' in line:
        lines[i] = '#include "Identity/KeyPair.hpp"\n'

with open('tests/zones/zone_manager_test.cpp', 'w') as f:
    f.writelines(lines)
