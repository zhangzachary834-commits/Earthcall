// Zone-native regression witness for legacy chess_app: a Person must be able
// to boot Earthcall, Move to the Chess Zone, and play without loading the
// legacy conglomerate World/session first.

#include "support/test_harness.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <cassert>
#include <ctime>
#include <iostream>
#include <string>

namespace {

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

int asInt(Singular& being, const char* name, int fallback = -999) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return fallback;
    if (const int* i = std::get_if<int>(&v)) return *i;
    double n = 0.0;
    return propertyValueToNumber(v, n) ? static_cast<int>(n) : fallback;
}

bool asBool(Singular& being, const char* name) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return false;
    if (const bool* b = std::get_if<bool>(&v)) return *b;
    double n = 0.0;
    return propertyValueToNumber(v, n) && n != 0.0;
}

bool hasRelation(const Zone& zone, const std::string& type,
                 const std::string& a, const std::string& b) {
    for (const auto& relation : zone.formation().relations().getAll()) {
        if (relation && relation->type == type &&
            relation->aId() == a && relation->bId() == b) return true;
    }
    return false;
}

void click(TestSupport::BootedEngineHarness& harness, Object* subject,
           float wx, float wy, float wz) {
    assert(harness.interaction);
    harness.interaction->pointerWorld = glm::vec3(wx, wy, wz);
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", subject, nullptr, std::time(nullptr)});
    harness.lawManager.tick();
}

} // namespace

int main() {
    TestSupport::RealSaveTreeGuard guard(TestSupport::GuardCurrentRoot);
    TestSupport::BootedEngineHarness harness("Zach");

    std::size_t chessIndex = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        const auto& zone = harness.zones.zones()[i];
        if (zone && zone->getIdentifier() == "Chess") {
            chessIndex = i;
            break;
        }
    }
    assert(chessIndex < harness.zones.zones().size());
    assert(harness.zones.switchTo(chessIndex));

    auto chess = harness.zones.zones()[chessIndex];
    assert(chess && chess->getIdentifier() == "Chess");

    std::size_t chessLawCount = 0;
    for (const auto& law : harness.lawManager.getAll()) {
        if (law && law->getIdentifier().rfind("law-chess-", 0) == 0) ++chessLawCount;
    }
    assert(chessLawCount == 69);

    // 143 is the complete preserved authored graph. The Law-category edges
    // cannot bind during boot before shared Laws exist; switchTo() must retry
    // relation hydration after activating those roots.
    assert(chess->formation().relations().getAll().size() == 143);
    assert(hasRelation(*chess, "instance-of",
                       "piece-white-pawn-4-1", "category.chess.piece"));
    assert(hasRelation(*chess, "instance-of",
                       "law-chess-click", "category.chess.law.interaction"));

    Object* board = findObj(*chess, "object.chess.board");
    Object* pawn = findObj(*chess, "piece-white-pawn-4-1");
    Object* state = findObj(*chess, "state.chess");
    Object* author = findObj(*chess, "grok-4.6");
    assert(board && pawn && state && author);
    assert(asInt(*pawn, "gridX") == 4 && asInt(*pawn, "gridY") == 1);

    Universe::instance().setClock(0.0, 1.0 / 60.0);
    click(harness, pawn, 0.5f, 0.3f, -2.5f);
    assert(asBool(*pawn, "isSelected"));

    // Same mechanical e2 -> e4 witness as chess_app_test, but with no legacy
    // loadState() call anywhere in this process.
    click(harness, board, 0.5f, 0.0f, -0.5f);
    assert(asInt(*pawn, "gridX") == 4);
    assert(asInt(*pawn, "gridY") == 3);

    std::cout << "Zone-native fresh boot entered Chess, hydrated all 69 Laws/143 relations, "
                 "and moved e2 to e4 without a legacy World load.\n";
    return 0;
}
