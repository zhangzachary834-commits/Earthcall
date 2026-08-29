import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_frag = """    let rgb = u.baseColor.rgb * texel.rgb * lit + vec3<f32>(spec);
    return vec4<f32>(rgb, u.baseColor.a * texel.a);"""

new_frag = """    let rgb = in.baseColor.rgb * texel.rgb * lit + vec3<f32>(spec);
    return vec4<f32>(rgb, in.baseColor.a * texel.a);"""

if old_frag in content:
    content = content.replace(old_frag, new_frag)
    print("Replaced fragment shader successfully")
else:
    print("Could not find fragment shader")

with open(file_path, "w") as f:
    f.write(content)
