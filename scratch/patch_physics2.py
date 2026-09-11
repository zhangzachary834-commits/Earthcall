import sys

file_path = "src/ZonesOfEarth/Physics/Physics.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """void updateBodies(std::vector<std::shared_ptr<Object>>& bodies, float dt, float boundX, float boundY, float boundZ) {"""

replace_str = """void updateBodies(std::vector<std::shared_ptr<Object>>& bodies, float dt, float boundX, float boundY, float boundZ) {
    return; // STUBBED FOR PROFILING"""

if replace_str in content:
    print("Already patched! But why is it still running?")
    # Maybe inline functions or multiple files?
