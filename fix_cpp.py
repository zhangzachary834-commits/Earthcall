import sys

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

# Fix drawMesh to populate baseColor in InstanceData and remove it from MeshBatchKey
old_drawMesh = """    MeshBatchKey key;
    key.mesh = &mesh;
    key.albedoView = albedoView;
    key.baseColor = glm::vec4(mat.baseColor, mat.opacity);
    key.shading = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);

    InstanceData inst;
    inst.model = _model;
    inst.normalMat = glm::transpose(glm::inverse(_model));
    _meshBatches[key].push_back(inst);"""

new_drawMesh = """    MeshBatchKey key;
    key.mesh = &mesh;
    key.albedoView = albedoView;
    key.shading = glm::vec4(mat.ambient, mat.diffuse, mat.specular, mat.shininess);

    InstanceData inst;
    inst.model = _model;
    inst.normalMat = glm::transpose(glm::inverse(_model));
    inst.baseColor = glm::vec4(mat.baseColor, mat.opacity);
    _meshBatches[key].push_back(inst);"""

if old_drawMesh in content:
    content = content.replace(old_drawMesh, new_drawMesh)
    print("Replaced drawMesh successfully")
else:
    print("Could not find drawMesh")

# Fix flushMeshDraws to remove baseColor from MeshUniforms
old_u = """        MeshUniforms u;
        u.viewProj  = _viewProj;
        u.baseColor = key.baseColor;
        u.lightPos  = glm::vec4(lightPos(), 1.0f);
        u.params    = key.shading;
        u.eyePos    = glm::vec4(_eyePos, 1.0f);"""

new_u = """        MeshUniforms u;
        u.viewProj  = _viewProj;
        u.lightPos  = glm::vec4(lightPos(), 1.0f);
        u.params    = key.shading;
        u.eyePos    = glm::vec4(_eyePos, 1.0f);"""

if old_u in content:
    content = content.replace(old_u, new_u)
    print("Replaced u.baseColor successfully")
else:
    print("Could not find u.baseColor")

# Fix WGSL struct U
old_wgsl = """struct U {
    viewProj:  mat4x4<f32>,
    baseColor: vec4<f32>,
    lightPos:  vec4<f32>,   // world-space POSITION (GL_LIGHT0 is positional)
    params:    vec4<f32>,   // x=ambient, y=diffuse, z=specular, w=shininess
    eyePos:    vec4<f32>,
};"""

new_wgsl = """struct U {
    viewProj:  mat4x4<f32>,
    lightPos:  vec4<f32>,   // world-space POSITION (GL_LIGHT0 is positional)
    params:    vec4<f32>,   // x=ambient, y=diffuse, z=specular, w=shininess
    eyePos:    vec4<f32>,
};"""

if old_wgsl in content:
    content = content.replace(old_wgsl, new_wgsl)
    print("Replaced WGSL struct U successfully")
else:
    print("Could not find WGSL struct U")

# Fix WGSL struct Instance
old_inst = """struct Instance {
    model:     mat4x4<f32>,
    normalMat: mat4x4<f32>,
};"""

new_inst = """struct Instance {
    model:     mat4x4<f32>,
    normalMat: mat4x4<f32>,
    baseColor: vec4<f32>,
};"""

if old_inst in content:
    content = content.replace(old_inst, new_inst)
    print("Replaced WGSL struct Instance successfully")
else:
    print("Could not find WGSL struct Instance")

# Fix WGSL fragment shader
old_frag = """    let base = u.baseColor * paint;"""
new_frag = """    let base = in.baseColor * paint;"""
if old_frag in content:
    content = content.replace(old_frag, new_frag)
    print("Replaced WGSL fragment shader base successfully")
else:
    print("Could not find WGSL fragment shader base")

# Fix VSOut struct to pass baseColor to fragment
old_vsout = """struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) normal: vec3<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) worldPos: vec3<f32>,
};"""
new_vsout = """struct VSOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) normal: vec3<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) worldPos: vec3<f32>,
    @location(3) baseColor: vec4<f32>,
};"""
if old_vsout in content:
    content = content.replace(old_vsout, new_vsout)
    print("Replaced VSOut successfully")
else:
    print("Could not find VSOut")

# Fix vertex shader
old_vs = """    out.worldPos = wp.xyz;
    return out;"""
new_vs = """    out.worldPos = wp.xyz;
    out.baseColor = inst.baseColor;
    return out;"""
if old_vs in content:
    content = content.replace(old_vs, new_vs)
    print("Replaced VS assignment successfully")
else:
    print("Could not find VS assignment")

with open(file_path, "w") as f:
    f.write(content)

