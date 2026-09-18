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
    const unsigned char png_data[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x15, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x64, 0x60, 0x60, 0xf8, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0x04, 0x22, 0x40, 0x18, 0x00, 0x0e, 0x28, 0x01, 0x03, 0x92, 0xc1, 0x42, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

    auto [obj, mat] = ImageCodecChannel::ingestPngFromMemory(png_data, sizeof(png_data), "witness_img", *activeZone);
    assert(obj != nullptr && mat != nullptr);
    activeZone->addObject(obj);

    // Create a property "witness_passed"
    obj->setDynamicProperty("witness_passed", PropertyValue(false));

    // Create Law
    auto law = std::make_shared<Law>("witness.law");
    law->addAuthor(*obj);
    
    glm::vec3 red(1.0f, 0.0f, 0.0f);
    auto expectedRedList = std::make_shared<PropertyList>();
    for (int i = 0; i < 2; i++) expectedRedList->elements.push_back(PropertyValue(red));
    
    ConditionNode cond = ConditionNode::compare("regionB", ConditionNode::Op::Eq, PropertyValue(expectedRedList));
    law->setConditionModel(cond);

    ActionNode act = ActionNode::set("witness_passed", PropertyValue(true));
    law->setActionModel(act);
    law->setActivation(Law::Activation::WhileTrue);

    harness.lawManager.add(law);

    std::string reason;

    // regionA: v - 0.5 <= 0
    auto selectorA_json = nlohmann::json::parse(R"({"input": "v", "pieces": [{"where": {"op": 5, "children": [{"op": 1, "var": "v"}, {"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}]}, "mathNode": {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {}}]}}}, {"guard": {"kind": 3, "children": []}}]})");
    OntoMath::Piecewise selectorA = OntoMath::Piecewise::fromJson(selectorA_json);

    bool successA = obj->elevateSurfaceRegionProperty("regionA", 0, selectorA, reason);
    if (!successA) std::cout << "FAIL REASON A: " << reason << "\n";
    assert(successA);

    // regionB: u - 0.5 <= 0
    auto selectorB_json = nlohmann::json::parse(R"({"input": "u", "pieces": [{"where": {"op": 5, "children": [{"op": 1, "var": "u"}, {"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}]}, "mathNode": {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {}}]}}}, {"guard": {"kind": 3, "children": []}}]})");
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
    std::cout << "setValue result: " << (int)result << "\n";
    assert(result == PropertyPath::PathResult::Ok);

    // Pump the engine (processes ChangeFeed -> Law)
    harness.lawManager.tick();

    // Law shouldn't be satisfied because regionB is unchanged
    PropertyValue val;
    assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);
    assert(std::get<bool>(val) == false);

    // Now write texels directly via rendering/Surface write
    obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.25f), red); // Top-left
    bool ret2 = obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.75f), red); // Bottom-left
    

    // Read the same region (regionB) through PropertyPath
    PropertyValue readB;
    obj->readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern("regionB"), readB);
    auto readBList = std::get<std::shared_ptr<PropertyList>>(readB);
    assert(readBList->elements.size() == 2);
    
    // In order for the Rete to see the change, we must simulate the screen channel notifying
    // that regionB changed (since we bypassed `setValue`)
    obj->notifyPropertyChanged(obj.get(), "regionB");

    // Pump the engine again
    harness.lawManager.tick();

    // Verify Law was satisfied
    assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);

    if (std::get<bool>(val) != true) {
        std::cout << "regionB actual:\n";
        for (auto& e : readBList->elements) {
            auto v = std::get<glm::vec3>(e);
            std::cout << v.x << ", " << v.y << ", " << v.z << "\n";
        }
        std::cout << "expected:\n";
        for (auto& e : expectedRedList->elements) {
            auto v = std::get<glm::vec3>(e);
            std::cout << v.x << ", " << v.y << ", " << v.z << "\n";
        }
        assert(std::get<bool>(val) == true);
    }

    
    std::cout << "TEST PASSED" << std::endl;
    return 0;
}
