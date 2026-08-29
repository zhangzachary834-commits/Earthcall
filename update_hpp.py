import sys
file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("""    struct MemoizedProgram {
        uint32_t revision = 0xffffffff;
        sdfwgsl::Program prog;
        const SdfPipeline* sp = nullptr;
    };""", """    struct MemoizedProgram {
        uint32_t revision = 0xffffffff;
        sdfwgsl::Program prog;
        const SdfPipeline* sp = nullptr;
        WGPUBuffer brickmap = nullptr;
        WGPUBindGroup brickmapBg = nullptr;
    };""")

# Need a bind group layout for the brickmap!
# We can just put the brickmap in group 2!
content = content.replace("""    WGPUBindGroupLayout _sdfInstanceBgl = nullptr;""", """    WGPUBindGroupLayout _sdfInstanceBgl = nullptr;
    WGPUBindGroupLayout _sdfBrickmapBgl = nullptr;""")

with open(file_path, "w") as f:
    f.write(content)
