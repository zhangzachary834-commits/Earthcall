// Object — rendering subsystem (split from Object.cpp).
// drawCube / smooth / complex / field / drawObject / highlight outline / drawPolyhedron + GL helpers.

#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate / glm::rotate for cap placement
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include "Singularity/Screen/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------
// Modified drawCube to bind per-face texture
// ---------------------------------------------------------------------

// --- Legacy primitive meshes (Milestone 2: behind the renderer boundary) ----
// The legacy ShapeKind::Sphere/Cylinder/Cone/Cube paths used to emit immediate-
// mode GL here. They are now built as TessMeshes and drawn through the Renderer
// like every other surface. These are UNIT shapes (radius 0.5, height 1) — the
// object's size comes from its transform — so each mesh is constant and built
// exactly once, then shared. The vertex math (axis +Z, outward normals, GLU-style
// texture coords) is preserved from the gluSphere/gluCylinder/gluDisk conventions.
//
// Winding note: there is no backface culling anywhere in the app, so triangulating
// the old quad strips in any consistent order is safe — per-vertex normals carry
// the lighting, and they are preserved exactly.
namespace {

geom::TessVertex vtx(const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv) {
    geom::TessVertex v; v.pos = p; v.normal = n; v.uv = uv; return v;
}

// A row of quad-strip vertices [a0,b0,a1,b1,...] → triangles, appended to m.
void appendQuadStrip(geom::TessMesh& m, const std::vector<geom::TessVertex>& s) {
    for (size_t i = 0; i + 3 < s.size(); i += 2) {
        m.tris.push_back(s[i]);   m.tris.push_back(s[i + 1]); m.tris.push_back(s[i + 2]);
        m.tris.push_back(s[i + 1]); m.tris.push_back(s[i + 3]); m.tris.push_back(s[i + 2]);
    }
}

// Bake a transform into a copy of a mesh (positions by xf, normals by its inverse-
// transpose). Lets the cylinder/cone caps be positioned once instead of via the GL
// matrix stack at draw time.
geom::TessMesh transformedMesh(const geom::TessMesh& src, const glm::mat4& xf) {
    glm::mat3 nrm = glm::mat3(glm::transpose(glm::inverse(xf)));
    geom::TessMesh m = src;
    for (auto& v : m.tris) {
        v.pos    = glm::vec3(xf * glm::vec4(v.pos, 1.0f));
        v.normal = glm::normalize(nrm * v.normal);
    }
    return m;
}

// gluSphere(radius, slices, stacks): lat/long sphere about the Z axis.
geom::TessMesh buildSphereMesh(float radius, int slices, int stacks) {
    geom::TessMesh m;
    std::vector<geom::TessVertex> row;
    for (int i = 0; i < stacks; ++i) {
        float phi0 = M_PI * float(i) / stacks, phi1 = M_PI * float(i + 1) / stacks;
        float z0 = radius * cosf(phi0), r0 = radius * sinf(phi0);
        float z1 = radius * cosf(phi1), r1 = radius * sinf(phi1);
        float t0 = 1.0f - float(i) / stacks, t1 = 1.0f - float(i + 1) / stacks;
        row.clear();
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * M_PI * float(j) / slices, ct = cosf(theta), st = sinf(theta);
            float s = float(j) / slices;
            row.push_back(vtx({r0 * ct, r0 * st, z0}, {r0 * ct / radius, r0 * st / radius, z0 / radius}, {s, t0}));
            row.push_back(vtx({r1 * ct, r1 * st, z1}, {r1 * ct / radius, r1 * st / radius, z1 / radius}, {s, t1}));
        }
        appendQuadStrip(m, row);
    }
    return m;
}

// gluCylinder(base, top, height, slices, stacks): open side surface along +Z from
// z=0 (radius base) to z=height (radius top). top=0 gives a cone.
geom::TessMesh buildCylinderSideMesh(float baseR, float topR, float height, int slices, int stacks) {
    float dr = topR - baseR;
    float invLen = 1.0f / sqrtf(height * height + dr * dr);
    float nr = height * invLen, nz = -dr * invLen;
    geom::TessMesh m;
    std::vector<geom::TessVertex> row;
    for (int k = 0; k < stacks; ++k) {
        float f0 = float(k) / stacks, f1 = float(k + 1) / stacks;
        float z0 = height * f0, r0 = baseR + dr * f0;
        float z1 = height * f1, r1 = baseR + dr * f1;
        row.clear();
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * M_PI * float(j) / slices, ct = cosf(theta), st = sinf(theta);
            float s = float(j) / slices;
            row.push_back(vtx({r0 * ct, r0 * st, z0}, {nr * ct, nr * st, nz}, {s, f0}));
            row.push_back(vtx({r1 * ct, r1 * st, z1}, {nr * ct, nr * st, nz}, {s, f1}));
        }
        appendQuadStrip(m, row);
    }
    return m;
}

// gluDisk(inner, outer, slices, loops): flat disk in the z=0 plane, +Z normal.
geom::TessMesh buildDiskMesh(float innerR, float outerR, int slices, int loops) {
    geom::TessMesh m;
    std::vector<geom::TessVertex> row;
    for (int l = 0; l < loops; ++l) {
        float r0 = innerR + (outerR - innerR) * float(l) / loops;
        float r1 = innerR + (outerR - innerR) * float(l + 1) / loops;
        row.clear();
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * M_PI * float(j) / slices, ct = cosf(theta), st = sinf(theta);
            row.push_back(vtx({r0 * ct, r0 * st, 0.0f}, {0, 0, 1},
                              {r0 * ct / (2 * outerR) + 0.5f, r0 * st / (2 * outerR) + 0.5f}));
            row.push_back(vtx({r1 * ct, r1 * st, 0.0f}, {0, 0, 1},
                              {r1 * ct / (2 * outerR) + 0.5f, r1 * st / (2 * outerR) + 0.5f}));
        }
        appendQuadStrip(m, row);
    }
    return m;
}

// One cube face (2 triangles) with an outward normal and full [0,1] UVs.
geom::TessMesh buildCubeFaceMesh(int f) {
    static const struct { float nx, ny, nz; float vx[4][3]; } faceData[6] = {
        { 1,0,0,  { {0.5f,-0.5f,-0.5f}, {0.5f,0.5f,-0.5f}, {0.5f,0.5f,0.5f}, {0.5f,-0.5f,0.5f} } },
        {-1,0,0,  { {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,0.5f}, {-0.5f,0.5f,0.5f}, {-0.5f,0.5f,-0.5f} } },
        { 0,1,0,  { {-0.5f,0.5f,-0.5f}, {-0.5f,0.5f,0.5f}, {0.5f,0.5f,0.5f}, {0.5f,0.5f,-0.5f} } },
        { 0,-1,0, { {-0.5f,-0.5f,-0.5f}, {0.5f,-0.5f,-0.5f}, {0.5f,-0.5f,0.5f}, {-0.5f,-0.5f,0.5f} } },
        { 0,0,1,  { {-0.5f,-0.5f,0.5f}, {0.5f,-0.5f,0.5f}, {0.5f,0.5f,0.5f}, {-0.5f,0.5f,0.5f} } },
        { 0,0,-1, { {-0.5f,-0.5f,-0.5f}, {-0.5f,0.5f,-0.5f}, {0.5f,0.5f,-0.5f}, {0.5f,-0.5f,-0.5f} } }
    };
    const auto& fd = faceData[f];
    glm::vec3 n(fd.nx, fd.ny, fd.nz);
    geom::TessVertex a = vtx({fd.vx[0][0], fd.vx[0][1], fd.vx[0][2]}, n, {0, 0});
    geom::TessVertex b = vtx({fd.vx[1][0], fd.vx[1][1], fd.vx[1][2]}, n, {1, 0});
    geom::TessVertex c = vtx({fd.vx[2][0], fd.vx[2][1], fd.vx[2][2]}, n, {1, 1});
    geom::TessVertex d = vtx({fd.vx[3][0], fd.vx[3][1], fd.vx[3][2]}, n, {0, 1});
    geom::TessMesh m;
    m.tris = {a, b, c, a, c, d};
    return m;
}

// Cached unit meshes, built on first use (pure CPU — no GL context needed). The
// cylinder/cone are centred on Z in [-0.5, 0.5] to match the old draw transforms.
const geom::TessMesh& cubeFace(int f)          { static geom::TessMesh m[6]; static bool init=false; if(!init){for(int i=0;i<6;++i)m[i]=buildCubeFaceMesh(i);init=true;} return m[f]; }
// All 6 faces concatenated into one mesh, for the common case where every face
// resolves the same paint (see Object::drawCube): one draw instead of six. A
// static built once, like cubeFace() above, so GpuMeshCache can also cache its
// vertex buffer across frames rather than re-uploading it every draw.
const geom::TessMesh& mergedCubeMesh() {
    static geom::TessMesh m = [] {
        geom::TessMesh merged;
        for (int f = 0; f < 6; ++f) {
            const geom::TessMesh& face = cubeFace(f);
            merged.tris.insert(merged.tris.end(), face.tris.begin(), face.tris.end());
        }
        return merged;
    }();
    return m;
}
const geom::TessMesh& sphereUnitMesh()         { static geom::TessMesh m = buildSphereMesh(0.5f, 16, 16); return m; }
const geom::TessMesh& cylinderSideMesh()       { static geom::TessMesh m = transformedMesh(buildCylinderSideMesh(0.5f, 0.5f, 1.0f, 16, 4), glm::translate(glm::mat4(1.0f), {0,0,-0.5f})); return m; }
const geom::TessMesh& coneSideMesh()           { static geom::TessMesh m = transformedMesh(buildCylinderSideMesh(0.5f, 0.0f, 1.0f, 16, 4), glm::translate(glm::mat4(1.0f), {0,0,-0.5f})); return m; }
// Bottom/base cap: at local z=-0.5, flipped to face -Z (as the old glRotatef 180 did).
const geom::TessMesh& capBottomMesh()          { static geom::TessMesh m = transformedMesh(buildDiskMesh(0.0f, 0.5f, 32, 1), glm::translate(glm::mat4(1.0f), {0,0,-0.5f}) * glm::rotate(glm::mat4(1.0f), float(M_PI), {1,0,0})); return m; }
// Top cap: at local z=+0.5, facing +Z.
const geom::TessMesh& capTopMesh()             { static geom::TessMesh m = transformedMesh(buildDiskMesh(0.0f, 0.5f, 32, 1), glm::translate(glm::mat4(1.0f), {0,0,0.5f})); return m; }

} // namespace

void Object::drawCube() const {
    // Each face CAN carry its own painted texture, so the general path is six
    // separate drawMesh calls — like drawComplexModel's patches. But the common
    // case is an unpainted cube, where every face resolves the exact same
    // RenderMaterial (no per-face texture at all), and six draws for one
    // uniform box is pure driver overhead. Decided fresh every draw, from the
    // resolved albedo, never cached on the Object: paint a single face with
    // Object::setFaceColor / ownMaterial and the very next draw sees that
    // face's FaceAlbedo differ and falls back to the six-draw path.
    const FaceAlbedo first = faceAlbedo(0);
    bool allFacesSame = true;
    for (int f = 1; f < 6 && allFacesSame; ++f) {
        const FaceAlbedo fa = faceAlbedo(f);
        allFacesSame = (fa.handle == first.handle && fa.pixels == first.pixels &&
                        fa.size == first.size);
    }
    if (allFacesSame) {
        currentRenderer().drawMesh(mergedCubeMesh(), resolveRenderMaterial(_materialId, first));
        return;
    }
    for (int f = 0; f < 6; ++f)
        currentRenderer().drawMesh(cubeFace(f), resolveRenderMaterial(_materialId, faceAlbedo(f)));
}


// Render a triangle-soup tessellation in immediate mode (legacy GL path).
// The four topology draw paths now go through the Renderer boundary
// (OPENGL_MIGRATION_PLAN.md, Milestone 2): resolve this object's Material being
// into a RenderMaterial (stamped with the per-face albedo texture) and hand the
// cached mesh to currentRenderer().drawMesh. No raw GL here anymore — the backend
// (OpenGLRenderer today, WebGpuRenderer at M5) owns it. faceTextures is the paint;
// the material is the tint + light response; drawMesh composes them.

// One face's albedo, in both the handle and CPU-pixel forms — see FaceAlbedo.
// Empty when this object has no paint for that face, which leaves the surface
// showing its material baseColor alone.
#include "ConstructedBeing/Material/MaterialManager.hpp"
extern MaterialManager materials;

FaceAlbedo Object::faceAlbedo(size_t face) const {
    auto mat = materials.resolveOrDefault(_materialId);
    if (!mat || face >= mat->faceTextures.size()) return {};
    const FaceTexture& ft = mat->faceTextures[face];
    // A backend uploading from `pixels` trusts `size` to describe it. If the two
    // ever disagree it would read past the end of the buffer, so treat a mismatch
    // as "no paint" rather than handing out an overrun.
    const size_t expected = static_cast<size_t>(ft.size) * ft.size * 4;
    if (ft.size <= 0 || ft.pixels.size() != expected) return {};
    // Loaded paint restores CPU pixels; the GPU handle is 0 until something
    // uploads. First draw after a load is that something. uploadToGPU is
    // const (the handle is mutable); the paint itself does not change.
    if (ft.id == 0) ft.uploadToGPU();
    return FaceAlbedo{ft.id, ft.pixels.data(), ft.size};
}

// ---------------------------------------------------------------------------
// Paint: copy-on-write over the shared Material beings. See the note on
// Object::ownMaterial in Object.hpp for why this is not simply "paint the
// material you reference".
// ---------------------------------------------------------------------------
#include "ConstructedBeing/Material/PaintToolSurface.hpp"

std::shared_ptr<Material> Object::ownMaterial() {
    const std::string ownName = getIdentifier();
    const std::string ownId   = "material." + ownName;
    if (_materialId == ownId) {
        if (auto mine = materials.get(ownId)) return mine;
    }

    // Carry over the appearance of whatever was being shared, so the first
    // stroke changes the paint and nothing else: an object painted while it
    // wore material.clay stays clay-coloured under the new paint.
    auto shared = materials.resolveOrDefault(_materialId);
    auto mine = materials.create(ownName);
    if (shared && shared.get() != mine.get()) {
        mine->baseColor    = shared->baseColor;
        mine->opacity      = shared->opacity;
        mine->shininess    = shared->shininess;
        mine->specular     = shared->specular;
        mine->ambient      = shared->ambient;
        mine->diffuse      = shared->diffuse;
        mine->faceTextures = shared->faceTextures;
        // The copies are this material's own paint now; a texture handle
        // belongs to the buffer the backend uploaded, so let each re-upload
        // rather than two materials sharing one handle.
        for (FaceTexture& ft : mine->faceTextures) ft.id = 0;
    }
    _materialId = ownId;
    return mine;
}

void Object::initFaceTextures() {
    auto mine = ownMaterial();
    if (!mine) return;
    const int faces = getFaces() > 0 ? getFaces() : 1;
    mine->initFaceTextures(faces);
}

void Object::setFaceColor(int faceIndex, float r, float g, float b) {
    if (faceIndex >= 0 && faceIndex < 6) {
        faceColors[faceIndex][0] = r;
        faceColors[faceIndex][1] = g;
        faceColors[faceIndex][2] = b;
    }
    auto mine = ownMaterial();
    if (!mine) return;
    // A face can only be painted once there is paint to put it on. The
    // textures follow the geometry's face count, so seeding them here is what
    // keeps a shape change from leaving stale faces behind.
    const int faces = getFaces() > 0 ? getFaces() : 1;
    if (static_cast<int>(mine->faceTextures.size()) != faces) {
        mine->initFaceTextures(faces);
    }
    PaintToolSurface(*mine).fillFaceColor(faceIndex, r, g, b);
}

void Object::drawSmoothModel() const {
    Renderer& r = currentRenderer();
    const RenderMaterial mat = resolveRenderMaterial(_materialId, faceAlbedo(0));
    // Identity is the quadric / parametric surface. The cached mesh is a
    // drawing approximation for backends that cannot march it. Routing
    // OpenGL through drawImplicit would re-tessellate every frame.
    // RenderMode::Mesh opts a Law OUT of the exact analytic path even where
    // the backend supports it — trading exactness for the instanced draw
    // path only tessellated meshes get.
    if (r.rendersImplicitExactly() && _renderMode != RenderMode::Mesh) {
        const float extent = std::max(std::max(std::abs(smoothData.axes.x),
                                               std::abs(smoothData.axes.y)),
                                      std::abs(smoothData.axes.z)) + 0.25f;
        r.drawImplicit(geom::sdfFromSmooth(smoothData), glm::vec3(std::max(extent, 0.6f)), mat, nullptr,
                       getMemoId(), getFieldRevision());
        return;
    }
    if (_smoothMesh) r.drawMesh(*_smoothMesh, mat);
}

void Object::drawComplexModel() const {
    Renderer& r = currentRenderer();
    // A capped cylinder / cone IS the round quadric plus planar caps.
    // The UV side mesh and N-gon disks are a drawing cache. Backends that
    // can march an SDF draw the primitive instead, same door as spheres.
    // RenderMode::Mesh opts out of that, same reasoning as drawSmoothModel.
    if (r.rendersImplicitExactly() && _renderMode != RenderMode::Mesh) {
        geom::SdfNode field;
        if (geom::sdfFromComplex(complexData, field)) {
            const float rExt = std::max(_shapeParams.r, _shapeParams.halfH) + 0.25f;
            r.drawImplicit(field, glm::vec3(std::max(rExt, 0.6f)),
                           resolveRenderMaterial(_materialId, faceAlbedo(0)), nullptr,
                           getMemoId(), getFieldRevision());
            return;
        }
        for (size_t i = 0; i < complexData.patches.size(); ++i) {
            const auto& patch = complexData.patches[i];
            const RenderMaterial mat = resolveRenderMaterial(_materialId, faceAlbedo(static_cast<int>(i)));
            if (patch.type == geom::SurfacePatch::Type::Smooth) {
                const float extent = std::max(std::abs(patch.smooth.axes.x),
                    std::max(std::abs(patch.smooth.zTrim.x), std::abs(patch.smooth.zTrim.y))) + 0.25f;
                r.drawImplicit(geom::sdfFromSmooth(patch.smooth), glm::vec3(std::max(extent, 0.6f)), mat, nullptr,
                               getMemoId(static_cast<int>(i) + 1), getFieldRevision());
            } else if (i < _complexMeshes.size()) {
                r.drawMesh(_complexMeshes[i], mat);
            }
        }
        return;
    }
    // Each patch is a real face, drawn with its own face texture so the round side
    // and the flat caps can be painted independently.
    for (size_t i = 0; i < _complexMeshes.size(); ++i)
        r.drawMesh(_complexMeshes[i],
                   resolveRenderMaterial(_materialId, faceAlbedo(static_cast<int>(i))));
}

void Object::drawFieldModel() const {
    Renderer& r = currentRenderer();
    const RenderMaterial mat = resolveRenderMaterial(_materialId, faceAlbedo(0));

    // A field IS its distance function; the mesh is only ever an approximation of
    // it at some resolution. Where the backend can evaluate the function directly
    // (WebGPU raymarches it), hand over the function and skip the mesh entirely —
    // no tessellation seams, and the surface is exact at any zoom. Backends that
    // cannot fall back to the cached mesh, which is why this asks rather than
    // always calling drawImplicit.
    if (r.rendersImplicitExactly()) {
        r.drawImplicit(getFieldData(), getFieldExtent(), mat, nullptr,
                       getMemoId(), getFieldRevision());
        return;
    }
    rebuildFieldMesh();
    r.drawMesh(_fieldMesh, mat);
}

void Object::drawPatchModel() const {
    // A Bezier patch is an OPEN surface, not a closed volume — its back is visible.
    // doubleSided tells the renderer to light both faces (GL two-sided model today,
    // cull-none in WebGPU), so the underside isn't dark.
    RenderMaterial mat = resolveRenderMaterial(_materialId, faceAlbedo(0));
    mat.doubleSided = true;
    currentRenderer().drawMesh(_patchMesh, mat);
}

void Object::drawObject() const {
    // Identity is topology (quadric / complex / field / patch). The 16-slice
    // glu sphere/cylinder/cone meshes are a render cache of last resort for
    // objects that lost _hasSmooth — they must not BE the sphere. Rebuild
    // the named kind instead of drawing that tessellation.
    if (!_hasField && !_hasComplex && !_hasSmooth && !_hasPatch) {
        switch (_shapeKind) {
            case ShapeKind::Sphere:
            case ShapeKind::Ellipsoid:
            case ShapeKind::Ovoid:
            case ShapeKind::Paraboloid:
            case ShapeKind::Torus:
            case ShapeKind::Cylinder:
            case ShapeKind::Cone:
            case ShapeKind::RoundedBox:
                const_cast<Object*>(this)->setShape(_shapeKind, _shapeParams);
                break;
            default:
                break;
        }
    }
    if (_hasField)   { drawFieldModel();   return; }
    if (_hasComplex) { drawComplexModel(); return; }
    if (_hasSmooth)  { drawSmoothModel();  return; }
    if (_hasPatch)   { drawPatchModel();   return; }
    switch (_shapeKind) {
        case ShapeKind::Cube:
            drawCube();
            break;
        case ShapeKind::Polyhedron:
            drawPolyhedron();
            break;
        default:
            break;
    }
}

// Render a glowing outline around the object's collision zone using multiple scaled passes
void Object::drawHighlightOutline() const {
    using Rendering::HighlightSystem;
    bool sel = HighlightSystem::isSelected(this);
    bool cand = HighlightSystem::isLawCandidate(this);
    if (!sel && !cand) return;

    // Choose color: yellow for selection, red for law-candidate
    glm::vec3 color = sel ? glm::vec3(1.0f, 0.9f, 0.2f) : glm::vec3(1.0f, 0.2f, 0.2f);

    // Trace the ACTUAL shape (local space — the model transform is already applied
    // by the caller), not an AABB. Flat-faced shapes get a crisp edge wireframe;
    // curved / field shapes get a translucent additive glow shell hugging them.
    // Two passes each: the outer pass is fainter and wider/larger for a soft glow.
    // All GL now lives in the renderer's drawLines/drawOverlay; here we only decide
    // the shape, colour, and pass falloff.
    Renderer& r = currentRenderer();
    auto wire = [&](const std::vector<std::pair<glm::vec3, glm::vec3>>& edges) {
        for (int p = 0; p < 2; ++p)
            r.drawLines(edges, glm::vec4(color, 0.65f - 0.3f * p), 2.0f + 2.5f * p, Blend::Alpha);
    };
    auto shell = [&](const geom::TessMesh& m) {
        for (int p = 0; p < 2; ++p)
            r.drawOverlay(m, glm::vec4(color, 0.16f - 0.06f * p), 1.0f + 0.02f * (p + 1), true);
    };

    if (_shapeKind == ShapeKind::Polyhedron && !polyhedronData.vertices.empty()) {
        std::vector<std::pair<glm::vec3, glm::vec3>> edges;
        for (const auto& face : polyhedronData.faces)
            for (size_t k = 0; k < face.size(); ++k)
                edges.emplace_back(polyhedronData.vertices[face[k]],
                                   polyhedronData.vertices[face[(k + 1) % face.size()]]);
        wire(edges);
    } else if (_hasField) {
        rebuildFieldMesh();
        shell(_fieldMesh);
    } else if (_hasSmooth) {
        if (_smoothMesh) shell(*_smoothMesh);
    } else if (_hasPatch) {
        shell(_patchMesh);
    } else if (_hasComplex) {
        // Additive blending sums the same triangles regardless of grouping, so
        // shelling per patch matches the old single merged mesh.
        for (const auto& pm : _complexMeshes) shell(pm);
    } else {
        // Legacy unit cube wireframe.
        const float h = 0.5f;
        glm::vec3 v[8] = {
            {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
            {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}
        };
        std::vector<std::pair<glm::vec3, glm::vec3>> edges = {
            {v[0],v[1]},{v[1],v[2]},{v[2],v[3]},{v[3],v[0]},
            {v[4],v[5]},{v[5],v[6]},{v[6],v[7]},{v[7],v[4]},
            {v[0],v[4]},{v[1],v[5]},{v[2],v[6]},{v[3],v[7]}
        };
        wire(edges);
    }
}

// Build one TessMesh per polyhedron face (centroid fan, Newell normal, per-face
// projected UVs — identical to what drawPolyhedron used to emit immediately).
// Cached: rebuilt only when polyhedronData changes (drawPolyhedron sets the dirty
// flag), never per frame — the same invariant the topology caches hold.
void Object::rebuildPolyhedronMeshes() const {
    _polyhedronDirty = false;
    _polyhedronFaceMeshes.clear();
    if (polyhedronData.vertices.empty() || polyhedronData.faces.empty()) return;

    for (const auto& face : polyhedronData.faces) {
        geom::TessMesh mesh;
        if (face.size() < 3) { _polyhedronFaceMeshes.push_back(std::move(mesh)); continue; }

        glm::vec3 v0 = polyhedronData.vertices[face[0]];
        glm::vec3 normal = PolyhedronData::computeNewellNormal(polyhedronData.vertices, face);
        glm::vec3 tangent = glm::normalize(glm::cross(fabs(normal.y) < 0.99f ? glm::vec3(0,1,0) : glm::vec3(1,0,0), normal));
        glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

        float minU = 1e9f, maxU = -1e9f, minV = 1e9f, maxV = -1e9f;
        std::vector<glm::vec2> projected;
        projected.reserve(face.size());
        for (int idx : face) {
            const glm::vec3& v = polyhedronData.vertices[idx];
            float u = glm::dot(v - v0, tangent);
            float vv = glm::dot(v - v0, bitangent);
            projected.emplace_back(u, vv);
            minU = std::min(minU, u); maxU = std::max(maxU, u);
            minV = std::min(minV, vv); maxV = std::max(maxV, vv);
        }

        glm::vec3 centroid(0.0f);
        for (int idx : face) centroid += polyhedronData.vertices[idx];
        centroid /= static_cast<float>(face.size());
        float du = std::max(1e-6f, maxU - minU);
        float dv = std::max(1e-6f, maxV - minV);
        float cU = (glm::dot(centroid - v0, tangent) - minU) / du;
        float cV = (glm::dot(centroid - v0, bitangent) - minV) / dv;

        for (size_t i = 0; i < face.size(); ++i) {
            size_t i0 = i;
            size_t i1 = (i + 1) % face.size();
            int vi0 = face[i0];
            int vi1 = face[i1];
            if (vi0 < 0 || vi0 >= static_cast<int>(polyhedronData.vertices.size())) continue;
            if (vi1 < 0 || vi1 >= static_cast<int>(polyhedronData.vertices.size())) continue;
            const glm::vec3& p0 = polyhedronData.vertices[vi0];
            const glm::vec3& p1 = polyhedronData.vertices[vi1];
            float u0 = (projected[i0].x - minU) / du; float v0uv = (projected[i0].y - minV) / dv;
            float u1 = (projected[i1].x - minU) / du; float v1uv = (projected[i1].y - minV) / dv;

            mesh.tris.push_back(vtx(centroid, normal, {cU, cV}));
            mesh.tris.push_back(vtx(p0, normal, {u0, v0uv}));
            mesh.tris.push_back(vtx(p1, normal, {u1, v1uv}));
        }
        _polyhedronFaceMeshes.push_back(std::move(mesh));
    }
}

void Object::drawPolyhedron() const {
    if (_polyhedronDirty) rebuildPolyhedronMeshes();
    // Each face is a separate mesh so it binds its own painted texture, exactly
    // as the immediate-mode version did — now through the renderer boundary.
    for (size_t f = 0; f < _polyhedronFaceMeshes.size(); ++f)
        currentRenderer().drawMesh(_polyhedronFaceMeshes[f],
                                   resolveRenderMaterial(_materialId, faceAlbedo(f)));
}

void Object::draw2DObject(uint32_t screenW, uint32_t screenH) const {
    // Shape2D is a screen-space axis-aligned rectangle. It is drawn AFTER the
    // 3D pass, in a begin2D / end2D bracket, so depth testing is off and the
    // ortho is already (0,0) top-left to (screenW,screenH) bottom-right.
    //
    // Color comes from faceColors[0], matching the 3D convention so a law
    // that sets "color" paints both 3D and 2D objects with the same path.
    // Width/height come from ShapeParams::width2D / height2D, already
    // registered as shape.width2D and shape.height2D.
    (void)screenW; (void)screenH;
    const float x0 = _x2D;
    const float y0 = _y2D;
    const float x1 = x0 + _shapeParams.width2D;
    const float y1 = y0 + _shapeParams.height2D;
    const glm::vec4 color(faceColors[0][0], faceColors[0][1], faceColors[0][2], 1.0f);

    // Two triangles covering the rect (CCW, top-left origin matches ortho).
    const std::vector<glm::vec2> tris = {
        {x0, y0}, {x1, y0}, {x1, y1},
        {x0, y0}, {x1, y1}, {x0, y1},
    };
    currentRenderer().drawTris2D(tris, color);

    // Border: a 1px outline (darkened fill color for contrast).
    const glm::vec4 borderColor(color.r * 0.6f, color.g * 0.6f, color.b * 0.6f, 1.0f);
    const std::vector<glm::vec2> border = {
        {x0, y0}, {x1, y0},
        {x1, y0}, {x1, y1},
        {x1, y1}, {x0, y1},
        {x0, y1}, {x0, y0},
    };
    currentRenderer().drawLines2D(border, borderColor, 1.0f);
}
