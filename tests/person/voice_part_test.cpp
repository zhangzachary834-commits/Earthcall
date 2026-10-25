#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <iostream>

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "PASSED: " << message << std::endl;
}

void testVoicePartCreation() {
    VoicePart part("VocalCords", VoicePart::Type::Resonance);

    check(part.getName() == "VocalCords", "getName returns correct name");
    check(part.getType() == VoicePart::Type::Resonance, "getType returns correct type");
    check(part.getIdentifier() == "VocalCords", "getIdentifier matches name");
    check(part.isLiteral == true, "VoicePart is literal by default");
    check(part.isSymbolic == true, "VoicePart is symbolic by default");
}

int main() {
    std::cout << "Running voice_part_test...\n";
    testVoicePartCreation();
    std::cout << "All voice_part_test assertions passed.\n";
    return 0;
}
