import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuContext.mm"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("cfg.presentMode = WGPUPresentMode_Fifo; // vsync", "cfg.presentMode = WGPUPresentMode_Immediate; // vsync OFF")
with open(file_path, "w") as f:
    f.write(content)
