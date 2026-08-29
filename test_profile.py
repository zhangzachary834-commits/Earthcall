import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectCollision.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace('float best = -std::numeric_limits<float>::max();\n        glm::vec3 bestV = _supportCloud[0];', 'static int cnt = 0; if (cnt++ % 6000 == 0) printf("supportCloud hit! size: %zu\\n", _supportCloud.size());\n        float best = -std::numeric_limits<float>::max();\n        glm::vec3 bestV = _supportCloud[0];')

with open(file_path, "w") as f:
    f.write(content)
