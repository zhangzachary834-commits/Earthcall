#include "Singularity/Storage/MigrationFramework.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace Earthcall::Storage;

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

const EcformGraph::PropertyVariant* findProperty(
    const EcformGraph& graph,
    const EcformGraph::PropertyStreamRecord& stream,
    const std::string& name) {
    for (const auto& property : stream.properties) {
        if (property.nameLexemeId < graph.lexemes.size() &&
            graph.lexemes[property.nameLexemeId].symbol == name) {
            return &property;
        }
    }
    return nullptr;
}
} // namespace

int main() {
    const nlohmann::json legacySave = {
        {"objects", {{{"id", "obj1"}, {"type", "testType"}, {"ownerId", "owner1"},
                      {"zoneId", "zone1"}, {"authoredProperties", {
                          {"propInt", 42}, {"propFloat", 3.14}, {"propString", "testString"}}}}}},
        {"semanticRoots", {{"relations", {{{"id", "rel1"}, {"from", "obj1"}, {"to", "obj2"},
                                           {"type", "testRelation"}, {"weight", 2.0},
                                           {"timestamp", 12345}}}}}}
    };

    const EcformGraph graph = MigrationFramework::migrateJsonToGraph(legacySave);
    check(graph.singulars.size() == 1, "one Singular migrates");
    check(graph.singulars[0].entityId == "obj1", "Singular identity is preserved");
    check(graph.lexemes.at(graph.singulars[0].kindLexemeId).symbol == "testType", "Singular kind is interned");
    check(graph.lexemes.at(graph.singulars[0].ownerLexemeId).symbol == "owner1", "owner identity is interned");
    check(graph.lexemes.at(graph.singulars[0].zoneLexemeId).symbol == "zone1", "zone identity is interned");

    check(graph.properties.size() == 1, "one property stream migrates");
    check(graph.properties[0].entityId == "obj1", "property stream stays attached to its entity");
    check(graph.properties[0].properties.size() == 3, "all authored properties migrate");
    const auto* intProperty = findProperty(graph, graph.properties[0], "propInt");
    const auto* floatProperty = findProperty(graph, graph.properties[0], "propFloat");
    const auto* stringProperty = findProperty(graph, graph.properties[0], "propString");
    check(intProperty && intProperty->typeTag == 0 && intProperty->intVal == 42, "integer property value survives");
    check(floatProperty && floatProperty->typeTag == 1 && std::abs(floatProperty->floatVal - 3.14) < 1e-12,
          "floating property value survives");
    check(stringProperty && stringProperty->typeTag == 2 &&
              graph.lexemes.at(stringProperty->strLexemeId).symbol == "testString", "string property value survives");

    check(graph.relations.size() == 1, "one Relation migrates");
    check(graph.relations[0].relationId == "rel1", "Relation identity is preserved");
    check(graph.relations[0].fromEntityId == "obj1" && graph.relations[0].toEntityId == "obj2",
          "Relation endpoints are preserved");
    check(graph.lexemes.at(graph.relations[0].typeLexemeId).symbol == "testRelation", "Relation type is interned");
    check(graph.relations[0].weight == 2.0f && graph.relations[0].timestamp == 12345u,
          "Relation payload is preserved");

    const nlohmann::json output = MigrationFramework::migrateGraphToJson(graph);
    check(output.at("objects").size() == 1, "graph reconstitutes one object");
    check(output.at("objects")[0].at("id") == "obj1", "output object identity is preserved");
    check(output.at("objects")[0].at("type") == "testType", "output object type is preserved");
    check(output.at("objects")[0].at("ownerId") == "owner1", "output owner is preserved");
    check(output.at("objects")[0].at("authoredProperties").at("propInt") == 42, "output integer property is preserved");
    check(output.at("objects")[0].at("authoredProperties").at("propString") == "testString",
          "output string property is preserved");
    check(output.at("semanticRoots").at("relations")[0].at("id") == "rel1", "output Relation identity is preserved");

    check(MigrationFramework::migrateLegacySave(legacySave) == legacySave,
          "legacy migration hook is lossless while graph persistence is staged");
    std::cout << "migration_framework_test: ALL OK\n";
    return 0;
}
