import sys

file_path = "src/Singularity/Screen/WebGPU/SdfWgsl.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("voxel.x += stepDir.x; tDDA += tMax.x; tMax.x += tDelta.x;", "voxel.x += stepDir.x; tDDA = t + tMax.x; tMax.x += tDelta.x;")
content = content.replace("voxel.y += stepDir.y; tDDA += tMax.y; tMax.y += tDelta.y;", "voxel.y += stepDir.y; tDDA = t + tMax.y; tMax.y += tDelta.y;")
content = content.replace("voxel.z += stepDir.z; tDDA += tMax.z; tMax.z += tDelta.z;", "voxel.z += stepDir.z; tDDA = t + tMax.z; tMax.z += tDelta.z;")

with open(file_path, "w") as f:
    f.write(content)
