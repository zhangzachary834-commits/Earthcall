import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

bad = '_particleShader = loadShader(_device, kParticleWGSL, "particle");'
good = """WGPUShaderSourceWGSL psrc = {};
    psrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    psrc.code = wgpu::Device::str(kParticleWGSL);
    WGPUShaderModuleDescriptor psmDesc = {};
    psmDesc.nextInChain = &psrc.chain;
    _particleShader = wgpuDeviceCreateShaderModule(_device, &psmDesc);"""

content = content.replace(bad, good)

with open(file_path, "w") as f:
    f.write(content)
print("Shader fixed")
