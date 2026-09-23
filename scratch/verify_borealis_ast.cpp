#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <zone.json>\n";
        return 1;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Cannot open " << argv[1] << "\n";
        return 1;
    }
    nlohmann::json j;
    in >> j;

    std::cout << "Loaded " << argv[1] << " successfully.\n";
    if (!j.contains("spatialFields") || !j["spatialFields"].is_array()) {
        std::cerr << "No spatialFields found!\n";
        return 1;
    }

    int fieldIdx = 0;
    for (const auto& sf : j["spatialFields"]) {
        const std::string id = sf.value("id", "unknown");
        std::cout << "\nTesting field [" << fieldIdx++ << "]: " << id << "\n";

        OntoMath::Piecewise density;
        OntoMath::Piecewise extinction;
        OntoMath::Piecewise scattering;
        OntoMath::Piecewise volumeChroma;

        const OntoMath::Piecewise* dPtr = nullptr;
        const OntoMath::Piecewise* ePtr = nullptr;
        const OntoMath::Piecewise* sPtr = nullptr;
        const OntoMath::Piecewise* cPtr = nullptr;

        if (sf.contains("volumeDensity")) {
            density = OntoMath::Piecewise::fromJson(sf["volumeDensity"]);
            dPtr = &density;
        }
        if (sf.contains("volumeExtinction")) {
            extinction = OntoMath::Piecewise::fromJson(sf["volumeExtinction"]);
            ePtr = &extinction;
        }
        if (sf.contains("volumeScattering")) {
            scattering = OntoMath::Piecewise::fromJson(sf["volumeScattering"]);
            sPtr = &scattering;
        }
        if (sf.contains("volumeChroma")) {
            volumeChroma = OntoMath::Piecewise::fromJson(sf["volumeChroma"]);
            cPtr = &volumeChroma;
        }

        auto dLayout = sdfwgsl::inspectDensityExpression(dPtr);
        std::cout << "  Density inspect: " << (dLayout.ok ? "OK" : "FAILED: " + dLayout.error) << "\n";

        auto eLayout = sdfwgsl::inspectExtinctionExpression(ePtr);
        std::cout << "  Extinction inspect: " << (eLayout.ok ? "OK" : "FAILED: " + eLayout.error) << "\n";

        auto sLayout = sdfwgsl::inspectScatteringExpression(sPtr);
        std::cout << "  Scattering inspect: " << (sLayout.ok ? "OK" : "FAILED: " + sLayout.error) << "\n";

        auto cLayout = sdfwgsl::inspectVolumeChromaExpression(cPtr);
        std::cout << "  Chroma inspect: " << (cLayout.ok ? "OK" : "FAILED: " + cLayout.error) << "\n";

        auto prog = sdfwgsl::compileVolume(dPtr, ePtr, sPtr, cPtr);
        std::cout << "  compileVolume: " << (prog.ok ? "OK" : "FAILED: " + prog.error) << "\n";
        if (!prog.ok) {
            std::cerr << "Compilation failed: " << prog.error << "\n";
            return 1;
        }
        std::cout << "  WGSL byte size: " << prog.wgsl.size() << ", params: " << prog.params.size() << "\n";
    }

    std::cout << "\nALL FIELDS COMPILED SUCCESSFULLY!\n";
    return 0;
}
