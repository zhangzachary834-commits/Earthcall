// GameRender.cpp – Game::render() method
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Form/Object/Object.hpp"
#include "Rendering/ShadingSystem.hpp"
#include "Rendering/BrushSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <GLFW/glfw3.h>
#include "Rendering/GL/GluCompat.hpp"
#include "Rendering/Renderer.hpp"
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

namespace {

// The unit cube, as a triangle list. Every drag grip in this file — morph vertex
// handles, the field operand-B handle, the blend bead, patch control points — was
// the same 24 hand-written glVertex3f calls around this shape.
const std::vector<glm::vec3>& unitCubeTris() {
    static const std::vector<glm::vec3> tris = draw::quadsToTris(std::vector<glm::vec3>{
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
        {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},
    });
    return tris;
}

// A cube grip centred on a world position. Sets the model transform the way the
// old glTranslatef/glScalef pair did, then restores world space so the next draw
// is not silently scaled by it.
void drawHandle(const glm::vec3& worldPos, float size, const glm::vec4& color,
                Blend blend = Blend::Opaque) {
    Renderer& r = currentRenderer();
    r.setModel(glm::scale(glm::translate(glm::mat4(1.0f), worldPos), glm::vec3(size)));
    r.drawSolid(unitCubeTris(), color, blend, true);
    r.setModel(glm::mat4(1.0f));
}

// Line segments for the boundary's drawLines verb, which wants explicit pairs.
using Seg = std::pair<glm::vec3, glm::vec3>;

} // namespace

namespace Core {

void Game::render() {
    if (!_window) return;

    // Apply active zone theme colour
    mgr.active().applyTheme();

    int fbW, fbH;
    glfwGetFramebufferSize(_window, &fbW, &fbH);
    if (fbH == 0) fbH = 1;
    float aspect = static_cast<float>(fbW) / fbH;

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

    // glm::frustum reproduces glFrustum exactly (same [-1,1] clip depth, since the
    // app is not built with GLM_FORCE_DEPTH_ZERO_TO_ONE).
    glm::mat4 proj = glm::frustum(left, right, bottom, top, nearZ, farZ);

    // ------------------------------------------------------------------
    // Model-view (camera)
    // ------------------------------------------------------------------
    vec3 eyePos   = _camera.pos;
    vec3 lookDir  = _camera.front;
    const float CAMERA_DISTANCE = 4.0f;

    if (_currentPerspective == PerspectiveMode::ThirdPerson) {
        eyePos  = _camera.pos - _camera.front * CAMERA_DISTANCE;
    } else if (_currentPerspective == PerspectiveMode::SecondPerson) {
        eyePos  = _camera.pos + _camera.front * CAMERA_DISTANCE;
    }

    vec3 lookTarget = _camera.pos + lookDir;
    glm::mat4 view = glm::lookAt(eyePos, lookTarget, _camera.up);

    // Hand the camera to the active backend. OpenGL loads the fixed-function
    // stacks (what glFrustum + gluLookAt did here until now); WebGPU keeps
    // view*proj as a uniform. Must precede ShadingSystem::update — fixed-function
    // light positions are transformed by whatever MODELVIEW is in force when set.
    currentRenderer().setCamera(view, proj, eyePos);

    // The picking matrices, which ecgl::project/unProject consume as GLdouble[16].
    // Computed straight from the camera instead of read back from the GL stack, so
    // they no longer depend on a GL context — or on nothing having disturbed the
    // stack between here and the end of render(), which the old readback did.
    for (int i = 0; i < 16; ++i) {
        _camera.modelview[i]  = static_cast<GLdouble>(glm::value_ptr(view)[i]);
        _camera.projection[i] = static_cast<GLdouble>(glm::value_ptr(proj)[i]);
    }
    _camera.viewport[0] = 0;    _camera.viewport[1] = 0;
    _camera.viewport[2] = fbW;  _camera.viewport[3] = fbH;

    // Update lighting position to follow camera
    ShadingSystem::update(_camera.pos);

    // Open the frame on the active renderer: sets the viewport and clears to the
    // active zone's colour. Under WebGPU this also acquires the surface and begins
    // the render pass. Bracketed by endFrame() at the bottom of render().
    {
        const Zone& _z = mgr.active();
        currentRenderer().beginFrame(static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH),
                                     glm::vec4(_z.r, _z.g, _z.b, 1.0f));
    }

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
        currentRenderer().setModel(objects[i]->getTransform());
        objects[i]->drawObject();
        objects[i]->drawHighlightOutline();
    }
    currentRenderer().setModel(glm::mat4(1.0f)); // back to world space

    // Morph tool: draw draggable vertex handles over the selected polyhedron.
    if (_current3DMode == Mode3D::Morph && _selectedObject3D &&
        _selectedObject3D->getGeometryType() == Object::GeometryType::Polyhedron) {
        Object* o = _selectedObject3D;
        for (int v = 0; v < o->getPolyhedronVertexCount(); ++v) {
            glm::vec3 w = glm::vec3(o->getTransform() * glm::vec4(o->getPolyhedronVertexLocal(v), 1.0f));
            bool sel = (v == _morphVertexIndex);
            drawHandle(w, sel ? 0.06f : 0.04f,
                       sel ? glm::vec4(1.0f, 0.85f, 0.2f, 1.0f)
                           : glm::vec4(0.2f, 0.8f, 1.0f, 1.0f));
        }
    }

    // Field-refinement gizmos (Morph / Combine / Clay): ghost of operand B + its
    // drag handle, plus a floating blend bead for morph/smooth-union fields.
    if ((_current3DMode == Mode3D::Morph || _current3DMode == Mode3D::Combine ||
         _current3DMode == Mode3D::Sculpt) && _selectedObject3D &&
        _selectedObject3D->isBinaryField()) {
        Object* o = _selectedObject3D;
        const glm::mat4& xf = o->getTransform();
        const geom::SdfNode& f = o->getFieldData();

        // Translucent ghost of operand B (rendered in the field's local space).
        if (f.children.size() == 2 && f.children[1]) {
            geom::TessMesh ghost = geom::tessellateSdf(*f.children[1], o->getFieldExtent(), 16);
            currentRenderer().setModel(xf);
            currentRenderer().drawOverlay(ghost, glm::vec4(0.35f, 0.85f, 1.0f, 0.16f),
                                          1.0f, /*additive=*/true);
            currentRenderer().setModel(glm::mat4(1.0f));
        }

        // The draggable handle at operand B's offset (gold cube, world space).
        glm::vec3 hw = glm::vec3(xf * glm::vec4(o->getFieldOperandBOffset(), 1.0f));
        drawHandle(hw, 0.06f, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f), Blend::Alpha);

        // Floating blend bead on a screen-aligned rail (replaces the t slider).
        if (o->isMorphField()) {
            glm::vec3 rs, rd; float rl = 1.0f;
            blendRail(o, rs, rd, rl);
            glm::vec3 bead = rs + rd * (o->getMorphParam() * rl);
            glm::vec3 rEnd = rs + rd * rl;
            currentRenderer().drawLines({Seg{rs, rEnd}},
                                        glm::vec4(0.55f, 0.55f, 0.6f, 1.0f), 2.0f,
                                        Blend::Alpha);
            drawHandle(bead, 0.05f,
                       _blendHandleDragging ? glm::vec4(1.0f, 0.85f, 0.2f, 1.0f)
                                            : glm::vec4(0.3f, 0.85f, 1.0f, 1.0f),
                       Blend::Alpha);
        }
    }

    // Clay tool: while dragging, outline the shape the dragged piece will fuse into.
    if (_current3DMode == Mode3D::Sculpt && _clayTarget) {
        Object* o = _clayTarget;
        o->updateCollisionZone(o->getTransform());
        glm::vec3 mn = o->collisionZone.corners[0], mx = mn;
        for (int i = 1; i < 8; ++i) {
            mn = glm::min(mn, o->collisionZone.corners[i]);
            mx = glm::max(mx, o->collisionZone.corners[i]);
        }
        const glm::vec3 c[8] = {
            {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},
            {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z} };
        const int e[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        std::vector<Seg> edges;
        edges.reserve(12);
        for (auto& pr : e) edges.push_back(Seg{c[pr[0]], c[pr[1]]});
        // gold = "release here to fuse"
        currentRenderer().drawLines(edges, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f), 2.5f, Blend::Alpha);
    }

    // Morph tool on a Bezier patch: control-net wireframe + draggable control points.
    if (_current3DMode == Mode3D::Morph && _selectedObject3D && _selectedObject3D->isPatch()) {
        Object* o = _selectedObject3D;
        const glm::mat4& xf = o->getTransform();
        const geom::BezierPatch& p = o->getPatchData();

        auto cp = [&](int i, int j) {
            return glm::vec3(xf * glm::vec4(p.at(i, j), 1.0f));
        };
        // Control net (lines between adjacent control points).
        std::vector<Seg> net;
        for (int j = 0; j < p.nv(); ++j)
            for (int i = 0; i < p.nu(); ++i) {
                glm::vec3 a = cp(i, j);
                if (i + 1 < p.nu()) net.push_back(Seg{a, cp(i + 1, j)});
                if (j + 1 < p.nv()) net.push_back(Seg{a, cp(i, j + 1)});
            }
        currentRenderer().drawLines(net, glm::vec4(0.3f, 0.8f, 1.0f, 0.6f), 1.5f, Blend::Alpha);

        // Control-point handles.
        for (int idx = 0; idx < o->getPatchControlCount(); ++idx) {
            glm::vec3 w = glm::vec3(xf * glm::vec4(o->getPatchControlLocal(idx), 1.0f));
            bool sel = (idx == _patchCtrlIndex);
            drawHandle(w, sel ? 0.05f : 0.035f,
                       sel ? glm::vec4(1.0f, 0.85f, 0.2f, 1.0f)
                           : glm::vec4(0.2f, 0.8f, 1.0f, 1.0f),
                       Blend::Alpha);
        }
    }

    // Gravity field visualization (holographic arrows)
    if (Physics::getGravityVisualization()) {
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
                    // One call per arrow: the colour varies per sample, and a single
                    // drawLines carries one colour. Same granularity as the glBegin/
                    // glEnd pair this replaces.
                    currentRenderer().drawLines({Seg{p, q}}, glm::vec4(col, 0.5f), 1.5f,
                                                Blend::Additive);
                }
            }
        }
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
            ecgl::unProject(winX,winY,0.0,_camera.modelview,_camera.projection,_camera.viewport,&nx,&ny,&nz);
            ecgl::unProject(winX,winY,1.0,_camera.modelview,_camera.projection,_camera.viewport,&fx,&fy,&fz);
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
                previewPos = hitPoint + nWorld * getBrushCreateSurfaceOffset(nWorld);
            } else previewPos = _camera.pos + _camera.front * 2.0f;
        }

        // Apply optional grid-snap just like the actual spawn logic
        if (_brush.gridSnap && _brush.gridSize > 1e-6f) {
            previewPos.x = std::round(previewPos.x / _brush.gridSize) * _brush.gridSize;
            previewPos.y = std::round(previewPos.y / _brush.gridSize) * _brush.gridSize;
            previewPos.z = std::round(previewPos.z / _brush.gridSize) * _brush.gridSize;
        }

        glm::mat4 previewT = buildBrushCreateTransform(previewPos);

        // Render as translucent wireframe so it does not occlude view
        currentRenderer().setWireframe(true);
        currentRenderer().setModel(previewT);
        // Draw primitive outline using the selected shape
        Object temp;
        temp.setShape(_polyhedron.shapeKind, _polyhedron.shapeParams);

        // Initialize polyhedron data for preview using the unified builder
        if (_polyhedron.shapeKind == Object::ShapeKind::Polyhedron) {
            temp.setPolyhedronData(buildCurrentPolyhedron());
        }

        temp.drawObject();
        temp.drawHighlightOutline();

        currentRenderer().setModel(glm::mat4(1.0f));
        currentRenderer().setWireframe(false);
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
    currentRenderer().begin2D(static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH));

    mgr.active().renderArt();

    if (_drawingStraightLine && _current3DMode == Mode3D::None && _currentTool.getType() == Tool::Type::Brush) {
        auto* brushSystem = mgr.active().getBrushSystem();
        const float previewOpacity = brushSystem ? std::clamp(brushSystem->getOpacity(), 0.25f, 0.95f) : 0.85f;
        const float previewWidth = brushSystem ? std::max(1.0f, brushSystem->getRadius() * 1000.0f) : 2.0f;

        currentRenderer().drawLines2D(
            {{_straightLineStartX, _straightLineStartY}, {_straightLineEndX, _straightLineEndY}},
            glm::vec4(_currentColor[0], _currentColor[1], _currentColor[2], previewOpacity),
            previewWidth);

        constexpr int handleSegments = 16;
        constexpr float handleRadius = 4.0f;
        std::vector<glm::vec2> handle;
        handle.reserve(handleSegments);
        for (int i = 0; i < handleSegments; ++i) {
            const float angle = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(handleSegments);
            handle.push_back({_straightLineEndX + std::cos(angle) * handleRadius,
                              _straightLineEndY + std::sin(angle) * handleRadius});
        }
        currentRenderer().drawLines2D(draw::stripToSegments(handle, /*closed=*/true),
                                      glm::vec4(1.0f, 1.0f, 1.0f, 0.65f), 1.0f);
    }

    currentRenderer().end2D();

    // Brush cursor rendering for Face Brush tool
    if (_current3DMode == Mode3D::FaceBrush && _brush.showCursor && _brush.cursorVisible) {
        currentRenderer().begin2D(static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH));

        float screenX = getCursorX();
        float screenY = getCursorY();
        float cursorSize = _faceBrush.radius * 100.0f * _brush.previewSize;

        auto circle = [&](float radius) {
            std::vector<glm::vec2> pts;
            pts.reserve(32);
            for (int i = 0; i < 32; ++i) {
                float angle = 2.0f * static_cast<float>(M_PI) * i / 32.0f;
                pts.push_back({screenX + std::cos(angle) * radius,
                               screenY + std::sin(angle) * radius});
            }
            return draw::stripToSegments(pts, /*closed=*/true);
        };

        // Draw brush cursor circle
        currentRenderer().drawLines2D(circle(cursorSize), glm::vec4(1.0f, 1.0f, 1.0f, 0.8f), 2.0f);

        // Draw inner circle for softness indication
        if (_faceBrush.softness < 1.0f)
            currentRenderer().drawLines2D(circle(cursorSize * _faceBrush.softness),
                                          glm::vec4(1.0f, 1.0f, 1.0f, 0.4f), 1.0f);

        // Draw crosshair at center
        currentRenderer().drawLines2D({{screenX - 5.0f, screenY}, {screenX + 5.0f, screenY},
                                       {screenX, screenY - 5.0f}, {screenX, screenY + 5.0f}},
                                      glm::vec4(1.0f, 1.0f, 1.0f, 0.6f), 1.0f);

        currentRenderer().end2D();
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
            ImGui::Separator();
            ImGui::Text("Create");
            ImGui::Separator();
            ImGui::BulletText("[ / ]: Yaw hologram");
            ImGui::BulletText("; / ': Pitch hologram");
            ImGui::BulletText(", / .: Roll hologram");
            ImGui::BulletText("Ctrl: Fine rotation  Shift: Fast rotation");
        }
        ImGui::End();
    }

    // Dynamic Debug Coordinates overlay (Top Right)
    if (_showDebugCoordinates) {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        ImGuiIO& io = ImGui::GetIO();
        const float PAD = 10.0f;
        ImVec2 window_pos = ImVec2(io.DisplaySize.x - PAD, PAD);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Debug Coordinates", &_showDebugCoordinates, window_flags)) {
            ImGui::Text("Player Position:");
            ImGui::Separator();
            ImGui::Text("X: %.2f", _player.position.x);
            ImGui::Text("Y: %.2f", _player.position.y);
            ImGui::Text("Z: %.2f", _player.position.z);
        }
        ImGui::End();
    }

    if (_showChatWindow) {
        _chat.renderUI(&_showChatWindow);
    }

    if (_showToolbar) {
        renderCreatorToolbar();
    }

    // In-scene SDF node graph for the selected field (Mode3D::Graph). Drawn after
    // the toolbar so its cards/panel overlay the scene.
    renderNodeGraph();

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

    // Close the frame: no-op under OpenGL (already drawn immediately); under
    // WebGPU this ends the render pass, submits, and presents the surface.
    currentRenderer().endFrame();
}

} // namespace Core
