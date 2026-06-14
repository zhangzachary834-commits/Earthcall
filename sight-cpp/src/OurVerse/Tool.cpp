#include "Tool.hpp"
#include "Core/Game.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "GLFW/glfw3.h"
#include "AdvancedFacePaint.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

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

float activeEraserRadius(Zone& zone) {
    if (auto* brushSystem = zone.getBrushSystem()) {
        return std::max(4.0f, brushSystem->getRadius() * kBrushRadiusToScreenPixels);
    }
    return 16.0f;
}

void flushKeptStroke(std::vector<Zone::Stroke>& output, Zone::Stroke stroke) {
    if (stroke.points.size() >= 4) {
        output.push_back(std::move(stroke));
    }
}

void eraseLegacyStrokeSegments(Zone& zone, const glm::vec2& cursor, float radius) {
    std::vector<Zone::Stroke> keptStrokes;
    const float radiusSq = radius * radius;

    for (const auto& stroke : zone.strokes) {
        if (stroke.points.size() < 4) {
            continue;
        }

        Zone::Stroke currentSegment;
        currentSegment.r = stroke.r;
        currentSegment.g = stroke.g;
        currentSegment.b = stroke.b;
        currentSegment.lineWidth = stroke.lineWidth;

        for (size_t i = 0; i + 3 < stroke.points.size(); i += 2) {
            const glm::vec2 a(stroke.points[i], stroke.points[i + 1]);
            const glm::vec2 b(stroke.points[i + 2], stroke.points[i + 3]);
            const bool eraseSegment = distanceSqToSegment(cursor, a, b) <= radiusSq;

            if (eraseSegment) {
                flushKeptStroke(keptStrokes, currentSegment);
                currentSegment.points.clear();
                currentSegment.r = stroke.r;
                currentSegment.g = stroke.g;
                currentSegment.b = stroke.b;
                currentSegment.lineWidth = stroke.lineWidth;
                continue;
            }

            if (currentSegment.points.empty()) {
                currentSegment.points.push_back(a.x);
                currentSegment.points.push_back(a.y);
            }
            currentSegment.points.push_back(b.x);
            currentSegment.points.push_back(b.y);
        }

        flushKeptStroke(keptStrokes, currentSegment);
    }

    zone.strokes = std::move(keptStrokes);
}

void deleteLegacyStrokesAt(Zone& zone, const glm::vec2& cursor, float radius, bool matchColor = false) {
    const float radiusSq = radius * radius;
    bool haveSampleColor = false;
    glm::vec3 sampleColor(0.0f);

    if (matchColor) {
        for (const auto& stroke : zone.strokes) {
            for (size_t i = 0; i + 3 < stroke.points.size(); i += 2) {
                const glm::vec2 a(stroke.points[i], stroke.points[i + 1]);
                const glm::vec2 b(stroke.points[i + 2], stroke.points[i + 3]);
                if (distanceSqToSegment(cursor, a, b) <= radiusSq) {
                    sampleColor = glm::vec3(stroke.r, stroke.g, stroke.b);
                    haveSampleColor = true;
                    break;
                }
            }
            if (haveSampleColor) break;
        }
    }

    auto& strokes = zone.strokes;
    for (auto it = strokes.begin(); it != strokes.end();) {
        bool eraseStroke = false;
        if (matchColor && haveSampleColor) {
            const glm::vec3 color(it->r, it->g, it->b);
            eraseStroke = glm::length(color - sampleColor) <= 0.06f;
        } else {
            for (size_t i = 0; i + 3 < it->points.size(); i += 2) {
                const glm::vec2 a(it->points[i], it->points[i + 1]);
                const glm::vec2 b(it->points[i + 2], it->points[i + 3]);
                if (distanceSqToSegment(cursor, a, b) <= radiusSq) {
                    eraseStroke = true;
                    break;
                }
            }
        }

        if (eraseStroke) {
            it = strokes.erase(it);
        } else {
            ++it;
        }
    }
}

void configureStrokeTool(Zone& zone, Tool::Type type) {
    if (!zone.getBrushSystem()) {
        zone.initializeBrushSystem();
    }

    zone.setCloneActive(false);
    switch (type) {
    case Tool::Type::Pencil:
        zone.setBrushType(BrushSystem::BrushType::Normal);
        zone.setBrushRadius(kPencilRadius);
        zone.setBrushOpacity(1.0f);
        zone.setBrushFlow(1.0f);
        zone.setBrushSpacing(spacingForBrushRadius(kPencilRadius));
        break;
    case Tool::Type::Pen:
        zone.setBrushType(BrushSystem::BrushType::Normal);
        zone.setBrushRadius(kPenRadius);
        zone.setBrushOpacity(1.0f);
        zone.setBrushFlow(1.0f);
        zone.setBrushSpacing(spacingForBrushRadius(kPenRadius));
        break;
    case Tool::Type::Marker:
        zone.setBrushType(BrushSystem::BrushType::Normal);
        zone.setBrushRadius(kMarkerRadius);
        zone.setBrushOpacity(0.55f);
        zone.setBrushFlow(0.75f);
        zone.setBrushSpacing(spacingForBrushRadius(kMarkerRadius));
        break;
    case Tool::Type::Airbrush:
        zone.setBrushType(BrushSystem::BrushType::Airbrush);
        break;
    case Tool::Type::Chalk:
        zone.setBrushType(BrushSystem::BrushType::Chalk);
        break;
    case Tool::Type::Spray:
        zone.setBrushType(BrushSystem::BrushType::Spray);
        break;
    case Tool::Type::Smudge:
        zone.setBrushType(BrushSystem::BrushType::Smudge);
        break;
    case Tool::Type::Clone:
        zone.setBrushType(BrushSystem::BrushType::Clone);
        zone.setCloneActive(true);
        zone.setCloneOffset(glm::vec2(-0.08f, -0.08f));
        break;
    case Tool::Type::Brush:
        zone.setBrushType(BrushSystem::BrushType::Normal);
        zone.setBrushRadius(kDefaultBrushRadius);
        zone.setBrushOpacity(1.0f);
        zone.setBrushFlow(1.0f);
        zone.setBrushSpacing(spacingForBrushRadius(kDefaultBrushRadius));
        break;
    default:
        zone.setBrushType(BrushSystem::BrushType::Normal);
        break;
    }
}

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

    obj->setTransform(worldTransform);
}

// Build a picking ray in world space. When the cursor is LOCKED (first-person
// look mode) the OS cursor position is an unbounded drifting value, so instead
// we shoot through the screen centre — a crosshair pick along the look
// direction. When unlocked we use the real cursor position.
bool buildMouseRay(GLFWwindow* window, Core::Game* game, glm::vec3& rayOrigin, glm::vec3& rayDir) {
    if (!window || !game) return false;

    const GLint* vp = game->getCameraViewport();
    if (!vp || vp[2] <= 0 || vp[3] <= 0) return false;

    double winX = 0.0, winY = 0.0; // in framebuffer / GL coords (origin bottom-left)
    if (game->isCursorLocked()) {
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
    gluUnProject(winX, winY, 0.0, game->getCameraModelview(), game->getCameraProjection(),
                 game->getCameraViewport(), &nearX, &nearY, &nearZ);
    gluUnProject(winX, winY, 1.0, game->getCameraModelview(), game->getCameraProjection(),
                 game->getCameraViewport(), &farX, &farY, &farZ);

    rayOrigin = glm::vec3(nearX, nearY, nearZ);
    rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayOrigin);
    return glm::length(rayDir) > 1e-6f;
}

// Rich surface pick used by tools that need the hit point / normal (shape
// placement, pottery). Uses the real per-face raycast (Object::raycastFace) for
// every geometry type instead of a bounding AABB/sphere approximation, so all
// 3D tools agree on what the ray hit.
struct SurfaceHit {
    Object* obj = nullptr;
    float   t = 0.0f;
    int     face = -1;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    bool    isCube = false;
    int     axis = -1; // cube only: 0=X,1=Y,2=Z
    int     sign = 1;  // cube only: +1/-1 (outward face direction)
};

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
    if (best.obj->getGeometryType() == Object::GeometryType::Cube && best.face >= 0) {
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
} // namespace

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

    default:
        return "Unknown";
    }
}

void Tool::use(GLFWwindow *window, ZoneManager &mgr, Zone &zone, Type type, Core::Game &game)
{
    // Implement tool-specific behavior here
    // For example, if it's a brush, apply color to the target object
    // If it's an eraser, remove parts of the target object, etc.

    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    const bool strokeTool =
        type == Type::Brush || type == Type::Pencil || type == Type::Pen ||
        type == Type::Marker || type == Type::Airbrush || type == Type::Chalk ||
        type == Type::Spray || type == Type::Smudge || type == Type::Clone;

    // Brush Implementation
    if (strokeTool)
    {
        if (mouseLeftNow && !game.getMouseLeftPressedLast()) {
            configureStrokeTool(zone, type);
        } else if (!zone.getBrushSystem()) {
            zone.initializeBrushSystem();
        }

        if (game.getAdvanced2DBrush())
        {
            if (mouseLeftNow && !game.getMouseLeftPressedLast())
            {
                zone.startStroke(game.getCursorX(), game.getCursorY());
            }
            else if (mouseLeftNow && game.getMouseLeftPressedLast())
            {
                zone.continueStroke(game.getCursorX(), game.getCursorY());
            }
            else if (!mouseLeftNow && game.getMouseLeftPressedLast())
            {
                zone.endStroke();
            }
        }
        // Normal strokes
        else
        {
            if (mouseLeftNow && !game.getMouseLeftPressedLast())
            {
                mgr.active().startStroke(game.getCursorX(), game.getCursorY());
            }
            else if (mouseLeftNow && game.getMouseLeftPressedLast())
            {
                mgr.active().continueStroke(game.getCursorX(), game.getCursorY());
            }
            else if (!mouseLeftNow && game.getMouseLeftPressedLast())
            {
                mgr.active().endStroke();
            }
        }
    }
    else if (type == Type::Eraser)
    {
        if (mouseLeftNow)
        {
            const float radius = activeEraserRadius(zone);
            if (zone.getBrushSystem()) {
                if (!game.getMouseLeftPressedLast()) {
                    zone.getBrushSystem()->saveStrokeState();
                }
                zone.getBrushSystem()->eraseDab(
                    glm::vec2(game.getCursorX(), game.getCursorY()),
                    std::max(0.01f, zone.getBrushSystem()->getRadius()));
            }

            eraseLegacyStrokeSegments(zone, glm::vec2(game.getCursorX(), game.getCursorY()), radius);
        }
    }
    else if (type == Type::Delete)
    {
        if (mouseLeftNow)
        {
            deleteLegacyStrokesAt(zone, glm::vec2(game.getCursorX(), game.getCursorY()), activeEraserRadius(zone));
        }
    }
    else if (type == Type::MagicEraser)
    {
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            if (zone.getBrushSystem()) {
                zone.getBrushSystem()->saveStrokeState();
                zone.getBrushSystem()->eraseDab(
                    glm::vec2(game.getCursorX(), game.getCursorY()),
                    std::max(0.02f, zone.getBrushSystem()->getRadius() * 1.75f));
            }
            deleteLegacyStrokesAt(zone,
                                  glm::vec2(game.getCursorX(), game.getCursorY()),
                                  activeEraserRadius(zone) * 1.5f,
                                  true);
        }
    }
    else if (type == Type::Line)
    {
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            game.begin2DToolDrag(type, glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            game.update2DToolDrag(glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            glm::vec2 start = game.get2DToolDragStart();
            zone.startStroke(start.x, start.y);
            zone.continueStroke(game.getCursorX(), game.getCursorY());
            zone.endStroke();
            game.end2DToolDrag();
        }
    }
    else if (type == Type::Rectangle ||
             type == Type::Ellipse ||
             type == Type::Polygon ||
             type == Type::Arrow ||
             type == Type::Star ||
             type == Type::Heart ||
             type == Type::CustomShape)
    {

        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            game.begin2DToolDrag(type, glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            game.update2DToolDrag(glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            glm::vec2 start = game.get2DToolDragStart();
            float endX = game.getCursorX();
            float endY = game.getCursorY();
            float minX = std::min(start.x, endX);
            float minY = std::min(start.y, endY);
            float width = std::abs(endX - start.x);
            float height = std::abs(endY - start.y);

            if (width > 5.0f && height > 5.0f)
            { // Minimum size threshold
                zone.addDesignShape(type, minX + width * 0.5f, minY + height * 0.5f, width, height);
            }

            game.end2DToolDrag();
        }

        // Color picker tool
    }
    else if (type == Type::ColorPicker || type == Type::Eyedropper)
    {
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            glm::vec3 sampledColor(0.0f);
            if (zone.getBrushSystem() &&
                zone.getBrushSystem()->sampleColor(glm::vec2(game.getCursorX(), game.getCursorY()), sampledColor)) {
                game.setCurrentColor(0, sampledColor.r);
                game.setCurrentColor(1, sampledColor.g);
                game.setCurrentColor(2, sampledColor.b);
                zone.setDrawColor(sampledColor.r, sampledColor.g, sampledColor.b);
                return;
            }

            float radius = 12.0f;
            for (const auto &stroke : zone.strokes)
            {
                for (size_t i = 0; i + 1 < stroke.points.size(); i += 2)
                {
                    float dx = stroke.points[i] - game.getCursorX();
                    float dy = stroke.points[i + 1] - game.getCursorY();
                    if (dx * dx + dy * dy < radius * radius)
                    {
                        // Update the game's current color with the picked color
                        game.setCurrentColor(0, stroke.r);
                        game.setCurrentColor(1, stroke.g);
                        game.setCurrentColor(2, stroke.b);
                        zone.setDrawColor(stroke.r, stroke.g, stroke.b);
                        break;
                    }
                }
            }
        }
    }

    // Selection tools
    else if (type == Tool::Type::Selection ||
             type == Tool::Type::Lasso ||
             type == Tool::Type::MagicWand ||
             type == Tool::Type::Marquee)
    {
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            game.begin2DToolDrag(type, glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            game.update2DToolDrag(glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            const auto& selectionPoints = game.get2DToolDragPoints();
            if (selectionPoints.size() >= 2)
            {
                // Create selection based on tool type
                SelectionSystem::SelectionType selectionType;
                switch (type)
                {
                case Tool::Type::Selection:
                case Tool::Type::Marquee:
                    selectionType = SelectionSystem::SelectionType::Rectangle;
                    break;
                case Tool::Type::Lasso:
                    selectionType = SelectionSystem::SelectionType::Lasso;
                    break;
                case Tool::Type::MagicWand:
                    selectionType = SelectionSystem::SelectionType::MagicWand;
                    break;
                default:
                    selectionType = SelectionSystem::SelectionType::Rectangle;
                }
                if (zone.getDesignSystem() && zone.getDesignSystem()->getSelectionSystem())
                {
                    zone.getDesignSystem()->getSelectionSystem()->createSelection(selectionType, selectionPoints);
                }
            }
            game.end2DToolDrag();
        }
    }

    // Effects tools
    else if (type == Tool::Type::Blur ||
             type == Tool::Type::Sharpen ||
             type == Tool::Type::Noise ||
             type == Tool::Type::Emboss ||
             type == Tool::Type::Glow ||
             type == Tool::Type::Shadow ||
             type == Tool::Type::Gradient ||
             type == Tool::Type::Pattern)
    {

        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            // Apply effect at click position
            EffectsSystem::EffectType effectType;
            switch (type)
            {
            case Tool::Type::Blur:
                effectType = EffectsSystem::EffectType::Blur;
                break;
            case Tool::Type::Sharpen:
                effectType = EffectsSystem::EffectType::Sharpen;
                break;
            case Tool::Type::Noise:
                effectType = EffectsSystem::EffectType::Noise;
                break;
            case Tool::Type::Emboss:
                effectType = EffectsSystem::EffectType::Emboss;
                break;
            case Tool::Type::Glow:
                effectType = EffectsSystem::EffectType::Glow;
                break;
            case Tool::Type::Shadow:
                effectType = EffectsSystem::EffectType::Shadow;
                break;
            case Tool::Type::Gradient:
                effectType = EffectsSystem::EffectType::Gradient;
                break;
            case Tool::Type::Pattern:
                effectType = EffectsSystem::EffectType::Pattern;
                break;
            default:
                effectType = EffectsSystem::EffectType::Blur;
            }

            if (zone.getDesignSystem() && zone.getDesignSystem()->getEffectsSystem())
            {
                zone.getDesignSystem()->getEffectsSystem()->addEffect(effectType, 1.0f);
            }
        }
    }
    // Text tools
    else if (type == Tool::Type::Text ||
             type == Tool::Type::TextVertical ||
             type == Tool::Type::TextPath)
    {

        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            // Add sample text at click position
            static int textCounter = 1;
            std::string text;
            if (type == Tool::Type::TextVertical) {
                text = "V\ne\nr\nt\n" + std::to_string(textCounter++);
            } else if (type == Tool::Type::TextPath) {
                text = "Path Text " + std::to_string(textCounter++);
            } else {
                text = "Text " + std::to_string(textCounter++);
            }
            zone.addDesignText(text, game.getCursorX(), game.getCursorY());
        }
    }
    // Transform tools
    else if (type == Tool::Type::Move ||
             type == Tool::Type::Scale ||
             type == Tool::Type::Rotate ||
             type == Tool::Type::Skew ||
             type == Tool::Type::Distort ||
             type == Tool::Type::Perspective)
    {

        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            game.begin2DToolDrag(type, glm::vec2(game.getCursorX(), game.getCursorY()));

            // Create a transform at the click position
            if (zone.getDesignSystem() && zone.getDesignSystem()->getTransformSystem())
            {
                TransformSystem::TransformType transformType;
                switch (type)
                {
                case Tool::Type::Move:
                    transformType = TransformSystem::TransformType::Move;
                    break;
                case Tool::Type::Scale:
                    transformType = TransformSystem::TransformType::Scale;
                    break;
                case Tool::Type::Rotate:
                    transformType = TransformSystem::TransformType::Rotate;
                    break;
                case Tool::Type::Skew:
                    transformType = TransformSystem::TransformType::Skew;
                    break;
                case Tool::Type::Distort:
                    transformType = TransformSystem::TransformType::Distort;
                    break;
                case Tool::Type::Perspective:
                    transformType = TransformSystem::TransformType::Perspective;
                    break;
                default:
                    transformType = TransformSystem::TransformType::Move;
                }

                zone.getDesignSystem()->getTransformSystem()->createTransform(transformType);
            }
        }
        else if (mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            // Update transform based on mouse movement
            glm::vec2 delta = glm::vec2(game.getCursorX(), game.getCursorY()) - game.get2DToolDragStart();
            (void)delta;
            game.update2DToolDrag(glm::vec2(game.getCursorX(), game.getCursorY()));

            if (zone.getDesignSystem() && zone.getDesignSystem()->getTransformSystem())
            {
                // For now, just move the transform
                TransformSystem::Transform transform;
                transform.position = glm::vec2(game.getCursorX(), game.getCursorY());
                transform.type = (type == Tool::Type::Move) ? TransformSystem::TransformType::Move : TransformSystem::TransformType::Scale;
                transform.active = true;

                // Update the most recent transform
                // This is a simplified approach - in a real system you'd track the active transform
            }
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && game.is2DToolDragging(type))
        {
            game.end2DToolDrag();
        }
    }
}

void Tool::ShapeGenerator3D(GLFWwindow *window, Core::Game *game, ZoneManager &mgr,
                            BodyPart* targetPart)
{
    // In the future, refactor mouse to be handled by Zone system and Perspective system, not Tool.
    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    // Implement 3D face brush functionality here
    if (mouseLeftNow && !game->getMouseLeftPressedLast())
    {
        glm::vec3 spawnPos;

        if (game->getPlacementMode() == Core::Game::BrushPlacementMode::InFront)
        {
            spawnPos = game->getCameraPos() + game->getCameraFront() * 2.0f;
        }
        else if (game->getPlacementMode() == Core::Game::BrushPlacementMode::ManualDistance)
        {
            if (!game->getManualAnchorValid())
            {
                game->setManualAnchorPos(game->getCameraPos() + game->getCameraFront() * 2.0f);
                game->setManualAnchorRight(glm::normalize(glm::cross(game->getCameraFront(), game->getCameraUp())));
                game->setManualAnchorUp(game->getCameraUp());
                game->setManualAnchorForward(game->getCameraFront());
                game->setManualAnchorValid(true);
            }
            spawnPos = game->getManualAnchorPos() + game->getManualAnchorRight() * game->getManualOffset().x + game->getManualAnchorUp() * game->getManualOffset().y + game->getManualAnchorForward() * game->getManualOffset().z;
        }
        else
        { // CursorSnap — place the new shape adjacent to the surface under the
          // cursor (or the crosshair when the cursor is locked).
            glm::vec3 rayO, rayDir;
            std::vector<Object*> targets;
            const auto &objects = mgr.active().world().getOwnedObjects();
            targets.reserve(objects.size());
            for (const auto &uptr : objects) if (uptr) targets.push_back(uptr.get());

            SurfaceHit hit;
            if (buildMouseRay(window, game, rayO, rayDir) && pickSurface(targets, rayO, rayDir, hit))
            {
                glm::vec3 half = glm::vec3(game->getBrushScale().x * game->getBrushSize(),
                                          game->getBrushScale().y * game->getBrushSize(),
                                          game->getBrushScale().z * game->getBrushSize()) * 0.5f;
                float offsetAmt = glm::dot(glm::abs(hit.normal), half) + 0.01f;
                spawnPos = hit.point + hit.normal * offsetAmt;
            }
            else
            {
                spawnPos = game->getCameraPos() + game->getCameraFront() * 2.0f;
            }
        }

        // Optional grid snapping for precision placement
        if (game->getBrushGridSnap() && game->getBrushGridSize() > 1e-6f)
        {
            spawnPos.x = std::round(spawnPos.x / game->getBrushGridSize()) * game->getBrushGridSize();
            spawnPos.y = std::round(spawnPos.y / game->getBrushGridSize()) * game->getBrushGridSize();
            spawnPos.z = std::round(spawnPos.z / game->getBrushGridSize()) * game->getBrushGridSize();
        }

        glm::mat4 t = glm::translate(glm::mat4(1.0f), spawnPos);
        glm::vec3 totalScale = glm::vec3(game->getBrushScale().x * game->getBrushSize(),
                                         game->getBrushScale().y * game->getBrushSize(),
                                         game->getBrushScale().z * game->getBrushSize());
        t = glm::scale(t, totalScale);

        if (targetPart) {
            // Convert world-space transform into body-part-local space
            glm::mat4 partWorld = targetPart->getTransform();
            glm::mat4 localT = glm::inverse(partWorld) * t;

            Object* sub = targetPart->addSubObject(game->getCurrentShapeKind(), localT);
            if (sub && game->getCurrentShapeKind() == Object::ShapeKind::Polyhedron) {
                sub->setPolyhedronData(game->buildCurrentPolyhedron());
            }
            if (sub) {
                for (int f = 0; f < sub->getFaces(); ++f)
                    sub->setFaceColor(f, game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2));
            }
        } else {
            std::unique_ptr<Object> obj(new Object());
            obj->setShape(game->getCurrentShapeKind());

            if (game->getCurrentShapeKind() == Object::ShapeKind::Polyhedron)
            {
                obj->setPolyhedronData(game->buildCurrentPolyhedron());
            }

            obj->setTransform(t);
            obj->updateCollisionZone(t);
            for (int f = 0; f < obj->getFaces(); ++f)
                obj->setFaceColor(f, game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2));
            mgr.active().world().addObject(std::move(obj));
        }
    }
}

void Tool::Pottery3D(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt,
                     const std::vector<Object*>& targets, const glm::mat4* avatarRoot)
{
    (void)mgr;
    // Implement 3D pottery functionality here
    // Pottery sculpting logic: modify existing object geometry by scaling along hit normal
    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow)
    {
        bool firstFrame = !game->getMouseLeftPressedLast();
        // Build picking ray (crosshair when the cursor is locked) and pick the
        // surface with the shared per-face raycast.
        glm::vec3 rayO, rayDir;
        SurfaceHit hit;
        if (buildMouseRay(window, game, rayO, rayDir) && pickSurface(targets, rayO, rayDir, hit))
        {
            Object *hitObj = hit.obj;
            int hitAxis = hit.axis;
            int hitSign = hit.sign;
            bool hitIsCube = hit.isCube;
            // Determine scale delta
            float dir = (game->getCurrentPotteryTool() == Core::Game::PotteryTool::Expand) ? 1.0f : -1.0f;
            float delta = dir * game->getPotteryStrength() * (firstFrame ? 1.0f : dt); // full step on click, smaller continuous after

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

void Tool::Rotate3D(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt,
                    const std::vector<Object*>& targets, const glm::mat4* avatarRoot)
{
    (void)mgr;
    // Drag state lives on Game (not function-local statics) so it can't leak
    // across tool or object switches.
    bool dragging = game->getRotateDragging();
    double lastCursorX = game->getRotateLastCursorX();
    double lastCursorY = game->getRotateLastCursorY();

    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    glm::vec3 rayOrigin(0.0f);
    glm::vec3 rayDir(0.0f, 0.0f, -1.0f);
    buildMouseRay(window, game, rayOrigin, rayDir);

    if (mouseLeftNow && !game->getMouseLeftPressedLast()) {
        if (Object* hit = pickNearestObject(targets, rayOrigin, rayDir)) {
            game->setSelectedObject3D(hit);
        }
        dragging = true;
        lastCursorX = cursorX;
        lastCursorY = cursorY;
    } else if (!mouseLeftNow) {
        dragging = false;
    }

    Object* selected = game->getSelectedObject3D();
    if (selected) {
        selected->setRotationResponsiveness(game->getRotationToolSmoothness());

        if (dragging && mouseLeftNow) {
            float dx = static_cast<float>(cursorX - lastCursorX);
            float dy = static_cast<float>(cursorY - lastCursorY);
            glm::vec3 deltaDegrees(0.0f);
            float sensitivity = game->getRotationToolSensitivity();

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                deltaDegrees.z = dx * sensitivity;
            } else {
                switch (game->getRotationAxisMode()) {
                    case Core::Game::RotationAxisMode::FreeXY:
                        deltaDegrees.x = -dy * sensitivity;
                        deltaDegrees.y = dx * sensitivity;
                        break;
                    case Core::Game::RotationAxisMode::X:
                        deltaDegrees.x = -dy * sensitivity;
                        break;
                    case Core::Game::RotationAxisMode::Y:
                        deltaDegrees.y = dx * sensitivity;
                        break;
                    case Core::Game::RotationAxisMode::Z:
                        deltaDegrees.z = dx * sensitivity;
                        break;
                    case Core::Game::RotationAxisMode::AuthoritativeAxis: {
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

    game->setRotateDragging(dragging);
    game->setRotateLastCursor(lastCursorX, lastCursorY);
}

void Tool::FacePaint(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt,
                     const std::vector<Object*>& targets)
{
    (void)mgr;
    (void)dt;
    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow && !game->getMouseLeftPressedLast())
    {
        // Build picking ray (crosshair when the cursor is locked).
        glm::vec3 rayO, rayDir;
        if (!buildMouseRay(window, game, rayO, rayDir)) return;

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
            // Check if advanced face paint is enabled
            if (game->isAdvancedFacePaintEnabled())
            {
                // Use advanced face paint system with current settings
                AdvancedFacePaint::GradientSettings* gradientSettings = game->getCurrentGradientSettings();
                AdvancedFacePaint::SmudgeSettings* smudgeSettings = game->getCurrentSmudgeSettings();
                
                bool success = AdvancedFacePaint::paintFaceAdvanced(hitObj, hitFace, hitUV, 
                                                                  gradientSettings, smudgeSettings);
                
                if (!success) {
                    // Fall back to basic fill if advanced painting fails
                    hitObj->fillFaceColor(hitFace, game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2));
                }
            }
            else
            {
                // Use basic fill for FacePaint click
                hitObj->fillFaceColor(hitFace, game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2));
            }
        }
    }
}

 void Tool::FaceBrush(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt,
                      const std::vector<Object*>& targets)
 {
    (void)mgr;
    (void)dt;
     bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
     if (mouseLeftNow)
    {
        // Continuous stroke painting while mouse button held.
        // Build picking ray (crosshair when the cursor is locked).
        glm::vec3 rayO, rayDir;
        if (!buildMouseRay(window, game, rayO, rayDir)) return;
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
            uv += glm::vec2(game->getFaceBrushUOffset(), game->getFaceBrushVOffset());
            uv = glm::clamp(uv, glm::vec2(0.0f), glm::vec2(1.0f));

            // Update brush cursor position
            game->setBrushCursorPos(uv);
            game->setBrushCursorVisible(true);

            // Calculate pressure simulation
            float pressure = game->getCurrentPressure();
            if (game->getUsePressureSimulation())
            {
                // Simulate pressure based on mouse speed and other factors
                float currentTime = static_cast<float>(glfwGetTime());
                if (game->getLastBrushTime() > 0.0f)
                {
                    float timeDelta = currentTime - game->getLastBrushTime();
                    if (timeDelta > 0.0f)
                    {
                        float speed = glm::length(uv - game->getLastBrushUV()) / timeDelta;
                        pressure = std::clamp(1.0f - speed * game->getPressureSensitivity(), 0.1f, 1.0f);
                    }
                }
                game->setLastBrushTime(currentTime);
            }

            // Apply brush based on type
            switch (game->getCurrentBrushType())
            {
            case Core::Game::PublicBrushType::Normal:
                // Interpolate only if staying on the same object and face
                if (game->getUseStrokeInterpolation() &&
                    game->getLastBrushUV().x >= 0.0f &&
                    game->getLastBrushObject() == hitObj &&
                    game->getLastBrushFace() == hitFace)
                {
                    hitObj->paintStroke(hitFace, game->getLastBrushUV(), uv,
                                        game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2),
                                        game->getFaceBrushRadius() * pressure, game->getFaceBrushSoftness(),
                                        game->getBrushOpacity(), game->getBrushSpacing());
                }
                else
                {
                    hitObj->paintFaceAdvanced(hitFace, uv,
                                              game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2),
                                              game->getFaceBrushRadius() * pressure, game->getFaceBrushSoftness(),
                                              game->getBrushOpacity(), game->getBrushFlow(), 0);
                }
                break;

            case Core::Game::PublicBrushType::Airbrush:
                 hitObj->airbrushFace(hitFace, uv,
                                      game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2),
                                      game->getFaceBrushRadius() * pressure, /*density*/ 0.5f, game->getBrushOpacity());
                break;

            case Core::Game::PublicBrushType::Chalk:
                hitObj->paintFaceAdvanced(hitFace, uv,
                                          game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2),
                                          game->getFaceBrushRadius() * pressure, game->getFaceBrushSoftness(),
                                          game->getBrushOpacity(), game->getBrushFlow(), 2);
                break;

            case Core::Game::PublicBrushType::Spray:
                hitObj->paintFaceAdvanced(hitFace, uv,
                                          game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2),
                                          game->getFaceBrushRadius() * pressure, game->getFaceBrushSoftness(),
                                          game->getBrushOpacity(), game->getBrushFlow(), 3);
                break;

            case Core::Game::PublicBrushType::Smudge:
                 hitObj->smudgeFace(hitFace, uv,
                                    game->getFaceBrushRadius() * pressure, /*strength*/ 0.5f);
                break;

            case Core::Game::PublicBrushType::Clone:
                if (game->getCloneToolActive())
                {
                    glm::vec2 sourceUV = uv + game->getCloneOffset();
                    hitObj->cloneFace(hitFace, uv, sourceUV,
                                      game->getFaceBrushRadius() * pressure, game->getBrushOpacity());
                }
                break;
            }

            // Remember last stroke context
            game->setLastBrushUV(uv);
            game->setLastBrushFace(hitFace);
            game->setLastBrushObject(hitObj);
        }
        else
        {
            game->setBrushCursorVisible(false);
        }
    }
    else
    {
        // Mouse released - reset stroke state
        game->setLastBrushUV(glm::vec2(-1.0f, -1.0f));
        game->setLastBrushFace(-1);
        game->setLastBrushObject(nullptr);
        game->setBrushCursorVisible(false);
    }
}

void Tool::Selection3D(GLFWwindow *window, Core::Game *game,
                       const std::vector<Object*>& targets)
{
    // Pick the object under the cursor (or crosshair when locked) and select it.
    // Uses the cached camera matrices via buildMouseRay rather than reading GL
    // matrix state during update().
    glm::vec3 rayO, rayDir;
    if (!buildMouseRay(window, game, rayO, rayDir)) return;
    game->setSelectedObject3D(pickNearestObject(targets, rayO, rayDir));
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
