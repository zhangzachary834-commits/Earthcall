#pragma once

#include "ConstructedBeing/Singular/Object/Geometry/SmoothSurface.hpp"
#include <webgpu/webgpu.h>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace Singularity {
namespace Screen {
namespace WebGPU {

// Persistent GPU Vertex Buffer Cache for static and cached geometric forms.
//
// When an Object has static geometry (Polyhedron, SmoothSurface, ComplexShape,
// Patch, or cached SdfMesh), GpuMeshCache retains the uploaded WGPUBuffer in VRAM
// across frames. Re-uploading occurs only when geometry is marked dirty.
class GpuMeshCache {
public:
    struct CachedMesh {
        WGPUBuffer buffer = nullptr;
        size_t     vertexCount = 0;
        size_t     sizeBytes = 0;
        uint64_t   meshId = 0;
        uint64_t   lastUsedFrame = 0;
    };

    GpuMeshCache() = default;
    ~GpuMeshCache() { shutdown(); }

    void init(WGPUDevice device, WGPUQueue queue) {
        _device = device;
        _queue  = queue;
    }

    void shutdown();

    void beginFrame(uint64_t frameIndex) {
        _currentFrame = frameIndex;
    }

    void endFrame();

    // Fetch or upload a GPU vertex buffer for a given TessMesh address.
    // If the mesh content version/address changes, re-uploads.
    WGPUBuffer getOrUpload(const geom::TessMesh& mesh);

    size_t cachedMeshCount() const { return _cache.size(); }
    size_t totalCachedBytes() const { return _totalCachedBytes; }

private:
    WGPUDevice _device = nullptr;
    WGPUQueue  _queue  = nullptr;
    std::unordered_map<const geom::TessMesh*, CachedMesh> _cache;
    size_t _totalCachedBytes = 0;
    uint64_t _currentFrame = 0;
};

} // namespace WebGPU
} // namespace Screen
} // namespace Singularity
