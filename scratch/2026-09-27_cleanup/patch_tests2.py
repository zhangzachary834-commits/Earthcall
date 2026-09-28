import os
import re

def fix_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    orig_content = content
    
    content = content.replace('"zone.json";', '"zone.ecform";')
    content = content.replace('"zone.json written to disk"', '"zone.ecform written to disk"')

    # Include SaveSystem if missing
    if 'SaveSystem::readSaveData' not in content and 'SaveSystem' not in content and 'readSaveData' not in content:
        # Actually it's probably better to just add it if we use it
        pass

    # Replace exact ifstream reading a json block
    pattern1 = r'std::ifstream\s+(\w+)\s*\(([^)]+)\);\s*nlohmann::json\s+(\w+);\s*\1\s*>>\s*\3;'
    def repl1(m):
        return f'nlohmann::json {m.group(3)} = SaveSystem::readSaveData({m.group(2)}.string());'
    
    content = re.sub(pattern1, repl1, content)

    # For the pre-embed block in zone_facetexture_test
    pattern2 = r'nlohmann::json\s+(\w+);\s*\{\s*std::ifstream\s+(\w+)\s*\(([^)]+)\);\s*\2\s*>>\s*\1;\s*\}'
    def repl2(m):
        return f'nlohmann::json {m.group(1)} = SaveSystem::readSaveData({m.group(3)}.string());'
    
    content = re.sub(pattern2, repl2, content)
    
    # For the ofstream writing back to zoneFilePath
    # std::ofstream out(zoneFilePath);
    # out << stripped.dump(2);
    pattern3 = r'std::ofstream\s+out\(zoneFilePath\);\s*out\s*<<\s*([^;]+);'
    def repl3(m):
        return f'std::filesystem::remove(zoneFilePath);\n            auto fallback = zoneFilePath; fallback.replace_extension(".json");\n            std::ofstream out(fallback);\n            out << {m.group(1)};'
    
    content = re.sub(pattern3, repl3, content)

    if content != orig_content:
        # Add include if needed
        if '#include "src/Singularity/Storage/SaveSystem.hpp"' not in content:
            content = '#include "src/Singularity/Storage/SaveSystem.hpp"\n' + content
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Patched {filepath}")

for root, _, files in os.walk('tests'):
    for file in files:
        if file.endswith('.cpp'):
            fix_file(os.path.join(root, file))

