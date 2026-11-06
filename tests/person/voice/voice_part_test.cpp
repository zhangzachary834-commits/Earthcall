#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <iostream>
#include <cassert>

using namespace Earthcall;

void testBasicCreation() {
    VoicePart p("vocal_cords", VoicePart::Type::Pitch);

    assert(p.getName() == "vocal_cords");
    assert(p.getType() == VoicePart::Type::Pitch);
}

int main() {
    testBasicCreation();
    std::cout << "All VoicePart tests passed." << std::endl;
    return 0;
}
