with open("tests/singularity/frame_lag_test.cpp", "r") as f:
    content = f.read()

content = content.replace("macro->_materialId = mat->getIdentifier();", "macro->setMaterialId(mat->getIdentifier());")
content = content.replace("zone.addBeing(macro);", "zone.addObject(macro);")

with open("tests/singularity/frame_lag_test.cpp", "w") as f:
    f.write(content)
