#include "support/test_harness.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ImageCodecChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include <cassert>
#include <iostream>

extern MaterialManager materials;

int main() {
    TestSupport::BootedEngineHarness harness;
    auto activeZone = harness.zones.zones()[harness.zones.currentIndex()];
    
    // A 4x4 PNG image (white)
    const unsigned char png_data[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x14, 0x49, 0x44, 0x41, 0x54, 0x08, 0x5b, 0x63, 0xfc, 0xcf, 0xc0, 0x00, 0x44, 0x0c, 0x0c, 0x8c, 0x30, 0x06, 0xb2, 0x00, 0x00, 0x39, 0x01, 0x01, 0xfd, 0xef, 0x3a, 0x63, 0x93, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    auto [obj, mat] = ImageCodecChannel::ingestPngFromMemory(png_data, sizeof(png_data), "witness_img", *activeZone);
    assert(obj != nullptr && mat != nullptr);

    // Create a property "witness_passed"
    obj->setDynamicProperty("witness_passed", PropertyValue(false));

    // Create Law
    auto law = std::make_shared<Law>("witness.law");
    
    glm::vec3 red(1.0f, 0.0f, 0.0f);
    auto expectedRedList = std::make_shared<PropertyList>();
    for (int i = 0; i < 2; i++) expectedRedList->elements.push_back(PropertyValue(red));
    
    ConditionNode cond = ConditionNode::compare("regionB", ConditionNode::Op::Eq, PropertyValue(expectedRedList));
    law->setConditionModel(cond);

    ActionNode act = ActionNode::set("witness_passed", PropertyValue(true));
    law->setActionModel(act);

    harness.lawManager.add(law);

    std::string reason;

    // regionA: v - 0.5 <= 0
    auto selectorA_json = nlohmann::json::parse(R"({"input":"x","pieces":[{"where":{"children":[{"op":1,"var":"v"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}},{"guard":{"children":[],"kind":3}}]})");
    OntoMath::Piecewise selectorA = OntoMath::Piecewise::fromJson(selectorA_json);

    bool successA = obj->elevateSurfaceRegionProperty("regionA", 0, selectorA, reason);
    assert(successA);

    // regionB: u - 0.5 <= 0
    auto selectorB_json = nlohmann::json::parse(R"({"input":"x","pieces":[{"where":{"children":[{"op":1,"var":"u"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}},{"guard":{"children":[],"kind":3}}]})");
    OntoMath::Piecewise selectorB = OntoMath::Piecewise::fromJson(selectorB_json);

    bool successB = obj->elevateSurfaceRegionProperty("regionB", 0, selectorB, reason);
    assert(successB);

    auto redList = std::make_shared<PropertyList>();
    for (int i = 0; i < 2; i++) redList->elements.push_back(PropertyValue(red));
    
    // Write the region through the ordinary Law/PropertyPath route
    std::cout << "Material Id: " << obj->materialId() << ", in cache? " << (materials.get(obj->materialId()) != nullptr) << "\n";
    std::cout << "Selection A ok? " << successA << "\n";
    PropertyValue beforeVal;
    auto getRes = PropertyPath::parse("regionA").getValue(*obj, beforeVal);
        auto it2 = obj->dynamicProperties().find(Earthcall::StringInterner::intern("regionA"));
        std::cout << "regionA exists: " << (it2 != obj->dynamicProperties().end()) << "\n";

        std::cout << "Wait! Does surface.selection.regionA exist?\n";
        auto it = obj->dynamicProperties().find(Earthcall::StringInterner::intern("surface.selection.regionA"));
        std::cout << "exists: " << (it != obj->dynamicProperties().end()) << "\n";

        auto mat_resolve = materials.resolveOrDefault(obj->materialId());
        std::cout << "mat found: " << (mat_resolve != nullptr) << "\n";
        if (mat_resolve) {
            std::cout << "face size: " << mat_resolve->faceTextures.size() << "\n";
            if (mat_resolve->faceTextures.size() > 0) {
                const auto& ft = mat_resolve->faceTextures[0];
                std::cout << "width: " << ft.width << ", height: " << ft.height << ", pixels.size: " << ft.pixels.size() << "\n";
                std::cout << "expected: " << (size_t)ft.width * ft.height * 4 << "\n";
            }
        }


        

    std::cout << "regionA getValue result: " << (getRes == PropertyPath::PathResult::Ok) << "\n";
    if (getRes == PropertyPath::PathResult::Ok) {
        std::cout << "Is PropertyList: " << std::holds_alternative<std::shared_ptr<PropertyList>>(beforeVal) << "\n";
    }

    auto result = PropertyPath::parse("regionA").setValue(*obj, PropertyValue(redList));
    assert(result == PropertyPath::PathResult::Ok);

    // Pump the engine (processes ChangeFeed -> Law)
    harness.lawManager.tick();

    // Law shouldn't be satisfied because regionB is unchanged
    PropertyValue val;
    assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);
    assert(std::get<bool>(val) == false);

    // Now write texels directly via rendering/Surface write
    obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.25f), red); // Top-left
    obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.75f), red); // Bottom-left

    // Read the same region (regionB) through PropertyPath
    PropertyValue readB;
    assert(PropertyPath::parse("regionB").getValue(*obj, readB) == PropertyPath::PathResult::Ok);
    auto readBList = std::get<std::shared_ptr<PropertyList>>(readB);
    assert(readBList->elements.size() == 2);
    
    // In order for the Rete to see the change, we must simulate the screen channel notifying
    // that regionB changed (since we bypassed `setValue`)
    obj->notifyPropertyChanged(obj.get(), "regionB");

    // Pump the engine again
    harness.lawManager.tick();

    // Verify Law was satisfied
    assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);
    assert(std::get<bool>(val) == true);
    
    std::cout << "TEST PASSED" << std::endl;
    return 0;
}
