#include "Singularity/Storage/Serialization/BinarySerializer.hpp"
#include <iostream>
#include <filesystem>
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
    std::cout << "Running Lexeme-Relation Graph Binary Roundtrip Test\n";
    std::cout << "============================================================\n";

    EcformGraph graph;
    
    // Setup some lexemes
    uint32_t typePersonId = graph.getOrAddLexeme("kind.person");
    uint32_t nameLexemeId = graph.getOrAddLexeme("property.name");
    
    // Setup a Singular
    EcformGraph::SingularRecord sig;
    sig.entityId = "person-123";
    sig.kindLexemeId = typePersonId;
    sig.ownerLexemeId = 0;
    sig.zoneLexemeId = graph.getOrAddLexeme("zone.sanctum");
    sig.flags = 0;
    graph.singulars.push_back(sig);
    
    // Setup a Relation
    EcformGraph::RelationRecord rel;
    rel.relationId = "rel-1";
    rel.fromEntityId = "person-123";
    rel.toEntityId = "object-456";
    rel.typeLexemeId = graph.getOrAddLexeme("relation.holds");
    rel.weight = 1.0f;
    rel.timestamp = 1000;
    graph.relations.push_back(rel);
    
    // Setup a Property
    EcformGraph::PropertyVariant pVar;
    pVar.nameLexemeId = nameLexemeId;
    pVar.typeTag = 2; // string
    pVar.strLexemeId = graph.getOrAddLexeme("John Doe");
    
    EcformGraph::PropertyStreamRecord pStream;
    pStream.entityId = "person-123";
    pStream.properties.push_back(pVar);
    graph.properties.push_back(pStream);
    
    std::string testPath = "test_graph.ecform";
    bool writeOk = BinarySerializer::writeBinary(graph, testPath);
    check(writeOk, "BinarySerializer writes binary file successfully");
    
    EcformGraph loadedGraph;
    bool readOk = BinarySerializer::readBinary(testPath, loadedGraph);
    check(readOk, "BinarySerializer reads binary file successfully");
    
    check(loadedGraph.lexemes.size() == graph.lexemes.size(), "Lexemes count matches");
    check(loadedGraph.singulars.size() == 1, "Singulars count matches");
    check(loadedGraph.singulars[0].entityId == "person-123", "Entity ID matches");
    check(loadedGraph.relations.size() == 1, "Relations count matches");
    check(loadedGraph.properties.size() == 1, "Properties count matches");
    check(loadedGraph.properties[0].properties[0].strLexemeId == pVar.strLexemeId, "Property variant value matches");
    
    std::filesystem::remove(testPath);
    
    std::cout << "------------------------------------------------------------\n";
    std::cout << "All checks passed.\n";
    return 0;
}
