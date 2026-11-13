#include "Singularity/Storage/Serialization/BinarySerializer.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cmath>

using namespace Earthcall::Storage;

int main() {
    std::cout << "=== Running BinarySerializer Tests ===" << std::endl;

    EcformGraph graph;

    // Setup lexemes
    graph.lexemes.push_back({1, "kind.object"});
    graph.lexemes.push_back({2, "prop.position"});
    graph.lexemeMap["kind.object"] = 1;
    graph.lexemeMap["prop.position"] = 2;

    // Setup singulars
    EcformGraph::SingularRecord sig;
    sig.entityId = "obj-1";
    sig.kindLexemeId = 1;
    sig.ownerLexemeId = 0;
    sig.zoneLexemeId = 0;
    sig.flags = 0;
    graph.singulars.push_back(sig);

    // Setup relations
    EcformGraph::RelationRecord rel;
    rel.relationId = "rel-1";
    rel.fromEntityId = "obj-1";
    rel.toEntityId = "obj-2";
    rel.typeLexemeId = 3;
    rel.weight = 1.0f;
    rel.timestamp = 12345;
    graph.relations.push_back(rel);

    // Setup formations
    EcformGraph::FormationRecord form;
    form.rootEntityId = "obj-1";
    form.memberIds.push_back("obj-2");
    graph.formations.push_back(form);

    // Setup properties
    EcformGraph::PropertyStreamRecord propStream;
    propStream.entityId = "obj-1";

    EcformGraph::PropertyVariant pVar;
    pVar.nameLexemeId = 2;
    pVar.typeTag = 'F';
    pVar.floatVal = 12.34;
    pVar.intVal = 0;
    pVar.strLexemeId = 0;
    propStream.properties.push_back(pVar);

    graph.properties.push_back(propStream);

    // Write out buffer
    std::vector<uint8_t> buffer = BinarySerializer::writeBinaryToBuffer(graph);
    assert(buffer.size() > 0);

    // Save to temp file
    std::string testPath = "test_binary_serializer.bin";
    {
        std::ofstream out(testPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }

    EcformGraph inGraph;
    bool readSuccess = BinarySerializer::readBinary(testPath, inGraph);
    assert(readSuccess);

    // Assert lexemes
    assert(inGraph.lexemes.size() == 2);
    assert(inGraph.lexemes[0].symbol == "kind.object");
    assert(inGraph.lexemes[0].id == 1);
    assert(inGraph.lexemes[1].symbol == "prop.position");
    assert(inGraph.lexemes[1].id == 2);

    // Assert singulars
    assert(inGraph.singulars.size() == 1);
    assert(inGraph.singulars[0].entityId == "obj-1");
    assert(inGraph.singulars[0].kindLexemeId == 1);

    // Assert relations
    assert(inGraph.relations.size() == 1);
    assert(inGraph.relations[0].relationId == "rel-1");
    assert(inGraph.relations[0].fromEntityId == "obj-1");
    assert(inGraph.relations[0].toEntityId == "obj-2");
    assert(std::abs(inGraph.relations[0].weight - 1.0f) < 0.001f);
    assert(inGraph.relations[0].timestamp == 12345);

    // Assert formations
    assert(inGraph.formations.size() == 1);
    assert(inGraph.formations[0].rootEntityId == "obj-1");
    assert(inGraph.formations[0].memberIds.size() == 1);
    assert(inGraph.formations[0].memberIds[0] == "obj-2");

    // Assert properties
    assert(inGraph.properties.size() == 1);
    assert(inGraph.properties[0].entityId == "obj-1");
    assert(inGraph.properties[0].properties.size() == 1);
    assert(inGraph.properties[0].properties[0].nameLexemeId == 2);
    assert(inGraph.properties[0].properties[0].typeTag == 'F');
    assert(std::abs(inGraph.properties[0].properties[0].floatVal - 12.34) < 0.001);

    std::filesystem::remove(testPath);

    std::cout << "BinarySerializer test OK\n";
    return 0;
}
