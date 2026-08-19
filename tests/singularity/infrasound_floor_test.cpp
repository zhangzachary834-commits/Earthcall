// The infrasound floor — a Kernel boundary on the channel to a Person's body.
//
// The manifesto's first Kernel boundary is that nothing may violate
// fundamental Person guards. `renderForm` turns arbitrary authored mathematics
// into pressure against a real human's ear and a real loudspeaker's cone, so
// the floor of human hearing is enforced in the channel itself: unconditional,
// unauthorable, and a refusal rather than a silent filter.
//
// This test pins all four properties that make it a guard rather than a
// setting: that it stops authored infrasound exactly, that it stops infrasound
// arriving by routes the symbolic reader cannot see, that it does NOT stop
// ordinary sound, and that it constrains only the channel — never the
// mathematics, which a Person may still author, evaluate and integrate freely.

#include "Singularity/Audio/AudioSystem.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

OntoMath::Piecewise everywhere(OntoMath::ScalarForm e) {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(std::move(e)));
    f.inputVariable = "t";
    return f;
}

bool says(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

}   // namespace

int main() {
    const int rate = 48000;

    // =====================================================================
    // 1. Authored infrasound is refused BEFORE a sample exists, and named.
    //    Only a symbolic waveform allows this: the frequency is read off the
    //    text exactly — no FFT, no window, no spectral leakage.
    // =====================================================================
    {
        for (const double hz : {0.5, 3.0, 7.0, 12.0, 19.0, 19.99}) {
            Core::Audio::SoundingReport report;
            const auto samples = Core::Audio::renderForm(
                everywhere(OntoMath::ScalarForm::sinusoid(0.9, hz, 0.0, 0.0, "t")),
                "t", 1.0, rate, {}, &report);

            assert(samples.empty());
            assert(report.refused);
            assert(says(report.refusal, "below the"));
            // The exact frequency, recovered from the TransFactor's scale.
            assert(std::fabs(report.lowestAuthoredHz - hz) < 1e-9);
        }

        // The lowest component decides, not the loudest or the first: a chord
        // of good notes with one infrasonic partial is still refused.
        auto chord = OntoMath::ScalarForm::sinusoid(0.5, 440.0, 0.0, 0.0, "t")
                         .plus(OntoMath::ScalarForm::sinusoid(0.5, 660.0, 0.0, 0.0, "t"))
                         .plus(OntoMath::ScalarForm::sinusoid(0.4, 9.0, 0.0, 0.0, "t"))
                         .normalized();
        Core::Audio::SoundingReport report;
        assert(Core::Audio::renderForm(everywhere(chord), "t", 1.0, rate, {}, &report).empty());
        assert(report.refused);
        assert(std::fabs(report.lowestAuthoredHz - 9.0) < 1e-9);

        // A term is a PRODUCT, and a product is not a mixture: by the
        // product-to-sum identities sin(a)·sin(b) is energy at |a−b| and a+b.
        // So two comfortably audible factors can put a difference tone under
        // the floor — 440 × 430 sounds a 10 Hz beat — and reading the factors
        // one at a time would wave it straight through.
        const auto beating =
            OntoMath::ScalarForm::sinusoid(0.9, 440.0, 0.0, 0.0, "t")
                .times(OntoMath::ScalarForm::sinusoid(1.0, 430.0, 0.0, 0.0, "t"))
                .normalized();
        Core::Audio::SoundingReport difference;
        assert(Core::Audio::renderForm(everywhere(beating), "t", 1.0, rate, {}, &difference)
                   .empty());
        assert(difference.refused);
        assert(std::fabs(difference.lowestAuthoredHz - 10.0) < 1e-9);
    }

    // =====================================================================
    // 2. Infrasound that the TEXT does not announce is caught anyway, by
    //    measuring what was actually produced. A guard that could be evaded
    //    by authoring the same energy a different way is not a guard.
    // =====================================================================
    {
        // A DC offset: 0 Hz. Not a sound at all — a constant displacement
        // held against the cone and the eardrum.
        Core::Audio::SoundingReport dc;
        assert(Core::Audio::renderForm(
                   everywhere(OntoMath::ScalarForm::constant(0.8)), "t", 1.0, rate, {}, &dc)
                   .empty());
        assert(dc.refused);
        assert(dc.lowestAuthoredHz == 0.0);        // the text named no sinusoid...
        assert(says(dc.refusal, "rendered signal"));   // ...the measurement caught it

        // A slow polynomial ramp — a sweep into the sub-audible, authored with
        // no sinusoid anywhere in its text.
        OntoMath::ScalarForm ramp;
        ramp.terms.push_back(OntoMath::Term(0.6, {{"t", 1.0}}));
        Core::Audio::SoundingReport sweep;
        assert(Core::Audio::renderForm(everywhere(ramp), "t", 1.0, rate, {}, &sweep).empty());
        assert(sweep.refused);

        // A piecewise square wave at 5 Hz, built from bounded constant pieces:
        // no TransFactor exists to read, and it is refused all the same.
        OntoMath::Piecewise square;
        square.inputVariable = "t";
        for (int i = 0; i < 10; ++i) {
            OntoMath::Piecewise::Piece piece;
            piece.hasLo = true;  piece.lo = i * 0.1;
            piece.hasHi = true;  piece.hi = (i + 1) * 0.1;
            piece.mathNode = OntoMath::MathNode::fromLegacyExpression(
                OntoMath::ScalarForm::constant(i % 2 == 0 ? 0.9 : -0.9));
            square.pieces.push_back(piece);
        }
        Core::Audio::SoundingReport stepped;
        assert(Core::Audio::renderForm(square, "t", 1.0, rate, {}, &stepped).empty());
        assert(stepped.refused);
        assert(stepped.lowestAuthoredHz == 0.0);
    }

    // =====================================================================
    // 3. It does not block ordinary sound. A guard that refuses everything is
    //    as useless as one that refuses nothing.
    // =====================================================================
    {
        for (const double hz : {20.0, 25.0, 60.0, 110.0, 440.0, 4000.0}) {
            Core::Audio::SoundingReport report;
            const auto samples = Core::Audio::renderForm(
                everywhere(OntoMath::ScalarForm::sinusoid(0.9, hz, 0.0, 0.0, "t")),
                "t", 0.5, rate, {}, &report);
            assert(!samples.empty());
            assert(!report.refused);
            assert(report.infrasonicRms <= Core::Audio::kInfrasonicRmsCeiling);
        }

        // An inaudibly quiet infrasonic term is harmless and passes: the
        // ceiling is on ENERGY reaching the body, not on the mere presence of
        // a low frequency.
        Core::Audio::SoundingReport faint;
        const auto whisper = Core::Audio::renderForm(
            everywhere(OntoMath::ScalarForm::sinusoid(0.001, 7.0, 0.0, 0.0, "t")),
            "t", 0.5, rate, {}, &faint);
        assert(!whisper.empty());
        assert(!faint.refused);

        // A slow AMPLITUDE ENVELOPE is not infrasound: modulating a 440 Hz
        // tone at 2 Hz puts its energy in sidebands around 440, not under the
        // floor. This is the case a naive "any slow term is infrasound" rule
        // would wrongly reject.
        const auto modulated =
            OntoMath::ScalarForm::sinusoid(0.5, 440.0, 0.0, 0.0, "t")
                .times(OntoMath::ScalarForm::sinusoid(0.5, 2.0, 0.0, 0.5, "t"))
                .normalized();
        Core::Audio::SoundingReport tremolo;
        const auto samples =
            Core::Audio::renderForm(everywhere(modulated), "t", 1.0, rate, {}, &tremolo);
        assert(!samples.empty());
        assert(!tremolo.refused);
    }

    // =====================================================================
    // 4. The floor is on the CHANNEL, never on the mathematics. A Person may
    //    author, evaluate, differentiate and integrate a 7 Hz field freely —
    //    it simply may not be pushed into a speaker aimed at a human.
    // =====================================================================
    {
        const auto infrasonic = OntoMath::ScalarForm::sinusoid(0.9, 7.0, 0.0, 0.0, "t");
        const auto form = everywhere(infrasonic);

        // Evaluating it: untouched.
        const auto value = form.evaluate({{"t", PropertyValue(0.01)}}, nullptr);
        double y = 0.0;
        assert(value && propertyValueToNumber(*value, y));
        assert(std::fabs(y - 0.9 * std::sin(2.0 * M_PI * 7.0 * 0.01)) < 1e-9);

        // Differentiating and integrating it: untouched, and still exact.
        const auto integrated = infrasonic.antiderivative("t");
        assert(integrated);
        const auto roundTrip = integrated->derivative("t").normalized();
        const auto original = infrasonic.evaluate({{"t", 0.03}});
        const auto returned = roundTrip.evaluate({{"t", 0.03}});
        assert(original && returned && std::fabs(*original - *returned) < 1e-9);

        // Only the sounding refuses.
        Core::Audio::SoundingReport report;
        assert(Core::Audio::renderForm(form, "t", 1.0, rate, {}, &report).empty());
        assert(says(report.refusal, "mathematics is untouched"));
    }

    // =====================================================================
    // 5. The measurement is available on its own, so any path that produces
    //    PCM can be held to the same line.
    // =====================================================================
    {
        std::vector<float> sevenHz(rate), fourForty(rate);
        for (int i = 0; i < rate; ++i) {
            const double t = static_cast<double>(i) / rate;
            sevenHz[i] = static_cast<float>(0.9 * std::sin(2.0 * M_PI * 7.0 * t));
            fourForty[i] = static_cast<float>(0.9 * std::sin(2.0 * M_PI * 440.0 * t));
        }
        assert(Core::Audio::measureInfrasonicRms(sevenHz, rate) >
               Core::Audio::kInfrasonicRmsCeiling);
        assert(Core::Audio::measureInfrasonicRms(fourForty, rate) <
               Core::Audio::kInfrasonicRmsCeiling);
        assert(Core::Audio::measureInfrasonicRms({}, rate) == 0.0);
    }

    std::printf("infrasound_floor_test: OK\n");
    return 0;
}
