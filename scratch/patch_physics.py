import sys

file_path = "src/ZonesOfEarth/Physics/Physics.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """void updateBodies(float dt) {
    auto& u = Universe::instance();"""

replace_str = """void updateBodies(float dt) {
    return; // STUBBED FOR PROFILING
    auto& u = Universe::instance();"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Physics.cpp")
else:
    print("Physics.cpp already patched or string not found.")
