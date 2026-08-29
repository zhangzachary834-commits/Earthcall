import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

# Fix drawImplicit insertions
content = content.replace("inst.paramOffset = static_cast<uint32_t>(_sdfParamsBatches[sp].size());", "inst.paramOffset = static_cast<uint32_t>(_sdfParamsBatches[{sp, memoId}].size());")
content = content.replace("_sdfBatches[sp].push_back(inst);", "_sdfBatches[{sp, memoId}].push_back(inst);")
content = content.replace("_sdfParamsBatches[sp].insert(_sdfParamsBatches[sp].end(), prog.params.begin(), prog.params.end());", "_sdfParamsBatches[{sp, memoId}].insert(_sdfParamsBatches[{sp, memoId}].end(), prog.params.begin(), prog.params.end());")

# Fix flushSdfDraws loop
# Originally it was:
#    for (auto& kv : _sdfBatches) {
#        const SdfPipeline* sp = kv.first;
content = content.replace("    for (auto& kv : _sdfBatches) {\\n        const SdfPipeline* sp = kv.first;", "    for (auto& kv : _sdfBatches) {\\n        SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;")
content = content.replace("        SdfBatchKey key = kv.first;\\n        SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;", "        SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;")

# In case it was already partly replaced:
content = content.replace("    for (auto& kv : _sdfBatches) {\\n        SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;\\n        const auto& instances = kv.second;", "    for (auto& kv : _sdfBatches) {\\n        SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;\\n        const auto& instances = kv.second;")

# Check if there's any remaining `kv.first` that we missed
content = content.replace("const SdfPipeline* sp = kv.first;", "SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;")

with open(file_path, "w") as f:
    f.write(content)
