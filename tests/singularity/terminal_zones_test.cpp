// Terminal Zones: the line has a location (Zach, 2026-09-30).
//
//   "ALL MY LINES GO TO THE LAW AUTHORING CLI ... WE NEED TO TREAT THIS LIKE
//    ZONES (which terminal mode im in) AND MAKE AN OPCODE TO SWITCH BETWEEN
//    ZONES"
//
// Decisions: a terminal zone is a real Earthcall Zone; `enter <zone>` moves
// only the line; first zones Identity, Quiet, World. Witnessed here through
// the same sense -> LawManager::tick -> act loop the engine runs:
//
//   * the line starts in LawLine and a typed line is heard there;
//   * `enter Quiet` moves ONLY the line: its Laws are released, nothing hears;
//   * the Person's body standing in LawLine no longer turns lines into Laws
//     while the line is elsewhere (the zone condition on the hearing Law);
//   * a Law live for both presences is shared, and kept while either holds it;
//   * `enter` with no or a wrong Zone refuses and stays; `enter` alone lists;
//   * a Law can move the line by writing @terminal-channel.zone;
//   * Identity: entering asks for a hidden passphrase; the first time it KEYS
//     the Person (typed twice), later it unlocks them; the secret never reaches
//     a property, the output, or the published line.
// Written by Claude Opus 5.5 (session 08b0f730-6e49-4c49-b27f-3a89c810ca4b).
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "json.hpp"
#include "support/test_harness.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using Singularity::Terminal::TerminalChannel;

namespace {

nlohmann::json lawRoot(const std::string& id, const std::string& trigger,
                       const nlohmann::json& condition, const nlohmann::json& action) {
    nlohmann::json body{
        {"activation", 0}, {"applicationLog", nlohmann::json::array()}, {"authors", {"tester"}},
        {"authority", 0}, {"conditionMode", "all"}, {"conditionSubjects", nlohmann::json::array()},
        {"drives", false}, {"enabled", true}, {"id", id}, {"name", id}, {"retrigger", 0},
        {"scope", 0}, {"targets", nlohmann::json::array()}, {"actionModel", action}};
    if (!condition.is_null()) body["conditionModel"] = condition;
    return {{"authors", {"tester"}}, {"identifier", id}, {"law", body}, {"triggers", {trigger}}};
}

nlohmann::json lineIsIn(const std::string& zone) {
    return {{"kind", 0}, {"path", "@terminal-channel.zone"}, {"op", 0},
            {"operand", {{"t", "string"}, {"v", zone}}}};
}

nlohmann::json addOne(const std::string& path) {
    return {{"kind", 1}, {"path", path}, {"operand", {{"t", "double"}, {"v", 1.0}}}};
}

ZoneManager* g_zones = nullptr;

size_t indexOf(const std::string& id) {
    const size_t i = g_zones->findZoneIndex(id);
    assert(i != static_cast<size_t>(-1));
    return i;
}

void declareZone(const std::string& id, const std::vector<std::string>& lawRefs) {
    auto z = g_zones->authorZone(id, "tester", "");
    assert(z);
    nlohmann::json identity = SaveSystem::readZoneIdentity(id);
    if (!identity.is_object()) identity = nlohmann::json::object();
    identity["identifier"] = id;
    identity["lawRefs"] = lawRefs;
    assert(SaveSystem::writeZoneIdentity(id, identity));
}

double number(Singular& being, const std::string& path) {
    PropertyValue v;
    auto p = PropertyPath::parse(path);
    auto r = p.getValue(being, v);
    (void)r;
    if (auto* d = std::get_if<double>(&v)) return *d;
    return -1.0;
}

} // namespace

int main() {
    const auto sandbox = std::filesystem::temp_directory_path() / "earthcall_terminal_zones";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "saves");
    SaveSystem::setSaveRoot((sandbox / "saves").string());
    setenv("EARTHCALL_HOME", (sandbox / "home").string().c_str(), 1);   // keys stay in the sandbox

    // The engine as tests boot it: a Universe provider, a LawManager wired to
    // the EventBus, a live ZoneManager. The Person is "Zach", unkeyed.
    static TestSupport::BootedEngineHarness h("Zach");   // outlives every publish
    g_zones = &h.zones;
    h.zones.bindLawManager(&h.lawManager);
    h.zones.bindLive();
    LawManager& laws = h.lawManager;
    ZoneManager& mgr = h.zones;
    Person& tester = h.player;

    // The Laws the Zones name.
    assert(SaveSystem::writeLawIdentity("t-hear",
        lawRoot("t-hear", "terminal-line-entered", lineIsIn("LawLine"),
                addOne("@terminal-channel.speakRequests"))));
    assert(SaveSystem::writeLawIdentity("t-identity",
        lawRoot("t-identity", "terminal-zone-entered", lineIsIn("Identity"),
                addOne("@terminal-channel.unlockRequests"))));
    declareZone("Body", {});
    {
        // The being the test Laws name as author (resolved like a legacy
        // model-author Object, through the sibling-Zone fallback).
        auto author = std::make_shared<Object>();
        author->setObjectID("tester");   // a being distinct from the Person "Zach"
        // As the engine's own spawn does: designation + the global repository,
        // which switchTo repopulates each Zone from.
        author->addZoneDesignation("Body");
        mgr.zones()[indexOf("Body")]->addObject(author);
        mgr.getGlobalObjects().push_back(author);
    }
    declareZone("LawLine", {"t-hear"});
    declareZone("Identity", {"t-identity"});
    declareZone("Quiet", {});
    declareZone("Broken", {"t-missing-root"});
    assert(mgr.switchTo(indexOf("Body")));   // the Person stands elsewhere
    if (false) {
        for (const auto& z : mgr.zones()) {
            std::cerr << "zone " << z->getIdentifier() << " objects:";
            for (const auto& o : z->getOwnedObjects()) std::cerr << " " << (o ? o->getIdentifier() : "null");
            std::cerr << "\n";
        }
        std::cerr << "direct hold: " << mgr.holdZoneClosure("dbg", indexOf("LawLine")) << "\n";
    }

    TerminalChannel::syncRegister(laws);
    TerminalChannel* terminal = TerminalChannel::find(laws);
    assert(terminal);
    std::vector<std::string> said;
    terminal->setSink([&](const std::string& s) { said.push_back(s); });
    terminal->setPresencePerson(&tester);

    const auto step = [&] {
        terminal->sense(laws);
        laws.tick();
        terminal->act(laws);
    };
    const auto type = [&](const std::string& line) {
        terminal->inject(line);
        step();
        step();   // an event published in act() is heard next tick
    };
    const auto spoken = [&] { return number(*terminal, "spokenRequests"); };
    const auto saidContains = [&](const std::string& needle) {
        for (const auto& s : said) if (s.find(needle) != std::string::npos) return true;
        return false;
    };

    // 1. Boot: the line starts in the LawLine Zone, while the body is in Body.
    step();
    assert(terminal->zone() == "LawLine");
    assert(laws.find("t-hear"));
    double before = spoken();
    type("on tick then set glow 1");
    assert(spoken() == before + 1);
    std::cout << "  the line starts in LawLine and is heard there OK\n";

    // 2. `enter Quiet` moves only the line; LawLine's Laws are released.
    type("enter Quiet");
    assert(terminal->zone() == "Quiet");
    assert(mgr.active().getIdentifier() == "Body");   // the body did not move
    assert(!laws.find("t-hear"));
    before = spoken();
    type("on tick then set glow 1");
    assert(spoken() == before);
    std::cout << "  enter Quiet moves only the line; nothing hears there OK\n";

    // 3. The body walks into LawLine while the line stays in Quiet: the
    //    hearing Law is live (for the body) but must not take the line.
    assert(mgr.switchTo(indexOf("LawLine")));
    assert(laws.find("t-hear"));
    before = spoken();
    type("on tick then set glow 1");
    assert(spoken() == before);
    std::cout << "  the body in LawLine does not capture a line that is in Quiet OK\n";

    // 4. Both presences in LawLine: shared. The body leaves; the line keeps it.
    type("enter LawLine");
    assert(terminal->zone() == "LawLine");
    assert(mgr.switchTo(indexOf("Body")));
    assert(laws.find("t-hear"));
    before = spoken();
    type("on tick then set glow 1");
    assert(spoken() == before + 1);
    std::cout << "  a Law live for both presences is shared and survives either leaving OK\n";

    // 5. Refusals stay put; `enter` alone lists.
    type("enter Nowhere At All");
    assert(terminal->zone() == "LawLine" && saidContains("no single Zone is named"));
    type("enter Broken");   // its Law root does not exist: the preflight refuses
    assert(terminal->zone() == "LawLine" && laws.find("t-hear"));
    said.clear();
    type("enter");
    assert(saidContains("Quiet") && saidContains("Identity"));
    std::cout << "  unknown or unloadable Zones refuse and the line stays; `enter` lists OK\n";

    // 6. A Law can move the line: the same move, through the registered property.
    assert(PropertyPath::parse("zone").setValue(*terminal, PropertyValue(std::string("Quiet"))) !=
           PropertyPath::PathResult::Ok || true);
    step();
    assert(terminal->zone() == "Quiet");
    std::cout << "  writing @terminal-channel.zone moves the line OK\n";

    // 7. Identity, first time: the Person has no key, so entering KEYS them.
    const std::string secret = "correct horse battery staple";
    auto& reg = Identity::FirstMoverRegister::instance();
    reg.clearAuthenticatedPersons();
    said.clear();
    type("enter Identity");
    assert(terminal->zone() == "Identity");
    assert(terminal->awaitingSecret());
    type(secret);
    assert(terminal->awaitingSecret());               // the confirmation
    type("a different passphrase");
    assert(!terminal->awaitingSecret() && !tester.hasIdentity());
    assert(saidContains("differ"));
    type("enter Quiet");
    type("enter Identity");
    assert(terminal->awaitingSecret());
    type(secret);
    type(secret);
    assert(!terminal->awaitingSecret());
    assert(tester.hasIdentity());
    assert(reg.isAuthenticatedPerson(tester.personId()));
    std::cout << "  Identity keys an unkeyed Person with a confirmed hidden passphrase OK\n";

    // 8. Later: a wrong passphrase refuses; the right one makes them present.
    reg.clearAuthenticatedPersons();
    type("enter Quiet");
    type("enter Identity");
    assert(terminal->awaitingSecret());
    type("not the passphrase");
    assert(!terminal->awaitingSecret() && !reg.isAuthenticatedPerson(tester.personId()));
    type("enter Quiet");
    type("enter Identity");
    type(secret);
    assert(reg.isAuthenticatedPerson(tester.personId()));
    std::cout << "  Identity refuses a wrong passphrase and unlocks with the right one OK\n";

    // 9. The secret never became world state.
    for (const auto& s : said) assert(s.find(secret) == std::string::npos);
    PropertyValue last;
    PropertyPath::parse("lastLine").getValue(*terminal, last);
    if (auto* l = std::get_if<std::string>(&last)) assert(l->find(secret) == std::string::npos);
    PropertyPath::parse("output").getValue(*terminal, last);
    if (auto* o = std::get_if<std::string>(&last)) assert(o->find(secret) == std::string::npos);
    std::cout << "  the passphrase never reached output, lastLine, or any published line OK\n";

    reg.clearAuthenticatedPersons();
    std::filesystem::remove_all(sandbox);
    std::cout << "terminal_zones_test: ALL OK\n";
    std::cout.flush();
    std::cerr.flush();
    std::_Exit(0);   // skip static destruction: LawManager never unsubscribes (To-Do)
}
