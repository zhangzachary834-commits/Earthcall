#include "Person/Voice/Voice.hpp"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testVoicePartIdentityAndMembership() {
    Voice voice("natural");
    VoicePart first("Echo", VoicePart::Type::Timbre);
    VoicePart second("Echo", VoicePart::Type::Timbre);
    VoicePart third("Echo", VoicePart::Type::Resonance);
    first.setIdentifier("echo_one");
    second.setIdentifier("echo_two");
    third.setIdentifier("echo_three");

    voice.addPart(&first);
    voice.addPart(&second);
    voice.addPart(&third);

    require(voice.getVoicePart("echo_one") == &first, "first exact identity");
    require(voice.getVoicePart("echo_two") == &second, "second exact identity");
    require(voice.getVoicePart("echo_three") == &third, "third exact identity");
    require(first.getName() == second.getName(), "display labels may coincide");
    require(voice.formation.hasMember(&first), "first Formation member");
    require(voice.formation.hasMember(&second), "second Formation member");
    require(voice.getVoicePartsByType(VoicePart::Type::Timbre).size() == 2, "type filter preserves both");

    voice.removeVoicePart("echo_one");
    require(voice.getVoicePart("echo_one") == nullptr, "exact removal");
    require(!voice.formation.hasMember(&first), "exact removal releases Formation membership");
    require(voice.getVoicePart("echo_two") == &second, "second remains after exact removal");

    voice.removeVoicePart("Echo");
    require(voice.getVoicePart("echo_two") == nullptr, "fallback removes first remaining match");
    require(voice.getVoicePart("echo_three") == &third, "fallback does not erase all same-name parts");
    require(!voice.formation.hasMember(&second), "fallback releases Formation membership");
    require(voice.formation.hasMember(&third), "third Formation member retained");

    voice.removeVoicePart("echo_three");
    require(!voice.formation.hasMember(&third), "cleanup releases last member");
}
}

int main() {
    testVoicePartIdentityAndMembership();
    std::cout << "voice_identity_test: ALL OK\n";
    return 0;
}
