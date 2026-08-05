// Property bridge milestone test (LAW_AND_CREATION_SYSTEM.md, commit 2):
//   "set position.y by path → the cube actually moves."
//
// Exercises: PropertyPath resolution (dotted names, vec3 components),
// ComputedProperty (position via transform, rotation via euler accessors),
// PropertyRef (center, shape.* params), arithmetic coercion, and clean
// failure on unknown paths.

#include "Form/Object/Object.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PhysicsLawBridge.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    // Object's constructor initializes face textures (GL calls), so give the
    // test a real — but hidden — context.
    if (!glfwInit()) {
        std::fprintf(stderr, "property_bridge_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "property_bridge_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "property_bridge_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object obj;

        // 1. The milestone: position.y by path moves the object.
        assert(PropertyPath::parse("position.y").setValue(obj, PropertyValue(3.0f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(obj.getTransform()[3].y - 3.0f) < 1e-5f);

        // Whole-vector set.
        assert(PropertyPath::parse("position")
                   .setValue(obj, PropertyValue(glm::vec3(1.0f, 2.0f, 3.0f))) == PropertyPath::PathResult::Ok);
        assert(std::fabs(obj.getTransform()[3].x - 1.0f) < 1e-5f);
        assert(std::fabs(obj.getTransform()[3].z - 3.0f) < 1e-5f);

        // 2. Rotation round-trips through the euler accessors.
        assert(PropertyPath::parse("rotation")
                   .setValue(obj, PropertyValue(glm::vec3(0.0f, 90.0f, 0.0f))) == PropertyPath::PathResult::Ok);
        PropertyValue out;
        assert(PropertyPath::parse("rotation.y").getValue(obj, out) == PropertyPath::PathResult::Ok);
        assert(std::fabs(std::get<float>(out) - 90.0f) < 1e-3f);

        // 3. Shape params registered under dotted names.
        assert(PropertyPath::parse("shape.majorR").setValue(obj, PropertyValue(0.5f)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("shape.majorR").getValue(obj, out) == PropertyPath::PathResult::Ok);
        assert(std::fabs(std::get<float>(out) - 0.5f) < 1e-6f);

        // Arithmetic coercion: an int arriving for a float slot still lands.
        assert(PropertyPath::parse("shape.minorR").setValue(obj, PropertyValue(1)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("shape.minorR").getValue(obj, out) == PropertyPath::PathResult::Ok);
        assert(std::fabs(std::get<float>(out) - 1.0f) < 1e-6f);

        // center via PropertyRef.
        assert(PropertyPath::parse("center.z").setValue(obj, PropertyValue(0.25f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(obj.getCenter().z - 0.25f) < 1e-6f);

        // Material reference: an object points at a Material being by identifier,
        // and a Law can reassign it by name. Defaults to material.default.
        assert(PropertyPath::parse("material").getValue(obj, out) == PropertyPath::PathResult::Ok);
        assert(std::get<std::string>(out) == "material.default");
        assert(PropertyPath::parse("material").setValue(obj, PropertyValue(std::string("material.clay"))) == PropertyPath::PathResult::Ok);
        assert(obj.materialId() == "material.clay");

        // 4. Unknown multi-segment paths fail cleanly — no crash, nullptr/false.
        assert(PropertyPath::parse("nonexistent.thing").resolve(obj) == nullptr);
        // Setting a single-segment nonexistent property creates a dynamic property
        assert(PropertyPath::parse("nonexistent").setValue(obj, PropertyValue(1)) == PropertyPath::PathResult::Ok);
        PropertyValue unused;
        assert(PropertyPath::parse("nonexistent").getValue(obj, unused) == PropertyPath::PathResult::Ok);
        assert(std::get<int>(unused) == 1);
        assert(PropertyPath::parse("position.w").getValue(obj, unused) != PropertyPath::PathResult::Ok);

        // 5. Wider legibility: physical participation and color are
        //    governable state like everything else.
        assert(PropertyPath::parse("physical").getValue(obj, out) == PropertyPath::PathResult::Ok);
        assert(std::get<bool>(out) == true);
        assert(PropertyPath::parse("physical").setValue(obj, PropertyValue(false)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("physical").getValue(obj, out) == PropertyPath::PathResult::Ok);
        assert(std::get<bool>(out) == false);

        assert(PropertyPath::parse("color").setValue(
            obj, PropertyValue(glm::vec3(0.2f, 0.4f, 0.6f))) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("color.g").getValue(obj, out) == PropertyPath::PathResult::Ok);
        double g = 0.0;
        assert(propertyValueToNumber(out, g) && std::fabs(g - 0.4) < 1e-5);
        assert(PropertyPath::parse("color.b").setValue(obj, PropertyValue(0.9f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(obj.faceColors[0][2] - 0.9f) < 1e-5f);   // written through

        // 5b. The paintable skin, face by face: color per face, and the
        //     layer structure legible (buffers/strokes stay code-only).
        assert(PropertyPath::parse("face.2.color").setValue(
            obj, PropertyValue(glm::vec3(1.0f, 0.0f, 0.5f))) == PropertyPath::PathResult::Ok);
        assert(std::fabs(obj.faceColors[2][2] - 0.5f) < 1e-5f);
        assert(PropertyPath::parse("face.2.color.b").getValue(obj, out) == PropertyPath::PathResult::Ok);
        double fb = 0.0;
        assert(propertyValueToNumber(out, fb) && std::fabs(fb - 0.5) < 1e-5);
        assert(PropertyPath::parse("face.0.textureSize").getValue(obj, out) == PropertyPath::PathResult::Ok);
        double texSize = 0.0;
        assert(propertyValueToNumber(out, texSize) && texSize > 0.0);
        assert(PropertyPath::parse("face.0.textureSize")
                    .setValue(obj, PropertyValue(128)) != PropertyPath::PathResult::Ok);   // structure: tools, not slots
        assert(PropertyPath::parse("face.0.layerCount").getValue(obj, out) == PropertyPath::PathResult::Ok);
        // Layer controls engage once a face actually has layers.
        obj.faceTextures[0].addLayer();
        obj.faceTextures[0].addLayer();
        assert(PropertyPath::parse("face.0.useLayers").setValue(obj, PropertyValue(true)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("face.0.activeLayer").setValue(obj, PropertyValue(1)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("face.0.layerOpacity").setValue(obj, PropertyValue(0.25f)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("face.0.layerOpacity").getValue(obj, out) == PropertyPath::PathResult::Ok);
        double opacity = 0.0;
        assert(propertyValueToNumber(out, opacity) && std::fabs(opacity - 0.25) < 1e-5);

        // 5c. Shape writes REGENERATE geometry where the params are the
        //     form's truth; the KIND itself is governable (transmutation).
        Object orb;
        Object::ShapeParams orbParams;
        orbParams.r = 0.5f;
        orb.setShape(Object::ShapeKind::Sphere, orbParams);
        assert(orb.getSpatialKind() == Object::SpatialKind::SmoothSurface);
        assert(PropertyPath::parse("shape.r").setValue(orb, PropertyValue(1.2f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(orb.getShapeParams().r - 1.2f) < 1e-5f);
        assert(orb.getSpatialKind() == Object::SpatialKind::SmoothSurface);
        assert(orb.getShapeKind() == Object::ShapeKind::Sphere);   // regen, not demote

        // A cube's visible form is vertex data — param writes stay raw and
        // do NOT rebuild (repainting would be wiped for nothing).
        assert(PropertyPath::parse("shape.fillet").setValue(obj, PropertyValue(0.3f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(obj.getShapeParams().fillet - 0.3f) < 1e-5f);

        // Transmutation by law-text: the cube becomes a sphere.
        assert(PropertyPath::parse("shape.kind").setValue(
            obj, PropertyValue(static_cast<int>(Object::ShapeKind::Sphere))) == PropertyPath::PathResult::Ok);
        assert(obj.getShapeKind() == Object::ShapeKind::Sphere);
        assert(obj.getSpatialKind() == Object::SpatialKind::SmoothSurface);
        assert(PropertyPath::parse("shape.kind").setValue(
            obj, PropertyValue(10)) != PropertyPath::PathResult::Ok);   // Field needs a payload, not an int
        assert(PropertyPath::parse("shape.kind").setValue(obj, PropertyValue(-1)) != PropertyPath::PathResult::Ok);

        // 5d. Motion state is legible: velocity/mass read and write the
        //     physics engine's rigid body — collision RESPONSE can be law.
        assert(PropertyPath::parse("velocity").setValue(
            obj, PropertyValue(glm::vec3(0.0f, 5.0f, 0.0f))) == PropertyPath::PathResult::Ok);
        assert(std::fabs(Physics::getBodyFor(&obj).velocity.y - 5.0f) < 1e-5f);
        assert(PropertyPath::parse("velocity.y").setValue(obj, PropertyValue(-2.0f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(Physics::getBodyFor(&obj).velocity.y + 2.0f) < 1e-5f);
        assert(PropertyPath::parse("mass").setValue(obj, PropertyValue(2.5f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(Physics::getBodyFor(&obj).mass - 2.5f) < 1e-5f);
        assert(PropertyPath::parse("mass").setValue(obj, PropertyValue(0.0)) != PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("mass").setValue(obj, PropertyValue(-1.0)) != PropertyPath::PathResult::Ok);

        // 6. Physics first movers are legible laws: the bridge's properties
        //    read and write the ENGINE, and an ordinary law governs gravity.
        Physics::PhysicsLaw gravity;
        gravity.name = "Test Gravity";
        gravity.type = Physics::LawType::Gravity;
        gravity.strength = 9.81f;
        const int gravityId = Physics::addLaw(gravity);

        LawManager mgr;
        PhysicsLawBridge::syncRegister(mgr);
        Law* bridge = nullptr;
        for (const auto& law : mgr.getAll()) {
            auto* b = dynamic_cast<PhysicsLawBridge*>(law.get());
            if (b && b->physicsLawName() == "Test Gravity") bridge = law.get();
        }
        assert(bridge != nullptr);
        assert(bridge->isFirstMover());

        assert(PropertyPath::parse("strength").getValue(*bridge, out) == PropertyPath::PathResult::Ok);
        double strength = 0.0;
        assert(propertyValueToNumber(out, strength) && std::fabs(strength - 9.81) < 1e-4);

        // Govern gravity through ordinary law-text.
        Object author;
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(bridge);
        });
        Law softer("soften-gravity");
        softer.addAuthor(author);
        softer.setActionModel(ActionNode::set(
            "@" + bridge->getIdentifier() + ".strength", PropertyValue(3.0)));
        assert(softer.applyTo(obj) == Law::ApplicationResult::Applied);
        assert(std::fabs(Physics::getLawById(gravityId)->strength - 3.0f) < 1e-5f);

        Law calmer("disable-gravity");
        calmer.addAuthor(author);
        calmer.setActionModel(ActionNode::set(
            "@" + bridge->getIdentifier() + ".enabled", PropertyValue(false)));
        assert(calmer.applyTo(obj) == Law::ApplicationResult::Applied);
        assert(!Physics::getLawById(gravityId)->enabled);

        // Bridges never serialize (their truth lives in the engine), and a
        // world load preserves them.
        const auto managerJson = mgr.toJson();
        assert(managerJson["laws"].empty());
        mgr.loadFromJson(managerJson);
        bool stillBridged = false;
        for (const auto& law : mgr.getAll()) {
            auto* b = dynamic_cast<PhysicsLawBridge*>(law.get());
            if (b && b->physicsLawName() == "Test Gravity") stillBridged = true;
        }
        assert(stillBridged);

        Universe::instance().setProvider({});
        Physics::removeLaw(gravityId);

        // 7. A Person is legible: position by path; name read-only.
        Soul soul("Witness");
        Body avatar = Body::createBasicAvatar("Voxel");
        Person person(soul, std::move(avatar), "default");
        assert(person.getIdentifier() == "Witness");
        assert(PropertyPath::parse("position.y").setValue(person, PropertyValue(4.0f)) == PropertyPath::PathResult::Ok);
        assert(std::fabs(person.position.y - 4.0f) < 1e-5f);
        assert(PropertyPath::parse("name").getValue(person, out) == PropertyPath::PathResult::Ok);
        assert(std::get<std::string>(out) == "Witness");
        assert(PropertyPath::parse("name").setValue(
            person, PropertyValue(std::string("Usurper"))) != PropertyPath::PathResult::Ok);   // identity is not a slot
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("property_bridge_test: ALL OK");
    return 0;
}
