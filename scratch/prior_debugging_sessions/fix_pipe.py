import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """    SdfPipeline out;
    out.bgl = wgpuDeviceCreateBindGroupLayout(_device, &bgld);
    WGPUBindGroupLayout meshLayouts[2] = { out.bgl, _sdfInstanceBgl };
    WGPUPipelineLayoutDescriptor pld = {};
    pld.bindGroupLayoutCount = 2;
    pld.bindGroupLayouts = meshLayouts;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(_device, &pld);"""

new_code = """    SdfPipeline out;
    out.bgl = wgpuDeviceCreateBindGroupLayout(_device, &bgld);
    WGPUBindGroupLayout meshLayouts[3] = { out.bgl, _sdfInstanceBgl, _sdfBrickmapBgl };
    WGPUPipelineLayoutDescriptor pld = {};
    pld.bindGroupLayoutCount = 3;
    pld.bindGroupLayouts = meshLayouts;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(_device, &pld);"""

content = content.replace(old_code, new_code)
with open(file_path, "w") as f:
    f.write(content)
