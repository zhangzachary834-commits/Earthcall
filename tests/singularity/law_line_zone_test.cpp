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
#include "Singularity/Screen/ScreenChannel.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"

#include <algorithm>
#include <array>
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
    std::filesystem::path saves = std::getenv("EARTHCALL_LAW_LINE_TEST_SAVES")
        ? std::getenv("EARTHCALL_LAW_LINE_TEST_SAVES") : "saves";
    if (!std::filesystem::exists(saves / "zones/LawLine")) saves = std::filesystem::path("..") / "saves";
    saves = std::filesystem::absolute(saves);
    // An in-app Save Zone writes the native zone.ecform (Zach's LawLine save,
    // e4d7a72d); a fresh seed writes zone.json. Read whichever the Person's
    // save holds, the way SaveSystem's zone identity path does.
    auto sourceZone = saves / "zones/LawLine/zone.ecform";
    if (!std::filesystem::exists(sourceZone)) sourceZone = saves / "zones/LawLine/zone.json";
    check(std::filesystem::exists(sourceZone), "LawLine Zone identity exists");
    if (!std::filesystem::exists(sourceZone)) return 1;

    nlohmann::json zoneJson = SaveSystem::readSaveData(sourceZone.string());

    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall_law_line_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directories(scratch.path / "zones/LawLine");
    std::filesystem::copy_file(sourceZone, scratch.path / "zones/LawLine" / sourceZone.filename());
    // Production exposes this inactive Zone alongside LawLine. Its literal
    // name must never steal birth from the active/rendered Zone.
    std::filesystem::create_directories(scratch.path / "zones/World");
    std::filesystem::copy_file(saves / "zones/World/zone.json", scratch.path / "zones/World/zone.json");
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
    // Zach keyed (2026-09-30), and his in-app Save Zone (e4d7a72d) rewrote the
    // shared seed Laws' authors from "Zach" to his DID. The Zone refuses to
    // activate Laws whose author no present Person answers to, so this scratch
    // Person carries whichever identity the seed records -- never a rewrite of
    // the copied Laws' authors, which would test a world that is not his.
    {
        const nlohmann::json hear = SaveSystem::readSaveData(
            (scratch.path / "laws/law-line-hear/law.json").string());
        const auto authors = hear.value("authors", nlohmann::json::array());
        const std::string author = authors.empty() ? "" : authors[0].get<std::string>();
        if (author.rfind("did:earthcall:", 0) == 0)
            check(harness.player.setPersonId(Identity::SingularId::parse(author)),
                  "the scratch Person answers to the seed Laws' keyed author");
    }

    std::size_t index = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        if (harness.zones.zones()[i] && harness.zones.zones()[i]->getIdentifier() == "LawLine") index = i;
    }
    check(index < harness.zones.zones().size(), "boot discovers the LawLine Zone");
    if (index >= harness.zones.zones().size()) return 1;
    check(harness.zones.switchTo(index), "entering LawLine loads its Laws (disabled presets included)");
    // The LawLine save is Zach's inhabited Zone: it also holds Laws he spoke
    // (minted `law_<uuid>`, e.g. his Stairmaker, which adds three steps on any
    // click). They are his program, not this fixture; quiet them in the
    // scratch copy so every count below measures only what this test speaks.
    for (const auto& law : harness.lawManager.getAll())
        if (law && law->getIdentifier().rfind("law_", 0) == 0) law->setEnabled(false);
    auto* inactiveWorld = harness.zones.zones()[harness.zones.findZoneIndex("World")].get();
    const auto worldObjectsBefore = inactiveWorld->getOwnedObjects().size();
    check(&harness.zones.active() != inactiveWorld, "the visible LawLine Zone is distinct from inactive World");
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

    // Exercise the actual decoder -> sensed edge -> persisted Metalaw -> menu
    // path. The app data home stays outside this isolated fixture.
    if (auto* binding = harness.lawManager.find("law-line-shortcut-ctrl-t")) {
        const auto read = [&](const char* path) {
            PropertyValue value;
            lawGetValue(*terminal, PropertyPath::parse(path), value);
            return value;
        };
        terminal->injectKeys("when \x14"); frame();
        check(terminal->editorForTests().buffer() == "when " &&
              terminal->editorForTests().menuVisible() && terminal->editorForTests().suggestions().size() == 2,
              "Ctrl+T Metalaw shows only stored click/hover templates matching the draft");
        check(std::get<std::string>(read("draft")) == "when " &&
              std::get<std::string>(read("key.chord")) == "ctrl+t" &&
              std::get<double>(read("cursor")) == 5,
              "the draft, cursor and chord are registered readable channel properties");
        terminal->injectKeys("\t"); frame();
        check(terminal->editorForTests().buffer() == "when clicked if ‹condition› then ‹action›" &&
              terminal->editorForTests().onPlaceholder() && !std::get<bool>(read("completion.active")),
              "selection inserts the stored template, selects its first blank and restores ordinary completion");
        terminal->injectKeys("\x03"); frame();
        terminal->injectKeys("unmatched \x1b[116;9u"); frame();
        check(terminal->editorForTests().suggestions().empty() &&
              mentions(terminal->editorForTests().render(120).text, "No stored completions match"),
              "Cmd+T with an unmatched prefix offers no unrelated vocabulary and reports the empty match");
        terminal->injectKeys("\x03"); frame();
        terminal->injectKeys("\x14"); frame();
        check(terminal->editorForTests().suggestions().size() == 4, "an empty draft offers the complete stored shelf");
        terminal->injectKeys("\x1b"); frame();
        // Injection uses the same ambiguous ESC decoder: resolve with an
        // explicit subsequent control rather than a wall-clock sleep.
        terminal->injectKeys("\x03"); frame();
        binding->setDynamicProperty("binding.chord", std::string("ctrl+g"));
        terminal->injectKeys("always \x14"); frame();
        check(!std::get<bool>(read("completion.active")), "changing the authored chord makes the old shortcut inactive");
        terminal->injectKeys("\x07"); frame();
        check(terminal->editorForTests().suggestions().size() == 1 &&
              mentions(terminal->editorForTests().suggestions()[0].text, "always"),
              "the new authored chord opens the same prefix-filtered shelf without C++ changes");
        binding->setDynamicProperty("binding.chord", std::string("ctrl+t"));
        terminal->injectKeys("\x03"); frame();
        // Two shortcuts in one transport read retain both drafts and order.
        const double requests = std::get<double>(read("completion.openRequests"));
        terminal->injectKeys("when \x14" "clicked \x14"); frame();
        check(terminal->editorForTests().buffer() == "when " &&
              std::get<double>(read("completion.openRequests")) == requests + 1,
              "the first queued chord is decided before subsequent draft edits");
        frame();
        check(terminal->editorForTests().buffer() == "when clicked " &&
              terminal->editorForTests().suggestions().size() == 1 &&
              std::get<double>(read("completion.openRequests")) == requests + 2,
              "the second queued chord sees the later draft exactly once");
        terminal->injectKeys("\x03"); frame();
        terminal->setPresenceCheckForTests([&](const Person& p) { return &p == &harness.player; });
        const double requestsBeforeCompilation = std::get<double>(read("completion.openRequests"));
        terminal->inject(R"(called "Completion Isolation" on "compile-isolation" then Sequence <children: [Set glow to 1]>)"); frame();
        check(std::get<double>(read("completion.openRequests")) == requestsBeforeCompilation &&
              !std::get<bool>(read("completion.active")),
              "compilation cannot reapply a shortcut merely because the last sensed chord still matches");
        terminal->inject(R"(called "My Alt J Templates" on "terminal-key-pressed" if @terminal-channel.zone is "LawLine" and @terminal-channel.key.chord is "alt+j" then Sequence <children: [Set @terminal-channel.completion.items to @law-line-template-shelf.items, Add @terminal-channel.completion.openRequests by 1]>)"); frame();
        check(mentions(printed.back(), "authored"), "a Person can author a new template shortcut as one Law sentence");
        terminal->injectKeys("when \x1b[106;3u"); frame();
        check(terminal->editorForTests().suggestions().size() == 2 &&
              std::get<std::string>(read("key.chord")) == "alt+j",
              "the sentence-authored Alt+J binding supplies the stored shelf to the generic channel");
        terminal->injectKeys("\x03"); frame();
        PropertyValue originalShelf;
        auto* shelf = harness.lawManager.find("law-line-template-shelf");
        if (shelf) shelf->getDynamicProperty("items", originalShelf);
        const std::string storageProgram = R"(called "Store My Templates" on "terminal-key-pressed" if Identity @terminal-channel and key.chord is "ctrl+g" then Set @law-line-template-shelf.items to Literal <value: [{text: "when clicked then Set color to gold", description: "My golden touch"}, {text: "always if hp is greater than 0 then Set color to blue", description: "My repeating Law"}]>)";
        terminal->injectKeys("\x1b[200~" + storageProgram + "\x1b[201~\r"); frame();
        terminal->injectKeys("\x07"); frame();
        PropertyValue storedShelf;
        check(shelf && shelf->getDynamicProperty("items", storedShelf) &&
              std::holds_alternative<std::shared_ptr<PropertyList>>(storedShelf) &&
              std::get<std::shared_ptr<PropertyList>>(storedShelf)->elements.size() == 2,
              "one Law sentence stores new template records through Literal Metalaws: " + printed.back());
        terminal->injectKeys("when \x14"); frame();
        check(terminal->editorForTests().suggestions().size() == 1 &&
              terminal->editorForTests().suggestions()[0].text == "when clicked then Set color to gold",
              "the same shortcut immediately reads the newly authored matching template");
        terminal->injectKeys("\x03"); frame();
        if (shelf) {
            auto restoredShelf = Law::fromJson(shelf->toJson());
            PropertyValue roundTrip;
            check(restoredShelf && restoredShelf->getDynamicProperty("items", roundTrip) &&
                  propertyValueToJson(roundTrip) == propertyValueToJson(storedShelf),
                  "newly authored template records survive the shelf Law codec");
            shelf->setDynamicProperty("items", originalShelf);
        }
        terminal->setPresenceCheckForTests({});
        auto* templateLaw = harness.lawManager.find("law-line-template-click");
        check(templateLaw && templateLaw->toJson().contains("authoredProperties") &&
              templateLaw->toJson()["authoredProperties"].contains("completion"),
              "the loaded template round-trips through the real Law codec");
        if (templateLaw) {
            auto restored = Law::fromJson(templateLaw->toJson());
            PropertyValue completion;
            check(restored && restored->getDynamicProperty("completion", completion) &&
                  propertyValueToJson(completion) == templateLaw->toJson()["authoredProperties"]["completion"],
                  "the complete editable candidate dictionary survives Law encode/decode");
        }
        const auto keysBeforeSecret = std::get<double>(read("key.presses"));
        terminal->setPresencePerson(&harness.player);
        PropertyPath::parse("unlockRequests").setValue(*terminal, 1.0); terminal->act(harness.lawManager);
        check(terminal->awaitingSecret(), "the real Identity operation enters secret input");
        terminal->injectKeys("hidden-test-input\x14"); frame();
        PropertyPath::parse("completion.openRequests").setValue(*terminal,
            std::get<double>(read("completion.openRequests")) + 1); terminal->act(harness.lawManager);
        check(std::get<std::string>(read("draft")).empty() && std::get<double>(read("cursor")) == 0 &&
              std::get<double>(read("key.presses")) == keysBeforeSecret &&
              !std::get<bool>(read("completion.visible")),
              "secret input exposes neither draft/cursor nor shortcut edges and refuses completion display");
        terminal->injectKeys("\x03\x03"); frame(); // discard input, then cancel; never submit it
        check(!terminal->awaitingSecret(), "secret test cancels without keying or unlocking the Person");

    }

    if (std::getenv("EARTHCALL_TEMPLATE_TEST_ONLY")) {
        check(harness.lawManager.find("law-line-shortcut-ctrl-t") != nullptr, "the shortcut seed was actually loaded");
        std::cout << "template witness: " << checks - failures << "/" << checks << " checks passed\n";
        return failures ? 1 : 0;
    }

    // Stdin trust (Zach, 2026-10-05: "Require presence"). This harness holds
    // no private key for the seed's keyed author, so it cannot be seated in
    // the First Mover Register; law_line_test witnesses that real path with a
    // freshly minted key. Here: the refusal first, then the C++-only seam.
    {
        const auto absentBefore = harness.lawManager.getAll().size();
        terminal->inject("called \"Absent\" on \"argument-test\" then set glow 1"); frame();
        check(harness.lawManager.getAll().size() == absentBefore && mentions(printed.back(), "not present"),
              "a typed line authors nothing until its Person is present: " + printed.back());
        terminal->inject("called \"Absent\" on \"argument-test\" then set glow 1?"); frame();
        check(harness.lawManager.getAll().size() == absentBefore && mentions(printed.back(), "preview"),
              "a preview stays open while the Person is not present");
        terminal->setPresenceCheckForTests([&](const Person& p) { return &p == &harness.player; });
    }
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
                  mentions(model.dump(), "@" + harness.player.getIdentifier() + ".position"),
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
        const auto pixels = Singularity::Terminal::LawSentence::suggest("my law called Blue when they collide Wri", live);
        check(std::any_of(pixels.begin(), pixels.end(), [](const auto& item) { return item.text == "WritePixel"; }) &&
              offered("AddElement") && offered("AuthorZone"),
              "bounded completion offers new argument forms when narrowed by their spelling");
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
        remove->applyTo(*cube);
        check(!cube->getDynamicProperty("batchNote", note),
              "repeated removal does not recreate an absent authored slot through its materialized accessor");
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

    // Every formerly missing kind traverses the actual saved compiler and
    // injected Terminal hearing path, while its exact model round-trips.
    const std::vector<std::pair<int, std::string>> argumentExamples = {
        {3, R"(Lerp <path: "@law-line-cube.glow", operand: 10, factor: 0.25>)"},
        {4, R"(Drive <path: "@law-line-cube.glow", curve: {form: 1, coeffs: [1, 2]}, input: "@law-line-cube.hp">)"},
        {5, R"(Sequence <children: [Set glow to 2, Add glow by 3]>)"},
        {6, R"(Parallel <children: [Set glow to 2, Add glow by 3]>)"},
        {8, R"(Map <path: "@law-line-cube.glow", expression: @law-line-cube.hp + 2>)"},
        {9, R"(Flow <path: "@law-line-cube.glow", expression: 2>)"},
        {13, R"(AddElement <container: "law-line-cube", element: "lexeme.law-line.gold.value-gold">)"},
        {15, R"(RemoveElement <container: "law-line-cube", element: "lexeme.law-line.gold.value-gold">)"},
        {17, R"(Synthesize <children: [Create <Object, properties: {color: blue}>]>)"},
        {18, R"(PlayAudio <frequencyPath: "tone.frequency", amplitudePath: "tone.amplitude", timbre: "sine">)"},
        {19, R"(AuthorZone <identifier: "args-test-zone", zoneKind: "empty", owner: "Zach", ownerKind: "Person">)"},
        {21, R"(WritePixel <facePath: "paint.face", uPath: "paint.u", vPath: "paint.v", colorPath: "paint.color">)"},
        {22, R"(ElevatePixels <name: "region", facePath: "paint.face", selector: {pieces: []}>)"},
        {23, R"(FileRead <input: "file.path", path: "file.content">)"},
        {24, R"(FileWrite <path: "file.path", input: "file.content">)"},
        {25, R"(CodecTransform <operation: "jsonCompact", input: "codec.source", path: "codec.result">)"},
    };
    for (const auto& example : argumentExamples) {
        const auto name = "Args " + std::to_string(example.first);
        auto before = harness.lawManager.getAll().size();
        terminal->inject("called \"" + name + "\" on \"argument-test\" then " + example.second); frame();
        const auto ids = lawNamed(name);
        auto* authored = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        const auto* model = authored ? authored->actionModel() : nullptr;
        check(harness.lawManager.getAll().size() == before + 1 && model && static_cast<int>(model->kind) == example.first,
              "saved compiler authors " + name + ": " + printed.back());
        if (!model) continue;
        check(ActionNode::fromJson(model->toJson()).toJson() == model->toJson(), name + " arguments round-trip exactly");
        if (example.first == 3 || example.first == 4 || example.first == 5 || example.first == 8) {
            cube->setDynamicProperty("glow", 2.0);
            authored->applyTo(*cube);
            PropertyValue actual;
            double value = 0;
            lawGetValue(*cube, PropertyPath::parse("glow"), actual); propertyValueToNumber(actual, value);
            PropertyValue hp;
            double hpNumber = 0;
            lawGetValue(*cube, PropertyPath::parse("hp"), hp); propertyValueToNumber(hp, hpNumber);
            const double expected = example.first == 3 ? 4.0 : example.first == 4 ? 1.0 + 2.0 * hpNumber : example.first == 5 ? 5.0 : hpNumber + 2.0;
            check(std::abs(value - expected) < 0.001, name + " changes the live cube by its authored arguments");
        }
    }
    // `set x to @other.path`: the binding movement's "copy value". The parser
    // senses the read; the saved law-line-compile-assignment-expression
    // Metalaw lowers it to a Map passthrough (Claude Opus 5.5, 2026-10-05).
    {
        const auto readNumber = [&](const char* path) {
            PropertyValue v; double n = 0;
            lawGetValue(*cube, PropertyPath::parse(path), v); propertyValueToNumber(v, n);
            return n;
        };
        {
            const auto menu = Singularity::Terminal::LawSentence::suggest(
                "when clicked then set @law-line-cube.glow to @law-line-cube.h", terminal->vocabulary(harness.lawManager));
            bool offersHp = false;
            for (const auto& sg : menu) offersHp = offersHp || mentions(sg.text, "hp");
            check(offersHp, "Tab completes the path a Set reads from");
        }
        {
            // Zach, 2026-10-05: "whenever I enter @ its really laggy". Each
            // offered being was described by re-walking the whole Universe:
            // O(N^2) per keystroke, 530 ms at 708 beings in his LawLine save
            // (Debug). Now one walk at vocabulary build: ~20 ms. The bound is
            // >10x the fixed cost and well under the quadratic one, so load
            // won't flake it but a regression to per-being lookups will trip it.
            const auto& vv = terminal->vocabulary(harness.lawManager);
            for (const char* line : {"when clicked then set @", "when clicked then set glow to @law"}) {
                const auto t0 = std::chrono::steady_clock::now();
                const auto menu = Singularity::Terminal::LawSentence::suggest(line, vv);
                const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                std::cout << "    '@' menu over " << vv.beings.size() << " beings: " << ms << " ms\n";
                check(!menu.empty() && (vv.beings.size() < 400 || ms < 250.0),
                      std::string("'@' completion is not quadratic in the beings: ") + line);
            }
        }
        auto before = harness.lawManager.getAll().size();
        terminal->inject(R"(called "Copy Hp" on "argument-test" then set @law-line-cube.glow to @law-line-cube.hp)"); frame();
        auto ids = lawNamed("Copy Hp");
        auto* copy = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        check(harness.lawManager.getAll().size() == before + 1 && copy && copy->actionModel() &&
              copy->actionModel()->kind == ActionNode::Kind::Map,
              "set to a path compiles to a Map passthrough by the saved Metalaw: " + printed.back());
        if (copy) {
            cube->setDynamicProperty("glow", 0.0);
            copy->applyTo(*cube);
            check(std::abs(readNumber("glow") - readNumber("hp")) < 0.001, "the copied value is the other path's live value");
        }
        terminal->inject(R"(called "Double Hp" on "argument-test" then set @law-line-cube.glow to @law-line-cube.hp * 2)"); frame();
        ids = lawNamed("Double Hp");
        auto* twice = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        if (twice) {
            cube->setDynamicProperty("glow", 0.0);
            twice->applyTo(*cube);
        }
        check(twice && std::abs(readNumber("glow") - 2.0 * readNumber("hp")) < 0.001, "arithmetic over a read path is copied too");
        before = harness.lawManager.getAll().size();
        terminal->inject(R"(called "Literal Glow" on "argument-test" then set @law-line-cube.glow to 1)"); frame();
        ids = lawNamed("Literal Glow");
        auto* literal = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        check(literal && literal->actionModel() && literal->actionModel()->kind == ActionNode::Kind::Set,
              "a literal Set keeps its exact existing model (no compiler)");
        if (auto* assign = harness.lawManager.find("law-line-compile-assignment-expression")) {
            assign->setEnabled(false);
            before = harness.lawManager.getAll().size();
            terminal->inject(R"(called "No Copy Compiler" on "argument-test" then set @law-line-cube.glow to @law-line-cube.hp)"); frame();
            check(harness.lawManager.getAll().size() == before, "without the assignment Metalaw, a path read refuses: " + printed.back());
            assign->setEnabled(true);
        } else {
            check(false, "the assignment compiler Metalaw is in the LawLine closure");
        }
    }
    // Arithmetic conditions (the "arithmetic notation in Zone conditions"
    // rung): sensed by the parser, compiled by the seven saved
    // law-line-compile-condition-* Metalaws into existing Zone conditions.
    {
        const auto speakCondition = [&](const std::string& name, const std::string& condition) -> Law* {
            terminal->inject("called \"" + name + "\" on \"argument-test\" if " + condition + " then set glow 1"); frame();
            const auto ids = lawNamed(name);
            return ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        };
        const auto holds = [&](Law* law) {
            return law && law->conditionModel() && law->conditionModel()->compile()(ECA::Event{}, *cube);
        };
        cube->setDynamicProperty("hp", 3.0);
        cube->setDynamicProperty("glow", 0.0);
        auto* twice = speakCondition("Twice Hp", "@law-line-cube.hp * 2 > 5");
        check(twice && twice->conditionModel(), "an arithmetic condition is authored through the saved Metalaws: " + printed.back());
        check(holds(twice), "hp * 2 > 5 holds at hp 3");
        auto* strict = speakCondition("Strict Six", "@law-line-cube.hp * 2 > 6");
        check(strict && !holds(strict), "a strict > is false at equality (6 > 6)");
        auto* atLeast = speakCondition("At Least Six", "@law-line-cube.hp * 2 >= 6");
        check(holds(atLeast), ">= holds at equality");
        auto* subject = speakCondition("Subject Relative", "5 < hp * 2");
        check(holds(subject), "bare paths read off the subject, and 5 < f reads as f > 5");
        auto* both = speakCondition("Both Sides", "@law-line-cube.hp > @law-line-cube.glow + 1");
        check(holds(both), "paths on both sides compare their difference (3 > 0 + 1)");
        cube->setDynamicProperty("glow", 2.5);
        check(!holds(both), "the comparison is live: 3 > 2.5 + 1 no longer holds");
        cube->setDynamicProperty("glow", 0.0);
        auto* range = speakCondition("In Range", "hp * 2 between 5 and 7");
        check(holds(range), "between bounds the function (5 <= 6 <= 7)");
        auto* undefinedGt = speakCondition("Undefined Gt", "@law-line-cube.nothing-here * 2 > 1");
        auto* undefinedNe = speakCondition("Undefined Ne", "@law-line-cube.nothing-here * 2 is not 1");
        check(undefinedGt && !holds(undefinedGt) && undefinedNe && !holds(undefinedNe),
              "undefined math never satisfies, even for > and is-not");
        cube->setDynamicProperty("hp", 2.0);
        check(!holds(twice), "hp * 2 > 5 stops holding at hp 2");
        cube->setDynamicProperty("hp", 3.0);
        auto* plain = speakCondition("Plain Compare", "hp > 2");
        check(plain && plain->conditionModel() && plain->conditionModel()->kind == ConditionNode::Kind::Compare,
              "a plain comparison keeps its exact Compare model");
        if (auto* gt = harness.lawManager.find("law-line-compile-condition-gt")) {
            const auto applications = gt->applicationLog().size();
            const auto before = harness.lawManager.getAll().size();
            terminal->inject(R"(called "Preview Cond" on "argument-test" if hp * 2 > 5 then set glow 1?)"); frame();
            check(harness.lawManager.getAll().size() == before && gt->applicationLog().size() == applications,
                  "a previewed arithmetic condition executes no compiler");
            gt->setEnabled(false);
            terminal->inject(R"(called "No Cond Compiler" on "argument-test" if hp * 2 > 5 then set glow 1)"); frame();
            check(harness.lawManager.getAll().size() == before, "without its Metalaw an arithmetic > refuses: " + printed.back());
            gt->setEnabled(true);
        } else {
            check(false, "the > condition compiler Metalaw is in the LawLine closure");
        }
    }
    // Event meanings (Zach, 2026-10-05): an authored string property
    // `meaning.<event>` on the Zone, written by an ordinary AddProperty Law.
    {
        terminal->inject("called \"Define Click\" on \"argument-test\" then add property "
                         "@LawLine.meaning.object-clicked to \"a Person pressed and released on a being\""); frame();
        const auto ids = lawNamed("Define Click");
        auto* define = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        check(define && define->actionModel() && define->actionModel()->toJson().dump().find("meaning.object-clicked") != std::string::npos,
              "add property splits @LawLine | meaning.object-clicked by the longest known being: " + printed.back());
        if (define) define->applyTo(*cube);
        PropertyValue stored;
        check(lawGetValue(harness.zones.active(), PropertyPath::parse("meaning.object-clicked"), stored) &&
              std::holds_alternative<std::string>(stored), "the Zone now carries the authored meaning");
        const auto vocab = terminal->vocabulary(harness.lawManager);
        check(vocab.describeEvent && mentions(vocab.describeEvent("object-clicked"), "pressed and released"),
              "the menu says what object-clicked means: " + (vocab.describeEvent ? vocab.describeEvent("object-clicked") : std::string{}));
        const auto menu = Singularity::Terminal::LawSentence::suggest("called X on object-cl", vocab);
        bool shown = false;
        for (const auto& sg : menu) shown = shown || (sg.text == "object-clicked" && mentions(sg.description, "pressed"));
        check(shown, "Tab on an event shows its authored meaning");
        check(harness.zones.persistActiveZone(), "Save Zone after authoring a meaning");
        const auto persisted = SaveSystem::readZoneIdentity("LawLine");
        check(persisted.dump().find("pressed and released on a being") != std::string::npos,
              "the meaning is saved with the Zone");
    }
    // Authored appearance (Zach, 2026-10-14: "make the Law Line ... more
    // beautiful using Lexeme symbols or objects with symbolic properties").
    // A Zone's `appearance.chrome.*` and a Lexeme's own `appearance.*` are
    // ordinary dynamic properties a Person writes with a sentence.
    {
        const auto vocab = terminal->vocabulary(harness.lawManager);
        std::string setLexeme;
        for (const auto& w : vocab.words)
            if (w.symbol == "set" && w.opcode == "action.Set" && !w.lexemeId.empty()) setLexeme = w.lexemeId;
        check(!setLexeme.empty(), "the seed's 'set' is an authored Lexeme");
        auto lexeme = Singularity::Language::LanguageSystem::instance().findById(setLexeme);
        check(lexeme != nullptr, "that Lexeme is reachable by its identifier");

        terminal->inject("called \"Selected Star\" on \"argument-test\" then add property "
                         "@LawLine.appearance.chrome.selected.glyph to \"\u2727\""); frame();
        auto ids = lawNamed("Selected Star");
        auto* star = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        check(star != nullptr, "a sentence authors the Zone's selection glyph: " + printed.back());
        if (star) star->applyTo(*cube);
        terminal->inject("called \"Set Is Gold\" on \"argument-test\" then add property @" + setLexeme +
                         ".appearance.color to gold"); frame();
        ids = lawNamed("Set Is Gold");
        auto* gold = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        check(gold != nullptr, "a sentence authors a Lexeme's own colour: " + printed.back());
        if (gold) gold->applyTo(*cube);
        PropertyValue authored;
        check(lexeme && lexeme->getDynamicProperty("appearance.color", authored) &&
                  std::holds_alternative<glm::vec3>(authored),
              "the Lexeme now carries appearance.color as a vec3, like an Object's color");
        if (lexeme) lexeme->setDynamicProperty("appearance.glyph", std::string("\u2726"));

        PropertyValue depthWas;
        lawGetValue(*terminal, PropertyPath::parse("colorDepth"), depthWas);
        lawSetValue(*terminal, PropertyPath::parse("colorDepth"), PropertyValue(std::string("truecolor")));
        terminal->injectKeys("when clicked then s"); frame();
        const auto shown = terminal->editorForTests().render(120).text;
        check(mentions(shown, "\u2727"), "the menu's selection mark is the Zone's authored glyph");
        check(mentions(shown, "\u2726"), "'set' shows its Lexeme's glyph in the menu");
        check(mentions(shown, "38;2;255;214;0m"), "'set' wears its Lexeme's truecolor gold (255, 214, 0)");
        {
            // The look cache is re-derived every frame; it must stay cheap.
            const auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < 200; ++i) terminal->sense(harness.lawManager);
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 200;
            std::cout << "    sense() with authored looks: " << us << " us per frame ("
                      << Singularity::Language::LanguageSystem::instance().getAll().size() << " Lexemes)\n";
            check(us < 2000.0, "sense() stays well under a frame's budget with appearance read every frame");
        }
        lawSetValue(*terminal, PropertyPath::parse("colorDepth"), PropertyValue(std::string("256")));
        frame();
        const auto lowered = terminal->editorForTests().render(120).text;
        check(!mentions(lowered, "38;2;") && mentions(lowered, "38;5;220m"), "at 256 colours the same gold is lowered (38;5;220), never dropped");
        lawSetValue(*terminal, PropertyPath::parse("colorDepth"), depthWas);
        terminal->injectKeys("\x03"); frame();

        // The real channel rejects control glyphs before the look cache and
        // keeps long help-heading glyphs inside the preformatted box.
        harness.zones.active().setDynamicProperty("appearance.chrome.selected.glyph", std::string("\nextra-row"));
        terminal->injectKeys("when clicked then s"); frame();
        const auto control = terminal->editorForTests().render(80);
        check(control.text.find("extra-row") == std::string::npos &&
                  std::count(control.text.begin(), control.text.end(), '\n') == control.rows - 1,
              "control glyphs cannot add unaccounted rows through the real Terminal channel");
        harness.zones.active().setDynamicProperty("appearance.chrome.selected.glyph", std::string("\u2727"));
        std::string heading;
        for (int i = 0; i < 50; ++i) heading += "\u2764\uFE0F";
        harness.zones.active().setDynamicProperty("appearance.chrome.help.mark.glyph", heading);
        terminal->injectKeys("\x1bOP"); frame();
        const auto help = terminal->editorForTests().render(80);
        bool helpFits = help.text.find("\u2764\uFE0F") != std::string::npos;
        for (std::size_t at = help.text.find("\r\n"); at != std::string::npos;) {
            at += 2;
            const auto end = help.text.find("\r\n", at);
            helpFits &= Singularity::Terminal::visibleWidth(help.text.substr(at, end == std::string::npos ? end : end - at)) <= 79;
            at = end;
        }
        check(helpFits, "the actual help page clips an authored emoji heading to its cell budget");
        terminal->injectKeys("\x03"); frame();
        terminal->injectKeys("\x03"); frame();
        harness.zones.active().removeDynamicProperty("appearance.chrome.help.mark.glyph");

        // Saved with the Zone, and read back: Lexemes' authoredProperties were
        // written before 2026-10-14 but never restored on load.
        check(harness.zones.persistActiveZone(), "Save Zone after authoring appearance");
        const auto persisted = SaveSystem::readZoneIdentity("LawLine");
        check(persisted.dump().find("appearance.chrome.selected.glyph") != std::string::npos,
              "the Zone's appearance is saved with it");
        if (lexeme) {
            lexeme->removeDynamicProperty("appearance.glyph");
            Zone probe("appearance-probe", "appearance-probe");
            auto records = nlohmann::json{{"lexemes", persisted.value("lexemes", nlohmann::json::array())},
                                          {"formationRelations", nlohmann::json::array()}};
            applyFormationRelations(probe, records);
            PropertyValue glyph;
            check(!lexeme->getDynamicProperty("appearance.glyph", glyph),
                  "reference hydration preserves a live Lexeme's deliberate glyph removal");
            lexeme->setDynamicProperty("appearance.glyph", std::string("live-glyph"));
            applyFormationRelations(probe, records);
            check(lexeme->getDynamicProperty("appearance.glyph", glyph) && glyph == PropertyValue(std::string("live-glyph")),
                  "reference hydration preserves a newer live glyph across Zones");
            applyFormationRelations(probe, records, true);
            check(lexeme->getDynamicProperty("appearance.glyph", glyph) && glyph == PropertyValue(std::string("\u2726")),
                  "explicit restoration may restore the saved glyph");
            // A never-interned identity exercises actual first hydration, also
            // when stored graphs need the Lexeme before relation decoding.
            auto fresh = nlohmann::json{{"id", "lexeme.appearance.fresh-hydration"}, {"symbol", "Fresh appearance"},
                                       {"authoredProperties", {{"appearance.glyph", "fresh-glyph"}}}};
            applyZoneJson(probe, nlohmann::json{{"lexemes", nlohmann::json::array({fresh})},
                                               {"storedSingulars", nlohmann::json::array()}}, true);
            const auto restored = Singularity::Language::LanguageSystem::instance().findById("lexeme.appearance.fresh-hydration");
            check(restored && restored->getDynamicProperty("appearance.glyph", glyph) && glyph == PropertyValue(std::string("fresh-glyph")),
                  "fresh Lexeme properties hydrate before stored graph references, even without formationRelations");
        }
        // Leave the shared fixture as it was for later checks.
        if (lexeme) {
            lexeme->removeDynamicProperty("appearance.glyph");
            lexeme->removeDynamicProperty("appearance.color");
        }
        harness.zones.active().removeDynamicProperty("appearance.chrome.selected.glyph");
    }
    // Multi-line blocks: typed one line per frame, folded when an empty line ends them.
    {
        const auto before = harness.lawManager.getAll().size();
        for (const char* line : {"called \"Block Guard\" on \"argument-test\":",
                                 "  if all:",
                                 "    @law-line-cube.hp * 2 > 5",
                                 "    @law-line-cube.glow < 1",
                                 "  then:",
                                 "    set @law-line-cube.glow to 1",
                                 "    add @law-line-cube.hp by 1"}) {
            terminal->inject(line); frame();
        }
        check(harness.lawManager.getAll().size() == before, "an open block authors nothing line by line");
        PropertyValue block;
        lawGetValue(*terminal, PropertyPath::parse("block"), block);
        check(std::holds_alternative<std::string>(block) && mentions(std::get<std::string>(block), "if all:"),
              "the open block is a registered, readable property");
        terminal->inject(""); frame();
        const auto ids = lawNamed("Block Guard");
        auto* guard = ids.empty() ? nullptr : harness.lawManager.find(ids.back());
        check(guard && harness.lawManager.getAll().size() == before + 1, "an empty line folds the block into one Law: " + printed.back());
        if (guard) {
            cube->setDynamicProperty("hp", 3.0);
            cube->setDynamicProperty("glow", 0.0);
            check(guard->conditionModel() && guard->conditionModel()->compile()(ECA::Event{}, *cube),
                  "the block's 'if all:' children hold together");
            guard->applyTo(*cube);
            PropertyValue glow, hp; double g = 0, h = 0;
            lawGetValue(*cube, PropertyPath::parse("glow"), glow); propertyValueToNumber(glow, g);
            lawGetValue(*cube, PropertyPath::parse("hp"), hp); propertyValueToNumber(hp, h);
            check(std::abs(g - 1.0) < 1e-6 && std::abs(h - 4.0) < 1e-6, "the block's 'then:' children both act");
            cube->setDynamicProperty("hp", 3.0);
            cube->setDynamicProperty("glow", 0.0);
        }
        const auto after = harness.lawManager.getAll().size();
        terminal->inject("if any:"); frame();
        terminal->inject("  all:"); frame();
        terminal->inject("    hp > 1"); frame();
        terminal->inject(""); frame();
        check(harness.lawManager.getAll().size() == after && mentions(printed.back(), "parentheses"),
              "a block mixing and/or refuses and authors nothing: " + printed.back());
    }
    {
        auto before = harness.lawManager.getAll().size();
        terminal->inject(R"(called "Missing Factor" on "argument-test" then Lerp <path: "glow", operand: 1>)"); frame();
        check(harness.lawManager.getAll().size() == before, "missing required argument refuses instead of using a hidden default");
        terminal->inject(R"(called "Typo Field" on "argument-test" then Lerp <path: "glow", operand: 1, factor: 0.5, typo: 3>)"); frame();
        check(harness.lawManager.getAll().size() == before && mentions(printed.back(), "not consumed"), "unknown argument refuses instead of disappearing");
        terminal->inject(R"(called "Duplicate Field" on "argument-test" then Lerp <path: "glow", operand: 1, factor: 0.5, factor: 0.2>)"); frame();
        check(harness.lawManager.getAll().size() == before && mentions(printed.back(), "duplicate"), "duplicate argument refuses");
        auto* compiler = harness.lawManager.find("law-line-compile-args-lerp");
        if (compiler) {
            auto applications = compiler->applicationLog().size();
            terminal->inject(R"(called "Preview Lerp" on "argument-test" then Lerp <path: "glow", operand: 1, factor: 0.5>?)"); frame();
            check(harness.lawManager.getAll().size() == before && compiler->applicationLog().size() == applications,
                  "parameterized action preview executes no compiler");
            compiler->setEnabled(false);
            terminal->inject(R"(called "No Lerp Compiler" on "argument-test" then Lerp <path: "glow", operand: 1, factor: 0.5>)"); frame();
            check(harness.lawManager.getAll().size() == before && mentions(printed.back(), "no authored Metalaw"), "new action argument form has no compiler fallback");
            compiler->setEnabled(true);
        }
        terminal->inject(R"(called "Default Timbre" on "argument-test" then PlayAudio <frequencyPath: "tone.frequency", amplitudePath: "tone.amplitude">)"); frame();
        auto defaults = lawNamed("Default Timbre");
        auto defaultLaw = defaults.empty() ? nullptr : harness.lawManager.find(defaults.back());
        check(defaultLaw && defaultLaw->actionModel() && defaultLaw->actionModel()->propertyName == "sine",
              "optional timbre is supplied by the authored template default");
        terminal->inject(R"(called "Bad Factor" on "argument-test" then Lerp <path: "glow", operand: 1, factor: "wrong type">)"); frame();
        check(lawNamed("Bad Factor").empty(), "malformed typed argument refuses authoring");
        auto menu = Singularity::Terminal::LawSentence::suggest("when clicked then Ler", terminal->vocabulary(harness.lawManager));
        check(std::any_of(menu.begin(), menu.end(), [](const auto& item) { return item.text == "Lerp" && mentions(item.snippet, "factor"); }),
              "completion displays the authored Lerp argument signature");
    }

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

    // Zach reported that the exact supplied stairway line did nothing after
    // clicking a cube. Cover both its specific-ID restriction and a corrected
    // program via the actual pointer press/release channel, not applyTo alone.
    {
        // Earlier independent fixtures intentionally installed click/hover
        // creation Laws. Quiet only those test fixtures before measuring this
        // program's effects; keep all compiler and Terminal wiring Laws live.
        for (const auto& law : harness.lawManager.getAll()) {
            if (!law || law->isFirstMover()) continue;
            const auto triggers = harness.lawManager.triggersOf(law->getIdentifier());
            if (std::find(triggers.begin(), triggers.end(), "object-clicked") != triggers.end() ||
                std::find(triggers.begin(), triggers.end(), "object-hover-entered") != triggers.end() ||
                std::find(triggers.begin(), triggers.end(), "object-hover-exited") != triggers.end()) law->setEnabled(false);
        }
        const std::string original = R"(called "Stairmaker" when clicked if Identity @law-line-cube then Sequence <children: [Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: blue, authored: {stair: true}}>, Create <Object, properties: {shape.kind: Cube, position: my.position + (1, -2, 0), color: blue, authored: {stair: true}}>, Create <Object, properties: {shape.kind: Cube, position: my.position + (2, -1, 0), color: blue, authored: {stair: true}}>]>; called "Golden Welcome" when hovered if stair is true then Set color to gold; called "Blue Rest" when the pointer leaves if stair is true then Set color to blue; called "Ascending Stone" when clicked if stair is true then Add position.y by 0.5)";
        auto before = harness.lawManager.getAll().size();
        terminal->inject(original); frame();
        check(harness.lawManager.getAll().size() == before + 4, "original stairway line authors four Laws: " + printed.back());
        auto launcher = std::make_shared<Object>();
        launcher->setObjectID("stairway-launcher");
        launcher->setPosition(glm::vec3(8, 5, -6));
        harness.zones.active().addObject(launcher);
        auto objectCount = harness.zones.active().getOwnedObjects().size();
        Core::EventBus::instance().publish(ECA::Event{"object-clicked", launcher.get(), nullptr, std::time(nullptr)});
        harness.lawManager.tick();
        check(harness.zones.active().getOwnedObjects().size() == objectCount,
              "original stairway silently ignores a different clicked cube because of its Identity condition");
        // Remove only this test's four newly registered Laws before comparing
        // the corrected line; never change a Person's saved world to diagnose.
        for (const auto& name : {"Stairmaker", "Golden Welcome", "Blue Rest", "Ascending Stone"})
            for (const auto& id : lawNamed(name)) harness.lawManager.remove(id);
        std::ifstream example(saves.parent_path() / "examples/law_line_stairway.txt");
        std::string corrected;
        std::getline(example, corrected);
        check(example.good() && !corrected.empty(), "the pasteable stairway example is available");
        before = harness.lawManager.getAll().size();
        terminal->inject(corrected); frame();
        check(harness.lawManager.getAll().size() == before + 4, "corrected stairway line authors four Laws: " + printed.back());
        Singularity::Input::InteractionChannel::Sense pointer;
        pointer.rayOrigin = launcher->getPosition() + glm::vec3(0, 0, 5);
        pointer.rayDirection = glm::vec3(0, 0, -1);
        auto click = [&](Object* target) {
            pointer.rayOrigin = target->getPosition() + glm::vec3(0, 0, 5);
            pointer.left = false; harness.interaction->observePending(pointer, {target}); harness.lawManager.tick();
            pointer.left = true; harness.interaction->observePending(pointer, {target}); harness.lawManager.tick();
            pointer.left = false; harness.interaction->observePending(pointer, {target}); harness.lawManager.tick();
        };
        click(launcher.get());
        auto& objects = harness.zones.active().getOwnedObjects();
        check(objects.size() == objectCount + 3, "real pointer click on an arbitrary cube creates three visible-height steps");
        if (objects.size() >= objectCount + 3) {
            for (int step = 0; step < 3; ++step) {
                auto* born = objects[objectCount + step].get();
                const auto expected = launcher->getPosition() + glm::vec3(step, step + 1, 0);
                check(glm::length(born->getPosition() - expected) < 0.001f, "step uses the clicked cube position and rises above it");
            }
            auto* step = objects[objectCount].get();
            pointer.rayOrigin = step->getPosition() + glm::vec3(0, 0, 5);
            pointer.left = false; harness.interaction->observePending(pointer, {step}); harness.lawManager.tick();
            PropertyValue color;
            lawGetValue(*step, PropertyPath::parse("color"), color);
            auto gold = std::get_if<glm::vec3>(&color);
            check(gold && std::abs(gold->x - 1) < 0.01 && std::abs(gold->y - 0.84) < 0.01, "pointer hover really turns a created step gold");
            float height = step->getPosition().y;
            click(step);
            check(std::abs(step->getPosition().y - height - 0.5) < 0.001 && objects.size() == objectCount + 3,
                  "clicking a step raises it without recursively creating more steps");
            pointer.rayOrigin = glm::vec3(100, 100, 100);
            harness.interaction->observePending(pointer, {step}); harness.lawManager.tick();
            lawGetValue(*step, PropertyPath::parse("color"), color);
            auto blue = std::get_if<glm::vec3>(&color);
            check(blue && blue->x == 0 && blue->y == 0 && blue->z == 1, "pointer departure really turns the step blue again");

            // Zach confirmed the original Stairmaker works and asked for a
            // richer program. This is an add-on: keep all four original Laws
            // live to catch hover/leave/click collisions in the actual context.
            std::ifstream skyExample(saves.parent_path() / "examples/law_line_sky_stairway.txt");
            std::string skyLine;
            std::getline(skyExample, skyLine);
            before = harness.lawManager.getAll().size();
            terminal->inject(skyLine); frame();
            check(!skyLine.empty() && harness.lawManager.getAll().size() == before + 3,
                  "spiral add-on authors three Laws through the existing Metalaws: " + printed.back());
            const auto skyStart = objects.size();
            click(step);
            check(objects.size() == skyStart + 8,
                  "clicking an existing step grows exactly eight spiral jewels alongside the original program");
            PropertyValue grown;
            step->getDynamicProperty("skyGrown", grown);
            check(grown == PropertyValue(true), "growth is latched on the clicked step as authored state");
            const std::array<glm::vec3, 8> spiralOffsets{{
                {1,.55f,0}, {2,1.1f,0}, {2,1.65f,1}, {2,2.2f,2},
                {1,2.75f,2}, {0,3.3f,2}, {0,3.85f,1}, {0,4.4f,0}}};
            if (objects.size() >= skyStart + 8) {
                for (std::size_t i = 0; i < spiralOffsets.size(); ++i) {
                    auto* jewel = objects[skyStart + i].get();
                    const auto& params = jewel->getShapeParams();
                    check(glm::length(jewel->getPosition() - step->getPosition() - spiralOffsets[i]) < .001f &&
                          jewel->getShapeKind() == Object::ShapeKind::Ellipsoid &&
                          std::abs(params.r - .8f) < .001f && std::abs(params.ry - .16f) < .001f &&
                          std::abs(params.rz - .65f) < .001f,
                          "spiral placement and flattened jewel geometry use the authored initializer values");
                }
                const auto count = objects.size();
                click(step);
                check(objects.size() == count, "clicking an already grown step does not duplicate its spiral");
                auto* jewel = objects[skyStart].get();
                pointer.rayOrigin = jewel->getPosition() + glm::vec3(0, 0, 5);
                pointer.left = false;
                harness.interaction->observePending(pointer, {jewel});
                const auto rotation = jewel->getRotationEulerDegrees();
                harness.lawManager.tick();
                lawGetValue(*jewel, PropertyPath::parse("color"), color);
                gold = std::get_if<glm::vec3>(&color);
                check(gold && glm::length(*gold - glm::vec3(1,.84f,0)) < .001f,
                      "the original hover Law highlights a new jewel gold");
                check(std::abs(jewel->getRotationEulerDegrees().y - rotation.y - 25) < .001f,
                      "hovering a jewel turns it by the authored 25 degrees");
                pointer.rayOrigin = glm::vec3(100, 100, 100);
                harness.interaction->observePending(pointer, {jewel}); harness.lawManager.tick();
                lawGetValue(*jewel, PropertyPath::parse("color"), color);
                auto cyan = std::get_if<glm::vec3>(&color);
                check(cyan && glm::length(*cyan - glm::vec3(0,1,1)) < .001f,
                      "the add-on restores jewel colour after the original Blue Steps Law");
                check(glm::length(jewel->getRotationEulerDegrees() - rotation) < .001f,
                      "leaving restores the jewel's original orientation");
                const auto stopped = jewel->getRotationEulerDegrees();
                harness.lawManager.tick();
                check(glm::length(jewel->getRotationEulerDegrees() - stopped) < .001f,
                      "a settled jewel does not rotate without another hover event");
                auto* crown = objects[skyStart + 7].get();
                click(crown);
                check(objects.size() == count + 8,
                      "a new crown can grow the next full turn of the sky tower");
            }
        }
    }

    // Zach's CLI authored the continuous example but its Identity @Zach guard
    // never matched his keyed Person. Display names are not identity aliases.
    // Keep the actual Terminal/compiler/tick path in addition to that predicate
    // witness; the earlier unkeyed harness concealed the example's mistake.
    {
        std::array<uint8_t, 32> fixtureKey{};
        fixtureKey.fill(42);
        // Already keyed when the seed records a keyed author (see boot above).
        const bool alreadyKeyed = harness.player.getIdentifier().rfind("did:earthcall:", 0) == 0;
        check(alreadyKeyed || harness.player.setPersonId(Identity::SingularId::fromPublicKey(fixtureKey)),
              "continuous creation fixture uses a keyed Person");
        check(harness.player.getIdentifier() != "Zach" &&
              !ConditionNode::identity("Zach").compile()(ECA::Event{}, harness.player),
              "Identity @Zach cannot match a keyed Person's display name");
        terminal->inject("called Wrong Name always if Identity @Zach then Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: gold}>");
        frame();
        auto& objects = harness.zones.active().getOwnedObjects();
        const auto beforeWrong = objects.size();
        harness.lawManager.tick();
        check(objects.size() == beforeWrong, "the original continuous example creates nothing for a keyed Person");

        std::ifstream source(saves.parent_path() / "examples/law_line_cubes_below.txt");
        std::string line; std::getline(source, line);
        check(source.good() && !line.empty(), "continuous example is read from the pasteable artifact");
        terminal->inject(line); frame();
        PropertyValue last;
        lawGetValue(*terminal, PropertyPath::parse("lastCreated"), last);
        auto* continuous = std::holds_alternative<std::string>(last)
            ? harness.lawManager.find(std::get<std::string>(last)) : nullptr;
        check(continuous && continuous->name() == "Cubes Below Me" &&
              continuous->activation() == Law::Activation::WhileTrue,
              "the corrected sentence authors a continuously firing Law");
        harness.player.position() = glm::vec3(7, 9, -4);
        const auto before = objects.size();
        harness.lawManager.tick();
        check(objects.size() == before + 1, "continuous tick creates exactly one cube for the keyed Person");
        if (objects.size() == before + 1)
            check(glm::length(objects.back()->getPosition() - glm::vec3(7, 6, -4)) < 0.001f,
                  "the continuous cube centre is three units below the actual author");
        harness.player.position() = glm::vec3(-2, 20, 6);
        harness.lawManager.tick();
        check(objects.size() == before + 2, "the next tick creates one more cube without clicking");
        if (objects.size() == before + 2)
            check(glm::length(objects.back()->getPosition() - glm::vec3(-2, 17, 6)) < 0.001f,
                  "continuous creation follows the author's changed position");
        if (continuous) continuous->setEnabled(false);
    }

    // Zach requested direct 2D CLI wizardry. Exercise real authored compilers,
    // LawManager activation, typed persistence and independent coordinate samples.
    // Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-06.
    {
        using Singularity::Screen::ScreenChannel;
        ScreenChannel::syncRegister(harness.lawManager);
        auto* screen = ScreenChannel::find(harness.lawManager);
        check(screen != nullptr, "direct Screen channel is available to CLI Laws");
        auto submit = [&](const char* file) {
            std::ifstream input(saves.parent_path() / "examples" / file);
            std::string line; std::getline(input, line);
            const auto before = harness.lawManager.getAll().size();
            terminal->inject(line); frame();
            // The channel adopts at act(), after the preceding three ticks.
            harness.lawManager.tick(); harness.lawManager.tick();
            check(harness.lawManager.getAll().size() > before,
                  std::string("Screen example authors through Metalaws: ") + file + " / " + printed.back());
        };
        std::ifstream pixelFile(saves.parent_path() / "examples/law_line_screen_pixel.txt");
        std::string pixelLine; std::getline(pixelFile, pixelLine);
        const auto previewCount = harness.lawManager.getAll().size();
        const auto* compiler = harness.lawManager.find("law-line-compile-value-vectorfield");
        const auto compilerLog = compiler ? compiler->applicationLog().size() : 0;
        terminal->inject(pixelLine + " ?"); frame();
        check(harness.lawManager.getAll().size() == previewCount && compiler &&
              compiler->applicationLog().size() == compilerLog,
              "nested Screen value preview executes no compiler and registers no Law");
        submit("law_line_screen_pixel.txt");
        PropertyValue value;
        bool typed = screen && screen->getDynamicProperty("output.color", value) &&
                     std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value);
        check(typed, "CLI supplies a typed AST VectorField directly on Screen");
        if (typed) {
            auto field = std::get<std::shared_ptr<OntoMath::VectorField>>(value);
            auto sample = [&](double x, double y) { return field->astDefinition.evaluate(
                {{"p", glm::vec3(x,y,0)}, {"x",x}, {"y",y}}); };
            const auto inside = sample(7.5, 9.5), outside = sample(8.5, 9.5);
            check(inside && std::holds_alternative<glm::vec3>(*inside) && !outside,
                  "physical pixel selector includes its centre and excludes the neighbouring centre");
            const auto encoded = propertyValueToJson(value);
            check(propertyValueToJson(propertyValueFromJson(encoded)) == encoded,
                  "CLI field preserves full mathematics through typed serialization");
        }
        auto* fieldCompiler = harness.lawManager.find("law-line-compile-value-vectorfield");
        if (fieldCompiler) fieldCompiler->setEnabled(false);
        const auto beforeMissing = harness.lawManager.getAll().size();
        terminal->inject(pixelLine); frame();
        check(harness.lawManager.getAll().size() == beforeMissing && mentions(printed.back(), "no authored Metalaw"),
              "missing field compiler refuses without a parser fallback");
        if (fieldCompiler) fieldCompiler->setEnabled(true);
        const auto beforeTypo = harness.lawManager.getAll().size();
        terminal->inject("when clicked then add property @screen-channel.output.color to VectorField <piecez: []>"); frame();
        check(harness.lawManager.getAll().size() == beforeTypo,
              "unconsumed value argument refuses rather than dropping an author typo");
        const auto badTypeCount = harness.lawManager.getAll().size();
        terminal->inject("when clicked then add property @screen-channel.output.color to VectorField <pieces: [Piece <value: $(1)>]>"); frame();
        check(harness.lawManager.getAll().size() == badTypeCount && mentions(printed.back(), "field expects Vector"),
              "statically wrong field result type refuses during compilation");
        auto competing = std::make_shared<Law>("competing value compiler", std::vector<Singular*>{&harness.player});
        competing->setLawIdentifier("test-conflicting-value-compiler");
        competing->addTarget(*terminal);
        competing->setConditionModel(ConditionNode::all({
            ConditionNode::compare("compilation.input.slot",ConditionNode::Op::Eq,std::string("value")),
            ConditionNode::compare("compilation.input.wordLaw",ConditionNode::Op::Eq,std::string("law-line-value-constructor-vectorfield"))}));
        // Consume the input argument, but produce a different valid envelope.
        competing->setActionModel(ActionNode::set("compilation.template",std::string(
            "{\"literal\":{\"$slot\":\"/arguments/pieces\"}}")));
        harness.lawManager.add(competing);
        const auto beforeConflict = harness.lawManager.getAll().size();
        terminal->inject(pixelLine); frame();
        check(harness.lawManager.getAll().size() == beforeConflict && mentions(printed.back(), "conflicting"),
              "conflicting value Metalaws refuse regardless of registration order");
        harness.lawManager.remove(competing->getIdentifier());
        // Re-grant after a read materializes a lazy accessor: it is still
        // authored state, not a first-mover path. Also cover scalar constructors.
        if (screen) screen->findProperty("output.color");
        terminal->inject("called Replace Field on \"screen-replace\" then add property @screen-channel.output.color to VectorField <pieces: [Piece <value: $(blue)>]>"); frame();
        Core::EventBus::instance().publish(ECA::Event{"screen-replace",cube,nullptr,std::time(nullptr)}); harness.lawManager.tick();
        bool replaced = screen && screen->getDynamicProperty("output.color",value) &&
                        std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value);
        if (replaced) {
            auto sample = std::get<std::shared_ptr<OntoMath::VectorField>>(value)->astDefinition.evaluate({});
            replaced = sample && std::holds_alternative<glm::vec3>(*sample) &&
                       glm::length(std::get<glm::vec3>(*sample)-glm::vec3(0,0,1))<.001f;
        }
        check(replaced,"AddProperty can replace a read authored field without shadowing an engine path");
        terminal->inject("called Opacity Field on \"screen-opacity\" then add property @screen-channel.output.opacity to ScalarField <pieces: [Piece <value: $(Component <value: $((0.5,0.2,0.3)), index: \"x\">)>]>"); frame();
        Core::EventBus::instance().publish(ECA::Event{"screen-opacity",cube,nullptr,std::time(nullptr)}); harness.lawManager.tick();
        bool scalar = screen && screen->getDynamicProperty("output.opacity",value) &&
                      std::holds_alternative<std::shared_ptr<OntoMath::ScalarField>>(value);
        if (scalar) {
            auto sample = std::get<std::shared_ptr<OntoMath::ScalarField>>(value)->astDefinition.evaluate({});
            double alpha=0; scalar = sample && propertyValueToNumber(*sample,alpha) && std::abs(alpha-.5)<.001;
        }
        check(scalar,"ScalarField and Component compile and sample authored opacity");
        terminal->inject("called Protect Engine Path on \"screen-protect\" then add property @screen-channel.enabled to false"); frame();
        Core::EventBus::instance().publish(ECA::Event{"screen-protect",cube,nullptr,std::time(nullptr)}); harness.lawManager.tick();
        check(screen && screen->isEnabled(),"AddProperty still refuses to shadow a registered engine property");
        if (screen) screen->setDynamicProperty("enabled",true); // deliberately malformed test storage
        Core::EventBus::instance().publish(ECA::Event{"screen-protect",cube,nullptr,std::time(nullptr)}); harness.lawManager.tick();
        PropertyValue duplicate;
        check(screen && screen->getDynamicProperty("enabled",duplicate) && duplicate==PropertyValue(true),
              "a duplicate authored-storage name cannot make an engine accessor shadowable");
        if (screen) screen->removeDynamicProperty("enabled");
        Universe::instance().setClock(1, 0.1);
        submit("law_line_screen_lens.txt");
        bool diamond = screen && screen->getDynamicProperty("output.color",value) &&
                       std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value);
        if (diamond) {
            auto sample=std::get<std::shared_ptr<OntoMath::VectorField>>(value)->astDefinition.evaluate(
                {{"p",glm::vec3(100,50,0)},{"x",100.0},{"y",50.0},{"u",.5},{"v",.5},
                 {"width",200.0},{"height",100.0},{"t",0.0}});
            diamond=sample && std::holds_alternative<glm::vec3>(*sample) &&
                    glm::length(std::get<glm::vec3>(*sample)-glm::vec3(1,.92,.55))<.001f;
        }
        check(diamond,"authored math-context Metalaw reads y as a coordinate in the centre diamond");
        terminal->inject("called Keep Yes on \"screen-yes\" then add property @screen-channel.testYes to y"); frame();
        Core::EventBus::instance().publish(ECA::Event{"screen-yes",cube,nullptr,std::time(nullptr)}); harness.lawManager.tick();
        PropertyValue yes;
        check(screen && screen->getDynamicProperty("testYes",yes) && yes==PropertyValue(true),
              "authored value-context Metalaw preserves the existing y/yes shorthand");
        if(screen) screen->removeDynamicProperty("testYes");
        double beforeTime = 0;
        if (screen && screen->getDynamicProperty("output.time", value)) propertyValueToNumber(value, beforeTime);
        harness.lawManager.tick();
        double afterTime = 0;
        if (screen && screen->getDynamicProperty("output.time", value)) propertyValueToNumber(value, afterTime);
        check(afterTime > beforeTime, "authored Flow advances the explicitly supplied Screen time");
        submit("law_line_screen_region.txt");
        PropertyValue regionColor,regionSelector;
        check(screen && lawGetValue(*screen,PropertyPath::parse("halo.color"),regionColor) &&
              std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(regionColor) &&
              lawGetValue(*screen,PropertyPath::parse("halo.selector"),regionSelector) &&
              std::holds_alternative<std::shared_ptr<OntoMath::ScalarField>>(regionSelector),
              "ScreenRegion Metalaw authors two typed predicates on an existing Singular");
        auto* regionCompiler=harness.lawManager.find("law-line-compile-value-screenregion");
        check(regionCompiler!=nullptr,"the named region compiler is an ordinary registered Metalaw");
        if (regionCompiler) regionCompiler->setEnabled(false);
        const auto withoutRegion=harness.lawManager.getAll().size();
        terminal->inject("called No Region when clicked then add property region to ScreenRegion <color: VectorField <pieces: [Piece <value: $(red)>]>, selector: ScalarField <pieces: [Piece <value: $(-1)>]>>"); frame();
        check(harness.lawManager.getAll().size()==withoutRegion && mentions(printed.back(),"no authored Metalaw"),
              "removing the ScreenRegion Metalaw refuses without bespoke CLI lowering");
        if (regionCompiler) regionCompiler->setEnabled(true);
        harness.player.setDynamicProperty("fieldWatch",false);
        terminal->inject("called Field Watch becomes true if is a Person and @screen-channel.halo.color.astDefinition.pieces.0.mathNode.children.0.scalarForm.terms.0.c below 0.5 then set my.fieldWatch to true");frame();
        harness.lawManager.tick();
        check(harness.player.getDynamicProperty("fieldWatch",value) && value==PropertyValue(false),
              "nested field watcher starts outside its satisfaction bound");
        submit("law_line_screen_region_edit.txt");
        check(harness.player.getDynamicProperty("fieldWatch",value) && value==PropertyValue(true),
              "canonical granular field edits wake a dependent authored Law through the change feed");
        check(screen && lawGetValue(*screen,PropertyPath::parse("halo.color"),value) &&
              std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value),
              "granular edit retains a typed field rather than replacing it with a syntax record");
        if (screen && std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value)) {
            const auto sample=std::get<std::shared_ptr<OntoMath::VectorField>>(value)->astDefinition.evaluate({{"p",glm::vec3(120.5,120.5,0)}});
            check(sample && std::get<glm::vec3>(*sample)==glm::vec3(0,1,1),"three Law-addressed coefficients recolour the named region cyan");
        }
        PropertyValue ignored;
        check(screen && PropertyPath::parse("sample.result").setValue(*screen,std::make_shared<PropertyDict>())==PropertyPath::PathResult::ReadOnly,
              "Screen observations cannot be overwritten as authored state");
        submit("law_line_screen_clear.txt");
        check(screen && !screen->getDynamicProperty("output.color", value),
              "CLI clear Law withdraws direct output without creating an Object or texture");
        submit("law_line_screen_pixel.txt");
        check(screen && screen->getDynamicProperty("output.color",value) &&
              std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value),
              "a cleared authored field can be granted again through its surviving accessor");
    }

    // Zach: a whole art editor authored by one paste, through the saved
    // sentence/compiler Metalaws. Its state is a Person predicate, not a class.
    {
        auto* screen=Singularity::Screen::ScreenChannel::find(harness.lawManager);
        std::ifstream in(saves.parent_path() / "examples/law_line_pixel_art_editor.txt");
        std::string program;std::getline(in,program);
        const auto before=harness.lawManager.getAll().size();
        auto syntax=Singularity::Terminal::LawSentence::parse(program.substr(0,program.find(';'))+"?",terminal->vocabulary(harness.lawManager));
        if(!syntax.ok)std::cout << "EDITOR SYNTAX " << syntax.errorOffset << " " << syntax.error << " NEAR " << program.substr(syntax.errorOffset>80?syntax.errorOffset-80:0,160) << '\n';
        terminal->inject(program);frame();harness.lawManager.tick();
        check(harness.lawManager.getAll().size()==before+276,"one editor line registers its 276 authored Laws: "+printed.back());
        PropertyValue state;
        check(lawGetValue(harness.player,PropertyPath::parse("atelier.installed"),state) && state==PropertyValue(true),"editor initialization grants Person-owned state");
        const auto colour=[&](int x,int y) {
            PropertyValue value;
            if(!lawGetValue(harness.player,PropertyPath::parse("atelier.canvas"),value) ||
               !std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(value))return glm::vec3(-1);
            const auto& form=std::get<std::shared_ptr<OntoMath::VectorField>>(value)->astDefinition;
            auto sample=form.evaluate({{"u",.18+.62*(x+.5)/16},{"v",.12+.76*(y+.5)/16}});
            return sample && std::holds_alternative<glm::vec3>(*sample)?std::get<glm::vec3>(*sample):glm::vec3(-1);
        };
        const auto point=[&](double u,double v,bool held,bool captured=false) {
            Singularity::Input::InteractionChannel::Sense sense;
            sense.windowWidth=1000;sense.windowHeight=500;
            sense.pointerX=u*1000;sense.pointerY=v*500;sense.left=held;sense.uiCaptured=captured;
            harness.interaction->pointerLocked=false;
            harness.interaction->observePending(sense,{});
            for(int i=0;i<3;++i)harness.lawManager.tick();
        };
        const auto click=[&](double u,double v) {point(u,v,false);point(u,v,true);point(u,v,false);};
        check(colour(0,0)==glm::vec3(1),"initial canvas is white");
        click(.199375,.14375);
        check(glm::length(colour(0,0)-glm::vec3(1,.84,0))<1e-6f && colour(1,0)==glm::vec3(1),"pencil writes only the addressed pixel, through actual input sense");
        click(.89,.20);check(colour(0,0)==glm::vec3(1),"authored undo restores immutable previous field");
        click(.89,.33);check(glm::length(colour(0,0)-glm::vec3(1,.84,0))<1e-6f,"authored redo restores the painted field");
        click(.08,.12+6*.059+.0215);click(.238125,.14375);
        check(glm::length(colour(1,0)-glm::vec3(0,.8,.85))<1e-6f,"swatch selection reaches the next pixel");
        click(.89,.72);click(.238125,.14375);
        check(colour(1,0)==glm::vec3(1),"eraser chooses the authored white ink");
        click(.08,.12+2*.059+.0215);
        point(.277,.14375,true,true);point(.277,.14375,false);
        check(colour(2,0)==glm::vec3(1),"foreign UI capture veto prevents paint");
        point(.277,.14375,true);point(.315,.14375,true);point(.315,.14375,false);
        check(glm::length(colour(2,0)-glm::vec3(1,.25,.35))<1e-6f && glm::length(colour(3,0)-glm::vec3(1,.25,.35))<1e-6f,"held pencil paints newly entered cells");
        click(.89,.46);check(colour(0,0)==glm::vec3(1) && colour(3,0)==glm::vec3(1),"clear restores the blank artwork");
        click(.89,.20);check(glm::length(colour(3,0)-glm::vec3(1,.25,.35))<1e-6f,"clear itself is undoable");
        PropertyValue canvas;lawGetValue(harness.player,PropertyPath::parse("atelier.canvas"),canvas);
        check(propertyValueToJson(propertyValueFromJson(propertyValueToJson(canvas)))==propertyValueToJson(canvas),"edited canvas preserves typed mathematics through storage codec");
        click(.89,.85);
        check(lawGetValue(harness.player,PropertyPath::parse("atelier.enabled"),state) && state==PropertyValue(false) &&
              !screen->hasDynamicProperty("output.color"),"close stops display without deleting artwork or reinstalling");
        check(glm::length(colour(3,0)-glm::vec3(1,.25,.35))<1e-6f,"closed editor retains authored artwork");
    }

    check(inactiveWorld->getOwnedObjects().size() == worldObjectsBefore,
          "inactive World receives no births from Laws running in the visible LawLine Zone");
    std::cout << "law_line_zone_test: " << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
