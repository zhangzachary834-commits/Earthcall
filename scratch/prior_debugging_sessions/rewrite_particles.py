import sys

header_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
with open(header_path, "r") as f:
    header = f.read()

new_header = header.replace("WGPURenderPipeline  _imagePipe   = nullptr;", "WGPURenderPipeline  _imagePipe   = nullptr;\n    WGPUShaderModule _particleShader = nullptr;\n    WGPUBindGroupLayout _particleBgl = nullptr;\n    WGPUPipelineLayout _particleLayout = nullptr;\n    WGPURenderPipeline _particlePipe = nullptr;")

if header == new_header:
    print("Could not find insertion point in header")
    sys.exit(1)

with open(header_path, "w") as f:
    f.write(new_header)
print("Header updated")
