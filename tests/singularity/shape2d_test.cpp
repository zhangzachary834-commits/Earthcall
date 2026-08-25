// Tests for Shape2D: the first screen-space 2D object kind.
//
// Verifies:
//   1. A Shape2D object has is2D() == true
//   2. is2D() false for a 3D shape
//   3. getRect2D() returns the correct AABB from x2D, y2D, width2D, height2D
//   4. Property paths x2D / y2D / zOrder2D resolve and write back correctly
//   5. 2D picking: pointer inside AABB hits, pointer outside misses
//   6. zOrder2D: higher z wins when two 2D rects overlap
//   7. raycastFace returns false for Shape2D (3D ray never hits a screen-space object)
//
// The test exercises InteractionChannel::observe() with a hand-built Sense
// (no GLFW, no window) — the same approach as interaction_channel_test.cpp.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using Singularity::Input::InteractionChannel;

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool ok, const char* msg) {
    if (ok) {
        ++g_passed;
        std::printf("  ok: %s\n", msg);
    } else {
        ++g_failed;
        std::printf("  FAIL: %s\n", msg);
    }
}

} // namespace

int main() {
    std::printf("Running shape2d_test...\n");

    // -------------------------------------------------------------------
    // [1] is2D() true for Shape2D
    // -------------------------------------------------------------------
    {
        Object obj;
        obj.setObjectID("test.s2d.1");
        obj.setShape(Object::ShapeKind::Shape2D, Object::ShapeParams{});
        check(obj.is2D(), "[1] Shape2D.is2D() is true");
    }

    // -------------------------------------------------------------------
    // [2] is2D() false for a default cube
    // -------------------------------------------------------------------
    {
        Object obj;
        obj.setObjectID("test.s2d.2");
        check(!obj.is2D(), "[2] Cube.is2D() is false");
    }

    // -------------------------------------------------------------------
    // [3] getRect2D() returns correct AABB: {x, y, x+w, y+h}
    // -------------------------------------------------------------------
    {
        Object obj;
        obj.setObjectID("test.s2d.3");
        Object::ShapeParams p;
        p.width2D  = 200.0f;
        p.height2D = 80.0f;
        obj.setShape(Object::ShapeKind::Shape2D, p);
        obj.setX2D(50.0f);
        obj.setY2D(120.0f);

        const glm::vec4 rect = obj.getRect2D();
        check(std::fabs(rect.x - 50.0f)  < 0.01f, "[3] rect.x == x2D");
        check(std::fabs(rect.y - 120.0f) < 0.01f, "[3] rect.y == y2D");
        check(std::fabs(rect.z - 250.0f) < 0.01f, "[3] rect.z == x2D + width2D");
        check(std::fabs(rect.w - 200.0f) < 0.01f, "[3] rect.w == y2D + height2D");
    }

    // -------------------------------------------------------------------
    // [4] x2D / y2D / zOrder2D resolve via PropertyPath and write back
    // -------------------------------------------------------------------
    {
        Object obj;
        obj.setObjectID("test.s2d.4");
        Object::ShapeParams p;
        p.width2D = 100.0f; p.height2D = 50.0f;
        obj.setShape(Object::ShapeKind::Shape2D, p);
        obj.setX2D(10.0f);
        obj.setY2D(20.0f);
        obj.setZOrder2D(3);

        // Read x2D via property path.
        {
            PropertyValue v;
            const auto res = PropertyPath::parse("x2D").getValue(obj, v);
            check(res == PropertyPath::PathResult::Ok, "[4] x2D resolves");
            double n = 0.0;
            propertyValueToNumber(v, n);
            check(std::fabs(static_cast<float>(n) - 10.0f) < 0.01f, "[4] x2D reads back 10");
        }

        // Write y2D via property path.
        {
            PropertyValue v;
            const auto res = PropertyPath::parse("y2D").getValue(obj, v);
            check(res == PropertyPath::PathResult::Ok, "[4] y2D resolves");

            const auto setRes = PropertyPath::parse("y2D").setValue(obj, PropertyValue(55.0f));
            check(setRes == PropertyPath::PathResult::Ok, "[4] y2D setValue returns Ok");
            check(std::fabs(obj.getY2D() - 55.0f) < 0.01f, "[4] y2D writes via property path");
        }

        // Read zOrder2D via property path.
        {
            PropertyValue v;
            const auto res = PropertyPath::parse("zOrder2D").getValue(obj, v);
            check(res == PropertyPath::PathResult::Ok, "[4] zOrder2D resolves");
            double n = 0.0;
            propertyValueToNumber(v, n);
            check(static_cast<int>(n) == 3, "[4] zOrder2D reads back 3");
        }
    }

    // -------------------------------------------------------------------
    // [5] 2D pick: pointer inside AABB hits; pointer outside misses
    // -------------------------------------------------------------------
    {
        LawManager laws;
        InteractionChannel::syncRegister(laws);
        InteractionChannel* channel = InteractionChannel::find(laws);
        assert(channel);

        Object btn;
        btn.setObjectID("test.s2d.btn");
        Object::ShapeParams p;
        p.width2D = 100.0f; p.height2D = 40.0f;
        btn.setShape(Object::ShapeKind::Shape2D, p);
        btn.setX2D(200.0f);
        btn.setY2D(300.0f);
        // AABB: x in [200, 300], y in [300, 340]

        std::vector<Object*> reachable = { &btn };

        // Pointer inside.
        {
            InteractionChannel::Sense s;
            s.pointerX = 250.0f;
            s.pointerY = 320.0f;
            s.rayOrigin    = glm::vec3(0.0f, 0.0f, 100.0f);
            s.rayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
            channel->observe(s, reachable);
            check(channel->hoveredId == std::string("test.s2d.btn"),
                  "[5] pointer inside AABB: hovered");
        }

        // Pointer outside.
        {
            InteractionChannel::Sense s;
            s.pointerX = 100.0f;
            s.pointerY = 100.0f;
            s.rayOrigin    = glm::vec3(0.0f, 0.0f, 100.0f);
            s.rayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
            channel->observe(s, reachable);
            check(channel->hoveredId.empty(), "[5] pointer outside AABB: not hovered");
        }
    }

    // -------------------------------------------------------------------
    // [6] zOrder2D: higher z wins when two 2D rects overlap
    // -------------------------------------------------------------------
    {
        LawManager laws2;
        InteractionChannel::syncRegister(laws2);
        InteractionChannel* channel = InteractionChannel::find(laws2);
        assert(channel);

        Object back, front;
        back.setObjectID("test.s2d.back");
        front.setObjectID("test.s2d.front");

        Object::ShapeParams p;
        p.width2D = 100.0f; p.height2D = 100.0f;
        back.setShape(Object::ShapeKind::Shape2D, p);
        back.setX2D(0.0f); back.setY2D(0.0f);
        back.setZOrder2D(0);

        front.setShape(Object::ShapeKind::Shape2D, p);
        front.setX2D(0.0f); front.setY2D(0.0f);
        front.setZOrder2D(5);

        std::vector<Object*> reachable = { &back, &front };

        InteractionChannel::Sense s;
        s.pointerX = 50.0f;
        s.pointerY = 50.0f;
        s.rayOrigin    = glm::vec3(0.0f, 0.0f, 100.0f);
        s.rayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        channel->observe(s, reachable);
        check(channel->hoveredId == std::string("test.s2d.front"),
              "[6] higher zOrder2D wins when rects overlap");
    }

    // -------------------------------------------------------------------
    // [7] raycastFace returns false for Shape2D
    // -------------------------------------------------------------------
    {
        Object obj;
        obj.setObjectID("test.s2d.7");
        obj.setShape(Object::ShapeKind::Shape2D, Object::ShapeParams{});
        float t = 0.0f;
        int face = -1;
        glm::vec2 uv;
        const bool hit = obj.raycastFace(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            t, face, uv);
        check(!hit, "[7] raycastFace returns false for Shape2D");
    }

    std::printf("\nshape2d_test: %d/%d passed\n", g_passed, g_passed + g_failed);
    return g_failed == 0 ? 0 : 1;
}
