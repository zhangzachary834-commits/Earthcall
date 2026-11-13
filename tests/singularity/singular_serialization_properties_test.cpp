#include "Singularity/Storage/Serialization/Relation/RelationSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include <iostream>
#include <cassert>

using namespace Singularity;
using namespace Earthcall;

int main() {
    Language::Lexeme lexeme("test.lexeme");
    lexeme.setDynamicProperty("mtg.turnPhase", std::string("combat"));

    Zone zone("TestZone", "foundation");
    zone.addToFormation(&lexeme);

    nlohmann::json zj = zoneToJson(zone);
    
    assert(zj.contains("lexemes"));
    bool foundLexeme = false;
    for (const auto& lex : zj["lexemes"]) {
        if (lex["symbol"] == "test.lexeme") {
            foundLexeme = true;
            assert(lex.contains("authoredProperties"));
            assert(lex["authoredProperties"]["mtg.turnPhase"]["v"].get<std::string>() == "combat");
        }
    }
    assert(foundLexeme);

    Relation rel("tracks-state", "Zone-1", "Lexeme-1");
    rel.setDynamicProperty("stackCounter", 42);

    nlohmann::json rj = relationToJson(rel);
    assert(rj.contains("authoredProperties"));
    assert(rj["authoredProperties"]["stackCounter"]["v"].get<int>() == 42);

    auto resolver = [](const std::string&) -> Singular* { return nullptr; };
    Relation rehydratedRel = relationFromJson(rj, resolver);
    
    bool hasProp = false;
    for (const auto& entry : rehydratedRel.dynamicProperties()) {
        if (Earthcall::StringInterner::resolve(entry.first) == "stackCounter") {
            hasProp = true;
            assert(std::get<int>(entry.second) == 42);
        }
    }
    assert(hasProp);

    std::cout << "singular_serialization_properties_test passed!" << std::endl;
    return 0;
}
