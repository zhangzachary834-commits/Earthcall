#include "Rendering/GL/OpenGLRenderer.hpp"

#include "Form/Object/Geometry/Sdf.hpp" // geom::SdfNode, tessellateSdf

#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>

namespace {
// The view matrix from the last setCamera. GL has no separate model matrix — the
// MODELVIEW stack holds view*model — so setModel has to recombine them.
glm::mat4 g_view{1.0f};

void applyBlend(Blend b) {
    switch (b) {
        case Blend::Opaque:   glDisable(GL_BLEND); break;
        case Blend::Alpha:    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
        case Blend::Additive: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
    }
}
} // namespace

void OpenGLRenderer::applyCamera(const glm::mat4& view, const glm::mat4& proj,
                                 const glm::vec3& /*eyePos*/) {
    // eyePos is unused here: fixed-function GL derives the view vector for specular
    // from the MODELVIEW itself. It exists on the interface for WGSL, which cannot.
    g_view = view;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&proj[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&view[0][0]);
}

void OpenGLRenderer::applyModel(const glm::mat4& model) {
    const glm::mat4 mv = g_view * model;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&mv[0][0]);
}

void OpenGLRenderer::drawMesh(const geom::TessMesh& mesh, const RenderMaterial& mat) {
    if (mesh.tris.empty()) return;

    // Appearance. baseColor drives ambient+diffuse (GL_COLOR_MATERIAL, set once in
    // ShadingSystem); shininess sizes the highlight. This replaces the hardcoded
    // glColor3f(1,1,1) the draw paths used — default baseColor is that same white,
    // so a default-material object looks identical. (specular/ambient/diffuse are
    // carried on `mat` for the WGSL shader; see the header.)
    glColor4f(mat.baseColor.r, mat.baseColor.g, mat.baseColor.b, mat.opacity);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat.shininess);

    // Open surfaces (patches): light the back face too. Scoped so it doesn't leak.
    if (mat.doubleSided) {
        glPushAttrib(GL_LIGHTING_BIT);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    }

    if (mat.textureId != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, mat.textureId);
    } else {
        glDisable(GL_TEXTURE_2D);
    }

    // Interleaved {pos(3), normal(3), uv(2)} client-side arrays: one draw call for
    // the whole mesh instead of a GL call per vertex.
    const geom::TessVertex* base = mesh.tris.data();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(geom::TessVertex), &base->pos);
    glNormalPointer(GL_FLOAT, sizeof(geom::TessVertex), &base->normal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(geom::TessVertex), &base->uv);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.tris.size()));
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (mat.textureId != 0) glDisable(GL_TEXTURE_2D);
    if (mat.doubleSided) glPopAttrib(); // restore one-sided light model
}

void OpenGLRenderer::drawImplicit(const geom::SdfNode& field, float extent,
                                  const RenderMaterial& material,
                                  const geom::FieldNode* fieldNode) {
    // Fixed-function GL has no raymarcher, so a field is drawn by tessellating it
    // and reusing drawMesh. Callers that already cache a field mesh (Object holds
    // _fieldMesh) should call drawMesh directly; this exists so the interface is
    // honest for callers that only have the SDF. WebGPU (M6) will raymarch here.
    drawMesh(geom::tessellateSdf(field, extent), material);
}

void OpenGLRenderer::drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& segments,
                               const glm::vec4& color, float width, Blend blend) {
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    applyBlend(blend);
    glColor4f(color.r, color.g, color.b, color.a);
    glLineWidth(width);
    glBegin(GL_LINES);
    for (const auto& e : segments) {
        glVertex3f(e.first.x, e.first.y, e.first.z);
        glVertex3f(e.second.x, e.second.y, e.second.z);
    }
    glEnd();
    glPopAttrib();
}

void OpenGLRenderer::drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                                 float scale, bool additive) {
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // glow reads depth but must not occlude itself
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_TRIANGLES);
    for (const auto& v : mesh.tris) { glm::vec3 q = v.pos * scale; glVertex3f(q.x, q.y, q.z); }
    glEnd();
    glPopAttrib(); // restores depth mask
}

// ---------------------------------------------------------------------------
// Flat-colour primitives. These are transcriptions, not reinterpretations: each
// one does exactly what the immediate-mode block at the call site did, including
// the glPushAttrib/glPopAttrib bracket that kept enable state from leaking.
// ---------------------------------------------------------------------------

void OpenGLRenderer::setWireframe(bool on) {
    glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

void OpenGLRenderer::drawSolid(const std::vector<glm::vec3>& tris, const glm::vec4& color,
                               Blend blend, bool depthWrite) {
    if (tris.empty()) return;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    applyBlend(blend);
    glDepthMask(depthWrite ? GL_TRUE : GL_FALSE);

    glColor4f(color.r, color.g, color.b, color.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(glm::vec3), tris.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tris.size()));
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopAttrib();
}

void OpenGLRenderer::begin2D(uint32_t width, uint32_t height) {
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height), 0.0,
            -1.0, 1.0); // (0,0) == top-left
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void OpenGLRenderer::end2D() {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void OpenGLRenderer::drawTris2D(const std::vector<glm::vec2>& tris, const glm::vec4& color) {
    if (tris.empty()) return;
    glColor4f(color.r, color.g, color.b, color.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(glm::vec2), tris.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tris.size()));
    glDisableClientState(GL_VERTEX_ARRAY);
}

void OpenGLRenderer::drawLines2D(const std::vector<glm::vec2>& segments,
                                 const glm::vec4& color, float width) {
    if (segments.empty()) return;
    glColor4f(color.r, color.g, color.b, color.a);
    glLineWidth(width);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(glm::vec2), segments.data());
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(segments.size()));
    glDisableClientState(GL_VERTEX_ARRAY);
    glLineWidth(1.0f);
}

void OpenGLRenderer::drawImage2D(const uint8_t* rgba, uint32_t width, uint32_t height,
                                 const glm::vec4& rect, const glm::vec4& tint) {
    if (!rgba || width == 0 || height == 0) return;

    // One texture name reused for every blit: the callers regenerate their pixels
    // each frame, so there is nothing to cache and allocating per call would leak.
    static GLuint s_blitTex = 0;
    if (s_blitTex == 0) glGenTextures(1, &s_blitTex);

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_blitTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    glColor4f(tint.r, tint.g, tint.b, tint.a);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(rect.x, rect.y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(rect.z, rect.y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(rect.z, rect.w);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(rect.x, rect.w);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();
}

// ---------------------------------------------------------------------------
// Lighting. This is the fixed-function GL_LIGHT0 setup that used to live in
// ShadingSystem — moved here because it is backend policy, not scene policy.
// The one-time state (shade model, colour material, depth test) is installed on
// first use, since there is no separate init hook on the boundary.
// ---------------------------------------------------------------------------
void OpenGLRenderer::applyLight() {
    static bool s_once = false;
    if (!s_once) {
        s_once = true;
        glEnable(GL_DEPTH_TEST);
        glShadeModel(GL_SMOOTH);        // Gouraud shading
        glEnable(GL_COLOR_MATERIAL);    // vertex colour drives ambient+diffuse
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
        glEnable(GL_LIGHT0);
    }

    const glm::vec3& a = lightAmbient();
    const glm::vec3& d = lightDiffuse();
    const glm::vec3& s = lightSpecular();
    const glm::vec3& p = lightPos();
    const GLfloat ambient[]  = {a.r, a.g, a.b, 1.0f};
    const GLfloat diffuse[]  = {d.r, d.g, d.b, 1.0f};
    const GLfloat specular[] = {s.r, s.g, s.b, 1.0f};
    // w=1 => a POSITIONAL light, as it always was. GL transforms this by the
    // MODELVIEW in force right now, so callers must set the camera first.
    const GLfloat position[] = {p.x, p.y, p.z, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

void OpenGLRenderer::applyLightingEnabled(bool on) {
    if (on) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
}

TextureHandle OpenGLRenderer::uploadTexture(TextureHandle handle, const uint8_t* rgba,
                                            uint32_t width, uint32_t height) {
    if (!rgba || width == 0 || height == 0) return handle;

    // Headless callers — tests, the save-migration tool — construct Objects with
    // no GL context at all, and Object construction paints its face textures. A GL
    // call there is a segfault, not a diagnosable error. With no context there is
    // genuinely no texture to make, so report no handle and let the CPU pixels
    // (RenderMaterial::albedoPixels) carry the paint for anyone who wants it.
    if (!glfwGetCurrentContext()) return 0;

    GLuint id = handle;
    if (id == 0) glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    // glGenerateMipmap is GL 3.0+, so it is resolved at runtime; without it the
    // MIN filter has to drop to plain LINEAR or sampling reads undefined levels.
    using GenerateMipmapFunc = void (*)(GLenum);
    auto generateMipmap = reinterpret_cast<GenerateMipmapFunc>(glfwGetProcAddress("glGenerateMipmap"));
    if (generateMipmap) {
        generateMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    return id;
}

void OpenGLRenderer::releaseTexture(TextureHandle handle) {
    if (handle == 0) return;
    GLuint id = handle;
    glDeleteTextures(1, &id);
}

void OpenGLRenderer::applyBeginFrame(uint32_t width, uint32_t height,
                                     const glm::vec4& clearColor) {
    // Viewport + clear used to be done by the caller before any renderer existed.
    // They belong to whoever owns the framebuffer, which is the backend.
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
