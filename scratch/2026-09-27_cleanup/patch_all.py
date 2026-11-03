import os
import re

def fix_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    orig = content

    # Replace "zone.json" -> "zone.ecform" in exist checks and string paths 
    # IF it's in a test that writes via persistZones or expects standard disk state.
    # WAIT! "saves/zones/Sun/zone.json" might be in the source repo as a real file.
    # We should only replace "zone.json" if it's constructed with "sandbox".
    content = content.replace('sandbox / "zones" / zoneId / "zone.json"', 'sandbox / "zones" / zoneId / "zone.ecform"')
    content = content.replace('sandbox / "zones" / "Alpha" / "zone.json"', 'sandbox / "zones" / "Alpha" / "zone.ecform"')
    content = content.replace('sandbox / "zones" / "Beta" / "zone.json"', 'sandbox / "zones" / "Beta" / "zone.ecform"')
    content = content.replace('sandbox / "zones" / "Home" / "zone.json"', 'sandbox / "zones" / "Home" / "zone.ecform"')

    # Fix ifstream to SaveSystem::readSaveData for the other tests too!
    # Tests that use std::ifstream in(p); in >> zj;
    # where p is a variable.
    
    # We'll use a regex for: std::ifstream IN(PATH); nlohmann::json J; IN >> J;
    # OR: nlohmann::json J; { std::ifstream IN(PATH); IN >> J; }
    
    pat_ifstream_bare = r'std::ifstream\s+(\w+)\s*\(([^)]+)\);\s*nlohmann::json\s+(\w+);\s*\1\s*>>\s*\3;'
    content = re.sub(pat_ifstream_bare, lambda m: f'nlohmann::json {m.group(3)} = SaveSystem::readSaveData({m.group(2)}.string());', content)

    pat_ifstream_bare2 = r'nlohmann::json\s+(\w+);\s*(?:\{\s*)?std::ifstream\s+(\w+)\s*\(([^)]+)\);\s*\2\s*>>\s*\1;\s*(?:\}\s*)?'
    content = re.sub(pat_ifstream_bare2, lambda m: f'nlohmann::json {m.group(1)} = SaveSystem::readSaveData({m.group(3)}.string());', content)

    # For tests using std::ios::binary: std::ifstream input(path, std::ios::binary); input >> j;
    pat_ifstream_bin = r'std::ifstream\s+(\w+)\s*\(([^,]+),\s*std::ios::binary\);\s*nlohmann::json\s+(\w+);\s*\1\s*>>\s*\3;'
    content = re.sub(pat_ifstream_bin, lambda m: f'nlohmann::json {m.group(3)} = SaveSystem::readSaveData({m.group(2)}.string());', content)

    pat_ifstream_bin2 = r'nlohmann::json\s+(\w+);\s*std::ifstream\s+(\w+)\s*\(([^,]+),\s*std::ios::binary\);\s*\2\s*>>\s*\1;'
    content = re.sub(pat_ifstream_bin2, lambda m: f'nlohmann::json {m.group(1)} = SaveSystem::readSaveData({m.group(3)}.string());', content)

    # Prism Cathedral specific (no nlohmann::json declared on the same line, or it's wrapped in `if (in)`)
    # std::ifstream in(zonePath);
    # check(static_cast<bool>(in), "Prism Cathedral save file exists and is readable");
    # nlohmann::json zoneJson;
    # if (in) in >> zoneJson;
    if 'Prism Cathedral save file exists' in content:
        content = content.replace('std::ifstream in(zonePath);', 'nlohmann::json zoneJson = SaveSystem::readSaveData(zonePath.string());')
        content = content.replace('check(static_cast<bool>(in), "Prism Cathedral save file exists and is readable");', 'check(!zoneJson.empty(), "Prism Cathedral save file exists and is readable");')
        content = content.replace('nlohmann::json zoneJson;', '')
        content = content.replace('if (in) in >> zoneJson;', '')

    # zone_facetexture_test specific rewrite
    if 'std::ofstream out(zoneFilePath);' in content:
        content = content.replace('std::ofstream out(zoneFilePath);', 'std::filesystem::remove(zoneFilePath);\n            auto fallback = zoneFilePath; fallback.replace_extension(".json");\n            std::ofstream out(fallback);')

    if content != orig:
        if '#include "src/Singularity/Storage/SaveSystem.hpp"' not in content and 'SaveSystem' in content:
            content = '#include "src/Singularity/Storage/SaveSystem.hpp"\n' + content
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Patched {filepath}")

for root, _, files in os.walk('tests'):
    for file in files:
        if file.endswith('.cpp'):
            fix_file(os.path.join(root, file))

