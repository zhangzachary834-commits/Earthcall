import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace('compile\\n");', 'compile\\n");') # if it already has it
# The actual broken ones have a literal newline between compile and "
content = content.replace('compile\n");', 'compile\\n");')
content = content.replace('ms\n", ms);', 'ms\\n", ms);')

with open(file_path, "w") as f:
    f.write(content)
