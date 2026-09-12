// OpenCode (GPT-6 Astra), session language-depth-20260910-115436.
// 2026-09-10 11:59 PDT. Diagnostic observations, not desired-behavior tests.
// Zach requested a deep Language-branch analysis without recycled proposals.
// Synthetic in-memory fixtures only; run via run_language_meaning_probe.py.
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Language/SyntacticParser.hpp"
#include "Singularity/Language/Utterance.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

extern ZoneManager mgr;
using namespace Singularity::Language;

namespace {
int observations = 0;
void observed(bool result, const char* description) {
    std::cout << (result ? "OBSERVED: " : "NOT REPRODUCED: ") << description << '\n';
    if (!result) throw std::runtime_error(description);
    ++observations;
}
}

int main() {
    auto& language = LanguageSystem::instance();
    language.clear();
    mgr.zones().clear();
    auto a = std::make_shared<Zone>("probe.language.a", "");
    auto b = std::make_shared<Zone>("probe.language.b", "");
    mgr.addZone(a);
    mgr.addZone(b);

    auto first = language.intern("bank", "probe.bank.river");
    a->addToFormation(first.get());
    auto second = language.intern("bank", "probe.bank.finance");
    b->addToFormation(second.get());
    observed(language.findById(first->getIdentifier()) == first &&
             language.findById(second->getIdentifier()) == second &&
             language.resolve("bank") == second,
             "two stable bank identities coexist; unscoped resolution chooses last interned");

    auto* symbol = second->findProperty("symbol");
    if (!symbol || !symbol->setValue(std::string("lender"))) return 2;
    observed(language.findBySymbol("bank") == second &&
             second->getSymbol() == "lender" && !language.findBySymbol("lender"),
             "a registered symbol write leaves the old lookup key and omits the new key");

    auto typeA = language.intern("owns", "probe.owns.a");
    auto typeB = language.intern("owns", "probe.owns.b");
    auto left = language.intern("left", "probe.left");
    auto right = language.intern("right", "probe.right");
    auto relA = std::make_shared<Relation>(*typeA, *left, *right, true, 0.5f);
    auto relB = std::make_shared<Relation>(*typeB, *left, *right, true, 0.5f);
    RelationManager distinctTypes;
    distinctTypes.add(relA);
    distinctTypes.add(relB);
    observed(relA->getTypeLexeme() != relB->getTypeLexeme() &&
             relA->getIdentifier() == relB->getIdentifier() &&
             distinctTypes.getAll().size() == 1,
             "distinct type Lexemes with the same spelling merge in RelationManager");

    auto noun = language.intern("noun", "probe.pos.noun");
    auto verb = language.intern("verb", "probe.pos.verb");
    auto arthur = language.intern("arthur", "probe.arthur");
    auto sword = language.intern("sword", "probe.sword");
    for (const auto& lex : {noun, verb, arthur, sword, typeB}) {
        a->addToFormation(lex.get());
    }
    a->formation().addRelation(std::make_shared<Relation>("is_pos", *arthur, *noun, true));
    a->formation().addRelation(std::make_shared<Relation>("is_pos", *sword, *noun, true));
    a->formation().addRelation(std::make_shared<Relation>("is_pos", *typeB, *verb, true));

    auto plain = SyntacticParser::parse("Arthur owns sword", *a);
    auto quoted = SyntacticParser::parse("\"Arthur owns sword\"", *a);
    observed(plain.size() == 1 && quoted.size() == 1 &&
             plain[0]->getIdentifier() == quoted[0]->getIdentifier(),
             "quotation and unquoted assertion produce the identical parsed Relation");
    observed(plain[0]->getTypeLexeme() == nullptr,
             "the parser emits a string-typed Relation despite Lexeme-typed support");

    language.queueUtterance("Arthur owns sword", "probe.source.one", arthur->getIdentifier());
    language.tick(0.0f);
    language.queueUtterance("Arthur owns sword", "probe.source.two", arthur->getIdentifier());
    language.tick(0.0f);
    int owns = 0, speaks = 0, occurrences = 0;
    float weight = 0.0f;
    for (const auto& relation : a->formation().relations().getAll()) {
        if (relation->type == "owns") { ++owns; weight = relation->getWeight(); }
        if (relation->type == "speaks") ++speaks;
        if (relation->type == "occurrence-of") ++occurrences;
    }
    observed(owns == 1 && std::abs(weight - 0.7f) < 0.0001f,
             "repeated text from two sources reinforces one edge from 0.5 to 0.7");
    observed(speaks == 0 && occurrences == 0,
             "successfully parsed targeted input creates neither speaks nor occurrence-of");

    language.queueUtterance("arrival_marker", "probe.source.one");
    mgr.switchTo(1);
    language.tick(0.0f);
    auto marker = language.findBySymbol("arrival_marker");
    observed(marker && !a->formation().hasMember(marker.get()) &&
             b->formation().hasMember(marker.get()),
             "input queued while A is active enters B if B is active at consumption");

    arthur->setConceptualWeight(0.25f);
    arthur->setDynamicProperty("probe.annotation", std::string("keep me"));
    const auto serialized = zoneToJson(*a);
    bool foundBare = false;
    for (const auto& lex : serialized.at("lexemes")) {
        if (lex.at("id") == arthur->getIdentifier()) {
            foundBare = lex.size() == 2 && lex.contains("symbol");
        }
    }
    observed(foundBare,
             "Zone Lexeme JSON emits only id/symbol, omitting weight and authored annotation");

    Utterance detached("echo echo", "probe.occurrence", 100.0);
    detached.addLexeme(arthur.get());
    detached.addLexeme(arthur.get());
    auto duplicateOccurrences = detached.createOccurrenceRelations();
    observed(duplicateOccurrences.size() == 2 &&
             duplicateOccurrences[0]->getIdentifier() == duplicateOccurrences[1]->getIdentifier(),
             "the detached occurrence helper gives repeated references identical relation ids");

    std::cout << "SUMMARY: " << observations << " current-behavior observations reproduced.\n";
    language.clear();
    mgr.zones().clear();
}
