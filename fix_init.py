import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """    sdfInstBglDesc.entryCount = 1;
    sdfInstBglDesc.entries = &sdfInstEntry;
    _sdfInstanceBgl = wgpuDeviceCreateBindGroupLayout(_device, &sdfInstBglDesc);"""

new_code = """    sdfInstBglDesc.entryCount = 1;
    sdfInstBglDesc.entries = &sdfInstEntry;
    _sdfInstanceBgl = wgpuDeviceCreateBindGroupLayout(_device, &sdfInstBglDesc);

    WGPUBindGroupLayoutEntry brickmapEntry = {};
    brickmapEntry.binding = 0;
    brickmapEntry.visibility = WGPUShaderStage_Fragment;
    brickmapEntry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    WGPUBindGroupLayoutDescriptor brickmapBglDesc = {};
    brickmapBglDesc.entryCount = 1;
    brickmapBglDesc.entries = &brickmapEntry;
    _sdfBrickmapBgl = wgpuDeviceCreateBindGroupLayout(_device, &brickmapBglDesc);"""

content = content.replace(old_code, new_code)
content = content.replace("if (_sdfInstanceBgl) { wgpuBindGroupLayoutRelease(_sdfInstanceBgl); _sdfInstanceBgl = nullptr; }", "if (_sdfInstanceBgl) { wgpuBindGroupLayoutRelease(_sdfInstanceBgl); _sdfInstanceBgl = nullptr; }\\n    if (_sdfBrickmapBgl) { wgpuBindGroupLayoutRelease(_sdfBrickmapBgl); _sdfBrickmapBgl = nullptr; }")

with open(file_path, "w") as f:
    f.write(content)
