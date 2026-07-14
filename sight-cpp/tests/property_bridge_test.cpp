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
        assert(PropertyPath::parse("position.y").setValue(obj, PropertyValue(3.0f)));
        assert(std::fabs(obj.getTransform()[3].y - 3.0f) < 1e-5f);

        // Whole-vector set.
        assert(PropertyPath::parse("position")
                   .setValue(obj, PropertyValue(glm::vec3(1.0f, 2.0f, 3.0f))));
        assert(std::fabs(obj.getTransform()[3].x - 1.0f) < 1e-5f);
        assert(std::fabs(obj.getTransform()[3].z - 3.0f) < 1e-5f);

        // 2. Rotation round-trips through the euler accessors.
        assert(PropertyPath::parse("rotation")
                   .setValue(obj, PropertyValue(glm::vec3(0.0f, 90.0f, 0.0f))));
        PropertyValue out;
        assert(PropertyPath::parse("rotation.y").getValue(obj, out));
        assert(std::fabs(std::get<float>(out) - 90.0f) < 1e-3f);

        // 3. Shape params registered under dotted names.
        assert(PropertyPath::parse("shape.majorR").setValue(obj, PropertyValue(0.5f)));
        assert(PropertyPath::parse("shape.majorR").getValue(obj, out));
        assert(std::fabs(std::get<float>(out) - 0.5f) < 1e-6f);

        // Arithmetic coercion: an int arriving for a float slot still lands.
        assert(PropertyPath::parse("shape.minorR").setValue(obj, PropertyValue(1)));
        assert(PropertyPath::parse("shape.minorR").getValue(obj, out));
        assert(std::fabs(std::get<float>(out) - 1.0f) < 1e-6f);

        // center via PropertyRef.
        assert(PropertyPath::parse("center.z").setValue(obj, PropertyValue(0.25f)));
        assert(std::fabs(obj.getCenter().z - 0.25f) < 1e-6f);

        // 4. Unknown paths fail cleanly — no crash, nullptr/false.
        assert(PropertyPath::parse("nonexistent.thing").resolve(obj) == nullptr);
        assert(!PropertyPath::parse("nonexistent").setValue(obj, PropertyValue(1)));
        PropertyValue unused;
        assert(!PropertyPath::parse("position.w").getValue(obj, unused));

        // 5. Wider legibility: physical participation and color are
        //    governable state like everything else.
        assert(PropertyPath::parse("physical").getValue(obj, out));
        assert(std::get<bool>(out) == true);
        assert(PropertyPath::parse("physical").setValue(obj, PropertyValue(false)));
        assert(PropertyPath::parse("physical").getValue(obj, out));
        assert(std::get<bool>(out) == false);

        assert(PropertyPath::parse("color").setValue(
            obj, PropertyValue(glm::vec3(0.2f, 0.4f, 0.6f))));
        assert(PropertyPath::parse("color.g").getValue(obj, out));
        double g = 0.0;
        assert(propertyValueToNumber(out, g) && std::fabs(g - 0.4) < 1e-5);
        assert(PropertyPath::parse("color.b").setValue(obj, PropertyValue(0.9f)));
        assert(std::fabs(obj.faceColors[0][2] - 0.9f) < 1e-5f);   // written through

        // 5b. The paintable skin, face by face: color per face, and the
        //     layer structure legible (buffers/strokes stay code-only).
        assert(PropertyPath::parse("face.2.color").setValue(
            obj, PropertyValue(glm::vec3(1.0f, 0.0f, 0.5f))));
        assert(std::fabs(obj.faceColors[2][2] - 0.5f) < 1e-5f);
        assert(PropertyPath::parse("face.2.color.b").getValue(obj, out));
        double fb = 0.0;
        assert(propertyValueToNumber(out, fb) && std::fabs(fb - 0.5) < 1e-5);
        assert(PropertyPath::parse("face.0.textureSize").getValue(obj, out));
        double texSize = 0.0;
        assert(propertyValueToNumber(out, texSize) && texSize > 0.0);
        assert(!PropertyPath::parse("face.0.textureSize")
                    .setValue(obj, PropertyValue(128)));   // structure: tools, not slots
        assert(PropertyPath::parse("face.0.layerCount").getValue(obj, out));
        // Layer controls engage once a face actually has layers.
        obj.faceTextures[0].addLayer();
        obj.faceTextures[0].addLayer();
        assert(PropertyPath::parse("face.0.useLayers").setValue(obj, PropertyValue(true)));
        assert(PropertyPath::parse("face.0.activeLayer").setValue(obj, PropertyValue(1)));
        assert(PropertyPath::parse("face.0.layerOpacity").setValue(obj, PropertyValue(0.25f)));
        assert(PropertyPath::parse("face.0.layerOpacity").getValue(obj, out));
        double opacity = 0.0;
        assert(propertyValueToNumber(out, opacity) && std::fabs(opacity - 0.25) < 1e-5);

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

        assert(PropertyPath::parse("strength").getValue(*bridge, out));
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
        Person person(soul, avatar);
        assert(person.getIdentifier() == "Witness");
        assert(PropertyPath::parse("position.y").setValue(person, PropertyValue(4.0f)));
        assert(std::fabs(person.position.y - 4.0f) < 1e-5f);
        assert(PropertyPath::parse("name").getValue(person, out));
        assert(std::get<std::string>(out) == "Witness");
        assert(!PropertyPath::parse("name").setValue(
            person, PropertyValue(std::string("Usurper"))));   // identity is not a slot
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("property_bridge_test: ALL OK");
    return 0;
}
