#include "Tool.hpp"
#include "Core/Game.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "GLFW/glfw3.h"
#include "AdvancedFacePaint.hpp"
#include <iostream>

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

    // Brush Implementation
    if (type == Type::Brush)
    {
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
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            float radius = 16.0f;
            auto &strokes = zone.strokes;
            for (auto it = strokes.begin(); it != strokes.end();)
            {
                bool erase = false;
                const auto &pts = it->points;
                if (pts.size() >= 4)
                {
                    for (size_t i = 0; i + 3 < pts.size(); i += 2)
                    {
                        // line segment p->q
                        float px = pts[i], py = pts[i + 1];
                        float qx = pts[i + 2], qy = pts[i + 3];
                        // distance from mouse to segment
                        float vx = qx - px;
                        float vy = qy - py;
                        float wx = game.getCursorX() - px;
                        float wy = game.getCursorY() - py;
                        float len2 = vx * vx + vy * vy;
                        float t = len2 > 0 ? (vx * wx + vy * wy) / len2 : 0.0f;
                        if (t < 0.0f)
                            t = 0.0f;
                        else if (t > 1.0f)
                            t = 1.0f;
                        float projX = px + t * vx;
                        float projY = py + t * vy;
                        float dx = game.getCursorX() - projX;
                        float dy = game.getCursorY() - projY;
                        if (dx * dx + dy * dy < radius * radius)
                        {
                            erase = true;
                            break;
                        }
                    }
                }

                if (erase)
                    it = strokes.erase(it);
                else
                    ++it;
            }
        }
    }
    else if (type == Type::Line)
    {
        static float startX = 0, startY = 0;
        static bool drawing = false;
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            startX = game.getCursorX();
            startY = game.getCursorY();
            drawing = true;
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && drawing)
        {
            zone.startStroke(startX, startY);
            zone.continueStroke(game.getCursorX(), game.getCursorY());
            zone.endStroke();
            drawing = false;
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

        static float startX = 0, startY = 0;
        static bool drawing = false;

        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            startX = game.getCursorX();
            startY = game.getCursorY();
            drawing = true;
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && drawing)
        {
            float width = abs(game.getCursorX() - startX);
            float height = abs(game.getCursorY() - startY);

            if (width > 5.0f && height > 5.0f)
            { // Minimum size threshold
                zone.addDesignShape(type, startX, startY, width, height);
            }

            drawing = false;
        }

        // Color picker tool
    }
    else if (type == Type::ColorPicker)
    {
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
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
        static std::vector<glm::vec2> selectionPoints;
        static bool selecting = false;
        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            selectionPoints.clear();
            selectionPoints.push_back(glm::vec2(game.getCursorX(), game.getCursorY()));
            selecting = true;
        }
        else if (mouseLeftNow && game.getMouseLeftPressedLast() && selecting)
        {
            selectionPoints.push_back(glm::vec2(game.getCursorX(), game.getCursorY()));
        }
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && selecting)
        {
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
            selecting = false;
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
            std::string text = "Text " + std::to_string(textCounter++);
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

        static glm::vec2 transformStart = glm::vec2(0, 0);
        static bool transforming = false;

        if (mouseLeftNow && !game.getMouseLeftPressedLast())
        {
            transformStart = glm::vec2(game.getCursorX(), game.getCursorY());
            transforming = true;

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
        else if (mouseLeftNow && game.getMouseLeftPressedLast() && transforming)
        {
            // Update transform based on mouse movement
            glm::vec2 delta = glm::vec2(game.getCursorX(), game.getCursorY()) - transformStart;

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
        else if (!mouseLeftNow && game.getMouseLeftPressedLast() && transforming)
        {
            transforming = false;
        }
    }
}

void Tool::ShapeGenerator3D(GLFWwindow *window, Core::Game *game, ZoneManager &mgr)
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
        { // CursorSnap
            // Ray originating from cursor into scene
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            double winX = xpos;
            double winY = ypos;
            int winW, winH;
            glfwGetWindowSize(window, &winW, &winH);
            int fW, fH;
            glfwGetFramebufferSize(window, &fW, &fH);
            float scaleX = static_cast<float>(fW) / winW;
            float scaleY = static_cast<float>(fH) / winH;
            winX *= scaleX;
            winY *= scaleY;
            winY = game->getCameraViewport()[3] - winY; // invert Y for OpenGL

            GLdouble nearX, nearY, nearZ, farX, farY, farZ;
            gluUnProject(winX, winY, 0.0, game->getCameraModelview(), game->getCameraProjection(), game->getCameraViewport(), &nearX, &nearY, &nearZ);
            gluUnProject(winX, winY, 1.0, game->getCameraModelview(), game->getCameraProjection(), game->getCameraViewport(), &farX, &farY, &farZ);
            glm::vec3 rayO(nearX, nearY, nearZ);
            glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayO);

            float nearestT = 1e9f;
            int hitAxis = -1;
            int hitSign = 1;
            Object *hitObj = nullptr;
            bool hitIsCube = false; // track geometry for normal computation
            const auto &objects = mgr.active().world().getOwnedObjects();
            for (const auto &uptr : objects)
            {
                Object *obj = uptr.get();

                if (obj->getGeometryType() == Object::GeometryType::Cube)
                {
                    // --- Existing AABB intersection in object local space ---
                    glm::mat4 inv = glm::inverse(obj->getTransform());
                    glm::vec3 oL = glm::vec3(inv * glm::vec4(rayO, 1.0f));
                    glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(rayDir, 0.0f)));
                    float tMin = -1e9f, tMax = 1e9f;
                    int axis = -1;
                    int sign = 1;
                    for (int a = 0; a < 3; ++a)
                    {
                        float o = oL[a], d = dL[a];
                        float t1, t2;
                        if (fabs(d) < 1e-6f)
                        {
                            if (o < -0.5f || o > 0.5f)
                            {
                                tMin = 1e9f;
                                break;
                            }
                            t1 = -1e9f;
                            t2 = 1e9f;
                        }
                        else
                        {
                            t1 = (-0.5f - o) / d;
                            t2 = (0.5f - o) / d;
                        }
                        if (t1 > t2)
                            std::swap(t1, t2);
                        if (t1 > tMin)
                        {
                            tMin = t1;
                            axis = a;
                            sign = (d > 0 ? -1 : 1);
                        }
                        if (t2 < tMax)
                            tMax = t2;
                        if (tMin > tMax)
                        {
                            tMin = 1e9f;
                            break;
                        }
                    }
                    if (tMin < nearestT && tMin > 0 && tMin < 1e8f)
                    {
                        nearestT = tMin;
                        hitAxis = axis;
                        hitSign = sign;
                        hitObj = obj;
                        hitIsCube = true;
                    }
                }
                else
                {
                    // --- Bounding-sphere intersection for non-cube primitives ---
                    glm::vec3 centerWorld = glm::vec3(obj->getTransform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    // Extract world scale along each axis from transform columns
                    glm::vec3 colX = glm::vec3(obj->getTransform()[0]);
                    glm::vec3 colY = glm::vec3(obj->getTransform()[1]);
                    glm::vec3 colZ = glm::vec3(obj->getTransform()[2]);
                    float scaleX = glm::length(colX);
                    float scaleY = glm::length(colY);
                    float scaleZ = glm::length(colZ);
                    float radius = 0.5f * std::max(scaleX, std::max(scaleY, scaleZ));

                    glm::vec3 oc = rayO - centerWorld;
                    float b = glm::dot(oc, rayDir);
                    float c = glm::dot(oc, oc) - radius * radius;
                    float h = b * b - c;
                    if (h >= 0.0f)
                    {
                        h = std::sqrt(h);
                        float t = -b - h; // nearer root
                        if (t < 0.0f)
                            t = -b + h; // if inside sphere
                        if (t > 0.0f && t < nearestT)
                        {
                            nearestT = t;
                            hitObj = obj;
                            hitIsCube = false;
                        }
                    }
                }
            }
            if (nearestT < 1e8f && hitObj)
            {
                glm::vec3 hitPoint = rayO + rayDir * nearestT;
                glm::vec3 nWorld;
                if (hitIsCube)
                {
                    glm::vec3 nLocal(0.0f);
                    nLocal[hitAxis] = static_cast<float>(hitSign);
                    nWorld = glm::normalize(glm::vec3(hitObj->getTransform() * glm::vec4(nLocal, 0.0f)));
                }
                else
                {
                    glm::vec3 centerWorld = glm::vec3(hitObj->getTransform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    nWorld = glm::normalize(hitPoint - centerWorld);
                }
                glm::vec3 half = glm::vec3(game->getBrushScale().x * game->getBrushSize(), game->getBrushScale().y * game->getBrushSize(), game->getBrushScale().z * game->getBrushSize()) * 0.5f;
                float offsetAmt = glm::dot(glm::abs(nWorld), half) + 0.01f;
                spawnPos = hitPoint + nWorld * offsetAmt;
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

        // Create a new Object
        std::unique_ptr<Object> obj(new Object());
        obj->setGeometryType(game->getCurrentPrimitive());

        // Add this Object to the Game's active zone object Formation/list
        // game->getActiveZone().addObject(std::move(obj)); // Removed for now

        // 3D Face Brush
        // Initialize polyhedron data if needed
        if (game->getCurrentPrimitive() == Object::GeometryType::Polyhedron)
        {
            if (game->getUseCustomPolyhedron() && !game->getCustomPolyhedronVertices().empty())
            {
                // Use custom polyhedron
                obj->setPolyhedronData(Object::PolyhedronData::createCustomPolyhedron(
                    game->getCustomPolyhedronVertices(), game->getCustomPolyhedronFaces()));
            }
            else
            {
                // Use concave variant based on selection
                switch (game->getCurrentConcaveType())
                {
                case 0: // Regular
                    obj->setPolyhedronData(Object::PolyhedronData::createRegularPolyhedron(game->getCurrentPolyhedronType()));
                    break;
                case 1: // Concave
                    obj->setPolyhedronData(Object::PolyhedronData::createConcavePolyhedron(game->getCurrentPolyhedronType(), 0.5f, game->getConcavityAmount()));
                    break;
                case 2: // Star
                    obj->setPolyhedronData(Object::PolyhedronData::createStarPolyhedron(game->getCurrentPolyhedronType(), 0.5f, game->getSpikeLength()));
                    break;
                case 3: // Crater
                    obj->setPolyhedronData(Object::PolyhedronData::createCraterPolyhedron(game->getCurrentPolyhedronType(), 0.5f, game->getCraterDepth()));
                    break;
                default:
                    obj->setPolyhedronData(Object::PolyhedronData::createRegularPolyhedron(game->getCurrentPolyhedronType()));
                    break;
                }
            }
        }

        glm::mat4 t = glm::translate(glm::mat4(1.0f), spawnPos);
        glm::vec3 totalScale = glm::vec3(game->getBrushScale().x * game->getBrushSize(),
                                         game->getBrushScale().y * game->getBrushSize(),
                                         game->getBrushScale().z * game->getBrushSize());
        t = glm::scale(t, totalScale);
        obj->setTransform(t);
        obj->updateCollisionZone(t);
        for (int f = 0; f < 6; ++f)
            obj->setFaceColor(f, game->getCurrentColor(0), game->getCurrentColor(1), game->getCurrentColor(2));
        mgr.active().world().addObject(std::move(obj));
    }
}

void Tool::Pottery3D(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt)
{
    // Implement 3D pottery functionality here
    // Pottery sculpting logic: modify existing object geometry by scaling along hit normal
    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow)
    {
        bool firstFrame = !game->getMouseLeftPressedLast();
        // Build picking ray similar to FacePaint
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        double winX = xpos;
        double winY = ypos;
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        int fW, fH;
        glfwGetFramebufferSize(window, &fW, &fH);
        float scaleX = static_cast<float>(fW) / winW;
        float scaleY = static_cast<float>(fH) / winH;
        winX *= scaleX;
        winY *= scaleY;
        winY = game->getCameraViewport()[3] - winY;
        GLdouble nearX, nearY, nearZ, farX, farY, farZ;
        gluUnProject(winX, winY, 0.0, game->getCameraModelview(), game->getCameraProjection(), game->getCameraViewport(), &nearX, &nearY, &nearZ);
        gluUnProject(winX, winY, 1.0, game->getCameraModelview(), game->getCameraProjection(), game->getCameraViewport(), &farX, &farY, &farZ);
        glm::vec3 rayO(nearX, nearY, nearZ);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayO);

        float nearestT = 1e9f;
        Object *hitObj = nullptr;
        int hitAxis = -1;
        int hitSign = 1;
        bool hitIsCube = false;
        const auto &objects = mgr.active().world().getOwnedObjects();
        for (const auto &uptr : objects)
        {
            Object *obj = uptr.get();
            if (obj->getGeometryType() == Object::GeometryType::Cube)
            {
                glm::mat4 inv = glm::inverse(obj->getTransform());
                glm::vec3 oL = glm::vec3(inv * glm::vec4(rayO, 1.0f));
                glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(rayDir, 0.0f)));
                float tMin = -1e9f, tMax = 1e9f;
                int axis = -1;
                int sign = 1;
                for (int a = 0; a < 3; ++a)
                {
                    float o = oL[a], d = dL[a];
                    float t1, t2;
                    if (fabs(d) < 1e-6f)
                    {
                        if (o < -0.5f || o > 0.5f)
                        {
                            tMin = 1e9f;
                            break;
                        }
                        t1 = -1e9f;
                        t2 = 1e9f;
                    }
                    else
                    {
                        t1 = (-0.5f - o) / d;
                        t2 = (0.5f - o) / d;
                    }
                    if (t1 > t2)
                        std::swap(t1, t2);
                    if (t1 > tMin)
                    {
                        tMin = t1;
                        axis = a;
                        sign = (d > 0 ? -1 : 1);
                    }
                    if (t2 < tMax)
                        tMax = t2;
                    if (tMin > tMax)
                    {
                        tMin = 1e9f;
                        break;
                    }
                }
                if (tMin < nearestT && tMin > 0 && tMin < 1e8f)
                {
                    nearestT = tMin;
                    hitAxis = axis;
                    hitSign = sign;
                    hitObj = obj;
                    hitIsCube = true;
                }
            }
            else
            {
                // Bounding sphere for other primitives
                glm::vec3 centerWorld = glm::vec3(obj->getTransform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                glm::vec3 colX = glm::vec3(obj->getTransform()[0]);
                glm::vec3 colY = glm::vec3(obj->getTransform()[1]);
                glm::vec3 colZ = glm::vec3(obj->getTransform()[2]);
                float radius = 0.5f * std::max({glm::length(colX), glm::length(colY), glm::length(colZ)});
                glm::vec3 oc = rayO - centerWorld;
                float b = glm::dot(oc, rayDir);
                float c = glm::dot(oc, oc) - radius * radius;
                float h = b * b - c;
                if (h >= 0.0f)
                {
                    h = std::sqrt(h);
                    float t = -b - h;
                    if (t < 0.0f)
                        t = -b + h;
                    if (t > 0.0f && t < nearestT)
                    {
                        nearestT = t;
                        hitObj = obj;
                        hitIsCube = false;
                    }
                }
            }
        }

        if (hitObj)
        {
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
            hitObj->setTransform(newT);
            hitObj->updateCollisionZone(newT);
        }
    }
}

void Tool::FacePaint(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt)
{
    bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow && !game->getMouseLeftPressedLast())
    {
        // Build picking ray
        // Ensure camera matrices and viewport are current for this frame
        const GLint *viewport = game->getCameraViewport();
        const GLdouble *modelview = game->getCameraModelview();
        const GLdouble *projection = game->getCameraProjection();
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        int fW, fH;
        glfwGetFramebufferSize(window, &fW, &fH);
        float scaleX = static_cast<float>(fW) / winW;
        float scaleY = static_cast<float>(fH) / winH;
        double winX = xpos * scaleX;
        double winY = ypos * scaleY;
        winY = viewport[3] - winY;
        GLdouble nearX, nearY, nearZ, farX, farY, farZ;
        gluUnProject(winX, winY, 0.0, modelview, projection, viewport, &nearX, &nearY, &nearZ);
        gluUnProject(winX, winY, 1.0, modelview, projection, viewport, &farX, &farY, &farZ);
        glm::vec3 rayO(nearX, nearY, nearZ);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayO);

        float nearestT = 1e9f;
        Object *hitObj = nullptr;
        int hitFace = -1;
        glm::vec2 hitUV(0.0f);
        const auto &objects = mgr.active().world().getOwnedObjects();
        for (const auto &up : objects)
        {
            Object *obj = up.get();
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

 void Tool::FaceBrush(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt)
 {
     bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
     if (mouseLeftNow)
    {
        // Continuous stroke painting while mouse button held
        // Build picking ray (same as FacePaint)
        // Ensure camera matrices and viewport are current for this frame
         const GLint* viewport = game->getCameraViewport();
         const GLdouble* modelview = game->getCameraModelview();
         const GLdouble* projection = game->getCameraProjection();
         double xpos, ypos; glfwGetCursorPos(window, &xpos, &ypos);
         int winW, winH; glfwGetWindowSize(window,&winW,&winH);
         int fW, fH; glfwGetFramebufferSize(window,&fW,&fH);
         float scaleX = static_cast<float>(fW)/winW;
         float scaleY = static_cast<float>(fH)/winH;
         double winX = xpos * scaleX;
         double winY = ypos * scaleY;
         winY = viewport[3] - winY;
        GLdouble nearX, nearY, nearZ, farX, farY, farZ;
         gluUnProject(winX, winY, 0.0, modelview, projection, viewport, &nearX, &nearY, &nearZ);
         gluUnProject(winX, winY, 1.0, modelview, projection, viewport, &farX, &farY, &farZ);
        glm::vec3 rayO(nearX, nearY, nearZ);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayO);
        float nearestT = 1e9f;
        Object *hitObj = nullptr;
        int hitFace = -1;
        glm::vec2 uv(0.0f);
        const auto &objects = mgr.active().world().getOwnedObjects();
        for (const auto &up : objects)
        {
            Object *obj = up.get();
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

void Tool::Selection3D(GLFWwindow *window, Core::Game *game, ZoneManager &mgr, float dt)
{
    // Implement 3D selection functionality here
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