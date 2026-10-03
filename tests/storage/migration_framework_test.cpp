#include "Singularity/Storage/MigrationFramework.hpp"
#include <iostream>
#include <cassert>

using namespace Earthcall::Storage;

void check(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "  ok: " << message << "\n";
    } else {
        std::cout << "  FAILED: " << message << "\n";
        exit(1);
    }
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Migration Framework Test\n";
    std::cout << "============================================================\n";

    nlohmann::json legacySave = {
        {"objects", {
            {
                {"id", "obj1"},
                {"type", "testType"},
                {"ownerId", "owner1"},
                {"zoneId", "zone1"},
                {"authoredProperties", {
                    {"propInt", 42},
                    {"propFloat", 3.14},
                    {"propString", "testString"}
                }}
            }
        }},
        {"semanticRoots", {
            {"relations", {
                {
                    {"id", "rel1"},
                    {"from", "obj1"},
                    {"to", "obj2"},
                    {"type", "testRelation"},
                    {"weight", 2.0},
                    {"timestamp", 12345}
                }
            }}
        }}
    };

    EcformGraph graph = MigrationFramework::migrateJsonToGraph(legacySave);

    check(graph.singulars.size() == 1, "Singulars migrated");
    check(graph.singulars[0].entityId == "obj1", "Singular ID correct");
    check(graph.lexemes[graph.singulars[0].kindLexemeId].symbol == "testType", "Type correct");

    check(graph.properties.size() == 1, "Properties migrated");
    check(graph.properties[0].properties.size() == 3, "Properties count correct");

    check(graph.relations.size() == 1, "Relations migrated");
    check(graph.relations[0].fromEntityId == "obj1", "Relation from correct");
    check(graph.relations[0].toEntityId == "obj2", "Relation to correct");

    nlohmann::json outputSave = MigrationFramework::migrateGraphToJson(graph);

    check(outputSave.contains("objects"), "Output has objects");
    check(outputSave["objects"].size() == 1, "Output objects count correct");
    check(outputSave["objects"][0]["id"] == "obj1", "Output object id correct");

    check(outputSave.contains("semanticRoots"), "Output has semanticRoots");
    check(outputSave["semanticRoots"]["relations"].size() == 1, "Output relations count correct");

    nlohmann::json migratedSave = MigrationFramework::migrateLegacySave(legacySave);
    check(migratedSave == legacySave, "migrateLegacySave roundtrip works");

    std::cout << "------------------------------------------------------------\n";
    std::cout << "migration_framework_test: ALL OK\n";
    return 0;
}
