#pragma once

#include <webgpu/webgpu.h>
#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>

namespace Singularity {
namespace Screen {
namespace WebGPU {

// Dynamic GPU Buffer Ring & Sub-Allocator for CPU-to-GPU micro-mastery.
//
// Solves the per-frame allocation problem: instead of calling wgpuDeviceCreateBuffer
// and wgpuBufferRelease dozens of times per frame (which destroys VRAM caches and
// creates driver sync bubbles), GpuBufferPool maintains large pre-allocated GPU
// buffers (Uniform and Vertex) and sub-allocates aligned slices using fast offset math.
//
// Uniform slices are 256-byte aligned (the WebGPU/Metal uniform buffer offset requirement).
class GpuBufferPool {
public:
    struct Allocation {
        WGPUBuffer buffer = nullptr;
        uint64_t   offset = 0;
        uint64_t   size   = 0;
        bool       valid  = false;
    };

    GpuBufferPool() = default;
    ~GpuBufferPool() { shutdown(); }

    void init(WGPUDevice device, WGPUQueue queue,
              size_t uniformChunkSize = 256 * 1024,
              size_t vertexChunkSize  = 512 * 1024);
    void shutdown();

    // Sub-allocate a chunk of uniform buffer memory and upload data to it.
    // Offset is automatically aligned to 256 bytes.
    Allocation suballocateUniform(const void* data, size_t size);

    // Sub-allocate a chunk of vertex buffer memory and upload data to it.
    // Offset is aligned to 16 bytes.
    Allocation suballocateVertex(const void* data, size_t size);

    // Reset allocation heads at frame boundary without releasing GPU VRAM.
    void resetFrame();

    size_t totalVramBytes() const { return _totalVramBytes; }
    size_t bytesWrittenThisFrame() const { return _bytesWrittenThisFrame; }
    uint32_t suballocationsThisFrame() const { return _suballocationsThisFrame; }

private:
    struct Chunk {
        WGPUBuffer buffer = nullptr;
        size_t capacity = 0;
        size_t head = 0;
    };

    WGPUDevice _device = nullptr;
    WGPUQueue  _queue  = nullptr;
    size_t _uniformChunkSize = 256 * 1024;
    size_t _vertexChunkSize  = 512 * 1024;

    std::vector<Chunk> _uniformChunks;
    std::vector<Chunk> _vertexChunks;
    size_t _currentUniformChunk = 0;
    size_t _currentVertexChunk = 0;

    size_t   _totalVramBytes = 0;
    size_t   _bytesWrittenThisFrame = 0;
    uint32_t _suballocationsThisFrame = 0;

    Chunk allocateChunk(WGPUBufferUsage usage, size_t size);
    Allocation suballocateFrom(std::vector<Chunk>& chunks, size_t& currentIdx,
                               WGPUBufferUsage usage, size_t defaultChunkSize,
                               const void* data, size_t size, size_t alignment);
};

} // namespace WebGPU
} // namespace Screen
} // namespace Singularity
