// Does the REAL picking pipeline (InteractionChannel::observe ->
// pickSurface -> Object::raycastFace) actually hit each chess piece shape
// kind, the way a Person's mouse ray would?

#include "support/test_harness.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

Object* findCat(const std::string& id) {
    auto c = categories.get(id);
    return c ? c.get() : nullptr;
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

std::string realClick(Singularity::Input::InteractionChannel* interaction,
                      LawManager& lawManager,
                      const std::vector<Object*>& reachable,
                      const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    Singularity::Input::InteractionChannel::Sense sense;
    sense.rayOrigin = rayOrigin;
    sense.rayDirection = rayDir;

    sense.left = true;
    interaction->observe(sense, reachable);
    const std::string hovered = interaction->hoveredId;

    sense.left = false;
    interaction->observe(sense, reachable);

    auto records = lawManager.tick();
    std::cout << "  [click on \"" << hovered << "\"] tick records: " << records.size() << "\n";
    for (const auto& r : records) {
        std::cout << "    " << r.lawId << " -> " << r.targetId
                  << " " << Law::resultName(r.result) << "\n";
    }
    return hovered;
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
        if (p.parent_path().filename() == "worlds" &&
            p.parent_path().parent_path().filename() == "saves") {
            SaveSystem::setSaveRoot(p.parent_path().parent_path().string());
        }
    }

    TestSupport::BootedEngineHarness harness;
    harness.loadWorld(filename);

    assert(!harness.zones.zones().empty());
    auto active = harness.zones.zones()[harness.zones.currentIndex()];
    assert(active);

    Universe::instance().setClock(0.0, 1.0 / 60.0);

    std::vector<Object*> reachable;
    for (const auto& obj : active->getOwnedObjects()) if (obj) reachable.push_back(obj.get());
    std::cout << "reachable objects: " << reachable.size() << "\n";

    // One piece per shape kind chess actually uses (author_chess.py SHAPES).
    struct Case { const char* id; const char* label; };
    Case cases[] = {
        {"piece-white-pawn-4-1",   "pawn (Sphere)"},
        {"piece-white-rook-0-0",   "rook (Cube)"},
        {"piece-white-knight-1-0", "knight (Ellipsoid)"},
        {"piece-white-bishop-2-0", "bishop (Cone)"},
        {"piece-white-queen-3-0",  "queen (Ovoid)"},
        {"piece-white-king-4-0",   "king (Cylinder)"},
    };

    bool allHit = true;
    for (const auto& c : cases) {
        Object* obj = findObj(*active, c.id);
        if (!obj) { std::cout << "  MISSING BEING: " << c.id << "\n"; allHit = false; continue; }
        glm::vec3 pos = obj->getPosition();

        Singularity::Input::InteractionChannel::Sense sense;
        sense.rayOrigin = pos + glm::vec3(0.0f, 5.0f, 0.0f);
        sense.rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
        sense.left = false;
        harness.interaction->observe(sense, reachable);
        const std::string hit = harness.interaction->hoveredId;
        const bool ok = (hit == c.id);
        std::cout << "  " << c.label << " id=" << c.id
                  << " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")"
                  << " -> hovered=\"" << hit << "\" "
                  << (ok ? "OK" : "**MISS**") << "\n";
        if (!ok) allHit = false;
    }

    // ------------------------------------------------------------------
    // Phase 2: an actual press-then-release move, driven end to end through
    // InteractionChannel::observe() — real picking chooses the subject.
    // ------------------------------------------------------------------
    Object* state = findCat("state.chess");
    Object* whitePawnE2 = findObj(*active, "piece-white-pawn-4-1");
    Object* board = findObj(*active, "object.chess.board");
    assert(state && whitePawnE2 && board);

    std::cout << "--- real click: e2 pawn, then e4 square (through observe(), not publish()) ---\n";
    glm::vec3 pawnPos = whitePawnE2->getPosition();
    std::string firstHit = realClick(harness.interaction, harness.lawManager, reachable,
                                     pawnPos + glm::vec3(0.0f, 5.0f, 0.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
    bool moveOk = (firstHit == "piece-white-pawn-4-1");
    if (!moveOk) std::cout << "  **the ray did not pick the pawn at all — got \"" << firstHit << "\"**\n";
    moveOk = moveOk && asBool(*whitePawnE2, "isSelected");
    if (!moveOk) std::cout << "  **pawn was picked but is not isSelected after the real click**\n";

    std::string secondHit = realClick(harness.interaction, harness.lawManager, reachable,
                                      glm::vec3(0.5f, 5.0f, -0.5f),
                                      glm::vec3(0.0f, -1.0f, 0.0f));
    moveOk = moveOk && (secondHit == "object.chess.board");
    if (secondHit != "object.chess.board")
        std::cout << "  **the second ray did not pick the board — got \"" << secondHit << "\"**\n";
    const int gx = asInt(*whitePawnE2, "gridX");
    const int gy = asInt(*whitePawnE2, "gridY");
    std::cout << "  pawn now at (" << gx << "," << gy << ") turn=" << asInt(*state, "turn") << "\n";
    moveOk = moveOk && (gx == 4 && gy == 3);
    std::cout << (moveOk ? "  pawn walked e2-e4 through a REAL click sequence\n"
                         : "  **pawn did NOT move — this is what a Person's click looks like when it fails**\n");
    allHit = allHit && moveOk;

    // ------------------------------------------------------------------
    // Phase 3: Camera projection pick with realistic mouse movement
    // (velocity deltas before click). Moves black pawn e7 -> e5.
    // ------------------------------------------------------------------
    Object* blackPawnE7 = findObj(*active, "piece-black-pawn-4-6");
    assert(blackPawnE7);

    std::cout << "--- Phase 3: black pawn e7 -> e5 via camera perspective picking ---\n";
    int fbW = 1280, fbH = 720;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)fbW / (float)fbH, 0.1f, 200.0f);
    glm::mat4 view = glm::lookAt(harness.camera.pos, harness.camera.pos + harness.camera.front, harness.camera.up);
    glm::mat4 invVP = glm::inverse(proj * view);

    auto unprojectScreen = [&](float sx, float sy, glm::vec3& outOrig, glm::vec3& outDir) {
        float ndcX = (sx / (float)fbW) * 2.0f - 1.0f;
        float ndcY = 1.0f - (sy / (float)fbH) * 2.0f;
        glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farW = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearW /= nearW.w;
        farW /= farW.w;
        outOrig = glm::vec3(nearW);
        outDir = glm::normalize(glm::vec3(farW - nearW));
    };

    glm::vec3 e7Pos = blackPawnE7->getPosition();
    glm::vec4 e7Clip = proj * view * glm::vec4(e7Pos, 1.0f);
    glm::vec3 e7Ndc = glm::vec3(e7Clip) / e7Clip.w;
    float e7Sx = (e7Ndc.x + 1.0f) * 0.5f * fbW;
    float e7Sy = (1.0f - e7Ndc.y) * 0.5f * fbH;

    glm::vec3 e5Pos(0.5f, 0.0f, 0.5f);
    glm::vec4 e5Clip = proj * view * glm::vec4(e5Pos, 1.0f);
    glm::vec3 e5Ndc = glm::vec3(e5Clip) / e5Clip.w;
    float e5Sx = (e5Ndc.x + 1.0f) * 0.5f * fbW;
    float e5Sy = (1.0f - e5Ndc.y) * 0.5f * fbH;

    Singularity::Input::InteractionChannel::Sense simSense;
    unprojectScreen(e7Sx - 25.0f, e7Sy - 30.0f, simSense.rayOrigin, simSense.rayDirection);
    simSense.pointerX = e7Sx - 25.0f;
    simSense.pointerY = e7Sy - 30.0f;
    simSense.left = false;
    harness.interaction->observe(simSense, reachable);

    unprojectScreen(e7Sx, e7Sy, simSense.rayOrigin, simSense.rayDirection);
    simSense.pointerX = e7Sx;
    simSense.pointerY = e7Sy;
    simSense.left = true;
    harness.interaction->observe(simSense, reachable);

    simSense.left = false;
    harness.interaction->observe(simSense, reachable);
    harness.lawManager.tick();

    bool blackPawnSelected = asBool(*blackPawnE7, "isSelected");
    if (!blackPawnSelected) {
        std::cout << "  **black pawn e7 was not selected after click**\n";
    }

    unprojectScreen(e5Sx - 40.0f, e5Sy - 40.0f, simSense.rayOrigin, simSense.rayDirection);
    simSense.pointerX = e5Sx - 40.0f;
    simSense.pointerY = e5Sy - 40.0f;
    simSense.left = false;
    harness.interaction->observe(simSense, reachable);

    unprojectScreen(e5Sx, e5Sy, simSense.rayOrigin, simSense.rayDirection);
    simSense.pointerX = e5Sx;
    simSense.pointerY = e5Sy;
    simSense.left = true;
    harness.interaction->observe(simSense, reachable);

    simSense.left = false;
    harness.interaction->observe(simSense, reachable);
    harness.lawManager.tick();

    const int b_gx = asInt(*blackPawnE7, "gridX");
    const int b_gy = asInt(*blackPawnE7, "gridY");
    std::cout << "  black pawn now at (" << b_gx << "," << b_gy << ") turn=" << asInt(*state, "turn") << "\n";
    bool blackMoveOk = blackPawnSelected && (b_gx == 4 && b_gy == 4) && (asInt(*state, "turn") == 0);
    allHit = allHit && blackMoveOk;

    std::cout << "==================================================\n";
    std::cout << (allHit ? "chess_click_geometry_test: ALL OK"
                          : "chess_click_geometry_test: FAILURES FOUND")
              << "\n";
    std::cout << "==================================================\n";
    return allHit ? 0 : 1;
}
