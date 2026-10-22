#include "Person/Voice/Voice.hpp"
#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <cassert>
#include <iostream>

static void testVoicePartIdentityAndLookup() {
    Voice voice("Melodic");

    auto* part1 = new VoicePart("Pitch", VoicePart::Type::Pitch);
    auto* part2 = new VoicePart("Pitch", VoicePart::Type::Pitch);

    // Display names match
    assert(part1->getName() == "Pitch");
    assert(part2->getName() == "Pitch");

    // Identifiers are distinct Formation IDs
    assert(!part1->getIdentifier().empty());
    assert(!part2->getIdentifier().empty());
    assert(part1->getIdentifier() != part2->getIdentifier());

    part1->setIdentifier("voice.pitch.base");
    part2->setIdentifier("voice.pitch.overtone");

    voice.addPart(part1);
    voice.addPart(part2);

    assert(voice.getVoicePart("voice.pitch.base") == part1);
    assert(voice.getVoicePart("voice.pitch.overtone") == part2);

    // Removing by identifier removes part1 and leaves part2
    voice.removeVoicePart("voice.pitch.base");
    assert(voice.getVoicePart("voice.pitch.base") == nullptr);
    assert(voice.getVoicePart("voice.pitch.overtone") == part2);

    delete part1;
    delete part2;

    std::cout << "  testVoicePartIdentityAndLookup OK\n";
}

int main() {
    std::cout << "Running voice_test...\n";
    testVoicePartIdentityAndLookup();
    std::cout << "voice_test: ALL OK\n";
    return 0;
}
