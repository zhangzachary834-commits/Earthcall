import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """    // The bounding cube, shared by every field: the vertex shader scales it by the
    // field extent, so one buffer serves all of them.
    if (!_sdfCubeVerts) {
        const float h = 1.0f;
        const glm::vec3 c[8] = {
            {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
            {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}};
        const int idx[36] = {
            0,1,2, 0,2,3,  4,6,5, 4,7,6,  0,4,5, 0,5,1,
            3,2,6, 3,6,7,  0,3,7, 0,7,4,  1,5,6, 1,6,2};
        std::vector<glm::vec3> tris(36);
        for (int i = 0; i < 36; ++i) tris[i] = c[idx[i]];

        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        bd.size = tris.size() * sizeof(glm::vec3);
        _sdfCubeVerts = wgpuDeviceCreateBuffer(_device, &bd);
        wgpuQueueWriteBuffer(_queue, _sdfCubeVerts, 0, tris.data(), bd.size);
    }"""

new_code = """    // The bounding cube, shared by every field: the vertex shader scales it by the
    // field extent, so one buffer serves all of them.
    if (!_sdfCubeVerts) {
        struct ProxyVert { glm::vec3 pos; glm::vec3 center; glm::vec3 extents; };
        const float h = 1.0f;
        const glm::vec3 c[8] = {
            {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
            {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}};
        const uint32_t idx[36] = {
            0,1,2, 0,2,3,  1,5,6, 1,6,2,  5,4,7, 5,7,6,  
            4,0,3, 4,3,7,  3,2,6, 3,6,7,  4,5,1, 4,1,0};
            
        std::vector<ProxyVert> verts(8);
        for (int i = 0; i < 8; ++i) {
            verts[i] = {c[i], glm::vec3(0.0f), glm::vec3(1.0f)};
        }

        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        bd.size = verts.size() * sizeof(ProxyVert);
        _sdfCubeVerts = wgpuDeviceCreateBuffer(_device, &bd);
        wgpuQueueWriteBuffer(_queue, _sdfCubeVerts, 0, verts.data(), bd.size);
        
        WGPUBufferDescriptor ibd = {};
        ibd.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        ibd.size = sizeof(idx);
        _sdfCubeIndices = wgpuDeviceCreateBuffer(_device, &ibd);
        wgpuQueueWriteBuffer(_queue, _sdfCubeIndices, 0, idx, ibd.size);
    }"""

content = content.replace(old_code, new_code)
content = content.replace("if (_sdfCubeVerts) { wgpuBufferRelease(_sdfCubeVerts); _sdfCubeVerts = nullptr; }", "if (_sdfCubeVerts) { wgpuBufferRelease(_sdfCubeVerts); _sdfCubeVerts = nullptr; }\\n    if (_sdfCubeIndices) { wgpuBufferRelease(_sdfCubeIndices); _sdfCubeIndices = nullptr; }")
with open(file_path, "w") as f:
    f.write(content)
