#include "Singularity/Storage/Serialization/BinarySerializer.hpp"
#include <iostream>
#include <string>

using namespace Earthcall::Storage;

void printUsage() {
    std::cout << "Usage: earthcall-fmt <command> <input> [output]\n";
    std::cout << "Commands:\n";
    std::cout << "  decompile <input.ecform>      - Dumps the binary graph to text (stdout)\n";
    std::cout << "  inspect <input.ecform> --find <name> - Finds an entity by name\n";
    std::cout << "  compile <input.eclang> <out.ecform> - Not implemented in Phase 4 stub\n";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];
    std::string input = argv[2];

    if (cmd == "decompile") {
        EcformGraph graph;
        if (!BinarySerializer::readBinary(input, graph)) {
            std::cerr << "Failed to read binary file: " << input << "\n";
            return 1;
        }
        std::cout << "Ecform Graph v2\n";
        std::cout << "Lexemes: " << graph.lexemes.size() << "\n";
        for (const auto& lex : graph.lexemes) {
            std::cout << "  [" << lex.id << "] " << lex.symbol << "\n";
        }
        std::cout << "Singulars: " << graph.singulars.size() << "\n";
        for (const auto& sig : graph.singulars) {
            std::cout << "  " << sig.entityId << " (Kind: " << sig.kindLexemeId << ")\n";
        }
        return 0;
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
