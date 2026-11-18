#include "Person/Voice/Voice.hpp"
#include "Person/Voice/VoicePart/VoicePart.hpp"
#include <iostream>
#include <cassert>

using namespace Earthcall;

void testBasicCreation() {
    Voice v("robotic");
    assert(v.style == "robotic");
}

void testCreateBasicVoice() {
    Voice v = Voice::createBasicVoice("whisper");
    assert(v.style == "whisper");
}

void testPartsManagement() {
    Voice v("natural");

    VoicePart* p1 = new VoicePart("larynx", VoicePart::Type::Pitch);
    VoicePart* p2 = new VoicePart("tongue", VoicePart::Type::Articulation);

    v.addPart(p1);
    v.addPart(p2);

    assert(v.getVoicePart("larynx") == p1);
    assert(v.getVoicePart("tongue") == p2);
    assert(v.getVoicePart("unknown") == nullptr);

    auto pitchParts = v.getVoicePartsByType(VoicePart::Type::Pitch);
    assert(pitchParts.size() == 1);
    assert(pitchParts[0] == p1);

    v.removeVoicePart("larynx");
    assert(v.getVoicePart("larynx") == nullptr);
    delete p1;

    v.removeVoicePart("tongue");
    delete p2;
}

int main() {
    testBasicCreation();
    testCreateBasicVoice();
    testPartsManagement();
    std::cout << "All Voice tests passed." << std::endl;
    return 0;
}
