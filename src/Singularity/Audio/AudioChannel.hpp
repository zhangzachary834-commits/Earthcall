#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <string>

namespace Singularity {
namespace Audio {

// First-mover modality boundary for authored acoustic reality -> machine audio.
//
// Like ScreenChannel and InteractionChannel, this class is a membrane, not a
// domain ontology. It governs how authored sound reaches the hardware and
// exposes truthful substrate telemetry back to Law. It does NOT define what a
// bell, voice, instrument, or timbre is.
//
// New authored timbres are ordinary Singulars/Formations carrying
// `acoustic.form` as serialized OntoMath::Piecewise plus optional
// `acoustic.timeVariable`, `acoustic.referenceFrequency`, and
// `acoustic.duration`. PlayAudio's legacy timbre string is interpreted as an
// identifier first; only the historical sine/triangle/square/sawtooth names
// fall back to the old miniaudio oscillator path.
class AudioChannel : public Law {
public:
    AudioChannel();
    ~AudioChannel() override;

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "audio-channel"; }
    const std::string& name() const { return _name; }

    static void syncRegister(LawManager& laws);
    static AudioChannel* find(LawManager& laws);

    // Re-assert channel governance onto the lower AudioSystem. Called every
    // engine audio step because Law::enabled is inherited state and can be
    // changed by authored Law without a channel-specific setter hook.
    void applyGovernance();

    // The checked PlayAudio sink. False means NO sound was produced and the
    // reason is propagated back into ActionNode's trace.
    bool sound(Singular& subject, double frequency, double amplitude,
               const std::string& timbre, std::string& reason);

    // Pure resolution half, exposed for headless witnesses. This does not
    // touch a device and therefore lets tests prove authored timbre semantics
    // separately from host audio availability.
    static bool resolveAuthoredForm(Singular& subject,
                                    const std::string& timbre,
                                    OntoMath::Piecewise& form,
                                    std::string& timeVariable,
                                    double& referenceFrequency,
                                    double& duration,
                                    std::string& reason);

    double masterGain() const { return _masterGain; }
    void setMasterGain(const double& value);

    int actualSampleRate() const;
    int activeVoices() const;
    bool backendInitialized() const;

    int authoredFormParses() const { return _authoredFormParses; }
    int authoredTimbresSounded() const { return _authoredTimbresSounded; }
    int legacyTimbresSounded() const { return _legacyTimbresSounded; }
    int unresolvedTimbres() const { return _unresolvedTimbres; }
    std::string lastTimbreStatus() const { return _lastTimbreStatus; }

private:
    void buildProperties() override;
    void bindSink();

    double _masterGain = 1.0;

    // Truthful counters only. These are deliberately not called graphCompiles
    // or cacheHits yet: no Audio IR cache exists in this rung.
    int _authoredFormParses = 0;
    int _authoredTimbresSounded = 0;
    int _legacyTimbresSounded = 0;
    int _unresolvedTimbres = 0;
    std::string _lastTimbreStatus{"idle"};

    std::string _name{"audio-channel"};
};

} // namespace Audio
} // namespace Singularity
