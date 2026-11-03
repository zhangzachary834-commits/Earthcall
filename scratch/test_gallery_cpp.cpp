#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream f("/Users/zacharyzhang/Documents/GitHub/Earthcall/scratch/gallery_piecewise.json");
    if (!f.is_open()) {
        std::cerr << "Failed to open json" << std::endl;
        return 1;
    }
    nlohmann::json j;
    f >> j;
    
    OntoMath::Piecewise pw = OntoMath::Piecewise::fromJson(j);
    std::cout << "Parsed Piecewise, pieces count: " << pw.pieces.size() << std::endl;
    
    auto layout = sdfwgsl::inspectScalarExpression(&pw);
    std::cout << "layout.ok: " << (layout.ok ? "true" : "false") << std::endl;
    if (!layout.ok) {
        std::cout << "layout.error: " << layout.error << std::endl;
        return 1;
    }
    std::cout << "layout.parameterCount: " << layout.parameterCount << std::endl;
    std::cout << "layout.structure length: " << layout.structure.size() << std::endl;
    std::cout << "WGSL structure snippet:\n" << layout.structure.substr(0, 500) << "...\n" << std::endl;

    // CPU evaluations
    auto evalAt = [&](double x, double y, double z) {
        std::map<std::string, PropertyValue> vars;
        vars["x"] = PropertyValue(x);
        vars["y"] = PropertyValue(y);
        vars["z"] = PropertyValue(z);
        vars["p"] = PropertyValue(glm::vec3(x, y, z));
        auto res = pw.evaluate(vars);
        if (!res) {
            std::cout << "Eval at (" << x << "," << y << "," << z << "): nullopt" << std::endl;
            return -1.0;
        }
        double val = 0.0;
        propertyValueToNumber(*res, val);
        std::cout << "Eval at (" << x << "," << y << "," << z << "): " << val << std::endl;
        return val;
    };

    std::cout << "--- Room 1 (Baseline Uniform) ---" << std::endl;
    evalAt(-40.0, -9.5, 0.0);
    evalAt(-45.0, -9.5, 4.0);

    std::cout << "--- Room 2 (Strong Falloff) ---" << std::endl;
    double r2_center = evalAt(-20.0, -9.5, 0.0);
    double r2_near = evalAt(-20.0, -9.5, 1.5);
    double r2_far = evalAt(-20.0, -9.5, 6.0);
    if (!(r2_center > r2_near && r2_near > r2_far)) {
        std::cerr << "FAIL: Room 2 falloff ordering failed!" << std::endl;
        return 1;
    }

    std::cout << "--- Room 3 (Halo / Shell) ---" << std::endl;
    double r3_core = evalAt(0.0, -9.5, 0.0);
    double r3_ring1 = evalAt(0.0, -9.5, 2.5);
    double r3_valley = evalAt(0.0, -9.5, 4.0);
    double r3_ring2 = evalAt(0.0, -9.5, 5.5);
    if (!(r3_ring1 > r3_core && r3_ring1 > r3_valley && r3_ring2 > r3_valley)) {
        std::cerr << "FAIL: Room 3 halo shells failed!" << std::endl;
        return 1;
    }

    std::cout << "--- Room 4 (Multi-Lobe) ---" << std::endl;
    double r4_lobe1 = evalAt(20.0, -9.5, 4.0);
    double r4_lobe2 = evalAt(16.5, -9.5, -3.0);
    double r4_lobe3 = evalAt(23.5, -9.5, -3.0);
    double r4_mid = evalAt(20.0, -9.5, 0.0);
    if (!(r4_lobe1 > r4_mid && r4_lobe2 > r4_mid && r4_lobe3 > r4_mid)) {
        std::cerr << "FAIL: Room 4 multi-lobes failed!" << std::endl;
        return 1;
    }

    std::cout << "--- Room 5 (Procedural / Cathedral) ---" << std::endl;
    evalAt(40.0, -9.5, 0.0);
    evalAt(42.0, -9.5, 2.0);
    evalAt(38.0, -9.5, -3.0);

    std::cout << "ALL GALLERY AST CHECKS PASSED!" << std::endl;
    return 0;
}
