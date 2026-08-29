import sys

file_path = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """        void paintNewObject(Object& obj, const glm::vec3& color) {
            for (int f = 0; f < obj.getFaces(); ++f)
                obj.setFaceColor(f, color.x, color.y, color.z);
        }"""

new_code = """        void paintNewObject(Object& obj, const glm::vec3& color) {
            if (auto mine = obj.ownMaterial()) {
                mine->baseColor = color;
            }
        }"""

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(file_path, "w") as f:
        f.write(content)
    print("Replaced successfully")
else:
    print("Could not find old code")
