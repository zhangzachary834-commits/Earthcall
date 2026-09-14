#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "Singularity/Storage/Serialization/Serialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <iostream>
#include <vector>

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
    std::cout << "PASS: " << message << std::endl;
}

// Access the internal helper to test the boundary logic
namespace {
    bool selectorIncludes(const OntoMath::Piecewise& selector, float u, float v, const Object& subject) {
        const std::map<std::string, PropertyValue> vars{
            {"u", PropertyValue(static_cast<double>(u))},
            {"v", PropertyValue(static_cast<double>(v))},
        };
        return selector.evaluate(vars, &subject).has_value();
    }
}

int main() {
    // 1. Create a macro image object
    Object macro("image.landscape");
    
    // 2. Create an OntoMath piecewise selector representing a region (e.g. left half: u < 0.5)
    OntoMath::Piecewise selector;
    selector.inputVariable = "u";
    OntoMath::Piecewise::Piece leftHalf;
    leftHalf.hasHi = true;
    leftHalf.hi = 0.5;
    leftHalf.includeHi = false; // strictly less than 0.5
    // Need to assign a non-null mathNode so it evaluates as 'defined'
    leftHalf.mathNode = OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::constant(1.0));
    selector.pieces.push_back(leftHalf);
    
    // 3. Test mathematical sample membership
    // Check left side
    check(selectorIncludes(selector, 0.25f, 0.5f, macro), "u=0.25 is inside the region (u < 0.5)");
    // Check right side
    check(!selectorIncludes(selector, 0.75f, 0.5f, macro), "u=0.75 is outside the region (u < 0.5)");
    
    // 4. Create the micro Singular (Region)
    Object microRegion("region.left_half");
    microRegion.setDynamicProperty("selector", PropertyValue(selector.toJson().dump()));
    
    // 5. Connect them via a Relation
    Relation regionOf("relation.region-of", microRegion, macro, true);
    
    // 6. Verify the property can be retrieved and parsed
    PropertyValue readVal;
    check(microRegion.getDynamicProperty("selector", readVal), "Micro region holds the selector property");
    
    check(regionOf.a() == &microRegion && regionOf.b() == &macro, "Relation endpoints correctly link micro to macro");
    
    std::cout << "All OntoMath region decomposition tests passed!" << std::endl;
    return 0;
}
