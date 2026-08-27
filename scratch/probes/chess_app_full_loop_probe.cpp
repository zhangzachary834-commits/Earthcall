// Probe: reproduce the RUNNING APP's click path for chess_app, not the test's
// idealized one.
//
// chess_click_geometry_test.cpp registers ONE first mover (the interaction
// channel), hands observe() a hand-built ray, and ticks the law manager only
// on the two frames of a click. The app registers every first mover, computes
// the ray from the render matrices, and ticks every frame whether or not
// anything was clicked. This probe does what the app does.
//
// How this was run (2026-08-26). tests/ is globbed at configure time and
// scratch/ is filtered out, so the probe is built by putting it where the glob
// can see it and taking it back afterwards:
//
//   cp scratch/probes/chess_app_full_loop_probe.cpp tests/law/
//   cmake -S . -B build <the flags in CLAUDE.md>
//   cmake --build build --target chess_app_full_loop_probe -j8
//   ./build/chess_app_full_loop_probe [--locked] [--jitter=N] [--hold=N] [--retina=N]
//   rm tests/law/chess_app_full_loop_probe.cpp && cmake -S . -B build <flags>
//
// Scenarios that reproduce the Person's bug:
//   --jitter=5   a real trackpad click (>6px of travel) becomes a drag, and
//                no chess law listens for a drag. Nothing happens.
//   --locked     the boot cursor state: the ray leaves the viewport CENTRE,
//                not the pointer, so every click lands on square (3,6).
//   (no args)    a perfectly still click with an unlocked cursor: works.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

extern MaterialManager materials;
extern CategoryManager categories;

namespace {

using Sense = Singularity::Input::InteractionChannel::Sense;

int asInt(Singular& being, const char* name, int fallback = -999) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return fallback;
    if (const int* i = std::get_if<int>(&v)) return *i;
    double n = 0.0;
    if (propertyValueToNumber(v, n)) return static_cast<int>(n);
    return fallback;
}

bool asBool(Singular& being, const char* name) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return false;
    if (const bool* b = std::get_if<bool>(&v)) return *b;
    double n = 0.0;
    if (propertyValueToNumber(v, n)) return n != 0.0;
    return false;
}

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

// EngineRender.cpp:30-72, verbatim in arithmetic: the matrices the channel
// unprojects through are the ones the renderer wrote LAST frame.
struct RenderMatrices {
    GLdouble modelview[16]{};
    GLdouble projection[16]{};
    int viewport[4]{};
};

RenderMatrices renderMatrices(const Core::Camera& cam, int fbW, int fbH) {
    RenderMatrices out;
    if (fbH == 0) fbH = 1;
    const float aspect = static_cast<float>(fbW) / fbH;
    const float fov = 45.0f, nearZ = 0.1f, farZ = 100.0f;
    const float top = tanf(fov * M_PI / 360.0f) * nearZ;
    const float bottom = -top;
    const float right = top * aspect;
    const float left = -right;
    // The desktop build is frustumNO; the WebGPU build is frustumZO. Both are
    // exercised below.
    glm::mat4 proj = glm::frustumNO(left, right, bottom, top, nearZ, farZ);
    glm::mat4 view = glm::lookAt(cam.pos, cam.pos + cam.front, cam.up);
    for (int i = 0; i < 16; ++i) {
        out.modelview[i] = static_cast<GLdouble>(glm::value_ptr(view)[i]);
        out.projection[i] = static_cast<GLdouble>(glm::value_ptr(proj)[i]);
    }
    out.viewport[0] = 0; out.viewport[1] = 0;
    out.viewport[2] = fbW; out.viewport[3] = fbH;
    return out;
}

// InteractionChannel::step()'s ray arithmetic, lines 346-390, with the window
// facts (retina scale, cursor lock) as parameters instead of GLFW calls.
void rayFromPointer(const RenderMatrices& m, float pointerX, float pointerY,
                    float scaleX, float scaleY, bool cursorLocked,
                    glm::vec3& origin, glm::vec3& dir) {
    const int* vp = m.viewport;
    float fbX, fbY;
    if (cursorLocked) {
        fbX = vp[0] + vp[2] * 0.5f;
        fbY = vp[1] + vp[3] * 0.5f;
    } else {
        fbX = pointerX * scaleX;
        fbY = pointerY * scaleY;
    }
    glm::mat4 V(1.0f), P(1.0f);
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            V[c][r] = static_cast<float>(m.modelview[c * 4 + r]);
            P[c][r] = static_cast<float>(m.projection[c * 4 + r]);
        }
    }
    const glm::mat4 invVP = glm::inverse(P * V);
    const float ndcX = ((fbX - vp[0]) / static_cast<float>(vp[2])) * 2.0f - 1.0f;
    const float ndcY = 1.0f - ((fbY - vp[1]) / static_cast<float>(vp[3])) * 2.0f;
    glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farW = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    if (nearW.w != 0.0f) nearW /= nearW.w;
    if (farW.w != 0.0f) farW /= farW.w;
    origin = glm::vec3(nearW);
    dir = glm::normalize(glm::vec3(farW - nearW));
}

// Project a world point to window coordinates — where a Person would have to
// put the mouse to be pointing at it.
void projectToWindow(const RenderMatrices& m, const glm::vec3& world,
                     float scaleX, float scaleY, float& outX, float& outY) {
    glm::mat4 V(1.0f), P(1.0f);
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            V[c][r] = static_cast<float>(m.modelview[c * 4 + r]);
            P[c][r] = static_cast<float>(m.projection[c * 4 + r]);
        }
    }
    glm::vec4 clip = P * V * glm::vec4(world, 1.0f);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const float fbX = (ndc.x + 1.0f) * 0.5f * m.viewport[2] + m.viewport[0];
    const float fbY = (1.0f - ndc.y) * 0.5f * m.viewport[3] + m.viewport[1];
    outX = fbX / scaleX;
    outY = fbY / scaleY;
}

} // namespace

int main(int argc, char** argv) {
    // ------------------------------------------------------------------
    // Scenario knobs — each maps to something true of the running app.
    // ------------------------------------------------------------------
    bool cursorLocked = false;   // MouseHandler boots locked; the Load panel unlocks
    float jitterPx = 0.0f;       // how far the pointer slides while the button is down
    int holdFrames = 6;          // ~100ms at 60fps
    float retina = 2.0f;         // MacBook framebuffer/window scale
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--locked") cursorLocked = true;
        else if (a.rfind("--jitter=", 0) == 0) jitterPx = std::stof(a.substr(9));
        else if (a.rfind("--hold=", 0) == 0) holdFrames = std::stoi(a.substr(7));
        else if (a.rfind("--retina=", 0) == 0) retina = std::stof(a.substr(9));
    }
    std::cout << "scenario: cursorLocked=" << (cursorLocked ? "yes" : "no")
              << " jitter=" << jitterPx << "px hold=" << holdFrames
              << " frames retina=" << retina << "x\n";

    std::string filename = "saves/worlds/chess_app.json";
    {
        const auto p = std::filesystem::absolute(filename);
        SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
    }

    Core::Camera camera;
    MouseHandler mouseHandler;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    LawManager lawManager;
    lawManager.connectToEventBus();

    // Every pointer edge the channel publishes, named as it happens. This is
    // the line the Person cannot see and the law table only half-hears.
    Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
        if (e.type.rfind("object-", 0) != 0) return;
        std::cout << "      EDGE " << e.type << " <- \""
                  << (e.subject ? e.subject->getIdentifier() : std::string("(none)"))
                  << "\"\n";
    });

    ZoneManager zones;

    // EngineInit.cpp:121-160 — the provider the running app installs.
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        if (zones.zones().empty()) return;
        beings.push_back(&zones.active());
        for (const auto& obj : zones.active().getOwnedObjects()) if (obj) beings.push_back(obj.get());
        for (const auto& law : lawManager.getAll()) if (law) beings.push_back(law.get());
        for (const auto& rel : zones.active().formation().relations().getAll()) if (rel) beings.push_back(rel.get());
        for (const auto& material : materials.getAll()) if (material) beings.push_back(material.get());
        for (const auto& category : categories.getAll()) if (category) beings.push_back(category.get());
        beings.push_back(&TransferPolicy::instance());
        beings.push_back(&player);
        for (auto& zone : zones.zones()) {
            if (zone.get() != &zones.active()) beings.push_back(zone.get());
        }
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& relations) {
        if (zones.zones().empty()) return;
        for (const auto& rel : zones.active().formation().relations().getAll()) {
            if (rel) relations.push_back(rel.get());
        }
    });

    // EngineInit.cpp:84-115 — every first mover the app registers, in order.
    Singularity::Core::CreationChannel::syncRegister(lawManager);
    Singularity::Input::LocomotionChannel::syncRegister(lawManager);
    Singularity::Input::InteractionChannel::syncRegister(lawManager);
    Singularity::Input::syncRegisterControlPatterns(lawManager, categories, player);
    Singularity::Screen::ScreenChannel::syncRegister(lawManager);
    for (const auto& law : Physics::createDefaultPhysicsLaws()) {
        law->setEnabled(!Physics::getLegacyEngineEnabled());
        lawManager.add(law);
        if (law->activation() == Law::Activation::OnEvent) {
            lawManager.bindTrigger(law->getIdentifier(), law->ecaLoop().eventType);
        }
    }
    Singularity::Core::syncRegisterCreatorTools(lawManager, player);

    auto* interaction = Singularity::Input::InteractionChannel::find(lawManager);
    assert(interaction);

    float currentColor[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouseHandler;
    ctx.currentColor = currentColor;
    ctx.player = &player;
    ctx.lawManager = &lawManager;
    ctx.worldTime = &worldTime;
    ctx.unpackForAuthoring = false;

    zones.loadState(filename, ctx);
    auto active = zones.zones()[zones.currentIndex()];
    assert(active);

    std::cout << "interaction-channel enabled after load: "
              << (interaction->isEnabled() ? "yes" : "NO") << "\n";
    std::cout << "camera pos=(" << camera.pos.x << "," << camera.pos.y << "," << camera.pos.z
              << ") front=(" << camera.front.x << "," << camera.front.y << "," << camera.front.z << ")\n";

    const int fbW = 2560, fbH = 1440;   // retina framebuffer
    RenderMatrices m = renderMatrices(camera, fbW, fbH);

    std::vector<Object*> reachable;
    for (const auto& obj : active->getOwnedObjects()) if (obj) reachable.push_back(obj.get());

    Object* pawn = findObj(*active, "piece-white-pawn-4-1");
    Object* state = nullptr;
    if (auto c = categories.get("state.chess")) state = c.get();
    assert(pawn && state);

    // Where a Person's pointer would sit to be on the e2 pawn, and on e4.
    float pawnSx, pawnSy, e4Sx, e4Sy;
    projectToWindow(m, pawn->getPosition(), retina, retina, pawnSx, pawnSy);
    projectToWindow(m, glm::vec3(0.5f, 0.0f, -0.5f), retina, retina, e4Sx, e4Sy);
    std::cout << "pointer for e2 pawn = (" << pawnSx << "," << pawnSy << ")   "
              << "pointer for e4 square = (" << e4Sx << "," << e4Sy << ")\n";

    // The app's frame: observe, then zone update, then a law tick — EVERY
    // frame, not just the ones with a click in them.
    auto frame = [&](float px, float py, bool leftDown, const char* tag) {
        Sense sense;
        sense.pointerX = px;
        sense.pointerY = py;
        sense.left = leftDown;
        sense.uiCaptured = false;
        rayFromPointer(m, px, py, retina, retina, cursorLocked,
                       sense.rayOrigin, sense.rayDirection);
        interaction->observe(sense, reachable);
        active->update(1.0f / 60.0f);
        active->applyFormationRelations();
        auto records = lawManager.tick();
        worldTime += 1.0 / 60.0;
        Universe::instance().setClock(worldTime, 1.0 / 60.0);
        if (!records.empty()) {
            std::cout << "  [" << tag << "] hovered=\"" << interaction->hoveredId
                      << "\" dragging=" << (interaction->dragging ? "YES" : "no")
                      << " -> " << records.size() << " law record(s)\n";
            for (const auto& r : records) {
                std::cout << "      " << r.lawId << " -> " << r.targetId << " "
                          << Law::resultName(r.result) << "\n";
            }
        }
    };

    auto humanClick = [&](float px, float py, const char* what) {
        std::cout << "--- human click on " << what << " at (" << px << "," << py << ") ---\n";
        frame(px, py, false, "hover");
        for (int i = 0; i < holdFrames; ++i) {
            const float t = holdFrames > 1 ? float(i) / float(holdFrames - 1) : 0.0f;
            frame(px + jitterPx * t, py + jitterPx * t, true, "down");
        }
        frame(px + jitterPx, py + jitterPx, false, "up");
        std::cout << "    after: hovered=\"" << interaction->hoveredId
                  << "\" dragging=" << (interaction->dragging ? "YES" : "no") << "\n";
    };

    // A few idle frames first, exactly as the app runs between the load and
    // the Person's first click.
    for (int i = 0; i < 5; ++i) frame(pawnSx, pawnSy, false, "idle");

    humanClick(pawnSx, pawnSy, "the e2 pawn");
    std::cout << "  pawn isSelected = " << (asBool(*pawn, "isSelected") ? "TRUE" : "false")
              << "  selectionActive = " << (asBool(*state, "selectionActive") ? "TRUE" : "false")
              << "  targetX=" << asInt(*state, "targetX") << " targetY=" << asInt(*state, "targetY")
              << "  selectedX=" << asInt(*state, "selectedX") << " selectedY=" << asInt(*state, "selectedY")
              << "\n";

    humanClick(e4Sx, e4Sy, "the e4 square");
    std::cout << "  pawn now at (" << asInt(*pawn, "gridX") << "," << asInt(*pawn, "gridY")
              << ")  turn=" << asInt(*state, "turn") << "\n";

    const bool moved = asInt(*pawn, "gridX") == 4 && asInt(*pawn, "gridY") == 3;
    std::cout << (moved ? "RESULT: pawn walked e2-e4.\n"
                        : "RESULT: **pawn did not move — this is the Person's bug**\n");
    return moved ? 0 : 1;
}
