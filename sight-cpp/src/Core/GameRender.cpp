// GameRender.cpp – Game::render() method
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Core/Engine.hpp"
#include "Form/Object/Object.hpp"
#include "Rendering/ShadingSystem.hpp"
#include "Rendering/BrushSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#ifdef USE_GL3_RENDERER
#include "Rendering/GL/GL3Renderer.hpp"
#endif

#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

extern ZoneManager mgr;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using glm::vec3;

namespace Core {

void Game::render() {
    if (!_window) return;

#ifdef USE_GL3_RENDERER
    // Initialize the GL3 renderer lazily once we have a window/context
    if (!_gl3Initialized) {
        _gl3Initialized = _gl3Renderer.init(_window, "#version 330 core");
    }
#endif

    // Apply active zone theme colour
    mgr.active().applyTheme();

    int fbW, fbH;
    glfwGetFramebufferSize(_window, &fbW, &fbH);
    if (fbH == 0) fbH = 1;
    float aspect = static_cast<float>(fbW) / fbH;

    glViewport(0, 0, fbW, fbH);

    // Current active zone's 3-D world (accessible throughout render)
    auto& zoneWorld = mgr.active().world();
    zoneWorld.setCamera(&_camera.pos);
    zoneWorld.setPlayerEyeHeight(_player.getBody().getEyeHeight());

    // ------------------------------------------------------------------
    // Projection
    // ------------------------------------------------------------------
    float fov = 45.0f;
    float nearZ = 0.1f;
    float farZ  = 100.0f;
    float top   = tanf(fov * M_PI / 360.0f) * nearZ;
    float bottom = -top;
    float right  = top * aspect;
    float left   = -right;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left, right, bottom, top, nearZ, farZ);

    // ------------------------------------------------------------------
    // Model-view (camera)
    // ------------------------------------------------------------------
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    vec3 eyePos   = _camera.pos;
    vec3 lookDir  = _camera.front;
    const float CAMERA_DISTANCE = 4.0f;

    if (_currentPerspective == PerspectiveMode::ThirdPerson) {
        eyePos  = _camera.pos - _camera.front * CAMERA_DISTANCE;
    } else if (_currentPerspective == PerspectiveMode::SecondPerson) {
        eyePos  = _camera.pos + _camera.front * CAMERA_DISTANCE;
    }

    vec3 lookTarget = _camera.pos + lookDir;
    gluLookAt(eyePos.x, eyePos.y, eyePos.z,
              lookTarget.x, lookTarget.y, lookTarget.z,
              _camera.up.x, _camera.up.y, _camera.up.z);

    // Update lighting position to follow camera
    ShadingSystem::update(_camera.pos);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --------------------------------------------------------------
    // Update transforms for demo cube + ground (only if tags still indicate baseline)
    // --------------------------------------------------------------
    if (!zoneWorld.getOwnedObjects().empty()) {
        auto& owned = zoneWorld.getOwnedObjectsMutable();
        if (!owned.empty() && owned[0] && owned[0]->hasAttribute("baseline") && owned[0]->getAttribute("baseline") == std::string("cube")) {
            glm::mat4 cubeTransform = glm::rotate(glm::mat4(1.0f), glm::radians(_cubeAngle), glm::vec3(0.5f, 1.0f, 0.0f));
            owned[0]->setTransform(cubeTransform);
        }
        if (owned.size() > 1 && owned[1] && owned[1]->hasAttribute("baseline") && owned[1]->getAttribute("baseline") == std::string("ground")) {
            glm::mat4 groundTransform = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 1.0f, 100.0f));
            owned[1]->setTransform(groundTransform);
        }
    }

    // --------------------------------------------------------------
    // Draw all owned objects except index 1 (ground placeholder)
    // --------------------------------------------------------------
    const auto& objects = zoneWorld.getOwnedObjects();
    for (size_t i = 0; i < objects.size(); ++i) {
        if (i == 1) continue; // skip ground placeholder
        glPushMatrix();
        glMultMatrixf(&objects[i]->getTransform()[0][0]);
        objects[i]->drawObject();
        objects[i]->drawHighlightOutline();
        glPopMatrix();
    }

    // Morph tool: draw draggable vertex handles over the selected polyhedron.
    if (_current3DMode == Mode3D::Morph && _selectedObject3D &&
        _selectedObject3D->getGeometryType() == Object::GeometryType::Polyhedron) {
        Object* o = _selectedObject3D;
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        for (int v = 0; v < o->getPolyhedronVertexCount(); ++v) {
            glm::vec3 w = glm::vec3(o->getTransform() * glm::vec4(o->getPolyhedronVertexLocal(v), 1.0f));
            bool sel = (v == _morphVertexIndex);
            float s = sel ? 0.06f : 0.04f;
            if (sel) glColor3f(1.0f, 0.85f, 0.2f); else glColor3f(0.2f, 0.8f, 1.0f);
            glPushMatrix();
            glTranslatef(w.x, w.y, w.z);
            glScalef(s, s, s);
            // small cube handle
            glBegin(GL_QUADS);
            const float h = 0.5f;
            glVertex3f(-h,-h, h); glVertex3f( h,-h, h); glVertex3f( h, h, h); glVertex3f(-h, h, h);
            glVertex3f(-h,-h,-h); glVertex3f(-h, h,-h); glVertex3f( h, h,-h); glVertex3f( h,-h,-h);
            glVertex3f(-h, h,-h); glVertex3f(-h, h, h); glVertex3f( h, h, h); glVertex3f( h, h,-h);
            glVertex3f(-h,-h,-h); glVertex3f( h,-h,-h); glVertex3f( h,-h, h); glVertex3f(-h,-h, h);
            glVertex3f( h,-h,-h); glVertex3f( h, h,-h); glVertex3f( h, h, h); glVertex3f( h,-h, h);
            glVertex3f(-h,-h,-h); glVertex3f(-h,-h, h); glVertex3f(-h, h, h); glVertex3f(-h, h,-h);
            glEnd();
            glPopMatrix();
        }
        glEnable(GL_LIGHTING);
    }

    // Morph tool on a blend/boolean field: ghost of operand B + its drag handle.
    if (_current3DMode == Mode3D::Morph && _selectedObject3D &&
        _selectedObject3D->isBinaryField()) {
        Object* o = _selectedObject3D;
        const glm::mat4& xf = o->getTransform();
        const geom::SdfNode& f = o->getFieldData();
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);

        // Translucent ghost of operand B (rendered in the field's local space).
        if (f.children.size() == 2) {
            geom::TessMesh ghost = geom::tessellateSdf(f.children[1], o->getFieldExtent(), 16);
            glPushMatrix();
            glMultMatrixf(&xf[0][0]);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
            glDepthMask(GL_FALSE);
            glColor4f(0.35f, 0.85f, 1.0f, 0.16f);
            glBegin(GL_TRIANGLES);
            for (const auto& v : ghost.tris) glVertex3f(v.pos.x, v.pos.y, v.pos.z);
            glEnd();
            glDepthMask(GL_TRUE);
            glPopMatrix();
        }

        // The draggable handle at operand B's offset (gold cube, world space).
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glm::vec3 hw = glm::vec3(xf * glm::vec4(o->getFieldOperandBOffset(), 1.0f));
        glColor3f(1.0f, 0.85f, 0.2f);
        glPushMatrix();
        glTranslatef(hw.x, hw.y, hw.z);
        glScalef(0.06f, 0.06f, 0.06f);
        glBegin(GL_QUADS);
        const float h = 0.5f;
        glVertex3f(-h,-h, h); glVertex3f( h,-h, h); glVertex3f( h, h, h); glVertex3f(-h, h, h);
        glVertex3f(-h,-h,-h); glVertex3f(-h, h,-h); glVertex3f( h, h,-h); glVertex3f( h,-h,-h);
        glVertex3f(-h, h,-h); glVertex3f(-h, h, h); glVertex3f( h, h, h); glVertex3f( h, h,-h);
        glVertex3f(-h,-h,-h); glVertex3f( h,-h,-h); glVertex3f( h,-h, h); glVertex3f(-h,-h, h);
        glVertex3f( h,-h,-h); glVertex3f( h, h,-h); glVertex3f( h, h, h); glVertex3f( h,-h, h);
        glVertex3f(-h,-h,-h); glVertex3f(-h,-h, h); glVertex3f(-h, h, h); glVertex3f(-h, h,-h);
        glEnd();
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    // Morph tool on a Bezier patch: control-net wireframe + draggable control points.
    if (_current3DMode == Mode3D::Morph && _selectedObject3D && _selectedObject3D->isPatch()) {
        Object* o = _selectedObject3D;
        const glm::mat4& xf = o->getTransform();
        const geom::BezierPatch& p = o->getPatchData();
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto cp = [&](int i, int j) {
            return glm::vec3(xf * glm::vec4(p.at(i, j), 1.0f));
        };
        // Control net (lines between adjacent control points).
        glColor4f(0.3f, 0.8f, 1.0f, 0.6f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        for (int j = 0; j < p.nv(); ++j)
            for (int i = 0; i < p.nu(); ++i) {
                glm::vec3 a = cp(i, j);
                if (i + 1 < p.nu()) { glm::vec3 b = cp(i + 1, j); glVertex3f(a.x,a.y,a.z); glVertex3f(b.x,b.y,b.z); }
                if (j + 1 < p.nv()) { glm::vec3 b = cp(i, j + 1); glVertex3f(a.x,a.y,a.z); glVertex3f(b.x,b.y,b.z); }
            }
        glEnd();

        // Control-point handles.
        for (int idx = 0; idx < o->getPatchControlCount(); ++idx) {
            glm::vec3 w = glm::vec3(xf * glm::vec4(o->getPatchControlLocal(idx), 1.0f));
            bool sel = (idx == _patchCtrlIndex);
            float s = sel ? 0.05f : 0.035f;
            if (sel) glColor3f(1.0f, 0.85f, 0.2f); else glColor3f(0.2f, 0.8f, 1.0f);
            glPushMatrix();
            glTranslatef(w.x, w.y, w.z);
            glScalef(s, s, s);
            glBegin(GL_QUADS);
            const float h = 0.5f;
            glVertex3f(-h,-h, h); glVertex3f( h,-h, h); glVertex3f( h, h, h); glVertex3f(-h, h, h);
            glVertex3f(-h,-h,-h); glVertex3f(-h, h,-h); glVertex3f( h, h,-h); glVertex3f( h,-h,-h);
            glVertex3f(-h, h,-h); glVertex3f(-h, h, h); glVertex3f( h, h, h); glVertex3f( h, h,-h);
            glVertex3f(-h,-h,-h); glVertex3f( h,-h,-h); glVertex3f( h,-h, h); glVertex3f(-h,-h, h);
            glVertex3f( h,-h,-h); glVertex3f( h, h,-h); glVertex3f( h, h, h); glVertex3f( h,-h, h);
            glVertex3f(-h,-h,-h); glVertex3f(-h,-h, h); glVertex3f(-h, h, h); glVertex3f(-h, h,-h);
            glEnd();
            glPopMatrix();
        }
        glEnable(GL_LIGHTING);
    }

    // Gravity field visualization (holographic arrows)
    if (Physics::getGravityVisualization()) {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glLineWidth(1.5f);

        // Build a small sample grid around the camera
        int N = Physics::getGravityVisualizationDensity();
        float span = 6.0f; // world units across the grid
        glm::vec3 center = _camera.pos + _camera.front * 4.0f;
        for (int xi = 0; xi < N; ++xi) {
            for (int yi = 0; yi < N; ++yi) {
                for (int zi = 0; zi < N; ++zi) {
                    float fx = (xi / (float)(N - 1)) - 0.5f;
                    float fy = (yi / (float)(N - 1)) - 0.5f;
                    float fz = (zi / (float)(N - 1)) - 0.5f;
                    glm::vec3 p = center + glm::vec3(fx, fy, fz) * span;
                    float G, eps; Physics::getGravityConstants(G, eps);
                    glm::vec3 a = Physics::sampleGravityField(p, mgr.active().world().getOwnedObjects(), G, eps);
                    float mag = glm::length(a);
                    if (mag < 1e-6f) continue;
                    glm::vec3 dir = a / mag;
                    float len = std::min(0.5f, 0.2f + 0.3f * logf(1.0f + mag));
                    glm::vec3 q = p + dir * len;
                    // Color by magnitude (teal to purple)
                    float t = glm::clamp(mag / 5.0f, 0.0f, 1.0f);
                    glm::vec3 col = glm::mix(glm::vec3(0.2f, 1.0f, 0.9f), glm::vec3(0.8f, 0.2f, 1.0f), t);
                    glColor4f(col.r, col.g, col.b, 0.5f);
                    glBegin(GL_LINES);
                    glVertex3f(p.x, p.y, p.z);
                    glVertex3f(q.x, q.y, q.z);
                    glEnd();
                }
            }
        }
        glPopAttrib();
    }

    // ------------------------------------------------------------------
    // Live preview ("hologram") for BrushCreate mode
    // ------------------------------------------------------------------
    if (_current3DMode == Mode3D::BrushCreate) {
        glm::vec3 previewPos;
        if (_placement.mode == BrushPlacementMode::InFront) {
            previewPos = _camera.pos + _camera.front * 2.0f;
        } else if (_placement.mode == BrushPlacementMode::ManualDistance) {
            if(!_placement.anchorValid){
                _placement.anchorPos      = _camera.pos + _camera.front * 2.0f;
                _placement.anchorRight    = glm::normalize(glm::cross(_camera.front, _camera.up));
                _placement.anchorUp       = _camera.up;
                _placement.anchorForward  = _camera.front;
                _placement.anchorValid    = true;
            }
            previewPos = _placement.anchorPos + _placement.anchorRight * _placement.manualOffset.x + _placement.anchorUp * _placement.manualOffset.y + _placement.anchorForward * _placement.manualOffset.z;
        } else {
            // CursorSnap – approximate using same raycast as spawn (without altering state)
            double mx,my; glfwGetCursorPos(_window,&mx,&my);
            int winW,winH; glfwGetWindowSize(_window,&winW,&winH);
            int fW,fH; glfwGetFramebufferSize(_window,&fW,&fH);
            float sx = static_cast<float>(fW)/winW;
            float sy = static_cast<float>(fH)/winH;
            double winX = mx*sx; double winY = my*sy;
            winY = _camera.viewport[3] - winY;
            GLdouble nx,ny,nz,fx,fy,fz;
            gluUnProject(winX,winY,0.0,_camera.modelview,_camera.projection,_camera.viewport,&nx,&ny,&nz);
            gluUnProject(winX,winY,1.0,_camera.modelview,_camera.projection,_camera.viewport,&fx,&fy,&fz);
            glm::vec3 rayO(nx,ny,nz);
            glm::vec3 rayDir = glm::normalize(glm::vec3(fx,fy,fz)-rayO);
            float nearestT=1e9f; int hitAxis=-1; int hitSign=1; Object* hitObj=nullptr;
            bool hitIsCube=false;
            const auto& objs=zoneWorld.getOwnedObjects();
            for(const auto& up:objs){
                Object* obj=up.get();
                if(obj->getGeometryType()==Object::GeometryType::Cube){
                    glm::mat4 inv=glm::inverse(obj->getTransform());
                    glm::vec3 oL=glm::vec3(inv*glm::vec4(rayO,1.0f));
                    glm::vec3 dL=glm::normalize(glm::vec3(inv*glm::vec4(rayDir,0.0f)));
                    float tMin=-1e9f,tMax=1e9f; int axis=-1; int sign=1;
                    for(int a=0;a<3;++a){float o=oL[a],d=dL[a];float t1,t2;if(fabs(d)<1e-6f){if(o<-0.5f||o>0.5f){tMin=1e9f;break;}t1=-1e9f;t2=1e9f;}else{t1=(-0.5f-o)/d; t2=(0.5f-o)/d;} if(t1>t2) std::swap(t1,t2); if(t1>tMin){tMin=t1; axis=a; sign=(d>0?-1:1);} if(t2<tMax) tMax=t2; if(tMin>tMax){tMin=1e9f;break;}}
                    if(tMin<nearestT && tMin>0 && tMin<1e8f){nearestT=tMin; hitAxis=axis; hitSign=sign; hitObj=obj; hitIsCube=true;}
                }else{
                    glm::vec3 centerWorld=glm::vec3(obj->getTransform()*glm::vec4(0.0f,0.0f,0.0f,1.0f));
                    glm::vec3 colX=glm::vec3(obj->getTransform()[0]);
                    glm::vec3 colY=glm::vec3(obj->getTransform()[1]);
                    glm::vec3 colZ=glm::vec3(obj->getTransform()[2]);
                    float scaleX=glm::length(colX);
                    float scaleY=glm::length(colY);
                    float scaleZ=glm::length(colZ);
                    float radius=0.5f*std::max(scaleX,std::max(scaleY,scaleZ));
                    glm::vec3 oc=rayO-centerWorld;
                    float b=glm::dot(oc,rayDir);
                    float c=glm::dot(oc,oc)-radius*radius;
                    float h=b*b-c;
                    if(h>=0.0f){h=std::sqrt(h); float t=-b-h; if(t<0.0f) t=-b+h; if(t>0.0f && t<nearestT){nearestT=t; hitObj=obj; hitIsCube=false;}}
                }
            }
            if(nearestT<1e8f && hitObj){
                glm::vec3 hitPoint=rayO + rayDir*nearestT;
                glm::vec3 nWorld;
                if(hitIsCube){glm::vec3 nLocal(0.0f); nLocal[hitAxis]=static_cast<float>(hitSign); nWorld=glm::normalize(glm::vec3(hitObj->getTransform()*glm::vec4(nLocal,0.0f)));}
                else{glm::vec3 centerWorld=glm::vec3(hitObj->getTransform()*glm::vec4(0.0f,0.0f,0.0f,1.0f)); nWorld=glm::normalize(hitPoint-centerWorld);}
                glm::vec3 half=glm::vec3(_brush.scale.x*_brush.size,_brush.scale.y*_brush.size,_brush.scale.z*_brush.size)*0.5f;
                float offAmt=glm::dot(glm::abs(nWorld),half)+0.01f;
                previewPos = hitPoint + nWorld*offAmt;
            } else previewPos = _camera.pos + _camera.front * 2.0f;
        }

        // Apply optional grid-snap just like the actual spawn logic
        if (_brush.gridSnap && _brush.gridSize > 1e-6f) {
            previewPos.x = std::round(previewPos.x / _brush.gridSize) * _brush.gridSize;
            previewPos.y = std::round(previewPos.y / _brush.gridSize) * _brush.gridSize;
            previewPos.z = std::round(previewPos.z / _brush.gridSize) * _brush.gridSize;
        }

        // Build transform: translate -> scale
        glm::mat4 previewT = glm::translate(glm::mat4(1.0f), previewPos);
        glm::vec3 totalScale = glm::vec3(_brush.scale.x * _brush.size,
                                         _brush.scale.y * _brush.size,
                                         _brush.scale.z * _brush.size);
        previewT = glm::scale(previewT, totalScale);

        // Render as translucent wireframe so it does not occlude view
        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

        glPushMatrix();
        glMultMatrixf(&previewT[0][0]);
        // Draw primitive outline using the selected shape
        Object temp;
        temp.setShape(_polyhedron.shapeKind, _polyhedron.shapeParams);

        // Initialize polyhedron data for preview using the unified builder
        if (_polyhedron.shapeKind == Object::ShapeKind::Polyhedron) {
            temp.setPolyhedronData(buildCurrentPolyhedron());
        }

        temp.drawObject();
        temp.drawHighlightOutline();
        glPopMatrix();

        glPopAttrib();
    }

    // Draw player avatar and nametag when not in first-person
    if (_currentPerspective != PerspectiveMode::FirstPerson) {
        _player.draw();
        _player.drawNametag();
    }

    // Draw demo avatars if enabled
    if (_showAvatarDemo) {
        for (auto* avatar : _avatarManager.getAllAvatars()) {
            avatar->draw();
            avatar->drawNametag();
        }
    }

    // 2-D overlays ---------------------------------------------------------
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, fbW, fbH, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    mgr.active().renderArt();

    if (_drawingStraightLine && _current3DMode == Mode3D::None && _currentTool.getType() == Tool::Type::Brush) {
        auto* brushSystem = mgr.active().getBrushSystem();
        const float previewOpacity = brushSystem ? std::clamp(brushSystem->getOpacity(), 0.25f, 0.95f) : 0.85f;
        const float previewWidth = brushSystem ? std::max(1.0f, brushSystem->getRadius() * 1000.0f) : 2.0f;

        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(previewWidth);
        glColor4f(_currentColor[0], _currentColor[1], _currentColor[2], previewOpacity);
        glBegin(GL_LINES);
        glVertex2f(_straightLineStartX, _straightLineStartY);
        glVertex2f(_straightLineEndX, _straightLineEndY);
        glEnd();

        glLineWidth(1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.65f);
        glBegin(GL_LINE_LOOP);
        constexpr int handleSegments = 16;
        constexpr float handleRadius = 4.0f;
        for (int i = 0; i < handleSegments; ++i) {
            const float angle = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(handleSegments);
            glVertex2f(_straightLineEndX + std::cos(angle) * handleRadius,
                       _straightLineEndY + std::sin(angle) * handleRadius);
        }
        glEnd();
        glPopAttrib();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();

#ifdef USE_GL3_RENDERER
    _gl3Renderer.render(fbW, fbH);
#endif

    // Brush cursor rendering for Face Brush tool
    if (_current3DMode == Mode3D::FaceBrush && _brush.showCursor && _brush.cursorVisible) {
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, fbW, fbH, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        float screenX = getCursorX();
        float screenY = getCursorY();
        float cursorSize = _faceBrush.radius * 100.0f * _brush.previewSize;

        // Draw brush cursor circle
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 32; ++i) {
            float angle = 2.0f * M_PI * i / 32.0f;
            float x = screenX + cos(angle) * cursorSize;
            float y = screenY + sin(angle) * cursorSize;
            glVertex2f(x, y);
        }
        glEnd();

        // Draw inner circle for softness indication
        if (_faceBrush.softness < 1.0f) {
            glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
            float innerSize = cursorSize * _faceBrush.softness;
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 32; ++i) {
                float angle = 2.0f * M_PI * i / 32.0f;
                float x = screenX + cos(angle) * innerSize;
                float y = screenY + sin(angle) * innerSize;
                glVertex2f(x, y);
            }
            glEnd();
        }

        // Draw crosshair at center
        glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(screenX - 5.0f, screenY);
        glVertex2f(screenX + 5.0f, screenY);
        glVertex2f(screenX, screenY - 5.0f);
        glVertex2f(screenX, screenY + 5.0f);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();
    }

    _mainMenu.draw();

    // Controls / Keymap window
    if (_showKeymapWindow) {
        ImGui::SetNextWindowSize(ImVec2(420, 420), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Controls / Keymap", &_showKeymapWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Core");
            ImGui::Separator();
            ImGui::BulletText("M: Toggle Main Menu");
            ImGui::BulletText("Esc: Toggle Cursor Lock");
            ImGui::BulletText("H: Toggle Chat");
            ImGui::BulletText("T: Toggle Toolbar");
            ImGui::BulletText("I: Toggle Integration UI");
            ImGui::BulletText("1/2/3: Perspective Modes");
            ImGui::BulletText("F: Toggle Flight (non-Survival)");
            ImGui::BulletText("C: Character Architect Forge Zone");
            ImGui::Separator();
            ImGui::Text("Saves");
            ImGui::Separator();
            ImGui::BulletText("S: Quick Save");
            ImGui::BulletText("A: Save As...  L: Load  G: Save Manager");
            ImGui::Separator();
            ImGui::Text("Camera");
            ImGui::Separator();
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Space: Up");
            ImGui::BulletText("Shift: Down");
            ImGui::BulletText("V: Sprint");
            ImGui::BulletText("Alt: Slow");
        }
        ImGui::End();
    }

    if (_showChatWindow) {
        _chat.renderUI(&_showChatWindow);
    }

    if (_showToolbar) {
        renderCreatorToolbar();
    }

    // Update cursor tools selection each frame
    _cursorTools.update(*this);

    // Integration System UI
    if (_showIntegrationUI) {
        // Integration disabled for stability; re-enable after refactor
        // Integration::IntegrationManager::instance().renderIntegrationUI();
    }

    // Avatar demo info panel
    if (_showAvatarDemo) {
        ImGui::Begin("Avatar System Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Avatar System Features:");
        ImGui::BulletText("Health, Energy, Mood, Experience");
        ImGui::BulletText("Body Part Damage & Healing");
        ImGui::BulletText("Clothing System");
        ImGui::BulletText("Inventory Management");
        ImGui::BulletText("Avatar Interactions");
        ImGui::BulletText("Animation System");
        ImGui::BulletText("AI Behavior");
        ImGui::BulletText("Customization Presets");

        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("O - Toggle Avatar Demo");
        ImGui::Text("H - Toggle Chat Window");
        ImGui::Text("T - Toggle Toolbar");

        ImGui::Separator();
        ImGui::Text("Demo Avatars: %d", _avatarManager.getTotalAvatars());
        ImGui::Text("Average Health: %.1f", _avatarManager.getAverageHealth());
        ImGui::Text("Average Level: %.1f", _avatarManager.getAverageLevel());
        ImGui::Text("Total Experience: %d", _avatarManager.getTotalExperience());

        if (ImGui::Button("Heal All Avatars")) {
            _avatarManager.healAllAvatars(50.0f);
        }
        if (ImGui::Button("Damage All Avatars")) {
            _avatarManager.damageAllAvatars(10.0f);
        }
        if (ImGui::Button("Restore All Avatars")) {
            _avatarManager.restoreAllAvatars();
        }

        ImGui::End();
    }

    // update camera matrices after gluLookAt in render() right after setting view:
    glGetIntegerv(GL_VIEWPORT, _camera.viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, _camera.modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, _camera.projection);
}

} // namespace Core
