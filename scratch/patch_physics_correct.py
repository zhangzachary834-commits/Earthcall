import sys

file_path = "src/ZonesOfEarth/Physics/Physics.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """void updateBodies(std::vector<std::shared_ptr<Object>>& bodies, float dt, float boundX, float boundY, float boundZ) {"""

replace_str = """void updateBodies(std::vector<std::shared_ptr<Object>>& bodies, float dt, float boundX, float boundY, float boundZ) {
    return; // STUBBED FOR PROFILING"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Physics.cpp correctly")
else:
    print("Could not find string in Physics.cpp")
