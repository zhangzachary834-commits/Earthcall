// End-to-end witness for the seeded Law Line (scripts/seed_law_line.py).
//
// Boots the LawLine Zone identity and its shared Law roots in an isolated save
// root, enters the Zone, and speaks at the Terminal channel exactly as the Mac
// Terminal would — through the injection seam instead of a TTY. The seeded
// Lexemes --denotes--> Laws supply the words, the seeded wiring Laws decide
// the line is spoken, and the Law it authors must govern the seeded cube.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "json.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
int checks = 0;
int failures = 0;

void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}

struct Scratch {
    std::filesystem::path path;
    ~Scratch() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

bool mentions(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool contains(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}
} // namespace

int main() {
    std::filesystem::path saves = "saves";
    if (!std::filesystem::exists(saves / "zones/LawLine/zone.json")) saves = std::filesystem::path("..") / "saves";
    saves = std::filesystem::absolute(saves);
    const auto sourceZone = saves / "zones/LawLine/zone.json";
    check(std::filesystem::exists(sourceZone), "LawLine Zone identity exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson;
    {
        std::ifstream in(sourceZone);
        in >> zoneJson;
    }

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_law_line_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directories(scratch.path / "zones/LawLine");
    std::filesystem::copy_file(sourceZone, scratch.path / "zones/LawLine/zone.json");
    for (const auto& ref : zoneJson["lawRefs"]) {
        const std::string id = ref.get<std::string>();
        std::filesystem::create_directories(scratch.path / "laws" / id);
        std::filesystem::copy_file(saves / "laws" / id / "law.json", scratch.path / "laws" / id / "law.json");
    }

    SaveSystem::setSaveRoot(scratch.path.string());
    TestSupport::BootedEngineHarness harness("Zach");
    harness.zones.bindLive();
    Singularity::Terminal::TerminalChannel::syncRegister(harness.lawManager);
    auto* terminal = Singularity::Terminal::TerminalChannel::find(harness.lawManager);
    std::vector<std::string> printed;
    terminal->setSink([&](const std::string& s) { printed.push_back(s); });
    harness.interaction->setPointingPerson(&harness.player);

    std::size_t index = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        if (harness.zones.zones()[i] && harness.zones.zones()[i]->getIdentifier() == "LawLine") index = i;
    }
    check(index < harness.zones.zones().size(), "boot discovers the LawLine Zone");
    if (index >= harness.zones.zones().size()) return 1;
    check(harness.zones.switchTo(index), "entering LawLine loads its Laws (disabled presets included)");
    check(harness.lawManager.find("law-line-preset-event") != nullptr, "the event-triggered preset is present");
    check(harness.lawManager.find("law-line-hear") != nullptr, "the hearing Law is present");
    harness.lawManager.tick();   // zone-entered -> the scope Law points bare paths at the cube

    // The seeded vocabulary: Lexemes denote Laws.
    const auto vocab = terminal->vocabulary(harness.lawManager);
    const auto has = [&](const std::string& symbol, const std::string& opcode) {
        return std::any_of(vocab.words.begin(), vocab.words.end(), [&](const auto& w) {
            return w.symbol == symbol && w.opcode == opcode && !w.lexemeId.empty();
        });
    };
    check(has("greater than", "op.Gt"), "'greater than' denotes the Gt comparison Law");
    check(has("my event-triggered law", "preset"), "'my event-triggered law' denotes a preset Law");
    check(has("always", "preset"), "'always' denotes the constantly-applied preset");
    check(has("grant", "action.AddProperty"), "'grant' denotes the AddProperty Law");
    check(contains(Singularity::Terminal::LawSentence::complete("my ev", vocab), "event-triggered law"),
          "Tab completes the preset phrase");
    check(contains(Singularity::Terminal::LawSentence::complete("on object-clicked then set co", vocab), "color"),
          "Tab completes a property of the scoped cube");

    Object* cube = nullptr;
    for (const auto& object : harness.zones.zones()[index]->getOwnedObjects()) {
        if (object && object->getIdentifier() == "law-line-cube") cube = object.get();
    }
    check(cube != nullptr, "the seeded cube exists");
    if (!cube) return 1;

    const auto frame = [&] {
        terminal->sense(harness.lawManager);
        for (int i = 0; i < 3; ++i) harness.lawManager.tick();
        terminal->act(harness.lawManager);
    };

    terminal->inject("my event-triggered law called Red fires on object-clicked if hp is greater than 2 "
                     "then set color 1 0 0");
    frame();
    PropertyValue last;
    lawGetValue(*terminal, PropertyPath::parse("lastCreated"), last);
    const std::string newbornId = std::holds_alternative<std::string>(last) ? std::get<std::string>(last) : "";
    check(!newbornId.empty() && newbornId.rfind("law_", 0) == 0, "the sentence authored a Law: " + newbornId);
    for (const auto& line : printed) std::cout << "    terminal> " << line << '\n';
    Law* newborn = harness.lawManager.find(newbornId);
    check(newborn && newborn->authors().getMembers().size() == 1 &&
              newborn->authors().getMembers().front() == &harness.player,
          "the Person at the machine is its author");

    Core::EventBus::instance().publish(ECA::Event{"object-clicked", cube, nullptr, std::time(nullptr)});
    harness.lawManager.tick();
    PropertyValue color;
    lawGetValue(*cube, PropertyPath::parse("color"), color);
    const glm::vec3* c = std::get_if<glm::vec3>(&color);
    check(c && std::fabs(c->x - 1.0f) < 0.03f && c->y < 0.03f && c->z < 0.03f,
          "clicking the cube turns it red under the spoken Law");

    // The second seed pass: value words and trigger presets, all authored.
    const auto words = terminal->vocabulary(harness.lawManager);
    const auto hasWord = [&](const std::string& symbol, const std::string& opcode) {
        return std::any_of(words.words.begin(), words.words.end(), [&](const auto& w) {
            return w.symbol == symbol && w.opcode == opcode;
        });
    };
    check(hasWord("gold", "value") && hasWord("on", "value") && hasWord("on", "clause.trigger"),
          "'gold' and 'on' are value words, and 'on' is still the trigger word where a clause begins");
    check(hasWord("when hovered", "preset"), "'when hovered' denotes a trigger preset");
    terminal->inject("when hovered then set color gold");
    frame();
    Core::EventBus::instance().publish(ECA::Event{"object-hover-entered", cube, nullptr, std::time(nullptr)});
    harness.lawManager.tick();
    lawGetValue(*cube, PropertyPath::parse("color"), color);
    c = std::get_if<glm::vec3>(&color);
    check(c && std::fabs(c->x - 1.0f) < 0.03f && std::fabs(c->y - 0.84f) < 0.03f && c->z < 0.03f,
          "'when hovered then set color gold' paints the cube gold on hover");

    // Zach's first unguided session (2026-09-25): after "when they collide"
    // the menu offered "always" (a contradicting preset) and actions with no
    // sentence form. It must offer only words that can work there.
    {
        const auto live = terminal->vocabulary(harness.lawManager);
        const auto menu = Singularity::Terminal::LawSentence::suggest("my law called Blue when they collide ", live);
        const auto offered = [&](const std::string& text) {
            return std::any_of(menu.begin(), menu.end(), [&](const auto& s) { return s.text == text; });
        };
        check(!offered("always") && !offered("my constantly-applied law"),
              "the menu never offers a preset that contradicts 'when they collide'");
        check(!offered("WritePixel") && !offered("AddElement") && !offered("AuthorZone"),
              "the menu never offers an action with no sentence form");
        check(offered("then"), "the menu offers 'then' after a trigger preset");
    }
    terminal->inject("my law called Blue when they collide then set color blue");
    frame();
    check(mentions(printed.back(), "authored"), "Zach's intended sentence authors: " + printed.back().substr(0, 60));

    // Zach's line (2026-09-25) made a Law waiting for an event called "when".
    {
        const std::size_t before = harness.lawManager.getAll().size();
        terminal->inject("my law called Blue fires when Spawn @material.concept-shape-3d.birth-74.member-0");
        frame();
        check(harness.lawManager.getAll().size() == before && mentions(printed.back(), "clause word"),
              "'fires when <action>' is refused: 'when' is not an event");
        terminal->inject("my law called Blue fires when clicked then set color blue");
        frame();
        Law* blue = harness.lawManager.getAll().back().get();
        check(mentions(printed.back(), "authored") &&
                  harness.lawManager.triggersOf(blue->getIdentifier()) == std::vector<std::string>{"object-clicked"},
              "'fires when clicked' reads the trigger preset and listens for object-clicked");
    }

    check(harness.zones.persistActiveZone(), "Save Zone keeps the spoken Law");
    const auto persisted = SaveSystem::readZoneIdentity("LawLine");
    const auto refs = persisted.value("lawRefs", nlohmann::json::array());
    check(std::find(refs.begin(), refs.end(), newbornId) != refs.end(),
          "the spoken Law is authored Zone membership");
    // Save files are sacred: saving must not shed the vocabulary it was built from.
    std::size_t kept = 0;
    std::vector<std::string> extra;
    for (const auto& seeded : zoneJson["lexemes"]) {
        for (const auto& saved : persisted.value("lexemes", nlohmann::json::array())) {
            if (saved.value("id", std::string{}) == seeded.value("id", std::string{})) {
                ++kept;
                break;
            }
        }
    }
    for (const auto& saved : persisted.value("lexemes", nlohmann::json::array())) {
        bool seededHere = false;
        for (const auto& seeded : zoneJson["lexemes"]) {
            if (saved.value("id", std::string{}) == seeded.value("id", std::string{})) seededHere = true;
        }
        if (!seededHere) extra.push_back(saved.value("symbol", std::string{}));
    }
    for (const auto& e : extra) std::cout << "    (the Zone also names Lexeme '" << e << "')\n";
    check(kept == zoneJson["lexemes"].size(), "Save Zone keeps every seeded Lexeme");
    std::size_t denotes = 0;
    for (const auto& r : persisted.value("formationRelations", nlohmann::json::array())) {
        if (r.value("type", std::string{}) == "denotes") ++denotes;
    }
    check(denotes == zoneJson["formationRelations"].size(), "Save Zone keeps every Lexeme --denotes--> Law Relation");

    std::cout << "law_line_zone_test: " << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
