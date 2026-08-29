import sys

file_path = "src/Singularity/Screen/WebGPU/SdfWgsl.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("let M = 4.0; // Lipschitz bound from 0.25 damping", "let M = 0.4;")

with open(file_path, "w") as f:
    f.write(content)
