import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("""    struct MemoizedProgram {
        uint32_t revision = 0xffffffff;
        sdfwgsl::Program prog;
        const SdfPipeline* sp = nullptr;
    };""", """    struct MemoizedProgram {
        uint32_t revision = 0xffffffff;
        sdfwgsl::Program prog;
        const SdfPipeline* sp = nullptr;
        WGPUBuffer proxyVB = nullptr;
        WGPUBuffer proxyIB = nullptr;
        uint32_t proxyIndexCount = 0;
    };""")

content = content.replace("""    WGPUBuffer _sdfCubeVerts = nullptr; // unit bounding cube, shared by every field""", """    WGPUBuffer _sdfCubeVerts = nullptr; // unit bounding cube, shared by every field
    WGPUBuffer _sdfCubeIndices = nullptr;""")

content = content.replace("""    std::map<const SdfPipeline*, std::vector<SdfInstanceData>> _sdfBatches;
    std::map<const SdfPipeline*, std::vector<float>> _sdfParamsBatches;""", """    struct SdfBatchKey {
        const SdfPipeline* sp;
        uint64_t memoId;
        bool operator<(const SdfBatchKey& o) const {
            if (sp != o.sp) return sp < o.sp;
            return memoId < o.memoId;
        }
    };
    std::map<SdfBatchKey, std::vector<SdfInstanceData>> _sdfBatches;
    std::map<SdfBatchKey, std::vector<float>> _sdfParamsBatches;""")

with open(file_path, "w") as f:
    f.write(content)
