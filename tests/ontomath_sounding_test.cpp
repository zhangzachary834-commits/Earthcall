// One authored expression, two senses.
//
// ONTOMATH_FRAMEWORK.md §1: what a thing IS mathematically must be distinct
// from how a particular channel renders it. The WebGPU channel compiles a
// Piecewise into WGSL and reads it as a density in space. This test proves
// the audio channel reads THE SAME Piecewise — the same object, not a copy,
// not a translation into frequency-and-waveform parameters — as a pressure
// wave in time.
//
// That is the whole claim behind "a building you can hear": the vault and its
// chord are not a resemblance between two artifacts. They are one text, read
// twice, by two channels neither of which is allowed to know what it means.

#include "Singularity/Audio/AudioSystem.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

namespace {

bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

OntoMath::Piecewise everywhere(OntoMath::ScalarForm e, const std::string& var) {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(std::move(e)));
    f.inputVariable = var;
    return f;
}

// The profile of a vault: a fundamental and two harmonics over one variable.
// Read along a spatial axis it is a cross-section; read along time it is a
// chord. Which it is depends only on who is reading.
OntoMath::ScalarForm harmonicSeries(double fundamental) {
    return OntoMath::ScalarForm::sinusoid(0.50, fundamental,       0.0, 0.0, "t")
        .plus(OntoMath::ScalarForm::sinusoid(0.25, fundamental * 2, 0.0, 0.0, "t"))
        .plus(OntoMath::ScalarForm::sinusoid(0.125, fundamental * 3, 0.0, 0.0, "t"))
        .normalized();
}

}   // namespace

int main() {
    // =====================================================================
    // 1. The expression IS the waveform — sampled, not approximated by a
    //    generator configured to resemble it.
    // =====================================================================
    {
        const int rate = 48000;
        const double a440 = 440.0;
        const auto tone = everywhere(
            OntoMath::ScalarForm::sinusoid(0.8, a440, 0.0, 0.0, "t"), "t");

        Core::Audio::SoundingReport report;
        const auto samples =
            Core::Audio::renderForm(tone, "t", 0.25, rate, {}, &report);

        assert(samples.size() == static_cast<std::size_t>(0.25 * rate));
        assert(report.undefinedSamples == 0);
        assert(report.clampedSamples == 0);
        assert(!report.refused);

        // Every sample is the authored function at that instant.
        for (std::size_t i = 0; i < samples.size(); i += 137) {
            const double t = static_cast<double>(i) / rate;
            assert(near(samples[i], 0.8 * std::sin(2.0 * M_PI * a440 * t), 1e-5));
        }
    }

    // =====================================================================
    // 2. Undefined is silence THAT IS COUNTED — never silence passed off as
    //    a value the author wrote.
    // =====================================================================
    {
        // Authored only on [0, 0.1]; the rest of the second is outside every
        // piece, and the model is undefined there.
        OntoMath::Piecewise halfWritten;
        halfWritten.inputVariable = "t";
        OntoMath::Piecewise::Piece piece;
        piece.hasLo = true;  piece.lo = 0.0;
        piece.hasHi = true;  piece.hi = 0.1;
        piece.mathNode = OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::sinusoid(0.5, 220.0, 0.0, 0.0, "t"));
        halfWritten.pieces.push_back(piece);

        Core::Audio::SoundingReport report;
        const auto samples =
            Core::Audio::renderForm(halfWritten, "t", 0.2, 1000, {}, &report);
        assert(samples.size() == 200);
        // t = 0.1 is INSIDE the piece (bounds are inclusive by default), so
        // 101 samples are written and 99 are not. The boundary is counted the
        // way the author wrote it, not rounded to the convenient number.
        assert(report.undefinedSamples == 99);
        assert(samples[150] == 0.0f);    // silent there, and the count says why

        // A model with no pieces says nothing anywhere: every sample silent,
        // every sample counted as undefined rather than reported as a signal.
        Core::Audio::SoundingReport muteReport;
        const auto mute =
            Core::Audio::renderForm(OntoMath::Piecewise{}, "t", 0.1, 1000, {}, &muteReport);
        assert(mute.size() == 100 && muteReport.undefinedSamples == 100);
    }

    // =====================================================================
    // 3. Amplitudes outside the speakers' range are clamped, and the clamping
    //    is reported rather than hidden.
    // =====================================================================
    {
        // Loud, but ABOVE the infrasound floor — clamping a 440 Hz tone to
        // [-1, 1] squares it off; it does not push energy under the floor.
        const auto loud = everywhere(
            OntoMath::ScalarForm::sinusoid(4.0, 440.0, 0.0, 0.0, "t"), "t");
        Core::Audio::SoundingReport report;
        const auto samples = Core::Audio::renderForm(loud, "t", 0.05, 48000, {}, &report);
        assert(!samples.empty());
        assert(!report.refused);
        assert(report.clampedSamples > samples.size() / 2);   // most of a squared-off sine
        assert(near(std::fabs(samples[24]), 1.0));
    }

    // =====================================================================
    // 4. THE CLAIM. One Piecewise, held by a ScalarField for the geometry
    //    channel, sounded by the audio channel — and the two agree because
    //    there is only one of them.
    // =====================================================================
    {
        const int rate = 44100;
        const auto vault = everywhere(harmonicSeries(110.0), "t");

        // The geometry channel's view: a field whose density the raymarcher
        // integrates along a ray.
        auto arch = std::make_shared<OntoMath::ScalarField>();
        arch->mode = OntoMath::ScalarField::EvaluationMode::AST;
        arch->astDefinition = vault;

        // The audio channel's view: the same definition, off the same field.
        const auto chord = Core::Audio::renderForm(arch->astDefinition, "t", 0.05, rate);
        assert(!chord.empty());

        // At every point, the shape's value and the sound's value are the
        // same number, because they are the same expression.
        for (std::size_t i = 0; i < chord.size(); i += 97) {
            const double t = static_cast<double>(i) / rate;
            std::map<std::string, PropertyValue> at{{"t", PropertyValue(t)}};
            const auto asShape = arch->astDefinition.evaluate(at, nullptr);
            assert(asShape);
            double density = 0.0;
            assert(propertyValueToNumber(*asShape, density));
            assert(near(chord[i], density, 1e-5));
        }

        // And it survives the round trip through a save file: what the world
        // stores is the text, so both channels reload the same one.
        const auto reloaded = OntoMath::ScalarField::fromJson(arch->toJson());
        assert(reloaded);
        const auto rechord = Core::Audio::renderForm(reloaded->astDefinition, "t", 0.05, rate);
        assert(rechord.size() == chord.size());
        for (std::size_t i = 0; i < rechord.size(); i += 97) {
            assert(near(rechord[i], chord[i], 1e-6));
        }
    }

    // =====================================================================
    // 5. The seam with reversal: the waveform's exact antiderivative is the
    //    position form of the rate that would sound it. Sound and motion are
    //    the same algebra one derivative apart, and the algebra is exact in
    //    both directions.
    // =====================================================================
    {
        const auto wave = harmonicSeries(110.0);
        const auto integrated = wave.antiderivative("t");
        assert(integrated);
        const auto back = integrated->derivative("t").normalized();

        // d/dt ∫ f = f, term for term, at every probe.
        for (double t = 0.0; t < 0.02; t += 0.0013) {
            const auto original = wave.evaluate({{"t", t}});
            const auto roundTripped = back.evaluate({{"t", t}});
            assert(original && roundTripped);
            assert(near(*original, *roundTripped, 1e-9));
        }
    }

    std::printf("ontomath_sounding_test: OK\n");
    return 0;
}
