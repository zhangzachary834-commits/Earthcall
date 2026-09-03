import sys

file_path = "src/Singularity/Screen/WebGPU/SdfWgsl.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_vs = """struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldPos: vec3<f32>,
    @location(1) @interpolate(flat) instIdx: u32,
};

@vertex
fn vs(@location(0) pos: vec3<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var o: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos * inst.extents.xyz, 1.0);
    o.clip = u.viewProj * world;
    // Clamp to far plane so proxy geometry is never lost to far-plane clipping.
    o.clip.z = min(o.clip.z, o.clip.w * 0.999999);
    o.worldPos = world.xyz;
    o.instIdx = instIdx;
    return o;
}"""

new_vs = """struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) worldPos: vec3<f32>,
    @location(1) @interpolate(flat) instIdx: u32,
    @location(2) @interpolate(flat) chunkCenter: vec3<f32>,
    @location(3) @interpolate(flat) chunkExtents: vec3<f32>,
};

@vertex
fn vs(@location(0) pos: vec3<f32>, @location(1) center: vec3<f32>, @location(2) extents: vec3<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var o: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos, 1.0);
    o.clip = u.viewProj * world;
    // Clamp to far plane so proxy geometry is never lost to far-plane clipping.
    o.clip.z = min(o.clip.z, o.clip.w * 0.999999);
    o.worldPos = world.xyz;
    o.instIdx = instIdx;
    o.chunkCenter = center;
    o.chunkExtents = extents;
    return o;
}"""

content = content.replace(old_vs, new_vs)
with open(file_path, "w") as f:
    f.write(content)
