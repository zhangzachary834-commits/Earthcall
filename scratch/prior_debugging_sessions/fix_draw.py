import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_drawMesh = """    MeshBatchKey key;
    key.mesh       = &mesh;
    key.albedoView = albedoView;
    key.baseColor  = glm::vec4(mat.baseColor, mat.opacity);
    key.shading    = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);

    InstanceData inst;
    inst.model     = _model;
    inst.normalMat = glm::transpose(glm::inverse(_model));
    _meshBatches[key].push_back(inst);"""

new_drawMesh = """    MeshBatchKey key;
    key.mesh       = &mesh;
    key.albedoView = albedoView;
    key.shading    = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);

    InstanceData inst;
    inst.model     = _model;
    inst.normalMat = glm::transpose(glm::inverse(_model));
    inst.baseColor = glm::vec4(mat.baseColor, mat.opacity);
    _meshBatches[key].push_back(inst);"""

if old_drawMesh in content:
    content = content.replace(old_drawMesh, new_drawMesh)
    print("Replaced drawMesh successfully")
else:
    print("Could not find drawMesh")
    
with open(file_path, "w") as f:
    f.write(content)
