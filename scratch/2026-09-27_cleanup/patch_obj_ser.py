import re
with open("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp", "r") as f:
    c = f.read()

# Let's revert it back to the original first
c = re.sub(r'    bool texturesAlreadyHere = false;\n    const Material\* currentMat = mgr\.resolveMaterial\(obj\.materialId\(\)\);\n    if \(currentMat && !currentMat->faceTextures\.empty\(\)\) \{\n        texturesAlreadyHere = true;\n    \}',
           r'    bool texturesAlreadyHere = obj.hasMaterial() && obj.ownMaterial() &&\n                              !obj.ownMaterial()->faceTextures.empty();', c)

# Now, instead of diverging to clear, we only clear if we ALREADY own the material!
c = re.sub(r'    \} else \{\n        // If not in JSON and no textures exist on the object, ensure empty\n        if \(!texturesAlreadyHere\) \{\n            if \(obj\.ownMaterial\(\)\) obj\.ownMaterial\(\)->faceTextures\.clear\(\);\n        \}\n    \}',
           r'    } else {\n        // If not in JSON, clear only if we ALREADY own the material\n        if (obj.hasMaterial() && !texturesAlreadyHere) {\n            obj.ownMaterial()->faceTextures.clear();\n        }\n    }', c)

with open("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp", "w") as f:
    f.write(c)
