#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "json.hpp"

#include <cassert>
#include <cstdio>
#include <string>

int main() {
    // 1. Test "mass" attribute handling with valid number, valid string, and corrupt data structures
    {
        // Valid number mass
        {
            nlohmann::json j = {{"mass", 42.5}};
            Object obj;
            from_json(j, obj);
            assert(obj.hasAttribute("mass"));
            assert(obj.getAttribute("mass") == "42.500000");
        }

        // Valid string mass
        {
            nlohmann::json j = {{"mass", "100.0"}};
            Object obj;
            from_json(j, obj);
            assert(obj.hasAttribute("mass"));
            assert(obj.getAttribute("mass") == "100.0");
        }

        // Corrupt array mass - should be safely caught without throwing or setting mass
        {
            nlohmann::json j = {{"mass", nlohmann::json::array({1, 2, 3})}};
            Object obj;
            from_json(j, obj);
            assert(!obj.hasAttribute("mass"));
        }

        // Corrupt object mass - should be safely caught
        {
            nlohmann::json j = {{"mass", {{"key", "value"}}}};
            Object obj;
            from_json(j, obj);
            assert(!obj.hasAttribute("mass"));
        }

        // Corrupt boolean mass - should be safely caught
        {
            nlohmann::json j = {{"mass", true}};
            Object obj;
            from_json(j, obj);
            assert(!obj.hasAttribute("mass"));
        }

        // Corrupt null mass - should be safely caught
        {
            nlohmann::json j = {{"mass", nullptr}};
            Object obj;
            from_json(j, obj);
            assert(!obj.hasAttribute("mass"));
        }
    }

    // 2. Test invalid shapeKind fallback to Cube
    {
        nlohmann::json j = {
            {"shapeKind", 999999}
        };
        Object obj;
        from_json(j, obj);
        assert(obj.getShapeKind() == Object::ShapeKind::Cube);
    }

    // 3. Test attributes and tags edge cases & round-trip
    {
        nlohmann::json j = {
            {"objectID", "test-obj-1"},
            {"baseline", "custom_baseline"},
            {"attributes", {
                {"color_override", "blue"},
                {"density", "high"},
                {"invalid_num", 12345} // Non-string attribute values should be ignored by attributes loop
            }},
            {"tags", nlohmann::json::array({"tag1", "tag2", 123, true, nullptr})}, // Non-string tags should be ignored
            {"x2D", 250.0f},
            {"y2D", 350.0f},
            {"zOrder2D", 5},
            {"textString", "Hello Object"}
        };
        Object obj;
        from_json(j, obj);

        assert(obj.getIdentifier() == "test-obj-1");
        assert(obj.hasAttribute("baseline"));
        assert(obj.getAttribute("baseline") == "custom_baseline");
        assert(obj.hasAttribute("color_override"));
        assert(obj.getAttribute("color_override") == "blue");
        assert(obj.hasAttribute("density"));
        assert(obj.getAttribute("density") == "high");
        assert(!obj.hasAttribute("invalid_num"));

        assert(obj.getTags().size() == 2);
        assert(obj.getTags()[0] == "tag1");
        assert(obj.getTags()[1] == "tag2");

        assert(obj.getX2D() == 250.0f);
        assert(obj.getY2D() == 350.0f);
        assert(obj.getZOrder2D() == 5);
        assert(obj.getTextString() == "Hello Object");

        // Verify to_json output
        nlohmann::json jOut;
        to_json(jOut, obj);
        assert(jOut["objectID"] == "test-obj-1");
        assert(jOut["baseline"] == "custom_baseline");
        assert(jOut["attributes"]["color_override"] == "blue");
        assert(jOut["attributes"]["density"] == "high");
        assert(jOut["tags"].size() == 2);
        assert(jOut["x2D"] == 250.0f);
        assert(jOut["y2D"] == 350.0f);
        assert(jOut["zOrder2D"] == 5);
        assert(jOut["textString"] == "Hello Object");
    }

    std::puts("object_serialization_test: ALL OK");
    return 0;
}
