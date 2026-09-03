import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """                    OntoMath::Interval r = geom::evalRange(field, minBox, maxBox);
                    
                    if (r.lo <= 0.0f && r.hi >= 0.0f) {"""

new_code = """                    glm::vec3 c = (minBox + maxBox) * 0.5f;
                    float d = geom::evalSdf(field, c);
                    float radius = glm::length(maxBox - c);
                    
                    // The field's maximum gradient (Lipschitz bound) is conservatively ~4.0 for damping=0.25
                    // If the distance at the center is greater than the max possible change across the chunk, it's empty.
                    if (std::abs(d) <= radius * 4.0f) {"""

content = content.replace(old_code, new_code)

# We also need to fix:
old_code2 = """                        uint32_t baseIdx = verts.size();
                        glm::vec3 c = (minBox + maxBox) * 0.5f;
                        glm::vec3 e = maxBox - c;"""

new_code2 = """                        uint32_t baseIdx = verts.size();
                        glm::vec3 e = maxBox - c;"""

content = content.replace(old_code2, new_code2)

with open(file_path, "w") as f:
    f.write(content)
