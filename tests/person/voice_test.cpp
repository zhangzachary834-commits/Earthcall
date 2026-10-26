#include "Person/Voice/Voice.hpp"
#include "Person/Voice/VoicePart/VoicePart.hpp"

#include <iostream>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << "\n";
        return;
    }
    std::cout << "  ok: " << description << "\n";
}

void run_tests() {
    std::cout << "=== Voice Test ===\n";

    Voice voice("Melodic");
    check(voice.style == "Melodic", "Initial style");

    VoicePart* part1 = new VoicePart("Pitch", VoicePart::Type::Pitch);
    VoicePart* part2 = new VoicePart("Timbre", VoicePart::Type::Timbre);

    voice.addPart(part1);
    voice.addPart(part2);

    check(voice.parts.size() == 2, "Voice has 2 parts");
    check(voice.getVoicePart("Pitch") == part1, "Retrieve part 1");
    check(voice.getVoicePart("Timbre") == part2, "Retrieve part 2");
    check(voice.getVoicePart("Volume") == nullptr, "Retrieve non-existent part");

    voice.style = "Harsh";
    check(voice.style == "Harsh", "Updated style");
    check(voice.getVoicePartsByType(VoicePart::Type::Pitch).size() == 1, "Pitch type query");
    check(voice.getVoicePartsByType(VoicePart::Type::Timbre).size() == 1, "Timbre type query");
    voice.removeVoicePart("Pitch");
    check(voice.parts.size() == 1, "Removed part from Voice");
    check(voice.getVoicePart("Pitch") == nullptr, "Removed part is not returned");
    check(voice.getVoicePart("Timbre") == part2, "Other part preserved");

    delete part1;
    delete part2;
}

} // namespace

int main() {
    run_tests();

    std::cout << "\nTest summary: " << (g_checks - g_failures) << "/" << g_checks << " passed.\n";
    return g_failures > 0 ? 1 : 0;
}
