import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_uniform = """struct MeshUniforms {
    glm::mat4 viewProj;
    glm::vec4 baseColor;
    glm::vec4 lightPos;
    glm::vec4 params;
    glm::vec4 eyePos;
};"""

new_uniform = """struct MeshUniforms {
    glm::mat4 viewProj;
    glm::vec4 lightPos;
    glm::vec4 params;
    glm::vec4 eyePos;
};"""

if old_uniform in content:
    content = content.replace(old_uniform, new_uniform)
    print("Replaced MeshUniforms successfully")
else:
    print("Could not find MeshUniforms")

with open(file_path, "w") as f:
    f.write(content)
