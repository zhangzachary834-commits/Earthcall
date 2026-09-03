import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

particle_shader = """const char* kParticleWGSL = R"(
struct PU { 
    mvp: mat4x4<f32>, 
    color: vec4<f32>, 
    originAndTravel: vec4<f32>, 
    flowDir: vec4<f32>, 
    scale: vec4<f32> 
};
@group(0) @binding(0) var<uniform> pu: PU;

var<private> h: u32;

fn rnd() -> f32 {
    h = h ^ (h << 13u);
    h = h ^ (h >> 17u);
    h = h ^ (h << 5u);
    return f32(h & 0xFFFFFFu) / f32(0xFFFFFFu);
}

@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4<f32> {
    h = vi * 2654435761u + 1u;
    let local = vec3<f32>(rnd() * 2.0 - 1.0, rnd() * 2.0 - 1.0, rnd() * 2.0 - 1.0);
    let phase = rnd();
    
    let pos = pu.originAndTravel.xyz + local * pu.scale.xyz + pu.flowDir.xyz * (phase * pu.originAndTravel.w);
    return pu.mvp * vec4<f32>(pos, 1.0);
}
@fragment fn fs() -> @location(0) vec4<f32> { return pu.color; }
)";

struct ParticleUniforms {
    glm::mat4 mvp;
    glm::vec4 color;
    glm::vec4 originAndTravel; // xyz: origin, w: travel
    glm::vec4 flowDir; // xyz: flowDir, w: unused
    glm::vec4 scale; // xyz: scale, w: unused
};
"""

content = content.replace("struct ImageVertex { glm::vec3 pos; glm::vec2 uv; };", "struct ImageVertex { glm::vec3 pos; glm::vec2 uv; };\n\n" + particle_shader)

with open(file_path, "w") as f:
    f.write(content)
print("Shader added")
