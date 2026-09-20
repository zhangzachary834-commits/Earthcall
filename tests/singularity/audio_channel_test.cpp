#include "Singularity/Audio/AudioChannel.hpp"
#include "Singularity/Audio/AudioSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

OntoMath::Piecewise normalizedSine() {
    auto form = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::sinusoid(
                0.5, 1.0, 0.0, 0.0, "phase")));
    form.inputVariable = "phase";
    return form;
}

bool near(double a, double b, double eps = 1e-5) {
    return std::fabs(a - b) < eps;
}

} // namespace

int main() {
    LawManager laws;
    Singularity::Audio::AudioChannel::syncRegister(laws);
    auto* channel = Singularity::Audio::AudioChannel::find(laws);
    assert(channel);
    assert(channel->isFirstMover());
    assert(channel->getIdentifier() == "audio-channel");

    // ------------------------------------------------------------------
    // 1. Governable vs derived channel state.
    // ------------------------------------------------------------------
    assert(lawSetValue(*channel, PropertyPath::parse("masterGain"),
                       PropertyValue(0.37)) ==
           PropertyPath::PathResult::Ok);
    PropertyValue gain;
    assert(lawGetValue(*channel, PropertyPath::parse("masterGain"), gain));
    assert(near(std::get<double>(gain), 0.37));

    // What the backend actually negotiated is telemetry, not an authorable lie.
    assert(lawSetValue(*channel, PropertyPath::parse("actualSampleRate"),
                       PropertyValue(96000)) ==
           PropertyPath::PathResult::ReadOnly);

    // ------------------------------------------------------------------
    // 2. A timbre identity resolves to ordinary authored acoustic math.
    // ------------------------------------------------------------------
    Object source;
    source.setObjectID("source.note");

    Object timbre;
    timbre.setObjectID("timbre.crystal.test");
    const auto authored = normalizedSine();
    timbre.setDynamicProperty(
        "acoustic.form", PropertyValue(authored.toJson().dump()));
    timbre.setDynamicProperty(
        "acoustic.timeVariable", PropertyValue(std::string("phase")));
    timbre.setDynamicProperty(
        "acoustic.referenceFrequency", PropertyValue(1.0));
    timbre.setDynamicProperty(
        "acoustic.duration", PropertyValue(0.05));

    Universe::instance().setProvider(
        [&](std::vector<Singular*>& beings) {
            beings.push_back(&source);
            beings.push_back(&timbre);
        });

    OntoMath::Piecewise resolved;
    std::string timeVariable;
    double referenceFrequency = 0.0;
    double duration = 0.0;
    std::string reason;
    assert(Singularity::Audio::AudioChannel::resolveAuthoredForm(
        source, "timbre.crystal.test", resolved, timeVariable,
        referenceFrequency, duration, reason));
    assert(timeVariable == "phase");
    assert(near(referenceFrequency, 1.0));
    assert(near(duration, 0.05));

    // ------------------------------------------------------------------
    // 3. Pitch is a retiming of the authored waveform, not a preset switch.
    //    One authored 1-cycle form becomes A4 at timeScale=440.
    // ------------------------------------------------------------------
    Core::Audio::SoundingReport report;
    const int rate = 48000;
    const auto samples = Core::Audio::renderForm(
        resolved, timeVariable, 0.05, rate, {}, &report, 440.0);
    assert(!samples.empty());
    assert(!report.refused);
    assert(near(report.lowestAuthoredHz, 440.0, 1e-6));

    for (std::size_t i = 0; i < samples.size(); i += 137) {
        const double t = static_cast<double>(i) / rate;
        const double expected =
            0.5 * std::sin(2.0 * M_PI * 440.0 * t);
        assert(near(samples[i], expected));
    }

    // The Person-body guard follows the RETIMED sound. The same authored
    // normalized timbre at 7 Hz is refused; the math object itself survives.
    Core::Audio::SoundingReport infra;
    const auto refused = Core::Audio::renderForm(
        resolved, timeVariable, 0.2, rate, {}, &infra, 7.0);
    assert(refused.empty());
    assert(infra.refused);
    assert(near(infra.lowestAuthoredHz, 7.0, 1e-6));

    // ------------------------------------------------------------------
    // 4. Unknown timbre identity is a refusal, never an alias for sine.
    //    This goes through the checked sink ActionNode will use.
    // ------------------------------------------------------------------
    assert(audioSink());
    reason.clear();
    assert(!audioSink()(source, 440.0, 0.5, "crystal-with-no-being", reason));
    assert(reason.find("unresolved authored timbre") != std::string::npos);

    PropertyValue unresolved;
    assert(lawGetValue(*channel, PropertyPath::parse("unresolvedTimbres"),
                       unresolved));
    assert(std::get<int>(unresolved) == 1);

    // ------------------------------------------------------------------
    // 5. PlayAudio must preserve that refusal as causal truth. A refused
    //    sound is NOT an audio-synthesized edge and NOT a successful action.
    // ------------------------------------------------------------------
    source.setDynamicProperty("acoustic.frequency", PropertyValue(440.0));
    source.setDynamicProperty("acoustic.amplitude", PropertyValue(0.5));
    int synthesizedEdges = 0;
    Core::EventBus::instance().subscribe<ECA::Event>(
        [&](const ECA::Event& event) {
            if (event.type == "audio-synthesized") ++synthesizedEdges;
        });

    const ActionNode unresolvedAction = ActionNode::playAudio(
        "acoustic.frequency", "acoustic.amplitude", "crystal-with-no-being");
    const auto unresolvedExecutor = unresolvedAction.compile();
    {
        ActionNode::TraceScope trace;
        unresolvedExecutor(ECA::Event{"audio-test", &source, nullptr, 0}, source);
        assert(trace.trace().allFailed());
        assert(trace.trace().failureCount() == 1);
        assert(!trace.trace().nodes.empty());
        assert(trace.trace().nodes.back().note.find("unresolved authored timbre") !=
               std::string::npos);
    }
    assert(synthesizedEdges == 0);

    assert(lawGetValue(*channel, PropertyPath::parse("unresolvedTimbres"),
                       unresolved));
    assert(std::get<int>(unresolved) == 2);

    // An explicitly authored but malformed form is authoritative enough to
    // REFUSE. It must never be erased by falling through to the old "sine"
    // compatibility word.
    source.setDynamicProperty(
        "acoustic.form", PropertyValue(std::string("{not-valid-ontomath")));
    reason.clear();
    assert(!audioSink()(source, 440.0, 0.5, "sine", reason));
    assert(reason.find("not valid OntoMath JSON") != std::string::npos);
    assert(lawGetValue(*channel, PropertyPath::parse("unresolvedTimbres"),
                       unresolved));
    assert(std::get<int>(unresolved) == 3);

    // ------------------------------------------------------------------
    // 6. A sounding being may carry its own form; timbre identity is not
    //    compulsory when the authored structure is local.
    // ------------------------------------------------------------------
    source.setDynamicProperty(
        "acoustic.form", PropertyValue(authored.toJson().dump()));
    OntoMath::Piecewise local;
    timeVariable.clear();
    referenceFrequency = 0.0;
    duration = 0.0;
    reason.clear();
    assert(Singularity::Audio::AudioChannel::resolveAuthoredForm(
        source, "", local, timeVariable, referenceFrequency, duration, reason));
    assert(timeVariable == "phase");

    std::printf("audio_channel_test: OK\n");
    return 0;
}
