#include "Person/Voice/Voice.hpp"
#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <iostream>

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "PASSED: " << message << std::endl;
}

void testVoiceCreation() {
    Voice voice("Melodic");
    check(voice.style == "Melodic", "Voice style is set correctly");
}

void testAddAndGetVoicePart() {
    Voice voice("Melodic");
    auto* timbre = new VoicePart("Timbre", VoicePart::Type::Timbre);

    voice.addPart(timbre);

    VoicePart* retrieved = voice.getVoicePart("Timbre");
    check(retrieved != nullptr, "Added voice part can be retrieved by name");
    check(retrieved == timbre, "Retrieved voice part is the same as the added one");

    std::vector<VoicePart*> timbres = voice.getVoicePartsByType(VoicePart::Type::Timbre);
    check(timbres.size() == 1, "getVoicePartsByType returns correct number of parts");
    check(timbres[0] == timbre, "getVoicePartsByType returns the correct part");
    delete timbre;
}

void testRemoveVoicePart() {
    Voice voice("Melodic");
    auto* timbre = new VoicePart("Timbre", VoicePart::Type::Timbre);

    voice.addPart(timbre);
    voice.removeVoicePart("Timbre");

    check(voice.getVoicePart("Timbre") == nullptr, "Removed voice part is no longer found");
    delete timbre;
}

void testBasicVoiceCreation() {
    Voice basicVoice = Voice::createBasicVoice("Harsh");
    check(basicVoice.style == "Harsh", "Basic voice style is Harsh");
    for (auto* p : basicVoice.parts) {
        delete p;
    }
}

int main() {
    std::cout << "Running voice_test...\n";
    testVoiceCreation();
    testAddAndGetVoicePart();
    testRemoveVoicePart();
    testBasicVoiceCreation();
    std::cout << "All voice_test assertions passed.\n";
    return 0;
}
