#include "Singularity/Storage/Serialization/BinarySerializer.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>

using namespace Earthcall::Storage;

void printUsage() {
    std::cout << "Usage: earthcall-fmt <command> <input> [output]\n";
    std::cout << "Commands:\n";
    std::cout << "  decompile <input.ecform>      - Dumps the binary graph to text (stdout)\n";
    std::cout << "  inspect <input.ecform> --find <name> - Finds an entity by name\n";
    std::cout << "  compile <input.eclang> <out.ecform> - Compiles a text graph into binary ecform\n";
}

std::string getLex(const EcformGraph& g, uint32_t id) {
    for (const auto& l : g.lexemes) {
        if (l.id == id) return l.symbol;
    }
    return "";
}

int decompileGraph(const std::string& input) {
    EcformGraph graph;
    if (!BinarySerializer::readBinary(input, graph)) {
        std::cerr << "Failed to read binary file: " << input << "\n";
        return 1;
    }
    
    std::cout << "# Earthcall Ecform Graph v2 (.eclang)\n";
    std::cout << "# Decompiled from " << input << "\n\n";
    
    for (const auto& sig : graph.singulars) {
        std::cout << "singular " << std::quoted(sig.entityId) << " " 
                  << std::quoted(getLex(graph, sig.kindLexemeId)) << " " 
                  << std::quoted(getLex(graph, sig.ownerLexemeId)) << " " 
                  << std::quoted(getLex(graph, sig.zoneLexemeId)) << " " 
                  << sig.flags << "\n";
    }
    
    std::cout << "\n";
    for (const auto& rel : graph.relations) {
        std::cout << "relation " << std::quoted(rel.relationId) << " "
                  << std::quoted(rel.fromEntityId) << " "
                  << std::quoted(rel.toEntityId) << " "
                  << std::quoted(getLex(graph, rel.typeLexemeId)) << " "
                  << rel.weight << " " << rel.timestamp << "\n";
    }

    std::cout << "\n";
    for (const auto& form : graph.formations) {
        std::cout << "formation " << std::quoted(form.rootEntityId);
        for (const auto& mem : form.memberIds) {
            std::cout << " " << std::quoted(mem);
        }
        std::cout << "\n";
    }
    
    std::cout << "\n";
    for (const auto& props : graph.properties) {
        for (const auto& pv : props.properties) {
            std::cout << "property_";
            if (pv.typeTag == 0) {
                std::cout << "int " << std::quoted(props.entityId) << " " << std::quoted(getLex(graph, pv.nameLexemeId)) << " " << pv.intVal << "\n";
            } else if (pv.typeTag == 1) {
                std::cout << "float " << std::quoted(props.entityId) << " " << std::quoted(getLex(graph, pv.nameLexemeId)) << " " << pv.floatVal << "\n";
            } else if (pv.typeTag == 2) {
                std::cout << "str " << std::quoted(props.entityId) << " " << std::quoted(getLex(graph, pv.nameLexemeId)) << " " << std::quoted(getLex(graph, pv.strLexemeId)) << "\n";
            } else if (pv.typeTag == 3) {
                std::cout << "ptr " << std::quoted(props.entityId) << " " << std::quoted(getLex(graph, pv.nameLexemeId)) << " " << std::quoted(pv.ptrEntityId) << "\n";
            }
        }
    }
    
    return 0;
}

int compileGraph(const std::string& input, const std::string& output) {
    std::ifstream file(input);
    if (!file.is_open()) {
        std::cerr << "Failed to open input file: " << input << "\n";
        return 1;
    }
    
    EcformGraph graph;
    std::string line;
    int lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') continue;
        
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;
        
        if (cmd == "singular") {
            EcformGraph::SingularRecord sig;
            std::string kindLex, ownerLex, zoneLex;
            if (!(ss >> std::quoted(sig.entityId) >> std::quoted(kindLex) >> std::quoted(ownerLex) >> std::quoted(zoneLex) >> sig.flags)) {
                std::cerr << "Error parsing singular at line " << lineNum << "\n";
                continue;
            }
            sig.kindLexemeId = graph.getOrAddLexeme(kindLex);
            sig.ownerLexemeId = graph.getOrAddLexeme(ownerLex);
            sig.zoneLexemeId = graph.getOrAddLexeme(zoneLex);
            graph.singulars.push_back(sig);
        } else if (cmd == "relation") {
            EcformGraph::RelationRecord rel;
            std::string typeLex;
            if (!(ss >> std::quoted(rel.relationId) >> std::quoted(rel.fromEntityId) >> std::quoted(rel.toEntityId) >> std::quoted(typeLex) >> rel.weight >> rel.timestamp)) {
                std::cerr << "Error parsing relation at line " << lineNum << "\n";
                continue;
            }
            rel.typeLexemeId = graph.getOrAddLexeme(typeLex);
            graph.relations.push_back(rel);
        } else if (cmd == "formation") {
            EcformGraph::FormationRecord form;
            if (!(ss >> std::quoted(form.rootEntityId))) {
                std::cerr << "Error parsing formation at line " << lineNum << "\n";
                continue;
            }
            std::string member;
            while (ss >> std::quoted(member)) {
                form.memberIds.push_back(member);
            }
            graph.formations.push_back(form);
        } else if (cmd == "property_int" || cmd == "property_float" || cmd == "property_str" || cmd == "property_ptr") {
            std::string entityId, nameLex;
            if (!(ss >> std::quoted(entityId) >> std::quoted(nameLex))) {
                std::cerr << "Error parsing property at line " << lineNum << "\n";
                continue;
            }
            
            // Find or create property stream
            EcformGraph::PropertyStreamRecord* stream = nullptr;
            for (auto& p : graph.properties) {
                if (p.entityId == entityId) {
                    stream = &p;
                    break;
                }
            }
            if (!stream) {
                graph.properties.push_back({entityId, {}});
                stream = &graph.properties.back();
            }
            
            EcformGraph::PropertyVariant pv;
            pv.nameLexemeId = graph.getOrAddLexeme(nameLex);
            
            if (cmd == "property_int") {
                pv.typeTag = 0;
                ss >> pv.intVal;
            } else if (cmd == "property_float") {
                pv.typeTag = 1;
                ss >> pv.floatVal;
            } else if (cmd == "property_str") {
                pv.typeTag = 2;
                std::string strVal;
                ss >> std::quoted(strVal);
                pv.strLexemeId = graph.getOrAddLexeme(strVal);
            } else if (cmd == "property_ptr") {
                pv.typeTag = 3;
                ss >> std::quoted(pv.ptrEntityId);
            }
            stream->properties.push_back(pv);
        }
    }
    
    if (!BinarySerializer::writeBinary(graph, output)) {
        std::cerr << "Failed to write binary file: " << output << "\n";
        return 1;
    }
    
    std::cout << "Compiled " << input << " to " << output << "\n";
    std::cout << "Lexemes: " << graph.lexemes.size() << "\n";
    std::cout << "Singulars: " << graph.singulars.size() << "\n";
    std::cout << "Relations: " << graph.relations.size() << "\n";
    
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];
    std::string input = argv[2];

    if (cmd == "decompile") {
        return decompileGraph(input);
    } else if (cmd == "compile") {
        if (argc < 4) {
            std::cerr << "Output file required for compile.\n";
            printUsage();
            return 1;
        }
        std::string output = argv[3];
        return compileGraph(input, output);
    } else if (cmd == "inspect") {
        if (argc < 5 || std::string(argv[3]) != "--find") {
            printUsage();
            return 1;
        }
        std::string target = argv[4];
        EcformGraph graph;
        if (!BinarySerializer::readBinary(input, graph)) {
            std::cerr << "Failed to read binary file: " << input << "\n";
            return 1;
        }
        
        for (const auto& sig : graph.singulars) {
            if (sig.entityId.find(target) != std::string::npos) {
                std::cout << "Found: " << sig.entityId << "\n";
            }
        }
        return 0;
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        printUsage();
        return 1;
    }
}
