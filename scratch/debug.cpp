
#include "tests/support/test_harness.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <iostream>

int main() {
    TestSupport::BootedEngineHarness harness("Zach");
    Object obj;
    std::cout << "Before findProperty: " << obj.findProperty("surface.pixel.0.3.4") << std::endl;
    std::cout << "recognizes: " << obj.recognizesAuthoredPropertyProjection(Earthcall::StringInterner::intern("surface.pixel.0.3.4")) << std::endl;
    return 0;
}
