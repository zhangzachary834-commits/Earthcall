// One expression, two senses — and the past computed from the same text.
//
// A probe for two claims that the docs now make (ONTOMATH_FRAMEWORK.md §6,
// §7). It authors ONE OntoMath::ScalarForm and then:
//
//   1. reads it as GEOMETRY  — the cross-section of a vault, printed here as
//      an ASCII profile, the same astDefinition a ScalarField hands the WGSL
//      raymarcher;
//   2. reads it as SOUND     — the same Piecewise sampled into PCM and
//      written to a playable .wav, no frequency parameter and no wave-type
//      preset anywhere in the path;
//   3. reads it BACKWARDS    — the exact antiderivative carrying a Flow's
//      property to where it stood N seconds ago, with nothing logged;
//   4. prints the irreversibility map of a small set of authored actions —
//      a Zone's fold, in miniature.
//
// Build (the sanctioned convenience — see CLAUDE.md "Working notes"):
//     cp scratch/probes/one_expression_two_senses.cpp tests/
//     cmake -S . -B build ...   # sources are globbed at configure time
//     cmake --build build --target one_expression_two_senses -j8
//     ./build/one_expression_two_senses
//     rm tests/one_expression_two_senses.cpp
//
// It writes scratch/fixtures/vault.wav. Play it: the chord you hear and the arch
// printed above it are the same equation.

#include "Singularity/Audio/AudioSystem.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

OntoMath::Piecewise everywhere(OntoMath::ScalarForm e, const std::string& var) {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(std::move(e)));
    f.inputVariable = var;
    return f;
}

void writeWav(const std::string& path, const std::vector<float>& samples, int rate) {
    std::ofstream out(path, std::ios::binary);
    if (!out) { std::printf("  (could not open %s)\n", path.c_str()); return; }

    const std::uint32_t dataBytes = static_cast<std::uint32_t>(samples.size() * 2);
    const std::uint32_t byteRate = static_cast<std::uint32_t>(rate) * 2;
    const auto u32 = [&out](std::uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };
    const auto u16 = [&out](std::uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };

    out.write("RIFF", 4);  u32(36 + dataBytes);  out.write("WAVE", 4);
    out.write("fmt ", 4);  u32(16);  u16(1);  u16(1);
    u32(static_cast<std::uint32_t>(rate));  u32(byteRate);  u16(2);  u16(16);
    out.write("data", 4);  u32(dataBytes);
    for (const float sample : samples) {
        u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(sample * 32767.0f)));
    }
    std::printf("  wrote %s (%zu samples, %d Hz)\n", path.c_str(), samples.size(), rate);
}

// The vault's profile: a fundamental and two harmonics. Read along a spatial
// axis it is a cross-section; read along time it is a chord.
OntoMath::ScalarForm harmonicSeries(double fundamental) {
    return OntoMath::ScalarForm::sinusoid(0.50, fundamental,       0.0, 0.0, "t")
        .plus(OntoMath::ScalarForm::sinusoid(0.25, fundamental * 2, 0.0, 0.0, "t"))
        .plus(OntoMath::ScalarForm::sinusoid(0.125, fundamental * 3, 0.0, 0.0, "t"))
        .normalized();
}

void printProfile(const OntoMath::Piecewise& form, double from, double to, int columns) {
    const int rows = 17;
    std::vector<std::string> canvas(rows, std::string(columns, ' '));
    for (int col = 0; col < columns; ++col) {
        const double t = from + (to - from) * col / (columns - 1);
        std::map<std::string, PropertyValue> at{{"t", PropertyValue(t)}};
        const auto value = form.evaluate(at, nullptr);
        double y = 0.0;
        if (!value || !propertyValueToNumber(*value, y)) continue;
        int row = static_cast<int>((1.0 - y) * 0.5 * (rows - 1));
        if (row < 0) row = 0;
        if (row >= rows) row = rows - 1;
        canvas[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = '#';
    }
    for (const auto& line : canvas) std::printf("  %s\n", line.c_str());
}

}   // namespace

int main() {
    const int rate = 44100;
    const double fundamental = 110.0;                 // A2
    const auto series = harmonicSeries(fundamental);
    const auto vault = everywhere(series, "t");

    // ------------------------------------------------------------------
    // ONE text, held where the geometry channel reads it.
    // ------------------------------------------------------------------
    OntoMath::ScalarField arch;
    arch.mode = OntoMath::ScalarField::EvaluationMode::AST;
    arch.astDefinition = vault;

    // (Piecewise::print renders a MathNode piece as "[MathNode]" — the tree
    // has no printer of its own yet — so the authored ScalarForm is printed
    // here directly.)
    std::printf("\n=== the expression ===\n  %s\n", series.print().c_str());

    std::printf("\n=== read as GEOMETRY (one period of the vault) ===\n");
    printProfile(arch.astDefinition, 0.0, 1.0 / fundamental, 78);

    // ------------------------------------------------------------------
    // The same object, read by the audio channel.
    // ------------------------------------------------------------------
    std::printf("\n=== read as SOUND (the same astDefinition) ===\n");
    Core::Audio::SoundingReport report;
    const auto chord =
        Core::Audio::renderForm(arch.astDefinition, "t", 2.0, rate, {}, &report);
    std::printf("  %zu samples, %zu undefined, %zu clamped\n",
                chord.size(), report.undefinedSamples, report.clampedSamples);
    std::printf("  lowest authored component: %.1f Hz; infrasonic RMS %.2e (ceiling %.2f)\n",
                report.lowestAuthoredHz, report.infrasonicRms,
                Core::Audio::kInfrasonicRmsCeiling);
    writeWav("scratch/fixtures/vault.wav", chord, rate);

    // The same channel, asked to sound a 7 Hz tone. It refuses — and the
    // refusal is not a setting anyone can reach.
    std::printf("\n=== the infrasound floor ===\n");
    Core::Audio::SoundingReport blocked;
    const auto felt = Core::Audio::renderForm(
        everywhere(OntoMath::ScalarForm::sinusoid(0.9, 7.0, 0.0, 0.0, "t"), "t"),
        "t", 1.0, rate, {}, &blocked);
    std::printf("  7 Hz sine  -> %zu samples\n  %s\n", felt.size(), blocked.refusal.c_str());

    Core::Audio::SoundingReport dc;
    const auto offset = Core::Audio::renderForm(
        everywhere(OntoMath::ScalarForm::constant(0.8), "t"), "t", 1.0, rate, {}, &dc);
    std::printf("  DC offset  -> %zu samples\n  %s\n", offset.size(), dc.refusal.c_str());

    // ------------------------------------------------------------------
    // Read backwards. dy/dt = cos t, so y(t) = sin t; stand the world at
    // t = 5 and walk back without a log.
    // ------------------------------------------------------------------
    std::printf("\n=== read BACKWARDS (exact, nothing logged) ===\n");
    Object bell;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&bell);
    });
    Universe::instance().setClock(5.0, 0.1);
    bell.setPosition(glm::vec3(0.0f, static_cast<float>(std::sin(5.0)), 0.0f));

    const MathBindings clockBinding{{"t", PropertyPath::parse("time")}};
    const auto rising = ActionNode::flow(
        "position.y",
        everywhere(OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, "t"), "t"),
        clockBinding);

    std::printf("  law: %s\n", rising.describe().c_str());
    std::printf("  %-10s %-14s %-14s %s\n", "clock", "computed", "sin(t)", "error");
    for (double ago = 0.0; ago <= 5.0; ago += 1.0) {
        const auto past = rising.valueSecondsAgo(bell, ago);
        double y = 0.0;
        if (past && propertyValueToNumber(*past, y)) {
            const double truth = std::sin(5.0 - ago);
            std::printf("  t=%-8.1f %-14.10f %-14.10f %.2e\n",
                        5.0 - ago, y, truth, std::fabs(y - truth));
        }
    }

    // ------------------------------------------------------------------
    // The irreversibility map — a Zone's fold, in miniature.
    // ------------------------------------------------------------------
    std::printf("\n=== the irreversibility map ===\n");
    struct Entry { const char* name; ActionNode action; };
    OntoMath::Term byParts(1.0, {{"t", 1.0}});
    byParts.addTrans(OntoMath::TransFactor(OntoMath::TransFactor::Kind::Sin, "t"));
    OntoMath::ScalarForm product;
    product.terms.push_back(byParts);

    const MathBindings selfReading{{"t", PropertyPath::parse("time")},
                                   {"y", PropertyPath::parse("position.y")}};
    OntoMath::ScalarForm decay;
    decay.terms.push_back(OntoMath::Term(-1.0, {{"y", 1.0}}));

    std::vector<Entry> world{
        {"rise as cos t",        rising},
        {"y := sin t",           ActionNode::map("position.y",
                                     everywhere(OntoMath::ScalarForm::transcendental(
                                         OntoMath::TransFactor::Kind::Sin, "t"), "t"),
                                     clockBinding)},
        {"drive by the clock",   ActionNode::drive("position.y",
                                     CurveModel::sinusoid(1.0, 1.0), "time")},
        {"rate t*sin t",         ActionNode::flow("position.y", everywhere(product, "t"),
                                     clockBinding)},
        {"decay dy/dt = -y",     ActionNode::flow("position.y", everywhere(decay, "t"),
                                     selfReading)},
        {"y := 3",               ActionNode::set("position.y", PropertyValue(3.0))},
        {"nudge y by 1",         ActionNode::add("position.y", 1.0)},
        {"destroy it",           ActionNode::destroy()},
    };

    int reversible = 0;
    for (const auto& entry : world) {
        const auto judgement = entry.action.reversibility();
        if (judgement.exact) ++reversible;
        std::printf("  %-20s %s\n", entry.name, judgement.summary().c_str());
    }
    std::printf("\n  %d of %zu authored actions are exactly reversible.\n",
                reversible, world.size());

    Universe::instance().setProvider(nullptr);
    std::printf("\n");
    return 0;
}
