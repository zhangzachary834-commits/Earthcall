import sys

file_path = "src/ZonesOfEarth/Physics/Physics.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("void updateBodies(std::vector<std::shared_ptr<Object>>& objects, float deltaTime,", "void updateBodies(std::vector<std::shared_ptr<Object>>& objects, float deltaTime, float gravityAccel, float airResistance, float groundY) { return; }\nvoid old_updateBodies(std::vector<std::shared_ptr<Object>>& objects, float deltaTime,")

with open(file_path, "w") as f:
    f.write(content)
print("Disabled physics!")
