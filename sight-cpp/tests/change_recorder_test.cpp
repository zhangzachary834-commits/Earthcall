// ChangeRecorder milestone test (LAW_AND_CREATION_SYSTEM.md, commit 5):
//   "record a pottery oscillation, get a sinusoid Drive law back."
//
// A simulated demonstration: the Person works the vase's radius rhythmically
// for three seconds (here synthesized), the recorder watches shape.r, and
// fit() recovers the mathematical shape of what they did — amplitude,
// frequency, bias — as an ActionModel. The demonstration IS the source code.

#include "ZonesOfEarth/AuthorsOfLaw/ChangeRecorder.hpp"
#include "Form/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;

bool neard(double a, double b, double eps) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "change_recorder_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "change_recorder_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "change_recorder_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        // ------------------------------------------------------------------
        // 1. fitSeries recovers the three model families from raw traces.
        // ------------------------------------------------------------------
        std::vector<std::pair<float, double>> flat, ramp, wave;
        for (int i = 0; i <= 360; ++i) {
            const float t = static_cast<float>(i) / 120.0f;   // 3s at 120Hz
            flat.emplace_back(t, 0.7);
            ramp.emplace_back(t, 2.0 + 3.0 * t);
            wave.emplace_back(t, 0.5 + 0.2 * std::sin(kTwoPi * 1.5 * t));
        }

        CurveModel cFlat = ChangeRecorder::fitSeries(flat);
        assert(cFlat.form == CurveModel::Form::Constant);
        assert(neard(cFlat.evaluate(1.0), 0.7, 1e-6));

        CurveModel cRamp = ChangeRecorder::fitSeries(ramp);
        assert(cRamp.form == CurveModel::Form::Polynomial);
        assert(neard(cRamp.coeffs[0], 2.0, 1e-3));   // intercept
        assert(neard(cRamp.coeffs[1], 3.0, 1e-3));   // slope

        CurveModel cWave = ChangeRecorder::fitSeries(wave);
        assert(cWave.form == CurveModel::Form::Sinusoid);
        assert(neard(cWave.amplitude, 0.2, 0.02));
        assert(neard(cWave.frequency, 1.5, 0.05));
        assert(neard(cWave.bias, 0.5, 0.01));

        // ------------------------------------------------------------------
        // 2. The embodied loop: watch → demonstrate → sample → fit.
        //    Simulated pottery: shape.r worked rhythmically; position.x
        //    watched but untouched (the demonstration wasn't about it).
        // ------------------------------------------------------------------
        Object vase;
        ChangeRecorder recorder;
        recorder.watch("shape.r");
        recorder.watch("position.x");
        recorder.begin(vase);

        const PropertyPath radius = PropertyPath::parse("shape.r");
        const float dt = 1.0f / 120.0f;
        for (int frame = 0; frame < 360; ++frame) {
            const float t = static_cast<float>(frame + 1) * dt;
            radius.setValue(vase, PropertyValue(
                static_cast<float>(0.5 + 0.2 * std::sin(kTwoPi * 1.5 * t))));
            recorder.sample(dt);
        }
        recorder.end();

        ActionModel learned = recorder.fit();
        // Only the variable that moved becomes a Drive; the untouched one is
        // silent — so the single change IS the root node.
        assert(learned.kind == ActionNode::Kind::Drive);
        assert(learned.path.toString() == "shape.r");
        assert(learned.curve.form == CurveModel::Form::Sinusoid);
        assert(neard(learned.curve.amplitude, 0.2, 0.02));
        assert(neard(learned.curve.frequency, 1.5, 0.05));
        assert(neard(learned.curve.bias, 0.5, 0.01));

        // The learned model is a law's action: serializable text.
        ActionModel reloaded = ActionNode::fromJson(learned.toJson());
        assert(reloaded.curve.form == CurveModel::Form::Sinusoid);
        assert(neard(reloaded.curve.frequency, learned.curve.frequency, 1e-9));

        // ------------------------------------------------------------------
        // 3. Periodic transform motion converts to the native playback
        //    engine: an additive Automation clip (bias relative to rest).
        // ------------------------------------------------------------------
        Object turner;
        ChangeRecorder spin;
        spin.watch("rotation.y");
        spin.begin(turner);
        const PropertyPath rotY = PropertyPath::parse("rotation.y");
        for (int frame = 0; frame < 360; ++frame) {
            const float t = static_cast<float>(frame + 1) * dt;
            rotY.setValue(turner, PropertyValue(
                static_cast<float>(10.0 * std::sin(kTwoPi * 0.5 * t))));
            spin.sample(dt);
        }
        spin.end();

        Automation::Clip clip = spin.toClip("recorded-spin");
        assert(clip.tracks.size() == 1);
        assert(clip.tracks[0].channel == Automation::Channel::RotY);
        assert(neard(clip.tracks[0].amplitude, 10.0, 1.0));
        assert(neard(clip.tracks[0].frequency, 0.5, 0.05));
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("change_recorder_test: ALL OK");
    return 0;
}
