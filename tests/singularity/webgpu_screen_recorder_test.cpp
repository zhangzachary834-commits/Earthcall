// Zach asked for a capture/fix/verify loop. This is the second witness beside
// screen_recorder_test: actual window surface -> GPU readback -> saved pixels.
// Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/WebGPU/WebGpuContext.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <zlib.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <thread>

namespace fs = std::filesystem;
using Singularity::Screen::ScreenRecorder;
static int failures = 0;
static void check(bool ok, const char* message) {
    std::printf("%s: %s\n", ok ? "ok" : "FAIL", message);
    if (!ok) ++failures;
}
static uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
static std::vector<uint8_t> bytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
// Decode the actual exported PNG, rather than judging a signature or file size.
static bool pngMatches(const fs::path& path, int w, int h,
                       const std::vector<uint8_t>& expected) {
    auto png = bytes(path);
    if (png.size() < 33 || be32(png.data() + 16) != uint32_t(w) ||
        be32(png.data() + 20) != uint32_t(h)) return false;
    std::vector<uint8_t> compressed;
    for (size_t i = 8; i + 12 <= png.size();) {
        size_t n = be32(png.data() + i);
        if (n > png.size() - i - 12) return false;
        if (std::string(reinterpret_cast<char*>(png.data() + i + 4), 4) == "IDAT")
            compressed.insert(compressed.end(), png.begin() + i + 8, png.begin() + i + 8 + n);
        i += n + 12;
    }
    std::vector<uint8_t> decoded(size_t(h) * (size_t(w) * 4 + 1));
    uLongf count = decoded.size();
    if (uncompress(decoded.data(), &count, compressed.data(), compressed.size()) != Z_OK ||
        count != decoded.size()) return false;
    for (int y = 0; y < h; ++y) {
        size_t row = size_t(y) * (size_t(w) * 4 + 1);
        if (decoded[row] != 0 || !std::equal(expected.begin() + size_t(y) * w * 4,
            expected.begin() + size_t(y + 1) * w * 4, decoded.begin() + row + 1)) return false;
    }
    return true;
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (!glfwInit()) { std::puts("FAIL: desktop session unavailable"); return 1; }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(130, 96, "Earthcall capture verification", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    auto ctx = wgpu::createWindowContext(window);
    if (!ctx.valid()) { std::puts("FAIL: native GPU surface unavailable"); return 1; }
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    wgpu::configureSurface(ctx, w, h);
    wgpu::Device gpu;
    gpu.instance = ctx.instance; gpu.adapter = ctx.adapter; gpu.device = ctx.device; gpu.queue = ctx.queue;
    WebGpuRenderer renderer;
    if (!renderer.init(gpu, wgpu::kSurfaceFormat)) return 1;
    renderer.attachSurface(ctx.surface, ctx.instance);
    setCurrentRenderer(&renderer);
    LawManager laws;
    ScreenRecorder::syncRegister(laws);
    auto* recorder = ScreenRecorder::find(laws);
    // Persistent artifacts are useful for Person inspection; every run has its
    // own directory. No authored world, home, Person, or identity save is edited.
    fs::path out = fs::temp_directory_path() / ("earthcall-capture-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(out);
    auto set = [&](const char* path, PropertyValue value) {
        return lawSetValue(*recorder, PropertyPath::parse(path), value);
    };
    set("outputPath", out.string());
    set("recordCursor", false);
    set("fps", 120.0);
    set("format", std::string("png_sequence"));
    set("recording", true);
    std::vector<uint8_t> pixels(size_t(w) * h * 4);
    int capturedFrames = 0;
    // Cocoa may supply an outdated drawable while the initial Retina resize
    // reaches the compositor. Mirror the live frame-skip contract, boundedly;
    // require three actual captures rather than assuming attempt 1 is ready.
    for (int attempt = 0; attempt < 10 && capturedFrames < 3; ++attempt) {
        glfwPollEvents();
        renderer.beginFrame(w, h, glm::vec4(0, 0, 1, 1));
        renderer.begin2D(w, h);
        renderer.drawTris2D({{0,0}, {float(w),0}, {0,float(h)}}, glm::vec4(1,0,0,1));
        renderer.end2D();
        renderer.endFrame();
        const bool frameReady = renderer.readPixels(pixels.data(), w, h);
        if (!frameReady) {
            std::puts("Waiting for initial native surface dimensions");
            renderer.present();
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            continue;
        }
        ++capturedFrames;
        check(!renderer.readPixels(pixels.data(), w + 1, h),
              "Oversized capture refuses before GPU validation or memory access");
        check(frameReady, "Real surface GPU readback succeeds");
        check(pixels[(size_t(h / 4) * w + w / 4) * 4] == 255 &&
              pixels[(size_t(3 * h / 4) * w + 3 * w / 4) * 4 + 2] == 255,
              "Top-left red / bottom-right blue verifies orientation and BGRA conversion");
        set("snapshot", true);
        check(recorder->checkPendingSnapshot(w, h), "Property-triggered screenshot captures before presentation");
        PropertyValue path;
        lawGetValue(*recorder, PropertyPath::parse("lastSnapshotPath"), path);
        check(pngMatches(std::get<std::string>(path), w, h, pixels), "Decoded screenshot equals every GPU pixel");
        check(recorder->stepFrame(w, h), "Real rendered frame reaches recording sequence");
        renderer.present();
        check(!renderer.readPixels(pixels.data(), w, h), "Presented surface refuses stale readback");
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    set("recording", false);
    size_t frames = 0;
    for (const auto& entry : fs::recursive_directory_iterator(out))
        if (entry.path().filename().string().find("frame_") == 0) {
            ++frames;
            check(pngMatches(entry.path(), w, h, pixels), "Decoded recording frame equals GPU pixels");
        }
    check(frames == 3, "Three sequential frames survive on disk");
    check(!recorder->captureSnapshot((out / "after_present.png").string(), w, h),
          "Capture outside frame lifetime fails honestly");
    check(!fs::exists(out / "after_present.png"), "Failed GPU capture produces no counterfeit screenshot");
    setCurrentRenderer(nullptr);
    renderer.shutdown();
    wgpu::destroyWindowContext(ctx);
    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("Artifacts: %s\nResult: %d failures\n", out.c_str(), failures);
    return failures ? 1 : 0;
}
