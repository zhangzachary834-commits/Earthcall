#include "Singularity/Storage/Serialization/BinarySerializer.hpp"
#include <iostream>
#include <filesystem>

using namespace Earthcall;
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
    std::cout << "Running BinarySerializer Test\n";

    EcformGraph graph;

    // Add Lexemes
    graph.lexemes.push_back({1, "TestLexeme1"});
    graph.lexemes.push_back({2, "TestLexeme2"});
    graph.lexemeMap["TestLexeme1"] = 1;
    graph.lexemeMap["TestLexeme2"] = 2;

    // Add Singular
    EcformGraph::SingularRecord sig;
    sig.entityId = "TestEntity1";
    sig.kindLexemeId = 1;
    sig.ownerLexemeId = 2;
    sig.zoneLexemeId = 1;
    sig.flags = 0xFF;
    graph.singulars.push_back(sig);

    // Add Relation
    EcformGraph::RelationRecord rel;
    rel.relationId = "Rel1";
    rel.fromEntityId = "TestEntity1";
    rel.toEntityId = "TestEntity2";
    rel.typeLexemeId = 1;
    rel.weight = 1.5f;
    rel.timestamp = 12345;
    graph.relations.push_back(rel);

    // Add Formation
    EcformGraph::FormationRecord form;
    form.rootEntityId = "TestEntity1";
    form.memberIds.push_back("TestEntity2");
    form.memberIds.push_back("TestEntity3");
    graph.formations.push_back(form);

    // Add Properties
    EcformGraph::PropertyStreamRecord propStream;
    propStream.entityId = "TestEntity1";

    EcformGraph::PropertyVariant intProp;
    intProp.nameLexemeId = 1;
    intProp.typeTag = 0; // int
    intProp.intVal = -1234567890;

    EcformGraph::PropertyVariant floatProp;
    floatProp.nameLexemeId = 2;
    floatProp.typeTag = 1; // float
    floatProp.floatVal = 3.14159;

    propStream.properties.push_back(intProp);
    propStream.properties.push_back(floatProp);
    graph.properties.push_back(propStream);

    std::vector<uint8_t> buffer = BinarySerializer::writeBinaryToBuffer(graph);

    // Write and read file
    std::string tempFile = "binary_serializer_test_temp.ecform";
    check(BinarySerializer::writeBinary(graph, tempFile), "Write binary file");

    EcformGraph readGraph;
    check(BinarySerializer::readBinary(tempFile, readGraph), "Read binary file");
    std::filesystem::remove(tempFile);

    check(readGraph.lexemes.size() == 2, "Lexeme count");
    check(readGraph.lexemes[0].id == 1, "Lexeme 0 id");
    check(readGraph.lexemes[0].symbol == "TestLexeme1", "Lexeme 0 symbol");

    check(readGraph.singulars.size() == 1, "Singular count");
    check(readGraph.singulars[0].entityId == "TestEntity1", "Singular entityId");
    check(readGraph.singulars[0].kindLexemeId == 1, "Singular kindLexemeId");
    check(readGraph.singulars[0].flags == 0xFF, "Singular flags");

    check(readGraph.relations.size() == 1, "Relation count");
    check(readGraph.relations[0].relationId == "Rel1", "Relation relationId");
    check(readGraph.relations[0].weight == 1.5f, "Relation weight");

    check(readGraph.formations.size() == 1, "Formation count");
    check(readGraph.formations[0].rootEntityId == "TestEntity1", "Formation rootEntityId");
    check(readGraph.formations[0].memberIds.size() == 2, "Formation member count");
    check(readGraph.formations[0].memberIds[0] == "TestEntity2", "Formation member 0");

    check(readGraph.properties.size() == 1, "Property stream count");
    check(readGraph.properties[0].entityId == "TestEntity1", "Property stream entityId");
    check(readGraph.properties[0].properties.size() == 2, "Property count");
    check(readGraph.properties[0].properties[0].intVal == -1234567890, "Int property value");
    check(readGraph.properties[0].properties[1].floatVal == 3.14159, "Float property value");

    std::cout << "All assertions passed.\n";
    return 0;
}
