import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
with open(file_path, "r") as f:
    content = f.read()

old_key = """    struct MeshBatchKey {
        const geom::TessMesh* mesh = nullptr;
        WGPUTextureView albedoView = nullptr;
        glm::vec4 baseColor{1.0f};
        glm::vec4 shading{0.2f, 0.8f, 1.0f, 32.0f}; // ambient, diffuse, specular, shininess
        bool operator<(const MeshBatchKey& o) const {
            return std::tie(mesh, albedoView, baseColor.x, baseColor.y, baseColor.z, baseColor.w,
                            shading.x, shading.y, shading.z, shading.w)
                 < std::tie(o.mesh, o.albedoView, o.baseColor.x, o.baseColor.y, o.baseColor.z, o.baseColor.w,
                            o.shading.x, o.shading.y, o.shading.z, o.shading.w);
        }
    };
    // Mirrors the WGSL `Instance` struct in kMeshWGSL byte-for-byte (two
    // mat4x4<f32>, both already 16-byte aligned — no std430 padding needed).
    struct InstanceData { glm::mat4 model; glm::mat4 normalMat; };"""

new_key = """    struct MeshBatchKey {
        const geom::TessMesh* mesh = nullptr;
        WGPUTextureView albedoView = nullptr;
        glm::vec4 shading{0.2f, 0.8f, 1.0f, 32.0f}; // ambient, diffuse, specular, shininess
        bool operator<(const MeshBatchKey& o) const {
            return std::tie(mesh, albedoView, shading.x, shading.y, shading.z, shading.w)
                 < std::tie(o.mesh, o.albedoView, o.shading.x, o.shading.y, o.shading.z, o.shading.w);
        }
    };
    // Mirrors the WGSL `Instance` struct in kMeshWGSL.
    struct InstanceData { 
        glm::mat4 model; 
        glm::mat4 normalMat; 
        glm::vec4 baseColor;
    };"""

if old_key in content:
    content = content.replace(old_key, new_key)
    with open(file_path, "w") as f:
        f.write(content)
    print("Replaced MeshBatchKey successfully")
else:
    print("Could not find MeshBatchKey")
