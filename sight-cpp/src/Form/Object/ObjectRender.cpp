// Object — rendering subsystem (split from Object.cpp).
// drawCube / smooth / complex / field / drawObject / highlight outline / drawPolyhedron + GL helpers.

#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include "Rendering/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------
// Modified drawCube to bind per-face texture
// ---------------------------------------------------------------------

void Object::drawCube() const {
    static const struct { GLfloat nx, ny, nz; GLfloat vx[4][3]; } faceData[6] = {
        { 1,0,0,  { {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {0.5,0.5,0.5}, {0.5,-0.5,0.5} } }, // +X
        {-1,0,0,  { {-0.5,-0.5,-0.5}, {-0.5,-0.5,0.5}, {-0.5,0.5,0.5}, {-0.5,0.5,-0.5} } }, // -X
        { 0,1,0,  { {-0.5,0.5,-0.5}, {-0.5,0.5,0.5}, {0.5,0.5,0.5}, {0.5,0.5,-0.5} } }, // +Y
        { 0,-1,0, { {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,-0.5,0.5}, {-0.5,-0.5,0.5} } }, // -Y
        { 0,0,1,  { {-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5} } }, // +Z
        { 0,0,-1, { {-0.5,-0.5,-0.5}, {-0.5,0.5,-0.5}, {0.5,0.5,-0.5}, {0.5,-0.5,-0.5} } }  // -Z
    };

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f,1.0f,1.0f);
    for (int f = 0; f < 6 && f < static_cast<int>(faceTextures.size()); ++f) {
        const FaceTexture& tex = faceTextures[f];
        glBindTexture(GL_TEXTURE_2D, tex.id);
    glBegin(GL_QUADS);
        glNormal3f(faceData[f].nx, faceData[f].ny, faceData[f].nz);
        glTexCoord2f(0,0); glVertex3fv(faceData[f].vx[0]);
        glTexCoord2f(1,0); glVertex3fv(faceData[f].vx[1]);
        glTexCoord2f(1,1); glVertex3fv(faceData[f].vx[2]);
        glTexCoord2f(0,1); glVertex3fv(faceData[f].vx[3]);
    glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}

// Helper to draw a smooth shaded sphere
static void drawSpherePrimitive() {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);
    gluSphere(quad, 0.5f, 16, 16);
    gluDeleteQuadric(quad);
}

// Helper to draw a cylinder primitive of height 1 (centered at origin)
static void drawCylinderPrimitive(float topRadius) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);
    gluCylinder(quad, 0.5f, topRadius, 1.0f, 16, 4);
    gluDeleteQuadric(quad);
}


// Render a triangle-soup tessellation in immediate mode (legacy GL path).
static void drawTessMesh(const geom::TessMesh& m) {
    if (m.tris.empty()) return;
    // Client-side vertex arrays: one draw call instead of ~3 GL calls per vertex.
    // TessVertex is interleaved {pos(3), normal(3), uv(2)}, so the strided
    // pointers all walk the same buffer. This is the difference between a handful
    // of GL calls and hundreds of thousands per frame for marching-tet / patch
    // meshes — the cause of the multi-object lag.
    const geom::TessVertex* base = m.tris.data();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(geom::TessVertex), &base->pos);
    glNormalPointer(GL_FLOAT, sizeof(geom::TessVertex), &base->normal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(geom::TessVertex), &base->uv);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m.tris.size()));
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Object::drawSmoothModel() const {
    glEnable(GL_TEXTURE_2D);
    if (!faceTextures.empty()) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTessMesh(geom::tessellateSmooth(smoothData));
    glDisable(GL_TEXTURE_2D);
}

void Object::drawComplexModel() const {
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    // Each patch is a real face — draw it with its own face texture so the
    // round side and the flat caps can be painted independently.
    for (int i = 0; i < complexData.patchCount(); ++i) {
        if (i < static_cast<int>(faceTextures.size())) {
            glBindTexture(GL_TEXTURE_2D, faceTextures[i].id);
        }
        drawTessMesh(geom::tessellatePatch(complexData.patches[i]));
    }
    glDisable(GL_TEXTURE_2D);
}

void Object::drawFieldModel() const {
    glEnable(GL_TEXTURE_2D);
    if (!faceTextures.empty()) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTessMesh(_fieldMesh); // cached: SDF tessellation is expensive, rebuilt on change
    glDisable(GL_TEXTURE_2D);
}

void Object::drawObject() const {
    if (_hasField)   { drawFieldModel();   return; }
    if (_hasComplex) { drawComplexModel(); return; }
    if (_hasSmooth)  { drawSmoothModel();  return; }
    switch (geometryType) {
        case GeometryType::Cube:
            drawCube();
            break;
        case GeometryType::Sphere:
        {
            glEnable(GL_TEXTURE_2D);
            if (!faceTextures.empty()) {
                glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
            }
            glColor3f(1.0f, 1.0f, 1.0f);
            drawSpherePrimitive();
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case GeometryType::Cylinder:
        {
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
            // Center cylinder along Z in [-0.5, 0.5]
            glTranslatef(0.0f, 0.0f, -0.5f);

            // Draw side
            if (faceTextures.size() >= 1) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
            drawCylinderPrimitive(0.5f);

            // Draw caps with second face texture if available
            GLUquadric* disk = gluNewQuadric();
            gluQuadricTexture(disk, GL_TRUE);
            if (faceTextures.size() >= 2) glBindTexture(GL_TEXTURE_2D, faceTextures[1].id);
            // Bottom cap at z = 0 (world z = -0.5) - outward normal should be -Z
            glPushMatrix();
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f); // flip to face -Z
            gluDisk(disk, 0.0f, 0.5f, 32, 1);
            glPopMatrix();
            // Top cap at z = 1 (world z = +0.5) - outward normal +Z
            glPushMatrix();
            glTranslatef(0.0f, 0.0f, 1.0f);
            gluDisk(disk, 0.0f, 0.5f, 32, 1);
            glPopMatrix();
            gluDeleteQuadric(disk);

            glPopMatrix();
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case GeometryType::Cone:
        {
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
            // Center cone along Z in [-0.5, 0.5] (base at -0.5, apex at +0.5)
            glTranslatef(0.0f, 0.0f, -0.5f);

            // Draw side
            if (faceTextures.size() >= 1) glBindTexture(GL_TEXTURE_2D, faceTextures[0].id);
            drawCylinderPrimitive(0.0f); // top radius 0 = cone

            // Draw base disk with second face texture if available
            if (faceTextures.size() >= 2) glBindTexture(GL_TEXTURE_2D, faceTextures[1].id);
            GLUquadric* disk = gluNewQuadric();
            gluQuadricTexture(disk, GL_TRUE);
            glPushMatrix();
            // Base is at local z=0 (world z = -0.5). Outward normal should be -Z → flip the disk.
            // This fixes the cap appearing on the wrong side.
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            gluDisk(disk, 0.0f, 0.5f, 32, 1);
            glPopMatrix();
            gluDeleteQuadric(disk);

            glPopMatrix();
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case GeometryType::Polyhedron:
            drawPolyhedron();
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

    // Draw 3-4 inflated shells of collision AABB as wireframes for a soft glow effect
    // Tailored to the object shape via its collisionZone corners (AABB). For more complex shapes, this can be extended.
    // IMPORTANT: collisionZone.corners are stored in world space, but this function is typically called
    // after the object's model transform has already been applied via glMultMatrixf(...).
    // If we draw world-space corners here, they get transformed again (double-transform), causing the outline
    // to appear offset from the object. Convert to local space first.
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);

    // Trace the ACTUAL shape (local space — the model transform is already applied
    // by the caller), not an AABB. Flat-faced shapes get a crisp edge wireframe;
    // curved / field shapes get a translucent additive glow shell hugging them.
    auto drawWireframe = [&](const std::vector<std::pair<glm::vec3, glm::vec3>>& edges) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (int p = 0; p < 2; ++p) {
            glColor4f(color.r, color.g, color.b, 0.65f - 0.3f * p);
            glLineWidth(2.0f + 2.5f * p);
            glBegin(GL_LINES);
            for (const auto& e : edges) {
                glVertex3f(e.first.x, e.first.y, e.first.z);
                glVertex3f(e.second.x, e.second.y, e.second.z);
            }
            glEnd();
        }
    };
    auto drawShell = [&](const geom::TessMesh& m) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
        glDepthMask(GL_FALSE);
        for (int p = 0; p < 2; ++p) {
            float s = 1.0f + 0.02f * (p + 1);
            glColor4f(color.r, color.g, color.b, 0.16f - 0.06f * p);
            glBegin(GL_TRIANGLES);
            for (const auto& v : m.tris) { glm::vec3 q = v.pos * s; glVertex3f(q.x, q.y, q.z); }
            glEnd();
        }
        glDepthMask(GL_TRUE);
    };

    if (geometryType == GeometryType::Polyhedron && !polyhedronData.vertices.empty()) {
        std::vector<std::pair<glm::vec3, glm::vec3>> edges;
        for (const auto& face : polyhedronData.faces)
            for (size_t k = 0; k < face.size(); ++k)
                edges.emplace_back(polyhedronData.vertices[face[k]],
                                   polyhedronData.vertices[face[(k + 1) % face.size()]]);
        drawWireframe(edges);
    } else if (_hasField) {
        drawShell(_fieldMesh);
    } else if (_hasSmooth) {
        drawShell(geom::tessellateSmooth(smoothData, 20, 12));
    } else if (_hasComplex) {
        drawShell(geom::tessellateComplex(complexData, 16));
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
        drawWireframe(edges);
    }
    glPopAttrib();
}

void Object::drawPolyhedron() const {
    if (polyhedronData.vertices.empty() || polyhedronData.faces.empty()) {
        return; // No polyhedron data to draw
    }
    
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Draw each face of the polyhedron
    for (size_t faceIndex = 0; faceIndex < polyhedronData.faces.size(); ++faceIndex) {
        const auto& face = polyhedronData.faces[faceIndex];
        if (face.size() < 3) continue; // Skip invalid faces
        
        // Bind texture for this face if available
        if (faceIndex < faceTextures.size()) {
            const FaceTexture& tex = faceTextures[faceIndex];
            glBindTexture(GL_TEXTURE_2D, tex.id);
        }
        
        // Compute per-face tangent space and UVs consistent with raycast mapping, and use Newell normal
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

        // Triangulate face with a fan around centroid to avoid GL_POLYGON issues
        glm::vec3 centroid(0.0f);
        for (int idx : face) centroid += polyhedronData.vertices[idx];
        centroid /= static_cast<float>(face.size());
        float du = std::max(1e-6f, maxU - minU);
        float dv = std::max(1e-6f, maxV - minV);
        float cU = (glm::dot(centroid - v0, tangent) - minU) / du;
        float cV = (glm::dot(centroid - v0, bitangent) - minV) / dv;

        glBegin(GL_TRIANGLES);
        glNormal3f(normal.x, normal.y, normal.z);
        for (size_t i = 0; i < face.size(); ++i) {
            size_t i0 = i;
            size_t i1 = (i + 1) % face.size();
            int vi0 = face[i0];
            int vi1 = face[i1];
            if (vi0 < 0 || vi0 >= static_cast<int>(polyhedronData.vertices.size())) continue;
            if (vi1 < 0 || vi1 >= static_cast<int>(polyhedronData.vertices.size())) continue;
            const glm::vec3& p0 = polyhedronData.vertices[vi0];
            const glm::vec3& p1 = polyhedronData.vertices[vi1];

            glm::vec2 proj0 = projected[i0];
            glm::vec2 proj1 = projected[i1];
            float u0 = (proj0.x - minU) / du; float v0uv = (proj0.y - minV) / dv;
            float u1 = (proj1.x - minU) / du; float v1uv = (proj1.y - minV) / dv;

            glTexCoord2f(cU, cV); glVertex3f(centroid.x, centroid.y, centroid.z);
            glTexCoord2f(u0, v0uv); glVertex3f(p0.x, p0.y, p0.z);
            glTexCoord2f(u1, v1uv); glVertex3f(p1.x, p1.y, p1.z);
        }
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}
