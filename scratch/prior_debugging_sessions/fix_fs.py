import sys

file_path = "src/Singularity/Screen/WebGPU/SdfWgsl.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_fs = """    let eps     = inst.misc.y;
    let damping = inst.misc.w;
    let box     = rayAabb(ro, rd, inst.extents.xyz);

    // Front-face culling handles internal ray origins gracefully by starting t at 0
    // instead of a negative box.x, which would march backwards.
    var t = max(box.x, 0.0);
    let maxDist = min(box.y, inst.misc.z);"""

new_fs = """    let eps     = inst.misc.y;
    let damping = inst.misc.w;
    let local_ro = ro - in.chunkCenter;
    let box      = rayAabb(local_ro, rd, in.chunkExtents);

    // Front-face culling handles internal ray origins gracefully by starting t at 0
    // instead of a negative box.x, which would march backwards.
    var t = max(box.x, 0.0);
    let maxDist = min(box.y, inst.misc.z);
    
    // If ray misses the chunk entirely, or the chunk is behind the camera
    if (box.x > box.y || box.y < 0.0) { discard; }"""

content = content.replace(old_fs, new_fs)
with open(file_path, "w") as f:
    f.write(content)
