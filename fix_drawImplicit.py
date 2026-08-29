import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """    // Memoize the WGSL string generation and pipeline lookup.
    sdfwgsl::Program prog;
    const SdfPipeline* sp = nullptr;
    bool needsCompile = true;
    if (memoId != 0) {
        auto& entry = _programCache[memoId];
        if (entry.revision == memoRevision) {
            prog = entry.prog;
            sp = entry.sp;
            needsCompile = false;
        }
    }
    if (needsCompile) {
        prog = sdfwgsl::compile(field, fieldNode);
        sp = sdfPipeline(prog.wgsl);
        if (memoId != 0) {
            auto& entry = _programCache[memoId];
            entry.revision = memoRevision;
            entry.prog = prog;
            entry.sp = sp;
        }
    }
    if (!sp) return;"""

new_code = """    // Memoize the WGSL string generation and pipeline lookup.
    sdfwgsl::Program prog;
    const SdfPipeline* sp = nullptr;
    bool needsCompile = true;
    if (memoId != 0) {
        auto& entry = _programCache[memoId];
        if (entry.revision == memoRevision) {
            prog = entry.prog;
            sp = entry.sp;
            needsCompile = false;
        }
    }
    if (needsCompile) {
        prog = sdfwgsl::compile(field, fieldNode);
        sp = sdfPipeline(prog.wgsl);
        if (memoId != 0) {
            auto& entry = _programCache[memoId];
            entry.revision = memoRevision;
            entry.prog = prog;
            entry.sp = sp;
            if (entry.brickmap) { wgpuBufferRelease(entry.brickmap); entry.brickmap = nullptr; }
            if (entry.brickmapBg) { wgpuBindGroupRelease(entry.brickmapBg); entry.brickmapBg = nullptr; }
        }
    }
    if (!sp) return;

    if (memoId != 0 && _programCache[memoId].brickmap == nullptr) {
        std::vector<uint32_t> bitmask((64 * 16 * 64) / 32, 0);
        glm::ivec3 res(64, 16, 64);
        glm::vec3 step = (extent * 2.0f) / glm::vec3(res);
        glm::vec3 halfStep = step * 0.5f;

        for (int z = 0; z < res.z; ++z) {
            for (int y = 0; y < res.y; ++y) {
                for (int x = 0; x < res.x; ++x) {
                    glm::vec3 minBox = -extent + glm::vec3(x, y, z) * step;
                    glm::vec3 maxBox = minBox + step;
                    glm::vec3 c = (minBox + maxBox) * 0.5f;
                    float d = geom::evalSdf(field, c);
                    float radius = glm::length(maxBox - c);

                    if (std::abs(d) <= radius * 4.0f) {
                        uint32_t bitIdx = z * (64 * 16) + y * 64 + x;
                        bitmask[bitIdx / 32] |= (1u << (bitIdx % 32));
                    }
                }
            }
        }
        
        WGPUBufferDescriptor bd = {};
        bd.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
        bd.size = bitmask.size() * sizeof(uint32_t);
        WGPUBuffer buf = wgpuDeviceCreateBuffer(_device, &bd);
        wgpuQueueWriteBuffer(_queue, buf, 0, bitmask.data(), bd.size);
        
        WGPUBindGroupEntry bge = {};
        bge.binding = 0;
        bge.buffer = buf;
        bge.size = bd.size;
        WGPUBindGroupDescriptor bgDesc = {};
        bgDesc.layout = _sdfBrickmapBgl;
        bgDesc.entryCount = 1;
        bgDesc.entries = &bge;
        WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgDesc);

        auto& entry = _programCache[memoId];
        entry.brickmap = buf;
        entry.brickmapBg = bg;
    }"""

content = content.replace(old_code, new_code)
with open(file_path, "w") as f:
    f.write(content)
