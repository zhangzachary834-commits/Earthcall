#include "GpuMeshCache.hpp"

namespace Singularity {
namespace Screen {
namespace WebGPU {

void GpuMeshCache::shutdown() {
    for (auto& kv : _cache) {
        if (kv.second.buffer) {
            wgpuBufferRelease(kv.second.buffer);
            kv.second.buffer = nullptr;
        }
    }
    _cache.clear();
    _totalCachedBytes = 0;
}

WGPUBuffer GpuMeshCache::getOrUpload(const geom::TessMesh& mesh) {
    if (!_device || !_queue || mesh.tris.empty()) {
        return nullptr;
    }

    const auto* meshPtr = &mesh;
    auto it = _cache.find(meshPtr);
    const size_t vbytes = mesh.tris.size() * sizeof(geom::TessVertex);

    if (it != _cache.end()) {
        // Check if size, identity, or revision changed. meshId catches a NEW
        // TessMesh landing at a freed address (the reincarnation bug); revision
        // catches an in-place edit of a mesh that kept both its address and id.
        if (it->second.vertexCount == mesh.tris.size() && it->second.meshId == mesh.id &&
            it->second.meshRevision == mesh.revision) {
            it->second.lastUsedFrame = _currentFrame;
            return it->second.buffer;
        }
        // Mesh changed: release old buffer
        if (it->second.buffer) {
            wgpuBufferRelease(it->second.buffer);
            _totalCachedBytes -= it->second.sizeBytes;
        }
    }

    // Allocate and upload persistent vertex buffer. CopySrc costs nothing a
    // vertex buffer wasn't already paying for and lets a debug/test readback
    // (gpu_mastery_test [4c]/[4e]) verify what actually landed in VRAM instead
    // of trusting the WGPUBuffer handle's address, which the driver is free to
    // recycle after a release.
    WGPUBufferDescriptor desc = {};
    desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    desc.size = vbytes;
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(_device, &desc);
    wgpuQueueWriteBuffer(_queue, vbuf, 0, mesh.tris.data(), vbytes);

    _totalCachedBytes += vbytes;
    _cache[meshPtr] = CachedMesh{ vbuf, mesh.tris.size(), vbytes, mesh.id, mesh.revision, _currentFrame };
    return vbuf;
}

void GpuMeshCache::endFrame() {
    // Garbage collect meshes not used in the last 10 frames
    for (auto it = _cache.begin(); it != _cache.end(); ) {
        if (_currentFrame > it->second.lastUsedFrame + 10) {
            if (it->second.buffer) {
                wgpuBufferRelease(it->second.buffer);
                _totalCachedBytes -= it->second.sizeBytes;
            }
            it = _cache.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace WebGPU
} // namespace Screen
} // namespace Singularity
