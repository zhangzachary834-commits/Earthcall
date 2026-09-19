import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

particle_shutdown = """    if (_particlePipe) { wgpuRenderPipelineRelease(_particlePipe); _particlePipe = nullptr; }
    if (_particleLayout) { wgpuPipelineLayoutRelease(_particleLayout); _particleLayout = nullptr; }
    if (_particleBgl) { wgpuBindGroupLayoutRelease(_particleBgl); _particleBgl = nullptr; }
    if (_particleShader) { wgpuShaderModuleRelease(_particleShader); _particleShader = nullptr; }"""

content = content.replace("    if (_imagePipe) { wgpuRenderPipelineRelease(_imagePipe); _imagePipe = nullptr; }", "    if (_imagePipe) { wgpuRenderPipelineRelease(_imagePipe); _imagePipe = nullptr; }\n" + particle_shutdown)

with open(file_path, "w") as f:
    f.write(content)
print("Particle shutdown added")
