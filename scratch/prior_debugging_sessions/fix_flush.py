import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_code = """        bindPipeline(sp->pipe);
        wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(_pass, 1, instBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, _sdfCubeVerts, 0, 36 * sizeof(glm::vec3));
        wgpuRenderPassEncoderDraw(_pass, 36, static_cast<uint32_t>(instances.size()), 0, 0);"""

new_code = """        bindPipeline(sp->pipe);
        wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(_pass, 1, instBindGroup, 0, nullptr);
        
        WGPUBindGroup brickBg = nullptr;
        if (key.memoId != 0 && _programCache.count(key.memoId)) {
            brickBg = _programCache[key.memoId].brickmapBg;
        }
        if (brickBg) {
            wgpuRenderPassEncoderSetBindGroup(_pass, 2, brickBg, 0, nullptr);
        }
        
        wgpuRenderPassEncoderSetVertexBuffer(_pass, 0, _sdfCubeVerts, 0, 36 * sizeof(glm::vec3));
        wgpuRenderPassEncoderDraw(_pass, 36, static_cast<uint32_t>(instances.size()), 0, 0);"""

content = content.replace(old_code, new_code)
with open(file_path, "w") as f:
    f.write(content)
