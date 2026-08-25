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
        // Check if size or meshId changed
        if (it->second.vertexCount == mesh.tris.size() && it->second.meshId == mesh.id) {
            it->second.lastUsedFrame = _currentFrame;
            return it->second.buffer;
        }
        // Mesh changed: release old buffer
        if (it->second.buffer) {
            wgpuBufferRelease(it->second.buffer);
            _totalCachedBytes -= it->second.sizeBytes;
        }
    }

    // Allocate and upload persistent vertex buffer
    WGPUBufferDescriptor desc = {};
    desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    desc.size = vbytes;
    WGPUBuffer vbuf = wgpuDeviceCreateBuffer(_device, &desc);
    wgpuQueueWriteBuffer(_queue, vbuf, 0, mesh.tris.data(), vbytes);

    _totalCachedBytes += vbytes;
    _cache[meshPtr] = CachedMesh{ vbuf, mesh.tris.size(), vbytes, mesh.id, _currentFrame };
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
