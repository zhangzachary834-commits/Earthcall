#include "Singularity/Storage/SaveSystem.hpp"
// End-to-end witness for the seeded Law Line (scripts/seed_law_line.py).
//
// Boots the LawLine Zone identity and its shared Law roots in an isolated save
// root, enters the Zone, and speaks at the Terminal channel exactly as the Mac
// Terminal would — through the injection seam instead of a TTY. The seeded
// Lexemes --denotes--> Laws supply the words, the seeded wiring Laws decide
// the line is spoken, and the Law it authors must govern the seeded cube.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
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

    nlohmann::json zoneJson = SaveSystem::readSaveData(sourceZone.string());

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
    const auto grantWord = std::find_if(vocab.words.begin(), vocab.words.end(), [](const auto& w) {
        return w.symbol == "grant" && !w.lexemeId.empty();
    });
    check(grantWord != vocab.words.end() && vocab.describeBeing &&
              vocab.describeBeing(grantWord->lexemeId) == "lexeme · grant",
          "Law Line describes a Lexeme with its authored symbol");
    check(contains(Singularity::Terminal::LawSentence::complete("my ev", vocab), "event-triggered law"),
          "Tab completes the preset phrase");
    check(contains(Singularity::Terminal::LawSentence::complete("on object-clicked then set co", vocab), "color"),
          "Tab completes a property of the scoped cube");
    check(contains(Singularity::Terminal::LawSentence::complete(
                       "on object-clicked if @event.ver", vocab), "@event.verb"),
          "Tab completes the registered Event Moment verb");

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

    // The general Create notation crosses the real Terminal -> authored
    // compiler Metalaws -> newborn action -> Zone persistence path.
    {
        auto* objectCompiler = harness.lawManager.find("law-line-compile-object");
        check(objectCompiler != nullptr, "authored Object compiler is loaded");
        auto& active = harness.zones.active();
        const std::string sentence = "called Beneath Me when clicked if Identity @law-line-cube then "
            "Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), "
            "color: gold, authored: {purpose: \"A foothold\", visits: 0}} >";
        harness.player.position() = glm::vec3(7, 9, -4);
        const auto lawCount = harness.lawManager.getAll().size();
        const auto objectsBefore = active.getOwnedObjects().size();
        const auto compilerLogBefore = objectCompiler ? objectCompiler->applicationLog().size() : 0;
        terminal->inject(sentence + "?"); frame();
        check(harness.lawManager.getAll().size() == lawCount &&
              (!objectCompiler || objectCompiler->applicationLog().size() == compilerLogBefore),
              "preview creates no Law and executes no compiler Metalaw");
        terminal->inject(sentence); frame();
        auto status = printed.back();
        std::cout << "    create> " << status << '\n';
        PropertyValue created;
        lawGetValue(*terminal, PropertyPath::parse("lastCreated"), created);
        auto* creationLaw = std::holds_alternative<std::string>(created)
            ? harness.lawManager.find(std::get<std::string>(created)) : nullptr;
        check(creationLaw && creationLaw->name() == "Beneath Me" && creationLaw->actionModel() &&
              creationLaw->actionModel()->kind == ActionNode::Kind::Create,
              "Create invocation compiles into a named Law through seeded Metalaws");
        if (creationLaw && creationLaw->name() == "Beneath Me") {
            const auto model = creationLaw->actionModel()->toJson();
            check(model["children"][1]["kind"] == static_cast<int>(ActionNode::Kind::Map) &&
                  mentions(model.dump(), "@Zach.position"),
                  "relative placement remains a Map bound to the actual speaking author");
            Core::EventBus::instance().publish(ECA::Event{"object-clicked", cube, nullptr, std::time(nullptr)});
            harness.lawManager.tick();
            check(active.getOwnedObjects().size() == objectsBefore + 1, "a real click creates exactly one Object");
            if (active.getOwnedObjects().size() > objectsBefore) {
                auto* born = active.getOwnedObjects().back().get();
                check(glm::length(born->getPosition() - glm::vec3(7, 6, -4)) < 0.0001f,
                      "newborn centre is exactly three Y units below the Person");
                PropertyValue purpose, visits, paint;
                born->getDynamicProperty("purpose", purpose); born->getDynamicProperty("visits", visits);
                lawGetValue(*born, PropertyPath::parse("color"), paint);
                check(purpose == PropertyValue(std::string("A foothold")) && visits == PropertyValue(0.0),
                      "authored initializers become newborn properties");
                auto rgb = std::get_if<glm::vec3>(&paint);
                check(rgb && glm::length(*rgb - glm::vec3(1, .84, 0)) < 0.0001f,
                      "registered color initializer paints the newborn gold");
                harness.player.position() = glm::vec3(-2, 20, 6);
                Core::EventBus::instance().publish(ECA::Event{"object-clicked", cube, nullptr, std::time(nullptr)});
                harness.lawManager.tick();
                check(glm::length(active.getOwnedObjects().back()->getPosition() - glm::vec3(-2, 17, 6)) < .0001f,
                      "another firing reads the Person's new position instead of freezing a preview value");
            }
            check(harness.zones.persistActiveZone(), "Save Zone persists the compiled creation Law and newborns");
            auto restored = Law::fromJson(creationLaw->toJson());
            check(restored->actionModel()->toJson() == model, "compiled creation and math models round-trip exactly");
        }
        if (objectCompiler) {
            const auto original = *objectCompiler->actionModel();
            objectCompiler->setEnabled(false);
            const auto count = harness.lawManager.getAll().size();
            terminal->inject("called Missing Compiler when clicked then Create <Object, properties: {}>"); frame();
            check(harness.lawManager.getAll().size() == count && mentions(printed.back(), "no authored Metalaw"),
                  "removing the compiler refuses authoring with no hidden Create fallback");
            objectCompiler->setEnabled(true);
            objectCompiler->setActionModel(ActionNode::set("compilation.template",
                PropertyValue(std::string("{\"kind\":10,\"eventType\":\"compiler-changed\"}"))));
            terminal->inject("called Compiler Changed when clicked then Create <Object, properties: {}>"); frame();
            lawGetValue(*terminal, PropertyPath::parse("lastCreated"), created);
            auto* changed = std::holds_alternative<std::string>(created)
                ? harness.lawManager.find(std::get<std::string>(created)) : nullptr;
            check(changed && changed->name() == "Compiler Changed" &&
                  changed->actionModel()->kind == ActionNode::Kind::Publish,
                  "editing the Metalaw changes the compiled model without editing C++");
            objectCompiler->setActionModel(ActionNode::set("compilation.template",
                PropertyValue(std::string("{\"kind\":999}"))));
            const auto malformedCount = harness.lawManager.getAll().size();
            terminal->inject("called Bad Compiler when clicked then Create <Object, properties: {}>"); frame();
            check(harness.lawManager.getAll().size() == malformedCount && mentions(printed.back(), "unsupported"),
                  "malformed compiler output refuses instead of becoming a default action");
            objectCompiler->setActionModel(original);
        }
        auto conflict = std::make_shared<Law>("competing compiler", std::vector<Singular*>{&harness.player});
        conflict->setLawIdentifier("test-conflicting-invocation-compiler");
        conflict->addTarget(*terminal);
        conflict->setConditionModel(ConditionNode::compare("compilation.input.slot", ConditionNode::Op::Eq,
                                                         PropertyValue(std::string("invocation"))));
        conflict->setActionModel(ActionNode::set("compilation.template",
            PropertyValue(std::string("{\"kind\":10,\"eventType\":\"conflict\"}"))));
        harness.lawManager.add(conflict);
        const auto conflictingCount = harness.lawManager.getAll().size();
        terminal->inject("called Conflict when clicked then Create <Object, properties: {}>"); frame();
        check(harness.lawManager.getAll().size() == conflictingCount && mentions(printed.back(), "conflicting"),
              "conflicting compiler outputs refuse without choosing register order");
        harness.lawManager.remove(conflict->getIdentifier());
        auto foreign = terminal->authorForeign(harness.lawManager,
            "when clicked then Create <Object, properties: {position: my.position}>", {}, "unowned-create-test");
        check(foreign.status == "refused", "a foreign sentence with no unique author cannot borrow the local Person's my root");
        auto* rootWord = harness.lawManager.find("law-line-root-my");
        PropertyValue rootMarker;
        auto restoredRoot = rootWord ? Law::fromJson(rootWord->toJson()) : nullptr;
        check(restoredRoot && restoredRoot->getDynamicProperty("sentence.root", rootMarker) && rootMarker == PropertyValue(true),
              "authored root vocabulary survives the Law identity codec");
        const auto menu = Singularity::Terminal::LawSentence::suggest("when clicked then Cre", terminal->vocabulary(harness.lawManager));
        check(std::any_of(menu.begin(), menu.end(), [](const auto& suggestion) {
            return suggestion.text == "Create" && mentions(suggestion.snippet, "properties:");
        }), "Create completion offers its authored initializer signature");
        terminal->inject("called New Lexeme when hovered if Identity @law-line-cube then "
            "Create <@lexeme.law-line.gold.value-gold, properties: {authored: {testOrigin: \"terminal\"}}>"); frame();
        const auto lexicalBefore = active.storedSingulars().size();
        Core::EventBus::instance().publish(ECA::Event{"object-hover-entered", cube, nullptr, std::time(nullptr)});
        harness.lawManager.tick();
        check(active.storedSingulars().size() == lexicalBefore + 1,
              "explicit Singular prototype uses the universal creation operation");
        if (active.storedSingulars().size() > lexicalBefore) {
            auto* lexical = active.storedSingulars().back().get();
            PropertyValue origin;
            lexical->getDynamicProperty("testOrigin", origin);
            check(dynamic_cast<Singularity::Language::Lexeme*>(lexical) && origin == PropertyValue(std::string("terminal")),
                  "prototype creation preserves Lexeme kind and applies authored newborn properties");
        }
        std::string many = "called Sixteen Fields when clicked then Create <Object, properties: {authored: {";
        for (int i = 0; i < 16; ++i) many += (i ? ", " : "") + std::string("field") + std::to_string(i) + ": 1";
        many += "}} >";
        terminal->inject(many); frame();
        check(mentions(printed.back(), "authored") && mentions(printed.back(), "Sixteen Fields"),
              "sixteen initializer models remain structural lists rather than being decoded as a matrix");
        const auto unknownCount = harness.lawManager.getAll().size();
        terminal->inject("when clicked then Create <ImaginaryKind, properties: {}>"); frame();
        check(harness.lawManager.getAll().size() == unknownCount && mentions(printed.back(), "no authored Metalaw"),
              "unknown kind has no invented birth semantics");
        terminal->inject("when clicked then Create <Object, properties: {position: (1, 2, 3), position: (4, 5, 6)}>"); frame();
        check(harness.lawManager.getAll().size() == unknownCount && mentions(printed.back(), "duplicate initializer"),
              "duplicate initializer refuses rather than silently overwriting authorial intent");
    }

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

    // ------------------------------------------------------------------
    // Confirmed deletion (Zach, 2026-09-25: "the metalaw that does the
    // deletions should say 'are you sure you want to delete?' … no means no
    // delete and requires your yes to delete").
    // ------------------------------------------------------------------
    const auto textOf = [&](const char* path) {
        PropertyValue v;
        lawGetValue(*terminal, PropertyPath::parse(path), v);
        return std::holds_alternative<std::string>(v) ? std::get<std::string>(v) : std::string{};
    };
    const auto lawNamed = [&](const std::string& name) -> std::vector<std::string> {
        std::vector<std::string> ids;
        for (const auto& law : harness.lawManager.getAll()) {
            if (law && law->name() == name) ids.push_back(law->getIdentifier());
        }
        return ids;
    };
    // Zach's one-line sentences are compiled by a saved Metalaw, then registered
    // in lexical order; registration ordering does not promise Rete firing order.
    const std::string batchLine = "called \"Batch Grant\" when clicked then add property @law-line-cube.batchNote to \"first;value\"; "
                                  "called \"Batch Modify\" when clicked then modify property @law-line-cube.batchNote to \"second\"; "
                                  "called \"Batch Remove\" when clicked then remove property @law-line-cube.batchNote";
    auto beforeBatch = harness.lawManager.getAll().size();
    terminal->inject(batchLine + "?"); frame();
    check(harness.lawManager.getAll().size() == beforeBatch && mentions(printed.back(), "preview:"),
          "a batch preview registers no Laws");
    terminal->inject(batchLine); frame();
    auto grantIds = lawNamed("Batch Grant"), modifyIds = lawNamed("Batch Modify"), removeIds = lawNamed("Batch Remove");
    check(grantIds.size() == 1 && modifyIds.size() == 1 && removeIds.size() == 1,
          "one line registers three separate Laws through the sentence Metalaw: " + printed.back());
    if (!grantIds.empty() && !modifyIds.empty() && !removeIds.empty()) {
        std::vector<std::string> order;
        for (const auto& law : harness.lawManager.getAll())
            if (law && law->name().rfind("Batch ", 0) == 0) order.push_back(law->name());
        check(order == std::vector<std::string>{"Batch Grant", "Batch Modify", "Batch Remove"},
              "LawManager registration follows source sentence order");
        auto grant = harness.lawManager.find(grantIds[0]);
        auto modify = harness.lawManager.find(modifyIds[0]);
        auto remove = harness.lawManager.find(removeIds[0]);
        PropertyValue note;
        grant->applyTo(*cube);
        check(cube->getDynamicProperty("batchNote", note) && std::get<std::string>(note) == "first;value",
              "add property grants a real authored property and preserves a quoted semicolon");
        modify->applyTo(*cube);
        check(cube->getDynamicProperty("batchNote", note) && std::get<std::string>(note) == "second",
              "modify property writes the existing property");
        remove->applyTo(*cube);
        check(!cube->getDynamicProperty("batchNote", note), "remove property erases the authored property");
        modify->applyTo(*cube);
        check(cube->getDynamicProperty("batchNote", note) && std::get<std::string>(note) == "second",
              "modify retains existing Set semantics for a materialized authored accessor");
        remove->applyTo(*cube);
    }
    beforeBatch = harness.lawManager.getAll().size();
    terminal->inject("called Invalid Batch First when clicked then set glow 1; called Invalid Batch Second when clicked then"); frame();
    check(harness.lawManager.getAll().size() == beforeBatch, "invalid later sentence registers no earlier sentence");
    auto batchCompiler = harness.lawManager.find("law-line-compile-sentences");
    check(batchCompiler != nullptr, "saved sentence compiler is present");
    if (batchCompiler) {
        batchCompiler->setEnabled(false);
        terminal->inject("called No Compiler A when clicked then set glow 1; called No Compiler B when clicked then set glow 2"); frame();
        check(harness.lawManager.getAll().size() == beforeBatch && mentions(printed.back(), "no authored Metalaw"),
              "without the authored sentence Metalaw no batch fallback registers Laws");
        batchCompiler->setEnabled(true);
        auto original = *batchCompiler->actionModel();
        batchCompiler->setActionModel(ActionNode::set("compilation.template", std::string("{\"sentences\":[]}")));
        terminal->inject("called Wrong Order One when clicked then set glow 1; called Wrong Order Two when clicked then set glow 2"); frame();
        check(harness.lawManager.getAll().size() == beforeBatch && mentions(printed.back(), "source order"),
              "a compiler cannot drop or reorder the sensed sentences");
        batchCompiler->setActionModel(original);
    }
    auto foreignBatch = terminal->authorForeign(harness.lawManager, batchLine, {&harness.player}, "foreign-batch");
    check(foreignBatch.status == "refused" && harness.lawManager.getAll().size() == beforeBatch,
          "foreign batch cannot escape its single-identifier authorization");
    check(!Singularity::Terminal::LawSentence::suggest("called Earlier when clicked then set glow 1; then modify prop", terminal->vocabulary(harness.lawManager)).empty(),
          "completion follows the final sentence in a batch");

    // Earlier in this test two Laws were spoken with the name Blue.
    const auto blues = lawNamed("Blue");
    check(blues.size() == 2, "two Laws named Blue exist");
    const std::string blueId = blues.front();
    check(harness.zones.persistActiveZone(), "Save Zone before deleting (so Blue has its own file)");
    check(std::filesystem::exists(scratch.path / "laws" / blueId / "law.json"), "Blue was saved as its own file");
    terminal->inject("delete Blue");
    frame();
    frame();
    check(textOf("question") == "Are you sure you want to delete" && mentions(textOf("pending.names"), "Blue"),
          "'delete Blue' makes the Metalaw ask: " + textOf("question"));
    terminal->inject("no");
    frame();
    check(harness.lawManager.find(blues[0]) != nullptr && harness.lawManager.find(blues[1]) != nullptr &&
              mentions(printed.back(), "kept"),
          "'no' keeps both Blues — nothing is deleted");

    terminal->inject("delete Blue");
    frame();
    frame();
    terminal->inject("1");
    frame();
    terminal->inject("yes");
    frame();
    check(harness.lawManager.find(blueId) == nullptr && mentions(printed.back(), "deleted"),
          "'yes' deletes Blue: " + printed.back().substr(0, 50));

    // Two Laws share a name: the question asks which, and only that one goes.
    terminal->inject("my law called Twin when hovered then set glow 1");
    frame();
    terminal->inject("my law called Twin when clicked then set glow 2");
    frame();
    const auto twins = lawNamed("Twin");
    check(twins.size() == 2, "two Laws named Twin exist");
    terminal->inject("delete Twin");
    frame();
    frame();
    check(textOf("pending.names").find('|') != std::string::npos, "the question names both Twins");
    terminal->inject("2");
    frame();
    terminal->inject("yes");
    frame();
    check(twins.size() == 2 && harness.lawManager.find(twins[0]) != nullptr &&
              harness.lawManager.find(twins[1]) == nullptr,
          "answering 2, then yes, deletes only the second Twin");

    check(harness.zones.persistActiveZone(), "Save Zone keeps the spoken Law");
    {
        const auto afterDelete = SaveSystem::readZoneIdentity("LawLine").value("lawRefs", nlohmann::json::array());
        check(std::find(afterDelete.begin(), afterDelete.end(), blueId) == afterDelete.end(),
              "Save Zone no longer names the deleted Law");
        check(std::filesystem::exists(scratch.path / "laws" / blueId / "law.json"),
              "the deleted Law's own file stays on disk as history");
    }

    // Without the deciding Metalaws, nothing can be deleted from the line.
    harness.lawManager.remove("law-line-ask-before-deleting");
    const std::size_t lawsBefore = harness.lawManager.getAll().size();
    terminal->inject("delete Red");
    frame();
    check(harness.lawManager.getAll().size() == lawsBefore && mentions(printed.back(), "no Law in this Zone decides"),
          "with no asking Metalaw, 'delete' deletes nothing and says why");
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
