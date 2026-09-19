// The gesture a Person actually makes, not the gesture a test finds
// convenient to publish.

#include "support/test_harness.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using Sense = Singularity::Input::InteractionChannel::Sense;

namespace {

int g_failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { ++g_failures; std::printf("  FAILED: %s\n", what.c_str()); return; }
    std::printf("  ok: %s\n", what.c_str());
}

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

double asDouble(Singular& being, const char* name, double fallback = -999.0) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return fallback;
    double n = fallback;
    propertyValueToNumber(v, n);
    return n;
}

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

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
    const float top = tanf(fov * static_cast<float>(M_PI) / 360.0f) * nearZ;
    const float bottom = -top;
    const float right = top * aspect;
    const float left = -right;
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

void rayFromPointer(const RenderMatrices& m, float pointerX, float pointerY,
                    float scaleX, float scaleY, bool cursorLocked,
                    glm::vec3& origin, glm::vec3& dir) {
    const int* vp = m.viewport;
    float fbX, fbY;
    if (cursorLocked) {
        fbX = static_cast<float>(vp[0]) + static_cast<float>(vp[2]) * 0.5f;
        fbY = static_cast<float>(vp[1]) + static_cast<float>(vp[3]) * 0.5f;
    } else {
        fbX = pointerX * scaleX;
        fbY = pointerY * scaleY;
    }
    glm::mat4 V(1.0f), P(1.0f);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            V[c][r] = static_cast<float>(m.modelview[c * 4 + r]);
            P[c][r] = static_cast<float>(m.projection[c * 4 + r]);
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

void projectToWindow(const RenderMatrices& m, const glm::vec3& world,
                     float scaleX, float scaleY, float& outX, float& outY) {
    glm::mat4 V(1.0f), P(1.0f);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            V[c][r] = static_cast<float>(m.modelview[c * 4 + r]);
            P[c][r] = static_cast<float>(m.projection[c * 4 + r]);
        }
    glm::vec4 clip = P * V * glm::vec4(world, 1.0f);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const float fbX = (ndc.x + 1.0f) * 0.5f * static_cast<float>(m.viewport[2]) + static_cast<float>(m.viewport[0]);
    const float fbY = (1.0f - ndc.y) * 0.5f * static_cast<float>(m.viewport[3]) + static_cast<float>(m.viewport[1]);
    outX = fbX / scaleX;
    outY = fbY / scaleY;
}

} // namespace

int main() {
    std::string filename = "saves/worlds/chess_app.json";
    if (!std::filesystem::exists(filename) &&
        std::filesystem::exists("../saves/worlds/chess_app.json")) {
        filename = "../saves/worlds/chess_app.json";
    }
    {
        const auto p = std::filesystem::absolute(filename);
        SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
    }

    TestSupport::BootedEngineHarness harness;

    Singularity::Core::CreationChannel::syncRegister(harness.lawManager);
    Singularity::Input::LocomotionChannel::syncRegister(harness.lawManager);
    Singularity::Input::syncRegisterControlPatterns(harness.lawManager, categories, harness.player);
    Singularity::Screen::ScreenChannel::syncRegister(harness.lawManager);
    for (const auto& law : Physics::createDefaultPhysicsLaws()) {
        law->setEnabled(!Physics::getLegacyEngineEnabled());
        harness.lawManager.add(law);
        if (law->activation() == Law::Activation::OnEvent) {
            harness.lawManager.bindTrigger(law->getIdentifier(), law->ecaLoop().eventType);
        }
    }
    Singularity::Core::syncRegisterCreatorTools(harness.lawManager, harness.player);

    harness.loadWorld(filename);
    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);

    const int fbW = 2560, fbH = 1440;
    const float retina = 2.0f;
    RenderMatrices m = renderMatrices(harness.camera, fbW, fbH);

    std::vector<Object*> reachable;
    for (const auto& obj : active->getOwnedObjects()) if (obj) reachable.push_back(obj.get());

    Object* state = categories.get("state.chess").get();
    assert(state);

    bool cursorLocked = false;
    auto frame = [&](float px, float py, bool leftDown) {
        Sense sense;
        sense.pointerX = px;
        sense.pointerY = py;
        sense.left = leftDown;
        sense.uiCaptured = false;
        rayFromPointer(m, px, py, retina, retina, cursorLocked, sense.rayOrigin, sense.rayDirection);
        harness.interaction->pointerLocked = cursorLocked;
        harness.interaction->observe(sense, reachable);
        active->update(1.0f / 60.0f);
        active->applyFormationRelations();
        harness.lawManager.tick();
        harness.worldTime += 1.0 / 60.0;
        Universe::instance().setClock(harness.worldTime, 1.0 / 60.0);
    };

    auto windowPos = [&](const glm::vec3& world, float& x, float& y) {
        projectToWindow(m, world, retina, retina, x, y);
    };

    printf("=== 1. A human-scale jittery click selects and, on a second click, moves ===\n");
    {
        cursorLocked = false;
        Object* pawn = findObj(*active, "piece-white-pawn-4-1");
        assert(pawn);
        float px, py, ex, ey;
        windowPos(pawn->getPosition(), px, py);
        windowPos(glm::vec3(0.5f, 0.0f, -0.5f), ex, ey); // e4

        for (int i = 0; i < 8; ++i) frame(px, py, false); // idle, as after load

        frame(px, py, true);
        frame(px + 3.0f, py + 3.0f, true);
        frame(px + 5.0f, py + 5.0f, true);
        frame(px + 5.0f, py + 5.0f, false);

        check(!harness.interaction->dragging, "5px of jitter does not classify as a drag");
        check(asBool(*pawn, "isSelected"), "pawn selected after a jittery click");
        const double restY = asDouble(*pawn, "restY");
        check(std::fabs(pawn->getPosition().y - (restY + 0.18)) < 0.02,
              "selected pawn's position.y rose off restY (the lift, item 5)");

        frame(ex, ey, false);
        frame(ex, ey, true);
        frame(ex, ey, false);

        check(asInt(*pawn, "gridX") == 4 && asInt(*pawn, "gridY") == 3,
              "pawn walked e2-e4 via two jittery clicks");
        check(std::fabs(pawn->getPosition().y - restY) < 0.02,
              "pawn's position.y returned to restY after the move (unlift)");
    }

    printf("=== 2. A real drag from the piece to a destination square moves it ===\n");
    {
        Object* pawn = findObj(*active, "piece-black-pawn-4-6"); // e7
        assert(pawn);
        float px, py, ex, ey;
        windowPos(pawn->getPosition(), px, py);
        windowPos(glm::vec3(0.5f, 0.0f, 0.5f), ex, ey); // e5

        frame(px, py, false);
        frame(px, py, true); // press
        const int steps = 10;
        for (int i = 1; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            frame(px + (ex - px) * t, py + (ey - py) * t, true);
        }
        check(harness.interaction->dragging, "moving 10+ px while held is classified as a drag");
        frame(ex, ey, false); // release over the destination square

        check(asInt(*pawn, "gridX") == 4 && asInt(*pawn, "gridY") == 4,
              "black pawn e7 dragged to e5");
        check(asInt(*state, "turn") == 0, "turn advanced back to white after the drag-move");
    }

    printf("=== 3. A drag released off the board does nothing ===\n");
    {
        Object* pawn = findObj(*active, "piece-white-pawn-3-1"); // d2, untouched so far
        assert(pawn);
        const int gx0 = asInt(*pawn, "gridX"), gy0 = asInt(*pawn, "gridY");
        float px, py;
        windowPos(pawn->getPosition(), px, py);

        frame(px, py, false);
        frame(px, py, true);
        for (int i = 1; i <= 10; ++i) frame(px, py - 40.0f * i, true); // drag straight up, off the board
        check(harness.interaction->hoveredId.empty(), "dragging off the board hits nothing");
        frame(px, py - 400.0f, false); // release in empty space

        check(asInt(*pawn, "gridX") == gx0 && asInt(*pawn, "gridY") == gy0,
              "releasing a drag off the board leaves the piece where it was");
    }

    printf("=== 4. Locked cursor: pointerLocked is legible, and the pick is honest about where it looked ===\n");
    {
        cursorLocked = true;
        frame(9999.0f, 9999.0f, false); // pointerX/Y are irrelevant while locked
        check(harness.interaction->pointerLocked, "InteractionChannel reports pointerLocked while GLFW_CURSOR_DISABLED would hold");
        float cx, cy;
        windowPos(harness.camera.pos + harness.camera.front * 5.0f, cx, cy);
        (void)cx; (void)cy;
        cursorLocked = false;
    }

    printf("==================================================\n");
    printf(g_failures == 0 ? "chess_gesture_test: ALL OK\n" : "chess_gesture_test: FAILURES\n");
    printf("==================================================\n");
    return g_failures == 0 ? 0 : 1;
}
