import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("    WGPUBindGroupLayout _sdfBrickmapBgl = nullptr;", "    WGPUBindGroupLayout _sdfBrickmapBgl = nullptr;\\n    WGPUBuffer _dummyBrickmap = nullptr;\\n    WGPUBindGroup _dummyBrickmapBg = nullptr;")
with open(file_path, "w") as f:
    f.write(content)

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """    _sdfBrickmapBgl = wgpuDeviceCreateBindGroupLayout(_device, &brickmapBglDesc);"""
new_code = """    _sdfBrickmapBgl = wgpuDeviceCreateBindGroupLayout(_device, &brickmapBglDesc);
    
    WGPUBufferDescriptor dummyDesc = {};
    dummyDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
    dummyDesc.size = (64*16*64) / 8; // 8192 bytes
    _dummyBrickmap = wgpuDeviceCreateBuffer(_device, &dummyDesc);
    std::vector<uint32_t> ones(dummyDesc.size / 4, 0xffffffff);
    wgpuQueueWriteBuffer(_queue, _dummyBrickmap, 0, ones.data(), dummyDesc.size);
    
    WGPUBindGroupEntry dummyBge = {};
    dummyBge.binding = 0;
    dummyBge.buffer = _dummyBrickmap;
    dummyBge.size = dummyDesc.size;
    WGPUBindGroupDescriptor dummyBgDesc = {};
    dummyBgDesc.layout = _sdfBrickmapBgl;
    dummyBgDesc.entryCount = 1;
    dummyBgDesc.entries = &dummyBge;
    _dummyBrickmapBg = wgpuDeviceCreateBindGroup(_device, &dummyBgDesc);"""

content = content.replace(old_code, new_code)
content = content.replace("if (_sdfBrickmapBgl) { wgpuBindGroupLayoutRelease(_sdfBrickmapBgl); _sdfBrickmapBgl = nullptr; }", "if (_sdfBrickmapBgl) { wgpuBindGroupLayoutRelease(_sdfBrickmapBgl); _sdfBrickmapBgl = nullptr; }\\n    if (_dummyBrickmapBg) { wgpuBindGroupRelease(_dummyBrickmapBg); _dummyBrickmapBg = nullptr; }\\n    if (_dummyBrickmap) { wgpuBufferRelease(_dummyBrickmap); _dummyBrickmap = nullptr; }")

content = content.replace("        if (brickBg) {\\n            wgpuRenderPassEncoderSetBindGroup(_pass, 2, brickBg, 0, nullptr);\\n        }", "        if (!brickBg) brickBg = _dummyBrickmapBg;\\n        wgpuRenderPassEncoderSetBindGroup(_pass, 2, brickBg, 0, nullptr);")

with open(file_path, "w") as f:
    f.write(content)
