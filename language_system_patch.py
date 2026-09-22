import sys

with open("src/Singularity/Language/LanguageSystem.cpp", "r") as f:
    lines = f.readlines()

new_lines = []
skip = False
for i, line in enumerate(lines):
    if "// 2. Synaptic Plasticity (Decay semantic weights)" in line:
        skip = True
        new_lines.append(line)
        new_lines.append("    // Removed. Synaptic Plasticity is now a fully authored OntoMath function\n")
        new_lines.append("    // running as a Law in the world.\n")
        new_lines.append("}\n")
    elif skip and "void LanguageSystem::queueUtterance" in line:
        skip = False
        new_lines.append("\n")
        new_lines.append(line)
    elif not skip:
        new_lines.append(line)

with open("src/Singularity/Language/LanguageSystem.cpp", "w") as f:
    f.writelines(new_lines)
