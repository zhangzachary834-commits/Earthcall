// The Law Line: the Mac Terminal's command line read as law text
// (docs/architecture/law/Natural Language Law Authoring.md, Zach 2026-09-25).
//
// Part 1 holds the pure grammar to its word: structural words, the engine's
// opcode spellings, Lexemes that DENOTE Laws (the Law holds the opcode),
// character-level spellings (phrases, glued symbols, bound prefixes and
// suffixes, keyboard smashes), presets that fix clauses, the Metalaw seam for
// shared spellings, named refusals, and Tab.
//
// Part 2 runs the TerminalChannel end to end with no TTY: a line is injected,
// the AUTHORED laws decide it should be spoken, the channel authors the Law
// under the Person's authority, and the new Law fires.

#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Terminal/LawSentence.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <algorithm>
#include <cassert>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace Singularity::Terminal;
namespace LS = Singularity::Terminal::LawSentence;

namespace {

bool contains(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

bool mentions(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

double number(const Singular& being, const char* name) {
    PropertyValue v;
    assert(being.getDynamicProperty(name, v));
    if (const auto* d = std::get_if<double>(&v)) return *d;
    if (const auto* f = std::get_if<float>(&v)) return *f;
    assert(false && "not a number");
    return 0.0;
}

LS::Vocabulary baseVocabulary() {
    LS::Vocabulary v;
    v.words = LS::canonicalWords();
    v.events = {"object-clicked", "tick", "terminal-line-entered"};
    v.beings = {"cube", "lamp"};
    v.scopeBeing = "cube";
    v.propertiesOf = [](const std::string& id) {
        if (id == "cube") return std::vector<std::string>{"color", "glow", "hp"};
        return std::vector<std::string>{"brightness"};
    };
    return v;
}

void grammar() {
    // --- canonical spellings alone: the line works before any alias exists
    {
        const auto p = LS::parse("on object-clicked then set color 1 0 0", baseVocabulary());
        assert(p.ok);
        assert(p.activation == Law::Activation::OnEvent);
        assert(p.triggers == std::vector<std::string>{"object-clicked"});
        assert(p.action && p.action->kind == ActionNode::Kind::Set);
        assert(p.action->path.toString() == "color");
        assert(std::holds_alternative<glm::vec3>(p.action->operand));
        assert(p.scope == Law::Scope::Subject);
    }
    // --- and binds tighter than or; glued symbols need no spaces
    {
        const auto p = LS::parse("on tick if hp>=3 and glow<1 or hp = 0 then add hp -1",
                                 baseVocabulary());
        assert(p.ok);
        assert(p.condition && p.condition->kind == ConditionNode::Kind::Any);
        assert(p.condition->children.size() == 2);
        assert(p.condition->children[0].kind == ConditionNode::Kind::All);
        assert(p.condition->children[0].children[0].op == ConditionNode::Op::Ge);
        assert(p.action->kind == ActionNode::Kind::Add);
        assert(p.action->operand == PropertyValue(-1.0));
    }
    // --- actions chain with then / and / comma; WhileTrue sweeps everyone
    {
        const auto p = LS::parse("WhileTrue if hp < 1 then set glow 0, scale hp by 2 and publish faded",
                                 baseVocabulary());
        assert(p.ok);
        assert(p.activation == Law::Activation::WhileTrue);
        assert(p.scope == Law::Scope::Everyone);
        assert(p.action->kind == ActionNode::Kind::Sequence);
        assert(p.action->children.size() == 3);
        assert(p.action->children[2].eventType == "faded");
    }

    // --- Lexeme <--denotes--> Law: phrases, keyboard smashes, bound morphemes
    LS::Vocabulary v = baseVocabulary();
    v.words.push_back({"greater than", "op.Gt", "lex_gt", "law_gt"});
    v.words.push_back({"asdfgh", "action.Set", "lex_smash", "law_set"});
    v.words.push_back({"un-", "condition.Not", "lex_un", "law_not"});
    v.words.push_back({"-ly", "scope.Everyone", "lex_ly", "law_everyone"});
    {
        const auto p = LS::parse("on tick if hp   greater   than 2 then asdfgh glow 1", v);
        assert(p.ok);
        assert(p.condition->op == ConditionNode::Op::Gt);
        assert(p.action->kind == ActionNode::Kind::Set && p.action->path.toString() == "glow");
    }
    {
        // "unvisible" = un- + visible: the prefix is its own Lexeme.
        const auto p = LS::parse("on tick if unvisible = true then set glow 1", v);
        assert(p.ok);
        assert(p.condition->kind == ConditionNode::Kind::Not);
        assert(p.condition->children[0].path.toString() == "visible");
    }
    {
        // "tickly" = tick + -ly: the suffix is its own Lexeme.
        const auto p = LS::parse("on tickly then set glow 1", v);
        assert(p.ok);
        assert(p.triggers == std::vector<std::string>{"tick"});
        assert(p.scope == Law::Scope::Everyone);
    }

    // --- presets: a denoted Law with no open slot fixes clauses
    Law eventPreset("my event-triggered law");
    eventPreset.setLawIdentifier("law-line-preset-event");
    LS::Preset fixedEvent;
    assert(LS::classify(eventPreset, {}, fixedEvent) == "preset");
    Law noCondition("my law with no condition");
    noCondition.setLawIdentifier("law-line-preset-unconditioned");
    noCondition.setConditionModel(ConditionNode::all({}));
    LS::Preset fixedNone;
    assert(LS::classify(noCondition, {}, fixedNone) == "preset");
    Law setOpcode("set");
    setOpcode.setActionModel(ActionNode::set("", PropertyValue{}));
    LS::Preset unused;
    assert(LS::classify(setOpcode, {}, unused) == "action.Set");
    Law gtOpcode("greater than");
    gtOpcode.setConditionModel(ConditionNode::compare("", ConditionNode::Op::Gt, PropertyValue{}));
    assert(LS::classify(gtOpcode, {}, unused) == "op.Gt");

    v.presets = {fixedEvent, fixedNone};
    v.words.push_back({"my event-triggered law", "preset", "lex_evt", "law-line-preset-event"});
    v.words.push_back({"my law with no condition", "preset", "lex_nocond", "law-line-preset-unconditioned"});
    {
        const auto p = LS::parse("my event-triggered law called Red Light fires on object-clicked then set color 1 0 0", v);
        assert(p.ok);
        assert(p.name == "Red Light");
        assert(contains(p.presetLawIds, "law-line-preset-event"));
        assert(p.triggers == std::vector<std::string>{"object-clicked"});
    }
    {
        // The preset leaves only the event and the action open.
        const auto p = LS::parse("my event-triggered law?", v);
        assert(p.ok && p.previewOnly);
        assert(contains(p.openClauses, "on <event>"));
        assert(contains(p.openClauses, "then <action>"));
    }
    {
        const auto p = LS::parse("my law with no condition on tick if hp > 1 then set glow 1", v);
        assert(!p.ok && mentions(p.error, "no condition"));
    }

    // --- shared spellings: grammar position first, then the Metalaw seam
    v.words.push_back({"is", "op.Eq", "lex_is_a", "law_eq"});
    v.words.push_back({"is", "op.Ne", "lex_is_b", "law_ne"});
    v.words.push_back({"is", "condition.IsKind", "lex_is_c", "law_iskind"});
    {
        // At a condition's start only IsKind is admissible: no ambiguity.
        const auto p = LS::parse("on tick if is Person then set glow 1", v);
        assert(p.ok && p.condition->kind == ConditionNode::Kind::IsKind);
    }
    {
        // After a path, two operator meanings remain and nobody resolves them.
        const auto p = LS::parse("on tick if hp is 3 then set glow 1", v);
        assert(!p.ok);
        assert(mentions(p.error, "no Metalaw resolves"));
        assert(p.candidates.size() == 2);
    }
    {
        LS::Vocabulary resolved = v;
        resolved.resolve = [](const LS::Ambiguity& a) {
            assert(a.symbol == "is" && a.slot == "operator");
            return LS::Resolution{"lex_is_b->law_ne", "resolved by Metalaw test"};
        };
        const auto p = LS::parse("on tick if hp is 3 then set glow 1", resolved);
        assert(p.ok);
        assert(p.condition->op == ConditionNode::Op::Ne);
        assert(!p.notes.empty() && mentions(p.notes.front(), "Metalaw"));
    }

    // --- refusals that name themselves
    {
        const auto p = LS::parse("on tick then Map glow", v);
        assert(!p.ok && mentions(p.error, "Law Graph"));
        const auto t = LS::parse("on timeline dawn then set glow 1", v);
        assert(!t.ok && mentions(t.error, "Timeline"));
        const auto b = LS::parse("on tick then set glow @lamp.brightness", v);
        assert(!b.ok && mentions(b.error, "binding"));
        const auto u = LS::parse("on tick then frobnicate glow", v);
        assert(!u.ok && u.errorOffset == std::string("on tick then ").size());
        const auto open = LS::parse("then set glow 1", v);
        assert(!open.ok && mentions(open.error, "on <event>"));
    }

    // --- a Law never listens for an event nobody publishes (Zach, 2026-09-25:
    //     "fires when" made a Law waiting for an event called "when")
    {
        const auto clause = LS::parse("my law called Blue fires when then set glow 1", v);
        assert(!clause.ok && mentions(clause.error, "clause word"));
        const auto unknown = LS::parse("on door-opened then set glow 1", v);
        assert(!unknown.ok && mentions(unknown.error, "not an event this world knows"));
        const auto near = LS::parse("on object then set glow 1", v);
        assert(!near.ok && contains(near.candidates, "object-clicked"));
        const auto minted = LS::parse("on \"door-opened\" then set glow 1", v);
        assert(minted.ok && minted.triggers == std::vector<std::string>{"door-opened"});
        const auto publish = LS::parse("on tick then publish door-opened", v);
        assert(publish.ok);   // publishing may always name a new event
    }

    // --- Tab
    {
        const auto c = LS::complete("on object-clicked then se", v);
        assert(contains(c, "set"));
        const auto phrase = LS::complete("on tick if hp greater th", v);
        assert(contains(phrase, "than"));
        const auto preset = LS::complete("my ev", v);
        assert(contains(preset, "event-triggered law"));
        const auto ev = LS::complete("on obj", v);
        assert(contains(ev, "object-clicked"));
        const auto path = LS::complete("on tick then set @cube.gl", v);
        assert(contains(path, "@cube.glow"));
        const auto scoped = LS::complete("on tick then set gl", v);
        assert(contains(scoped, "glow"));
        const auto glued = LS::complete("on tick if hp>", v);
        assert(contains(glued, "hp>="));
        const auto next = LS::complete("on tick ", v);
        assert(contains(next, "then") && contains(next, "if") && contains(next, "or"));
        const auto hits = LS::search("glow", v);
        assert(!hits.empty());
    }
}

void channel() {
    LawManager laws;
    laws.connectToEventBus();
    TerminalChannel::syncRegister(laws);
    TerminalChannel* terminal = TerminalChannel::find(laws);
    assert(terminal);
    std::vector<std::string> printed;
    terminal->setSink([&](const std::string& s) { printed.push_back(s); });

    Object zach;
    zach.setObjectID("zach");
    zach.setDynamicProperty("who", PropertyValue(std::string("zach")));
    Object cube;
    cube.setObjectID("cube");
    cube.setDynamicProperty("hp", PropertyValue(3.0));
    cube.setDynamicProperty("glow", PropertyValue(0.0));

    // Lexeme <--denotes--> Law.
    Singularity::Language::Lexeme gtWord("greater than", "lex_gt");
    auto gtLaw = std::make_shared<Law>("greater than", std::vector<Singular*>{&zach});
    gtLaw->setLawIdentifier("law-line-op-gt");
    gtLaw->setEnabled(false);
    gtLaw->setConditionModel(ConditionNode::compare("", ConditionNode::Op::Gt, PropertyValue{}));
    laws.add(gtLaw);
    Singularity::Language::Lexeme isEq("is", "lex_is_eq");
    Singularity::Language::Lexeme isNe("is", "lex_is_ne");
    auto eqLaw = std::make_shared<Law>("equals", std::vector<Singular*>{&zach});
    eqLaw->setLawIdentifier("law-line-op-eq");
    eqLaw->setEnabled(false);
    eqLaw->setConditionModel(ConditionNode::compare("", ConditionNode::Op::Eq, PropertyValue{}));
    laws.add(eqLaw);
    auto neLaw = std::make_shared<Law>("differs", std::vector<Singular*>{&zach});
    neLaw->setLawIdentifier("law-line-op-ne");
    neLaw->setEnabled(false);
    neLaw->setConditionModel(ConditionNode::compare("", ConditionNode::Op::Ne, PropertyValue{}));
    laws.add(neLaw);

    RelationManager graph;
    graph.add(std::make_shared<Relation>("denotes", gtWord, *gtLaw, true, 1.0f));
    graph.add(std::make_shared<Relation>("denotes", isEq, *eqLaw, true, 1.0f));
    graph.add(std::make_shared<Relation>("denotes", isNe, *neLaw, true, 1.0f));

    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.push_back(&zach);
        out.push_back(&cube);
        for (const auto& law : laws.getAll()) {
            if (law) out.push_back(law.get());
        }
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& r : graph.getAll()) {
            if (r) out.push_back(r.get());
        }
    });

    // The authored seed: whether a line is spoken is the world's decision.
    auto hear = std::make_shared<Law>("Law Line · hear", std::vector<Singular*>{&zach});
    hear->setLawIdentifier("law-line-hear");
    hear->setActionModel(ActionNode::publish("law-sentence-spoken"));
    laws.add(hear);
    laws.bindTrigger("law-line-hear", "terminal-line-entered");
    auto speak = std::make_shared<Law>("Law Line · speak", std::vector<Singular*>{&zach});
    speak->setLawIdentifier("law-line-speak");
    speak->setActionModel(ActionNode::add("@terminal-channel.speakRequests", 1.0));
    laws.add(speak);
    laws.bindTrigger("law-line-speak", "law-sentence-spoken");

    PropertyPath::parse("authorPath").setValue(*terminal, PropertyValue(std::string("@zach.who")));

    // The world has heard these before anyone speaks of them.
    Core::EventBus::instance().publish(ECA::Event{"object-clicked", &cube, nullptr, std::time(nullptr), ""});
    Core::EventBus::instance().publish(ECA::Event{"tick", &cube, nullptr, std::time(nullptr), ""});
    laws.tick();

    const auto frame = [&] {
        terminal->sense(laws);
        for (int i = 0; i < 3; ++i) laws.tick();
        terminal->act(laws);
    };

    // 1. A sentence becomes a Law, written by the Person.
    const std::size_t before = laws.getAll().size();
    terminal->inject("my law called Red fires on object-clicked if hp greater than 2 then set glow 1");
    frame();
    assert(laws.getAll().size() == before + 1);
    Law* red = laws.getAll().back().get();
    assert(red->name() == "Red");
    assert(red->getIdentifier().rfind("law_", 0) == 0);
    assert(red->authors().getMembers().size() == 1 && red->authors().getMembers().front() == &zach);
    assert(laws.triggersOf(red->getIdentifier()) == std::vector<std::string>{"object-clicked"});
    assert(!printed.empty() && mentions(printed.back(), "authored " + red->getIdentifier()));

    // ...and it is a real Law: the event fires it.
    Core::EventBus::instance().publish(ECA::Event{"object-clicked", &cube, nullptr, std::time(nullptr), ""});
    laws.tick();
    assert(number(cube, "glow") == 1.0);

    // 2. Shared display names are legitimate; identity is the minted id.
    terminal->inject("my law called Red fires on tick then set glow 2");
    frame();
    Law* red2 = laws.getAll().back().get();
    assert(red2 != red && red2->name() == "Red" && red2->getIdentifier() != red->getIdentifier());

    // 3. A shared spelling with no resolving Metalaw is refused, and says so.
    const std::size_t beforeAmbiguity = laws.getAll().size();
    terminal->inject("on tick if hp is 3 then set glow 3");
    frame();
    assert(laws.getAll().size() == beforeAmbiguity);
    assert(mentions(printed.back(), "no Metalaw resolves"));

    // ...a Metalaw targeting the terminal decides which meaning is used.
    auto resolver = std::make_shared<Law>("'is' means equals", std::vector<Singular*>{&zach});
    resolver->setLawIdentifier("law-line-metalaw-is");
    resolver->setActivation(Law::Activation::WhileTrue);
    resolver->setConditionModel(ConditionNode::compare("ambiguity.symbol", ConditionNode::Op::Eq,
                                                       PropertyValue(std::string("is"))));
    resolver->setActionModel(ActionNode::set("ambiguity.resolved",
                                             PropertyValue(std::string("lex_is_eq->law-line-op-eq"))));
    resolver->addTarget(*terminal);
    laws.add(resolver);
    terminal->inject("on tick if hp is 3 then set glow 3");
    frame();
    Law* resolved = laws.getAll().back().get();
    assert(resolved->hasConditionModel() && resolved->conditionModel()->op == ConditionNode::Op::Eq);
    assert(mentions(printed.back(), "law-line-metalaw-is"));

    // 4. Nothing enters the world without an author.
    PropertyPath::parse("authorPath").setValue(*terminal, PropertyValue(std::string("@nobody.who")));
    const std::size_t beforeUnauthored = laws.getAll().size();
    terminal->inject("on tick then set glow 4");
    frame();
    assert(laws.getAll().size() == beforeUnauthored);
    assert(mentions(printed.back(), "no author"));

    // 5. A line nobody authored a Law to hear is answered, not swallowed.
    laws.remove("law-line-hear");
    const std::size_t printedBefore = printed.size();
    terminal->inject("on tick then set glow 5");
    frame();
    assert(printed.size() == printedBefore + 1 && mentions(printed.back(), "no Law in this Zone hears"));
}

} // namespace

int main() {
    grammar();
    channel();
    std::cout << "law_line_test: OK\n";
    return 0;
}
