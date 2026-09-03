import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;", "SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;")

# Actually it wrote literally "\n" if I didn't interpret it.
content = content.replace("SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;", "SdfBatchKey key = kv.first;\\n        const SdfPipeline* sp = key.sp;")
with open(file_path, "w") as f:
    f.write(content)
