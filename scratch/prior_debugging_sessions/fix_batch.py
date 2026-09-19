import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
with open(file_path, "r") as f:
    content = f.read()

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

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("inst.paramOffset = static_cast<uint32_t>(_sdfParamsBatches[sp].size());", "inst.paramOffset = static_cast<uint32_t>(_sdfParamsBatches[{sp, memoId}].size());")
content = content.replace("_sdfBatches[sp].push_back(inst);", "_sdfBatches[{sp, memoId}].push_back(inst);")
content = content.replace("_sdfParamsBatches[sp].insert(_sdfParamsBatches[sp].end(), prog.params.begin(), prog.params.end());", "_sdfParamsBatches[{sp, memoId}].insert(_sdfParamsBatches[{sp, memoId}].end(), prog.params.begin(), prog.params.end());")

content = content.replace("    for (auto& kv : _sdfBatches) {\\n        const SdfPipeline* sp = kv.first;", "    for (auto& kv : _sdfBatches) {\\n        SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;")
content = content.replace("const auto& params = _sdfParamsBatches[sp];", "const auto& params = _sdfParamsBatches[key];")

with open(file_path, "w") as f:
    f.write(content)
