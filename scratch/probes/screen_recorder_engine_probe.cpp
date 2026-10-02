// Real Engine::tick witness. Run with screen_recorder_engine_probe.py, which
// links the production WebGPU app objects and boots an isolated first-seed save
// root. No inhabited save or First Mover grant is read or changed.
// Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include <filesystem>
#include <cstdio>
#include <thread>
#include <fstream>
#include <iterator>

extern ZoneManager mgr;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    auto& engine = Core::Engine::instance();
    if (!engine.init(argc, argv)) return 1;
    if (argc > 1) {
        SaveContext context;
        context.camera = engine.getCamera();
        context.person = engine.getPerson();
        context.mouseHandler = engine.getMouseHandler();
        context.lawManager = engine.getLawManager();
        mgr.loadTestObservation(argv[1], context);
    }
    auto* recorder = Singularity::Screen::ScreenRecorder::find(*engine.getLawManager());
    if (!recorder) return 2;
    auto out = std::filesystem::current_path() / "capture";
    auto set = [&](const char* path, PropertyValue value) {
        return lawSetValue(*recorder, PropertyPath::parse(path), value);
    };
    set("outputPath", out.string());
    set("format", std::string("png_sequence"));
    set("fps", 10.0);
    set("recordCursor", false);
    set("recording", true);
    for (int i = 0; i < 8; ++i) {
        if (i == 2) set("snapshot", true);
        engine.tick(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(110));
    }
    set("recording", false);
    PropertyValue snapshot, count, error;
    lawGetValue(*recorder, PropertyPath::parse("lastSnapshotPath"), snapshot);
    lawGetValue(*recorder, PropertyPath::parse("frameCount"), count);
    lawGetValue(*recorder, PropertyPath::parse("lastError"), error);
    auto path = std::get<std::string>(snapshot);
    bool ok = !path.empty() && std::filesystem::exists(path) && std::get<double>(count) >= 2 &&
        !recorder->isSnapshotPending() && std::get<std::string>(error).empty();
    std::printf("ENGINE_CAPTURE_RESULT %s frames=%.0f snapshot=%s error=%s\n",
        ok ? "PASS" : "FAIL", std::get<double>(count), path.c_str(), std::get<std::string>(error).c_str());
    engine.shutdown();
    return ok ? 0 : 3;
}
