#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testVoicePartCreation() {
    VoicePart part("VocalCords", VoicePart::Type::Resonance);
    check(part.getName() == "VocalCords", "name preserved");
    check(part.getType() == VoicePart::Type::Resonance, "type preserved");
    // Do not require identity == display name: independent identity work
    // must remain free to disambiguate same-named VoiceParts.
    check(!part.getIdentifier().empty(), "part has an identifier");
    check(part.isLiteral, "literal defaults to true");
    check(part.isSymbolic, "symbolic defaults to true");

    part.isLiteral = false;
    part.isSymbolic = false;
    check(!part.isLiteral && !part.isSymbolic, "flags are independently mutable");
}

void testDefaultVoicePart() {
    VoicePart part;
    check(part.getName().empty(), "default name is empty");
    check(part.getType() == VoicePart::Type::Undefined,
          "default type is undefined");
}
}

int main() {
    testVoicePartCreation();
    testDefaultVoicePart();
    std::cout << "voice_part_test: PASS\n";
    return 0;
}
