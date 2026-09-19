import sys

file_path = "src/Singularity/Screen/WebGPU/SdfWgsl.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("let s = max(d * 0.85, max(0.4, t * 0.02));", "let s = max(d * 0.85, max(1.5, t * 0.05));")
with open(file_path, "w") as f:
    f.write(content)
