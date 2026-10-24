#include "Person/Voice/Voice.hpp"
#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testVoiceCreation() {
    Voice voice("Melodic");
    check(voice.style == "Melodic", "explicit voice style");
    check(voice.getIdentifier() == "Melodic_voice", "voice identifier");
    check(voice.parts.empty(), "new voice starts without parts");

    Voice defaultVoice;
    check(defaultVoice.style == "natural", "default voice style");
}

void testPartQueriesAndRemoval() {
    // Parts are owned by the surrounding scope, not Voice. Declare them
    // first so Voice (including its non-owning Formation) dies before them.
    VoicePart timbre("Timbre", VoicePart::Type::Timbre);
    VoicePart pitch("Pitch", VoicePart::Type::Pitch);
    VoicePart secondTimbre("Bright", VoicePart::Type::Timbre);
    Voice voice("Melodic");

    voice.addPart(nullptr);
    check(voice.parts.empty(), "null part is ignored");

    voice.addPart(&timbre);
    voice.addPart(&pitch);
    voice.addPart(&secondTimbre);
    check(voice.parts.size() == 3, "all non-null parts added");
    check(voice.formation.hasMember(&timbre), "part registered in Formation");
    check(voice.getVoicePart("Timbre") == &timbre, "lookup returns original part");
    check(voice.getVoicePart("Missing") == nullptr, "missing name returns null");

    const std::vector<VoicePart*> timbres =
        voice.getVoicePartsByType(VoicePart::Type::Timbre);
    check(timbres.size() == 2, "type query includes both matching parts");
    check(timbres[0] == &timbre && timbres[1] == &secondTimbre,
          "type query preserves insertion order");
    check(voice.getVoicePartsByType(VoicePart::Type::Resonance).empty(),
          "absent type returns empty result");

    voice.removeVoicePart("Timbre");
    check(voice.getVoicePart("Timbre") == nullptr, "removed name is absent");
    check(voice.parts.size() == 2, "removal retains other parts");
    check(voice.getVoicePart("Pitch") == &pitch, "other type remains");
    check(voice.getVoicePart("Bright") == &secondTimbre, "other matching type remains");
    voice.removeVoicePart("Missing");
    check(voice.parts.size() == 2, "removing missing name is a no-op");
}

void testBasicVoiceFactory() {
    Voice basic = Voice::createBasicVoice("Harsh");
    check(basic.style == "Harsh", "factory preserves requested style");
    check(basic.parts.empty(), "current basic factory creates no parts");
}
}

int main() {
    testVoiceCreation();
    testPartQueriesAndRemoval();
    testBasicVoiceFactory();
    std::cout << "voice_test: PASS\n";
    return 0;
}
