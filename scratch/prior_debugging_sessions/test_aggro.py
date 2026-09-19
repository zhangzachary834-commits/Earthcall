import sys

file_path = "src/Singularity/Screen/WebGPU/SdfWgsl.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("let s = max(d * 0.85, max(0.4, t * 0.02));", "let s = max(d * 0.95, max(1.5, t * 0.06));")
content = content.replace("for (var i = 0; i < 48; i = i + 1) {", "for (var i = 0; i < 28; i = i + 1) {")

with open(file_path, "w") as f:
    f.write(content)
