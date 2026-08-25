#include "GpuBufferPool.hpp"

namespace Singularity {
namespace Screen {
namespace WebGPU {

void GpuBufferPool::init(WGPUDevice device, WGPUQueue queue,
                         size_t uniformChunkSize, size_t vertexChunkSize) {
    _device = device;
    _queue  = queue;
    _uniformChunkSize = uniformChunkSize;
    _vertexChunkSize  = vertexChunkSize;
    _totalVramBytes = 0;
    _bytesWrittenThisFrame = 0;
    _suballocationsThisFrame = 0;
}

void GpuBufferPool::shutdown() {
    for (auto& c : _uniformChunks) {
        if (c.buffer) {
            wgpuBufferRelease(c.buffer);
            c.buffer = nullptr;
        }
    }
    _uniformChunks.clear();

    for (auto& c : _vertexChunks) {
        if (c.buffer) {
            wgpuBufferRelease(c.buffer);
            c.buffer = nullptr;
        }
    }
    _vertexChunks.clear();

    _currentUniformChunk = 0;
    _currentVertexChunk = 0;
    _totalVramBytes = 0;
}

GpuBufferPool::Chunk GpuBufferPool::allocateChunk(WGPUBufferUsage usage, size_t size) {
    WGPUBufferDescriptor desc = {};
    desc.usage = usage;
    desc.size = size;
    WGPUBuffer buf = wgpuDeviceCreateBuffer(_device, &desc);
    _totalVramBytes += size;
    return Chunk{ buf, size, 0 };
}

GpuBufferPool::Allocation GpuBufferPool::suballocateFrom(
    std::vector<Chunk>& chunks, size_t& currentIdx,
    WGPUBufferUsage usage, size_t defaultChunkSize,
    const void* data, size_t size, size_t alignment) {

    if (!_device || !_queue || size == 0) {
        return Allocation{};
    }

    if (chunks.empty()) {
        const size_t initialCapacity = std::max(defaultChunkSize, size);
        chunks.push_back(allocateChunk(usage, initialCapacity));
        currentIdx = 0;
    }

    // Align head
    auto& chunk = chunks[currentIdx];
    size_t alignedHead = (chunk.head + (alignment - 1)) & ~(alignment - 1);

    if (alignedHead + size > chunk.capacity) {
        // Need next chunk or new chunk
        bool found = false;
        for (size_t i = currentIdx + 1; i < chunks.size(); ++i) {
            if (chunks[i].capacity >= size) {
                currentIdx = i;
                chunks[currentIdx].head = 0;
                alignedHead = 0;
                found = true;
                break;
            }
        }
        if (!found) {
            const size_t newCapacity = std::max(defaultChunkSize, size);
            chunks.push_back(allocateChunk(usage, newCapacity));
            currentIdx = chunks.size() - 1;
            alignedHead = 0;
        }
    }

    auto& activeChunk = chunks[currentIdx];
    activeChunk.head = alignedHead + size;

    if (data) {
        wgpuQueueWriteBuffer(_queue, activeChunk.buffer, alignedHead, data, size);
    }

    _bytesWrittenThisFrame += size;
    _suballocationsThisFrame++;

    return Allocation{ activeChunk.buffer, static_cast<uint64_t>(alignedHead), static_cast<uint64_t>(size), true };
}

GpuBufferPool::Allocation GpuBufferPool::suballocateUniform(const void* data, size_t size) {
    // 256-byte alignment as per WebGPU specification for uniform buffer offsets
    return suballocateFrom(_uniformChunks, _currentUniformChunk,
                           WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
                           _uniformChunkSize, data, size, 256);
}

GpuBufferPool::Allocation GpuBufferPool::suballocateVertex(const void* data, size_t size) {
    // 16-byte alignment for SIMD/Float4 vertex vectors
    return suballocateFrom(_vertexChunks, _currentVertexChunk,
                           WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
                           _vertexChunkSize, data, size, 16);
}

void GpuBufferPool::resetFrame() {
    _currentUniformChunk = 0;
    _currentVertexChunk = 0;
    for (auto& c : _uniformChunks) c.head = 0;
    for (auto& c : _vertexChunks)  c.head = 0;
    _bytesWrittenThisFrame = 0;
    _suballocationsThisFrame = 0;
}

} // namespace WebGPU
} // namespace Screen
} // namespace Singularity
