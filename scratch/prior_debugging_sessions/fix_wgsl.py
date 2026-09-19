import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_vsout = """struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldNormal: vec3<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) worldPos: vec3<f32>,
};

@vertex
fn vs_main(@location(0) pos: vec3<f32>, @location(1) normal: vec3<f32>,
           @location(2) uv: vec2<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var out: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos, 1.0);
    out.clip = u.viewProj * world;
    out.worldNormal = (inst.normalMat * vec4<f32>(normal, 0.0)).xyz;
    out.uv = uv;
    out.worldPos = world.xyz;
    return out;
}"""

new_vsout = """struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldNormal: vec3<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) worldPos: vec3<f32>,
    @location(3) baseColor: vec4<f32>,
};

@vertex
fn vs_main(@location(0) pos: vec3<f32>, @location(1) normal: vec3<f32>,
           @location(2) uv: vec2<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var out: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos, 1.0);
    out.clip = u.viewProj * world;
    out.worldNormal = (inst.normalMat * vec4<f32>(normal, 0.0)).xyz;
    out.uv = uv;
    out.worldPos = world.xyz;
    out.baseColor = inst.baseColor;
    return out;
}"""

if old_vsout in content:
    content = content.replace(old_vsout, new_vsout)
    print("Replaced VSOut and vs_main successfully")
else:
    print("Could not find VSOut and vs_main")

with open(file_path, "w") as f:
    f.write(content)
