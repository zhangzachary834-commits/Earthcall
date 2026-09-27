with open('src/ConstructedBeing/Singular/Singular.cpp', 'r') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "bool Singular::getDynamicProperty(Earthcall::StringId id, PropertyValue& out) const {" in line:
        # replace the method
        new_lines = lines[:i+1]
        new_lines.extend([
            "    if (recognizesAuthoredPropertyProjection(id)) {\n",
            "        return readAuthoredPropertyProjection(id, out);\n",
            "    }\n",
            "    auto it = _dynamicProperties.find(id);\n",
            "    if (it != _dynamicProperties.end()) {\n",
            "        out = it->second;\n",
            "        return true;\n",
            "    }\n",
            "    return false;\n",
            "}\n"
        ])
        
        # skip lines until next }
        j = i + 1
        while j < len(lines):
            if lines[j] == "}\n":
                j += 1
                break
            j += 1
        
        new_lines.extend(lines[j:])
        
        with open('src/ConstructedBeing/Singular/Singular.cpp', 'w') as out:
            out.writelines(new_lines)
        break
