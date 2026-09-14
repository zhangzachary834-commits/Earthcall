import os

files_to_edit = [
    "src/Singularity/Screen/Renderer.hpp",
    "src/Singularity/Screen/Renderer.cpp",
    "src/Singularity/Screen/GL/OpenGLRenderer.hpp",
    "src/Singularity/Screen/GL/OpenGLRenderer.cpp",
    "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp",
    "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp",
]

def apply():
    # 1. Renderer.hpp
    with open("src/Singularity/Screen/Renderer.hpp", "r") as f:
        content = f.read()
    if "uploadTextureRegion" not in content:
        content = content.replace(
            "virtual TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,\n                                        uint32_t width, uint32_t height) = 0;",
            "virtual TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,\n                                        uint32_t width, uint32_t height) = 0;\n    virtual TextureHandle uploadTextureRegion(TextureHandle handle, const uint8_t* rgba,\n                                        uint32_t texWidth, uint32_t texHeight,\n                                        uint32_t x, uint32_t y, uint32_t width, uint32_t height) { return handle; }"
        )
        with open("src/Singularity/Screen/Renderer.hpp", "w") as f:
            f.write(content)

    # 2. WebGpuRenderer.hpp
    with open("src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp", "r") as f:
        content = f.read()
    if "uploadTextureRegion" not in content:
        content = content.replace(
            "TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,\n                                uint32_t width, uint32_t height) override;",
            "TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,\n                                uint32_t width, uint32_t height) override;\n    TextureHandle uploadTextureRegion(TextureHandle handle, const uint8_t* rgba,\n                                uint32_t texWidth, uint32_t texHeight,\n                                uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;"
        )
        with open("src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp", "w") as f:
            f.write(content)
            
    # 3. WebGpuRenderer.cpp
    with open("src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp", "r") as f:
        content = f.read()
    if "uploadTextureRegion" not in content:
        impl = """
TextureHandle WebGpuRenderer::uploadTextureRegion(TextureHandle handle, const uint8_t* rgba,
                                                  uint32_t texWidth, uint32_t texHeight,
                                                  uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!_device || !rgba || handle == 0 || width == 0 || height == 0) return handle;

    auto it = _textures.find(handle);
    if (it == _textures.end()) return handle;

    WGPUTexelCopyTextureInfo dst = {};
    dst.texture = it->second.tex; dst.aspect = WGPUTextureAspect_All; 
    dst.origin = { x, y, 0 };
    
    WGPUTexelCopyBufferLayout lay = {};
    lay.bytesPerRow = texWidth * 4; lay.rowsPerImage = texHeight; lay.offset = (y * texWidth + x) * 4;
    
    WGPUExtent3D ext = { width, height, 1 };
    
    // Some WebGPU implementations require buffer copies rather than queueWriteTexture with offset
    // wgpuQueueWriteTexture is allowed to specify data layout offset, but we just pass the shifted pointer
    lay.offset = 0;
    const uint8_t* subdata = rgba + (y * texWidth + x) * 4;
    // Wait, wgpuQueueWriteTexture expects the pointer to point to the start of the data being written, 
    // AND lay.bytesPerRow must be the full row pitch. 
    wgpuQueueWriteTexture(_queue, &dst, subdata, ((height - 1) * texWidth + width) * 4, &lay, &ext);
    
    return handle;
}
"""
        content = content.replace("void WebGpuRenderer::releaseTexture", impl + "\nvoid WebGpuRenderer::releaseTexture")
        with open("src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp", "w") as f:
            f.write(content)

apply()
