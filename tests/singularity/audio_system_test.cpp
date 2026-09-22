#include "Singularity/Audio/AudioSystem.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

extern ZoneManager mgr;

int main() {
    auto& audio = Core::Audio::AudioSystem::instance();

    // Setup active zone in ZoneManager for testing tick()
    auto testZone = std::make_shared<Zone>("AudioTestZone", "default");
    auto emitterObj = std::make_shared<Object>();
    emitterObj->setName("SoundEmitterObject");
    emitterObj->setDynamicProperty("acoustic.isSoundEmitter", PropertyValue(std::string("true")));
    testZone->addObject(emitterObj);
    mgr.addZone(testZone);

    // 1. Initialization and Shutdown
    bool initResult = audio.init();
    if (!initResult) {
        std::printf("AudioSystem failed to initialize, which might be expected in a headless CI environment.\n");
    } else {
        std::printf("AudioSystem initialized successfully.\n");

        // 2. Test Sound Emitter Parameter Parsing & Exception Handling in tick()
        // Scenario A: Non-numeric / invalid strings for frequency, amplitude, lowpassCutoff
        emitterObj->setDynamicProperty("acoustic.frequency", PropertyValue(std::string("not_a_number")));
        emitterObj->setDynamicProperty("acoustic.amplitude", PropertyValue(std::string("invalid_amplitude")));
        emitterObj->setDynamicProperty("acoustic.lowpassCutoff", PropertyValue(std::string("bad_cutoff")));
        audio.tick(); // Should catch std::stod exceptions internally and fall back to defaults without throwing

        // Scenario B: Numeric double values
        emitterObj->setDynamicProperty("acoustic.frequency", PropertyValue(880.0));
        emitterObj->setDynamicProperty("acoustic.amplitude", PropertyValue(0.5));
        emitterObj->setDynamicProperty("acoustic.lowpassCutoff", PropertyValue(5000.0));
        audio.tick();

        // Scenario C: Numeric integer values
        emitterObj->setDynamicProperty("acoustic.frequency", PropertyValue(440));
        emitterObj->setDynamicProperty("acoustic.amplitude", PropertyValue(1));
        audio.tick();

        // Scenario D: Valid numeric string values
        emitterObj->setDynamicProperty("acoustic.frequency", PropertyValue(std::string("220.5")));
        emitterObj->setDynamicProperty("acoustic.amplitude", PropertyValue(std::string("0.8")));
        audio.tick();

        // 3. Playback functions that don't depend on spatial API (like music)
        audio.stopMusic(); // Safe to call even if nothing is playing

        audio.shutdown();
        std::printf("AudioSystem shutdown cleanly.\n");

        // 4. Re-initialization
        assert(audio.init() == true);
        assert(audio.init() == true); // Should return true immediately

        audio.shutdown();
        audio.shutdown(); // Should be safe
    }

    std::printf("audio_system_test: OK\n");
    return 0;
}
