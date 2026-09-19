import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """    WGPUVertexAttribute attr = {};
    attr.format = WGPUVertexFormat_Float32x3; attr.offset = 0; attr.shaderLocation = 0;
    WGPUVertexBufferLayout vbl = {};
    vbl.stepMode = WGPUVertexStepMode_Vertex; vbl.arrayStride = 12;
    vbl.attributeCount = 1; vbl.attributes = &attr;"""

new_code = """    WGPUVertexAttribute attr[3] = {};
    attr[0].format = WGPUVertexFormat_Float32x3; attr[0].offset = 0; attr[0].shaderLocation = 0;
    attr[1].format = WGPUVertexFormat_Float32x3; attr[1].offset = 12; attr[1].shaderLocation = 1;
    attr[2].format = WGPUVertexFormat_Float32x3; attr[2].offset = 24; attr[2].shaderLocation = 2;
    WGPUVertexBufferLayout vbl = {};
    vbl.stepMode = WGPUVertexStepMode_Vertex; vbl.arrayStride = 36;
    vbl.attributeCount = 3; vbl.attributes = attr;"""

content = content.replace(old_code, new_code)
with open(file_path, "w") as f:
    f.write(content)
