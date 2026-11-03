import os
import re

def fix_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # 1. Fix "zone.json written to disk" checks to "zone.ecform"
    content = content.replace('"zone.json";', '"zone.ecform";')
    content = content.replace('"zone.json written to disk"', '"zone.ecform written to disk"')

    # 2. Fix std::ifstream in(zoneFilePath); in >> zj; 
    # to nlohmann::json zj = SaveSystem::readSaveData(zoneFilePath.string());
    
    # We will use regex to find:
    # std::ifstream in(zoneFilePath);
    # nlohmann::json zj;
    # in >> zj;
    # OR similar
    
    pattern1 = r'std::ifstream\s+(\w+)\s*\(([^)]+)\);\s*nlohmann::json\s+(\w+);\s*\1\s*>>\s*\3;'
    def repl1(m):
        return f'nlohmann::json {m.group(3)} = SaveSystem::readSaveData({m.group(2)}.string());'
    
    content = re.sub(pattern1, repl1, content)

    # For the pre-embed block in zone_facetexture_test where it strips materials
    pattern2 = r'nlohmann::json\s+(\w+);\s*\{\s*std::ifstream\s+(\w+)\s*\(([^)]+)\);\s*\2\s*>>\s*\1;\s*\}'
    def repl2(m):
        return f'nlohmann::json {m.group(1)} = SaveSystem::readSaveData({m.group(3)}.string());'
    
    content = re.sub(pattern2, repl2, content)

    # Make sure to remove zone.ecform if we are writing out to zone.json!
    # Wait, the code says:
    # std::ofstream out(zoneFilePath); out << stripped.dump(2);
    # zoneFilePath is now "zone.ecform", which means it writes JSON text into a file named "zone.ecform"!
    # SaveSystem::readSaveData actually checks the EXTENSION! It will see ".ecform" and try to msgpack unpack it!
    # If it's JSON text inside ".ecform", it will crash msgpack!
    
    # We must explicitly write to ".json" and delete ".ecform"!
    # Let's fix that.
    pattern3 = r'std::ofstream\s+(\w+)\s*\(([^)]+)\);\s*\1\s*<<\s*([^;]+);'
    def repl3(m):
        return f'std::filesystem::remove({m.group(2)}); // remove ecform to force json fallback\n            auto fallback = {m.group(2)}; fallback.replace_extension(".json");\n            std::ofstream {m.group(1)}(fallback);\n            {m.group(1)} << {m.group(3)};'
    
    content = re.sub(pattern3, repl3, content)

    if content != open(filepath, 'r').read():
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Patched {filepath}")

for root, _, files in os.walk('tests'):
    for file in files:
        if file.endswith('.cpp'):
            fix_file(os.path.join(root, file))

