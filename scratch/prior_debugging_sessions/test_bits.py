import sys
file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

# I will inject a printf to count how many bits are 1
old_code = """        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;"""

new_code = """        int count = 0;
        for (uint32_t w : bitmask) {
            for (int i=0; i<32; ++i) if (w & (1u << i)) count++;
        }
        printf("BRICKMAP GENERATED: %d / %d voxels are active (%.1f%%)\\n", count, 64*16*64, (count * 100.0f) / (64*16*64));
        
        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;"""

content = content.replace(old_code, new_code)
with open(file_path, "w") as f:
    f.write(content)
