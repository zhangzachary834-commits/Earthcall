#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Tool.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "Singularity/Screen/GL/GluCompat.hpp"
#include "AdvancedFacePaint.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/PaintToolSurface.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"
#include <unordered_set>
#include <algorithm>

extern MaterialManager materials;
#include <cmath>
#include <iostream>
#include <utility>

extern ZoneManager mgr;

namespace {
constexpr float kBrushRadiusToScreenPixels = 1000.0f;
constexpr float kDefaultBrushRadius = 0.001f;
constexpr float kPencilRadius = 0.001f;
constexpr float kPenRadius = 0.0015f;
constexpr float kMarkerRadius = 0.003f;

float spacingForBrushRadius(float radius) {
    return std::max(0.0005f, radius * 0.35f);
}

float distanceSqToSegment(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b) {
    const glm::vec2 ab = b - a;
    const float lenSq = glm::dot(ab, ab);
    if (lenSq <= 1e-6f) {
        return glm::dot(point - a, point - a);
    }
    const float t = std::clamp(glm::dot(point - a, ab) / lenSq, 0.0f, 1.0f);
    const glm::vec2 projection = a + ab * t;
    return glm::dot(point - projection, point - projection);
}

float activeEraserRadius(Zone& zone) { return 16.0f; }

// void flushKeptStroke removed

void eraseLegacyStrokeSegments(Zone& zone, const glm::vec2& cursor, float radius) {}

void deleteLegacyStrokesAt(Zone& zone, const glm::vec2& cursor, float radius, bool matchColor = false) {}

void configureStrokeTool(Zone& zone, Tool::Type type) {}

void applyToolTransform(Object* obj, const glm::mat4& worldTransform, const glm::mat4* avatarRoot) {
    if (!obj) return;

    // Primary body part — update via local transform relative to avatar root
    if (auto* part = dynamic_cast<BodyPart*>(obj)) {
        if (avatarRoot) {
            glm::mat4 local = glm::inverse(*avatarRoot) * worldTransform;
            part->setLocalTransform(local);
        } else {
            part->setTransform(worldTransform);
        }
        return;
    }

    // Sub-object belonging to a body part — convert world transform to
    // body-part-local offset so the sub-object stays attached correctly
    // Since getOwnerBodyPart is removed, we search the active person's body if possible.
    // Or we can rely on a helper to find it. For now, we'll just check if it's in the active person's body.
    // (A better architectural fix is needed to find parent formations).
    
    // As a temporary fix since we removed obj->getOwnerBodyPart():
    // For now, if we can't find the owner, just set transform directly. 
    // The true fix is to pass the owner down or use relation graph.
    // For now, just set the transform.
    /*
    if (BodyPart* owner = obj->getOwnerBodyPart()) {
        glm::mat4 localOffset = glm::inverse(owner->getTransform()) * worldTransform;
        for (size_t i = 0; i < owner->getSubObjectCount(); ++i) {
            if (owner->getSubObject(i) == obj) {
                owner->setSubObjectLocalOffset(i, localOffset);
                break;
            }
        }
        return;
    }
    */

    obj->setTransform(worldTransform);
}

} // namespace

// Build a picking ray in world space. When the cursor is LOCKED (first-person
// look mode) the OS cursor position is an unbounded drifting value, so instead
// we shoot through the screen centre — a crosshair pick along the look
// direction. When unlocked we use the real cursor position.
bool buildMouseRay(GLFWwindow* window, Core::Engine* engine, glm::vec3& rayOrigin, glm::vec3& rayDir) {
    if (!window || !engine) return false;

    const int* vp = engine->getCamera()->getViewport();
    if (!vp || vp[2] <= 0 || vp[3] <= 0) return false;

    double winX = 0.0, winY = 0.0; // in framebuffer / GL coords (origin bottom-left)
    if (engine->getMouseHandler()->isCursorLocked()) {
        // Crosshair: centre of the viewport.
        winX = vp[0] + vp[2] * 0.5;
        winY = vp[1] + vp[3] * 0.5;
    } else {
        double xpos = 0.0, ypos = 0.0;
        glfwGetCursorPos(window, &xpos, &ypos);
        int winW = 0, winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        int fW = 0, fH = 0;
        glfwGetFramebufferSize(window, &fW, &fH);
        if (winW <= 0 || winH <= 0 || fW <= 0 || fH <= 0) return false;
        float scaleX = static_cast<float>(fW) / static_cast<float>(winW);
        float scaleY = static_cast<float>(fH) / static_cast<float>(winH);
        winX = xpos * scaleX;
        winY = vp[3] - ypos * scaleY; // invert Y for OpenGL
    }

    GLdouble nearX = 0.0, nearY = 0.0, nearZ = 0.0;
    GLdouble farX = 0.0, farY = 0.0, farZ = 0.0;
    ecgl::unProject(winX, winY, 0.0, engine->getCamera()->getModelview(), engine->getCamera()->getProjection(),
                 engine->getCamera()->getViewport(), &nearX, &nearY, &nearZ);
    ecgl::unProject(winX, winY, 1.0, engine->getCamera()->getModelview(), engine->getCamera()->getProjection(),
                 engine->getCamera()->getViewport(), &farX, &farY, &farZ);

    rayOrigin = glm::vec3(nearX, nearY, nearZ);
    rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayOrigin);
    return glm::length(rayDir) > 1e-6f;
}

// Rich surface pick used by tools that need the hit point / normal (shape
// placement, pottery). Uses the real per-face raycast (Object::raycastFace) for
// every geometry type instead of a bounding AABB/sphere approximation, so all
// 3D tools agree on what the ray hit.

bool pickSurface(const std::vector<Object*>& targets,
                 const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                 SurfaceHit& out) {
    float nearestT = 1e9f;
    SurfaceHit best;
    for (auto* obj : targets) {
        if (!obj) continue;
        float t = 0.0f; int face = -1; glm::vec2 uv(0.0f);
        if (obj->raycastFace(rayOrigin, rayDir, t, face, uv) && t > 0.0f && t < nearestT) {
            nearestT = t;
            best.obj = obj; best.t = t; best.face = face;
        }
    }
    if (!best.obj) return false;

    best.point = rayOrigin + rayDir * best.t;
    const glm::mat4 xf = best.obj->getRaycastTransform();
    if (best.obj->getShapeKind() == Object::ShapeKind::Cube && best.face >= 0) {
        // raycastFace encodes a cube face as axis*2 + (sign>0 ? 0 : 1).
        best.isCube = true;
        best.axis = best.face / 2;
        best.sign = (best.face % 2 == 0) ? 1 : -1;
        glm::vec3 nLocal(0.0f);
        nLocal[best.axis] = static_cast<float>(best.sign);
        best.normal = glm::normalize(glm::vec3(xf * glm::vec4(nLocal, 0.0f)));
    } else {
        // Non-cube: approximate the outward normal as (hit - centre).
        glm::vec3 centre = glm::vec3(xf * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        glm::vec3 n = best.point - centre;
        best.normal = (glm::dot(n, n) > 1e-12f) ? glm::normalize(n) : -rayDir;
    }
    out = best;
    return true;
}

Object* pickNearestObject(const std::vector<Object*>& targets,
                          const glm::vec3& rayOrigin,
                          const glm::vec3& rayDir) {
    float nearestT = 1e9f;
    Object* hitObj = nullptr;
    for (auto* obj : targets) {
        if (!obj) continue;
        float t = 0.0f;
        int face = -1;
        glm::vec2 uv(0.0f);
        if (obj->raycastFace(rayOrigin, rayDir, t, face, uv) && t > 0.0f && t < nearestT) {
            nearestT = t;
            hitObj = obj;
        }
    }
    return hitObj;
}

// Commented out - no longer needed since BodyParts now have proper faceTextures
// and can use raycastFace() like regular Objects.
// bool raycastCollisionAABB(const Object* obj, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outT) {
//     if (!obj) return false;
//     glm::vec3 minCorner = obj->collisionZone.corners[0];
//     glm::vec3 maxCorner = obj->collisionZone.corners[0];
//     for (int i = 1; i < 8; ++i) {
//         minCorner = glm::min(minCorner, obj->collisionZone.corners[i]);
//         maxCorner = glm::max(maxCorner, obj->collisionZone.corners[i]);
//     }
//
//     float tMin = 0.0f;
//     float tMax = 1e9f;
//     for (int axis = 0; axis < 3; ++axis) {
//         float origin = rayOrigin[axis];
//         float dir = rayDir[axis];
//         if (fabs(dir) < 1e-6f) {
//             if (origin < minCorner[axis] || origin > maxCorner[axis]) {
//                 return false;
//             }
//         } else {
//             float invD = 1.0f / dir;
//             float t1 = (minCorner[axis] - origin) * invD;
//             float t2 = (maxCorner[axis] - origin) * invD;
//             if (t1 > t2) std::swap(t1, t2);
//             tMin = std::max(tMin, t1);
//             tMax = std::min(tMax, t2);
//             if (tMin > tMax) {
//                 return false;
//             }
//         }
//     }
//
//     outT = tMin;
//     return outT >= 0.0f;
// }

// Tool class methods are already implemented inline in the header file
// This file can be used for additional tool functionality in the future

// Let's call the tools the Primordial Tools!

std::string Tool::getTypeName() const
{

    switch (type)
    {
    // Drawing Tools
    case Type::Brush:
        return "Brush";
    case Type::Pencil:
        return "Pencil";
    case Type::Pen:
        return "Pen";
    case Type::Marker:
        return "Marker";
    case Type::Airbrush:
        return "Airbrush";
    case Type::Chalk:
        return "Chalk";
    case Type::Spray:
        return "Spray";
    case Type::Smudge:
        return "Smudge";
    case Type::Clone:
        return "Clone";

    // Erasing Tools
    case Type::Eraser:
        return "Eraser";
    case Type::Delete:
        return "Delete";
    case Type::MagicEraser:
        return "Magic Eraser";

    // Selection Tools
    case Type::Selection:
        return "Selection";
    case Type::Lasso:
        return "Lasso";
    case Type::MagicWand:
        return "Magic Wand";
    case Type::Marquee:
        return "Marquee";

    // Shape Tools
    case Type::Rectangle:
        return "Rectangle";
    case Type::Ellipse:
        return "Ellipse";
    case Type::Polygon:
        return "Polygon";
    case Type::Line:
        return "Line";
    case Type::Arrow:
        return "Arrow";
    case Type::Star:
        return "Star";
    case Type::Heart:
        return "Heart";
    case Type::CustomShape:
        return "Custom Shape";

    // Text Tools
    case Type::Text:
        return "Text";
    case Type::TextVertical:
        return "Vertical Text";
    case Type::TextPath:
        return "Text on Path";

    // Transform Tools
    case Type::Move:
        return "Move";
    case Type::Scale:
        return "Scale";
    case Type::Rotate:
        return "Rotate";
    case Type::Skew:
        return "Skew";
    case Type::Distort:
        return "Distort";
    case Type::Perspective:
        return "Perspective";

    // Effects Tools
    case Type::Blur:
        return "Blur";
    case Type::Sharpen:
        return "Sharpen";
    case Type::Noise:
        return "Noise";
    case Type::Emboss:
        return "Emboss";
    case Type::Glow:
        return "Glow";
    case Type::Shadow:
        return "Shadow";
    case Type::Gradient:
        return "Gradient";
    case Type::Pattern:
        return "Pattern";

    // Utility Tools
    case Type::ColorPicker:
        return "Color Picker";
    case Type::Eyedropper:
        return "Eyedropper";
    case Type::Hand:
        return "Hand";
    case Type::Zoom:
        return "Zoom";
    case Type::Crop:
        return "Crop";
    case Type::Slice:
        return "Slice";

    // Layer Tools
    case Type::Layer:
        return "Layer";
    case Type::LayerMask:
        return "Layer Mask";
    case Type::LayerStyle:
        return "Layer Style";

    // 3D Tools
    case Type::FaceBrush:
        return "Face Brush";
    case Type::FacePaint:
        return "Face Paint";

    // Special Tools
    case Type::Symmetry:
        return "Symmetry";
    case Type::Mirror:
        return "Mirror";
    case Type::Grid:
        return "Grid";
    case Type::Ruler:
        return "Ruler";
    case Type::Measure:
        return "Measure";
    case Type::Identity:
        return "Identity";

    default:
        return "Unknown";
    }
}

void Tool::use(GLFWwindow* window, ZoneManager& mgr, Zone& zone, Type type, Core::Engine& engine) { (void)window; (void)mgr; (void)zone; (void)type; (void)engine; }


// Restored from the pre-law implementation (Tool::ShapeGenerator3D, deleted in
// 0da7237 when the tool's placement/shape/colour state moved to
// Singularity::Core::CreationChannel). The placement arithmetic below is
// unchanged from that recovery (see e9f4295); only the state it reads has
// moved off local Game fields and onto `channel`, whose computeSpawnPosition/
// getCursorSpawnTransform already carry the InFront/CursorSnap/ManualDistance
// and grid-snap logic verified by basic_cube_law_test.cpp -- this function
// intentionally duplicates none of that math, it only supplies the live
// camera and raycast the channel's fields need each frame.
//
// This is the DEVELOPER bypass: it calls World::addObject directly and never
// touches Law::applyTo, so it works regardless of whether any spawn law is
// loaded. What it creates is not unauthored, though -- CreationChannel is a
// registered First Mover (isFirstMover() == true, syncRegister'd in
// EngineInit.cpp), and every Object this function spawns gets an
// "authored-by" relation recording CreationChannel as its author, in the
// channel's own (inherited from Law) provenance ledger. Promoting the
// restored tool to a First Mover meant exactly this: giving it real authorial
// standing to invoke, not inventing a new kind of author.
void Tool::UpdateShapeGeneratorPlacement(GLFWwindow *window, Core::Engine *engine,
                                         ZoneManager &mgr,
                                         Singularity::Core::CreationChannel &channel)
{
    if (!window || !engine) return;

    const glm::vec3 camPos = engine->getCamera() ? engine->getCamera()->getPos() : glm::vec3(0.0f);
    const glm::vec3 camFront = engine->getCamera() ? engine->getCamera()->getFront() : glm::vec3(0.0f, 0.0f, -1.0f);

    if (channel.placementMode == "CursorSnap") {
        glm::vec3 rayO, rayDir;
        std::vector<Object*> targets;
        const auto &objects = mgr.active().objects();
        targets.reserve(objects.size());
        for (const auto &uptr : objects) if (uptr) targets.push_back(uptr.get());

        SurfaceHit hit;
        if (buildMouseRay(window, engine, rayO, rayDir) && pickSurface(targets, rayO, rayDir, hit)) {
            channel.cursorHitPos = hit.point;
            channel.cursorHitNormal = hit.normal;
        }
    } else if (channel.placementMode == "ManualDistance" && !channel.manualAnchorValid) {
        channel.manualAnchorPos = camPos + camFront * 2.0f;
        channel.manualAnchorRight = glm::normalize(glm::cross(camFront, engine->getCamera() ? engine->getCamera()->getUp() : glm::vec3(0.0f, 1.0f, 0.0f)));
        channel.manualAnchorUp = engine->getCamera() ? engine->getCamera()->getUp() : glm::vec3(0.0f, 1.0f, 0.0f);
        channel.manualAnchorForward = camFront;
        channel.manualAnchorValid = true;
    }

    channel.updatePlacement(camPos, camFront);
}

void Tool::ShapeGenerator3D(GLFWwindow *window, Core::Engine *engine, ZoneManager &mgr,
                            Singularity::Core::CreationChannel &channel,
                            BodyPart* targetPart)
{
    if (!window || !engine) return;

    // The edge is tracked BEFORE any gate below, and unconditionally. A gate
    // that returns early without updating it leaves the tracker stale, so the
    // first poll after the gate opens reads a button that has been held down
    // for a while as a fresh press -- disarming the law mid-hold, or dragging
    // off an ImGui window, would spawn on release of nothing.
    static bool devToolMouseLeftPressedLast = false;
    const bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool justPressed = mouseLeftNow && !devToolMouseLeftPressedLast;
    devToolMouseLeftPressedLast = mouseLeftNow;
    if (!justPressed) return;

    // First mover set down: the developer bypass does not spawn. The
    // authored shape-generator law is a different being and has its own
    // enabled bit.
    if (!channel.isEnabled()) return;

    // If the spawn law is armed, it owns the click. This bypass steps
    // aside so one press is not two objects. Console Create mode is
    // independent — it is what DISPATCHES this function, not this gate.
    // Callers: CreationChannel::spawnLawArmed.
    if (channel.spawnLawArmed) return;

    // The law path's publisher (EngineInit::registerCallbacks) skips clicks
    // ImGui has captured; this one polls GLFW directly and did not, so
    // pressing this window's own "Refresh Test Saves" button spawned a cube
    // behind it.
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (channel.activeShapeKind == static_cast<int>(Object::ShapeKind::Polyhedron)) {
        // Polyhedron authoring needs a live polyhedron builder; Engine::
        // buildCurrentPolyhedron() is still a stub ("dummy for now"), so
        // refuse rather than spawn a shape with no topology.
        return;
    }

    // Placement is already fresh: UpdateShapeGeneratorPlacement ran this frame,
    // for BOTH paths. Reading it here is all this function does with it.
    glm::mat4 t = channel.getCursorSpawnTransform();

    Object::ShapeKind kind = static_cast<Object::ShapeKind>(channel.activeShapeKind);

    Object* newObj = nullptr;
    if (targetPart) {
        glm::mat4 partWorld = targetPart->getTransform();
        glm::mat4 localT = glm::inverse(partWorld) * t;
        Object* sub = targetPart->addSubObject(kind, localT);
        if (sub) sub->setShape(kind);
        if (sub) {
            for (int f = 0; f < sub->getFaces(); ++f)
                sub->setFaceColor(f, channel.activeColor.x, channel.activeColor.y, channel.activeColor.z);
        }
        newObj = sub;
    } else {
        auto obj = std::make_unique<Object>();
        obj->setShape(kind);
        obj->setTransform(t);
        obj->updateCollisionZone(t);
        for (int f = 0; f < obj->getFaces(); ++f)
            obj->setFaceColor(f, channel.activeColor.x, channel.activeColor.y, channel.activeColor.z);
        newObj = obj.get();
        mgr.active().addObject(std::move(obj));
    }

    if (newObj) {
        channel.recordProvenance("authored-by", *newObj, channel, true, 1.0f);
    }
}

void Tool::Pottery3D(GLFWwindow *window, Core::Engine *engine, ZoneManager &mgr, float dt,
                     const std::vector<Object*>& targets, const glm::mat4* avatarRoot)
{
    (void)mgr;
    // Implement 3D pottery functionality here
    // Pottery sculpting logic: modify existing object geometry by scaling along hit normal
    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow)
    {
        bool firstFrame = !engine->isMouseLeftPressedLast();
        // Build picking ray (crosshair when the cursor is locked) and pick the
        // surface with the shared per-face raycast.
        glm::vec3 rayO, rayDir;
        SurfaceHit hit;
        if (buildMouseRay(window, engine, rayO, rayDir) && pickSurface(targets, rayO, rayDir, hit))
        {
            Object *hitObj = hit.obj;
            int hitAxis = hit.axis;
            int hitSign = hit.sign;
            bool hitIsCube = hit.isCube;
            // Determine scale delta
            float dir = (engine->getCurrentPotteryTool() == Core::PotteryTool::Expand) ? 1.0f : -1.0f;
            float delta = dir * engine->getPotteryStrength() * (firstFrame ? 1.0f : dt); // full step on click, smaller continuous after

            glm::mat4 t = hitObj->getTransform();
            glm::vec3 translation = glm::vec3(t[3]);
            glm::vec3 colX = glm::vec3(t[0]);
            glm::vec3 colY = glm::vec3(t[1]);
            glm::vec3 colZ = glm::vec3(t[2]);
            float scaleX = glm::length(colX);
            float scaleY = glm::length(colY);
            float scaleZ = glm::length(colZ);

            if (hitIsCube && hitAxis >= 0)
            {
                // Get axis basis and current scale separately
                glm::vec3 axisVec;
                float *scalePtr = nullptr;
                if (hitAxis == 0)
                {
                    axisVec = glm::normalize(colX);
                    scalePtr = &scaleX;
                }
                else if (hitAxis == 1)
                {
                    axisVec = glm::normalize(colY);
                    scalePtr = &scaleY;
                }
                else
                {
                    axisVec = glm::normalize(colZ);
                    scalePtr = &scaleZ;
                }

                // Clamp minimum scale
                float newScale = std::max(0.05f, *scalePtr + delta);
                float actualDelta = newScale - *scalePtr; // may differ due to clamp
                *scalePtr = newScale;

                // Shift translation so opposite face stays fixed
                translation += axisVec * actualDelta * 0.5f * static_cast<float>(hitSign);
            }
            else
            {
                scaleX = std::max(0.05f, scaleX + delta);
                scaleY = std::max(0.05f, scaleY + delta);
                scaleZ = std::max(0.05f, scaleZ + delta);
            }

            glm::mat4 newT = glm::translate(glm::mat4(1.0f), translation);
            newT = glm::scale(newT, glm::vec3(scaleX, scaleY, scaleZ));
            applyToolTransform(hitObj, newT, avatarRoot);
            // hitObj->updateCollisionZone(newT); // handled by setTransform/setLocalTransform
        }
    }
}

void Tool::Rotate3D(GLFWwindow *window, Core::Engine *engine, ZoneManager &mgr, float dt,
                    const std::vector<Object*>& targets, const glm::mat4* avatarRoot)
{
    (void)mgr;
    // Drag state lives on Game (not function-local statics) so it can't leak
    // across tool or object switches.
    bool dragging = engine->getRotateDragging();
    double lastCursorX = engine->getRotateLastCursorX();
    double lastCursorY = engine->getRotateLastCursorY();

    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    glm::vec3 rayOrigin(0.0f);
    glm::vec3 rayDir(0.0f, 0.0f, -1.0f);
    buildMouseRay(window, engine, rayOrigin, rayDir);

    if (mouseLeftNow && !engine->isMouseLeftPressedLast()) {
        if (Object* hit = pickNearestObject(targets, rayOrigin, rayDir)) {
            // engine->setSelectedObject3D(hit);
        }
        dragging = true;
        lastCursorX = cursorX;
        lastCursorY = cursorY;
    } else if (!mouseLeftNow) {
        dragging = false;
    }

    Object* selected = nullptr;
    if (selected) {
        selected->setRotationResponsiveness(engine->getRotationToolSmoothness());

        if (dragging && mouseLeftNow) {
            float dx = static_cast<float>(cursorX - lastCursorX);
            float dy = static_cast<float>(cursorY - lastCursorY);
            glm::vec3 deltaDegrees(0.0f);
            float sensitivity = engine->getRotationToolSensitivity();

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                deltaDegrees.z = dx * sensitivity;
            } else {
                switch (engine->getRotationAxisMode()) {
                    case Core::RotationAxisMode::FreeXY:
                        deltaDegrees.x = -dy * sensitivity;
                        deltaDegrees.y = dx * sensitivity;
                        break;
                    case Core::RotationAxisMode::X:
                        deltaDegrees.x = -dy * sensitivity;
                        break;
                    case Core::RotationAxisMode::Y:
                        deltaDegrees.y = dx * sensitivity;
                        break;
                    case Core::RotationAxisMode::Z:
                        deltaDegrees.z = dx * sensitivity;
                        break;
                    case Core::RotationAxisMode::AuthoritativeAxis: {
                        float axisAmount = (dx - dy) * sensitivity;
                        deltaDegrees = selected->getAuthoritativeAxis() * axisAmount;
                        break;
                    }
                }
            }

            if (glm::dot(deltaDegrees, deltaDegrees) > 1e-12f) {
                selected->addTargetRotationDegrees(deltaDegrees);
            }
        }

        if (avatarRoot) {
            glm::mat4 steppedTransform(1.0f);
            if (selected->advanceRotation(selected->getTransform(), dt, steppedTransform)) {
                applyToolTransform(selected, steppedTransform, avatarRoot);
            }
        }
    }

    lastCursorX = cursorX;
    lastCursorY = cursorY;

    engine->setRotateDragging(dragging);
    engine->setRotateLastCursor(lastCursorX, lastCursorY);
}

void Tool::FacePaint(GLFWwindow *window, Core::Engine *engine, ZoneManager &mgr, float dt,
                     const std::vector<Object*>& targets)
{
    (void)mgr;
    (void)dt;
    if (!window || !engine) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow && !engine->isMouseLeftPressedLast())
    {
        // Build picking ray (crosshair when the cursor is locked).
        glm::vec3 rayO, rayDir;
        if (!buildMouseRay(window, engine, rayO, rayDir)) return;

        float nearestT = 1e9f;
        Object *hitObj = nullptr;
        int hitFace = -1;
        glm::vec2 hitUV(0.0f);
        const auto &objects = targets;
        for (auto *obj : objects)
        {
            if (!obj) continue;

            // Use raycastFace for all objects including BodyParts
            // BodyParts now have proper faceTextures initialized
            float t;
            int f;
            glm::vec2 uv;
            if (obj->raycastFace(rayO, rayDir, t, f, uv))
            {
                if (t < nearestT)
                {
                    nearestT = t;
                    hitObj = obj;
                    hitFace = f;
                    hitUV = uv;
                }
            }
        }
        if (hitObj && hitFace >= 0)
        {
            // Paint the object's OWN material, not the shared one it is
            // referencing: ownMaterial() diverges it on the first
            // stroke so a brush touches this object and no other.
            if (auto mat = hitObj->ownMaterial()) {
                const int faces = hitObj->getFaces() > 0 ? hitObj->getFaces() : 6;
                if (static_cast<int>(mat->faceTextures.size()) != faces) {
                    mat->initFaceTextures(faces);
                }
                PaintToolSurface pts(*mat);
                const float r = engine->getCurrentColor(0);
                const float g = engine->getCurrentColor(1);
                const float b = engine->getCurrentColor(2);

                if (engine->isAdvancedFacePaintEnabled())
                {
                    bool success = AdvancedFacePaint::paintFaceAdvanced(hitObj, hitFace, hitUV, 
                                                                      nullptr, nullptr);
                    if (!success) {
                        // Fall back to basic fill if advanced painting fails
                        pts.fillFaceColor(hitFace, r, g, b);
                    }
                }
                else
                {
                    // Use basic fill for FacePaint click
                    pts.fillFaceColor(hitFace, r, g, b);
                }
                if (hitFace < 6) {
                    hitObj->faceColors[hitFace][0] = r;
                    hitObj->faceColors[hitFace][1] = g;
                    hitObj->faceColors[hitFace][2] = b;
                }
            }
        }
    }
}

 void Tool::FaceBrush(GLFWwindow *window, Core::Engine *engine, ZoneManager &mgr, float dt,
                      const std::vector<Object*>& targets)
 {
    (void)mgr;
    (void)dt;
    if (!window || !engine) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

     bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
     if (mouseLeftNow)
    {
        // Continuous stroke painting while mouse button held.
        // Build picking ray (crosshair when the cursor is locked).
        glm::vec3 rayO, rayDir;
        if (!buildMouseRay(window, engine, rayO, rayDir)) return;
        float nearestT = 1e9f;
        Object *hitObj = nullptr;
        int hitFace = -1;
        glm::vec2 uv(0.0f);
        const auto &objects = targets;
        for (auto *obj : objects)
        {
            if (!obj) continue;
            
            // Use raycastFace for all objects including BodyParts
            // BodyParts now have proper faceTextures initialized
            float t;
            int f;
            glm::vec2 hitUV;
            if (obj->raycastFace(rayO, rayDir, t, f, hitUV))
            {
                if (t < nearestT)
                {
                    nearestT = t;
                    hitObj = obj;
                    hitFace = f;
                    uv = hitUV;
                }
            }
        }
        if (hitObj && hitFace >= 0)
        {
            uv += glm::vec2(engine->getFaceBrushUOffset(), engine->getFaceBrushVOffset());
            uv = glm::clamp(uv, glm::vec2(0.0f), glm::vec2(1.0f));

            // Update brush cursor position
            // engine->setBrushCursorPos(uv);
            engine->setBrushCursorVisible(true);

            // Calculate pressure simulation
            float pressure = engine->getCurrentPressure();
            if (engine->getUsePressureSimulation())
            {
                // Simulate pressure based on mouse speed and other factors
                float currentTime = static_cast<float>(glfwGetTime());
                if (0.0f > 0.0f)
                {
                    float timeDelta = currentTime - 0.0f;
                    if (timeDelta > 0.0f)
                    {
                        float speed = glm::length(uv - glm::vec2(0)) / timeDelta;
                        pressure = std::clamp(1.0f - speed * 0.0f, 0.1f, 1.0f);
                    }
                }
                // engine->setLastBrushTime(currentTime);
            }

            auto& st = Rendering::getCreatorConsoleState();
            // Apply brush based on type
            // Paint the object's OWN material, not the shared one it is
            // referencing: ownMaterial() diverges it on the first
            // stroke so a brush touches this object and no other.
            if (auto mat = hitObj->ownMaterial()) {
                const int faces = hitObj->getFaces() > 0 ? hitObj->getFaces() : 6;
                if (static_cast<int>(mat->faceTextures.size()) != faces) {
                    mat->initFaceTextures(faces);
                }
                PaintToolSurface pts(*mat);
                const float radius = std::max(0.005f, st.faceBrushRadius * pressure);
                const float softness = st.faceBrushSoftness;
                const float opacity = st.faceBrushOpacity;
                const float flow = st.faceBrushFlow;
                const int brushType = st.faceBrushType;
                const float r = engine->getCurrentColor(0);
                const float g = engine->getCurrentColor(1);
                const float b = engine->getCurrentColor(2);

                switch (brushType)
                {
                case 1: // Airbrush
                    pts.airbrushFace(hitFace, uv, r, g, b, radius, /*density*/ 0.5f, opacity);
                    break;
                case 2: // Chalk
                    pts.paintFaceAdvanced(hitFace, uv, r, g, b, radius, softness, opacity, flow, 2);
                    break;
                case 3: // Spray
                    pts.paintFaceAdvanced(hitFace, uv, r, g, b, radius, softness, opacity, flow, 3);
                    break;
                case 4: // Smudge
                    pts.smudgeFace(hitFace, uv, radius, /*strength*/ 0.5f * opacity);
                    break;
                case 5: // Clone
                    if (st.lastBrushUV.x >= 0.0f)
                    {
                        pts.cloneFace(hitFace, uv, st.lastBrushUV, radius, opacity);
                    }
                    break;
                default: // Normal
                    if (st.lastBrushUV.x >= 0.0f &&
                        st.lastBrushObject == hitObj &&
                        st.lastBrushFace == hitFace)
                    {
                        pts.paintStroke(hitFace, st.lastBrushUV, uv, r, g, b,
                                        radius, softness, opacity, std::max(0.002f, radius * 0.25f));
                    }
                    else
                    {
                        pts.paintFaceAdvanced(hitFace, uv, r, g, b, radius, softness, opacity, flow, 0);
                    }
                    break;
                }
            }

            st.lastBrushUV = uv;
            st.lastBrushFace = hitFace;
            st.lastBrushObject = hitObj;
        }
        else
        {
            engine->setBrushCursorVisible(false);
        }
    }
    else
    {
        auto& st = Rendering::getCreatorConsoleState();
        st.lastBrushUV = glm::vec2(-1.0f, -1.0f);
        st.lastBrushFace = -1;
        st.lastBrushObject = nullptr;
        engine->setBrushCursorVisible(false);
    }
}

void Tool::Selection3D(GLFWwindow *window, Core::Engine *engine,
                       const std::vector<Object*>& targets)
{
    if (!window) return;
    if (ImGui::GetIO().WantCaptureMouse) return;
    static bool selectMouseLeftPressedLast = false;
    const bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool justPressed = mouseLeftNow && !selectMouseLeftPressedLast;
    selectMouseLeftPressedLast = mouseLeftNow;
    if (!justPressed) return;

    glm::vec3 rayO, rayDir;
    if (!buildMouseRay(window, engine, rayO, rayDir)) return;
    Object* hit = pickNearestObject(targets, rayO, rayDir);
    auto& state = Rendering::getCreatorConsoleState();
    state.selectedObject3D = hit;
    Rendering::HighlightSystem::setSelected(hit);
    if (hit) {
        Rendering::HighlightSystem::setSelectedIds({hit->getIdentifier()});
    } else {
        Rendering::HighlightSystem::setSelectedIds({});
    }
}

Object* Tool::PickObject3D(GLFWwindow *window, Core::Engine *engine,
                           const std::vector<Object*>& targets)
{
    // Same ray/pick path as Selection3D, but returns the hit instead of
    // mutating the current selection (so a tool can pick operands freely).
    glm::vec3 rayO, rayDir;
    if (!buildMouseRay(window, engine, rayO, rayDir)) return nullptr;
    return pickNearestObject(targets, rayO, rayDir);
}

Tool::Type Tool::getType() const
{
    return type;
}

std::string Tool::getIcon() const
{
    switch (type)
    {
    // Drawing Tools
    case Type::Brush:
        return "🖌";
    case Type::Pencil:
        return "✏️";
    case Type::Pen:
        return "🖊";
    case Type::Marker:
        return "🖍";
    case Type::Airbrush:
        return "💨";
    case Type::Chalk:
        return "🖼";
    case Type::Spray:
        return "🎨";
    case Type::Smudge:
        return "👆";
    case Type::Clone:
        return "📋";

    // Erasing Tools
    case Type::Eraser:
        return "🧽";
    case Type::Delete:
        return "⌫";
    case Type::MagicEraser:
        return "✨";

    // Selection Tools
    case Type::Selection:
        return "⬜";
    case Type::Lasso:
        return "🔗";
    case Type::MagicWand:
        return "🪄";
    case Type::Marquee:
        return "📦";

    // Shape Tools
    case Type::Rectangle:
        return "⬜";
    case Type::Ellipse:
        return "⭕";
    case Type::Polygon:
        return "🔷";
    case Type::Line:
        return "➖";
    case Type::Arrow:
        return "➡️";
    case Type::Star:
        return "⭐";
    case Type::Heart:
        return "❤️";
    case Type::CustomShape:
        return "🔶";

    // Text Tools
    case Type::Text:
        return "T";
    case Type::TextVertical:
        return "T↕️";
    case Type::TextPath:
        return "T〰️";

    // Transform Tools
    case Type::Move:
        return "✋";
    case Type::Scale:
        return "🔍";
    case Type::Rotate:
        return "🔄";
    case Type::Skew:
        return "📐";
    case Type::Distort:
        return "🔀";
    case Type::Perspective:
        return "🏗️";

    // Effects Tools
    case Type::Blur:
        return "🌫️";
    case Type::Sharpen:
        return "🔪";
    case Type::Noise:
        return "📻";
    case Type::Emboss:
        return "🏛️";
    case Type::Glow:
        return "💡";
    case Type::Shadow:
        return "👤";
    case Type::Gradient:
        return "🌈";
    case Type::Pattern:
        return "🔲";

    // Utility Tools
    case Type::ColorPicker:
        return "🎯";
    case Type::Eyedropper:
        return "💉";
    case Type::Hand:
        return "✋";
    case Type::Zoom:
        return "🔍";
    case Type::Crop:
        return "✂️";
    case Type::Slice:
        return "🔪";

    // Layer Tools
    case Type::Layer:
        return "📄";
    case Type::LayerMask:
        return "🎭";
    case Type::LayerStyle:
        return "🎨";

    // 3D Tools
    case Type::FaceBrush:
        return "🎨";
    case Type::FacePaint:
        return "🖼️";

    // Special Tools
    case Type::Symmetry:
        return "🔄";
    case Type::Mirror:
        return "🪞";
    case Type::Grid:
        return "📊";
    case Type::Ruler:
        return "📏";
    case Type::Measure:
        return "📐";

    default:
        return "❓";
    }
}

Tool::Category Tool::getCategory() const
{
    switch (type)
    {
    case Type::Brush:
    case Type::Pencil:
    case Type::Pen:
    case Type::Marker:
    case Type::Airbrush:
    case Type::Chalk:
    case Type::Spray:
    case Type::Smudge:
    case Type::Clone:
        return Category::Drawing;

    case Type::Eraser:
    case Type::Delete:
    case Type::MagicEraser:
        return Category::Erasing;

    case Type::Selection:
    case Type::Lasso:
    case Type::MagicWand:
    case Type::Marquee:
        return Category::Selection;

    case Type::Rectangle:
    case Type::Ellipse:
    case Type::Polygon:
    case Type::Line:
    case Type::Arrow:
    case Type::Star:
    case Type::Heart:
    case Type::CustomShape:
        return Category::Shape;

    case Type::Text:
    case Type::TextVertical:
    case Type::TextPath:
        return Category::Text;

    case Type::Move:
    case Type::Scale:
    case Type::Rotate:
    case Type::Skew:
    case Type::Distort:
    case Type::Perspective:
        return Category::Transform;

    case Type::Blur:
    case Type::Sharpen:
    case Type::Noise:
    case Type::Emboss:
    case Type::Glow:
    case Type::Shadow:
    case Type::Gradient:
    case Type::Pattern:
        return Category::Effects;

    case Type::ColorPicker:
    case Type::Eyedropper:
    case Type::Hand:
    case Type::Zoom:
    case Type::Crop:
    case Type::Slice:
        return Category::Utility;

    case Type::Layer:
    case Type::LayerMask:
    case Type::LayerStyle:
        return Category::Layer;

    case Type::Symmetry:
    case Type::Mirror:
    case Type::Grid:
    case Type::Ruler:
    case Type::Measure:
        return Category::Special;

    default:
        return Category::Utility;
    }
}
