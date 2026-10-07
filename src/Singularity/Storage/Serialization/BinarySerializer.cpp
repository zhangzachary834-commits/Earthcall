#include "Singularity/Storage/Serialization/BinarySerializer.hpp"
#include <fstream>
#include <iostream>

namespace Earthcall {
namespace Storage {

void BinarySerializer::writeU32(std::vector<uint8_t>& buffer, uint32_t val) {
    buffer.push_back(val & 0xFF);
    buffer.push_back((val >> 8) & 0xFF);
    buffer.push_back((val >> 16) & 0xFF);
    buffer.push_back((val >> 24) & 0xFF);
}

void BinarySerializer::writeU16(std::vector<uint8_t>& buffer, uint16_t val) {
    buffer.push_back(val & 0xFF);
    buffer.push_back((val >> 8) & 0xFF);
}

void BinarySerializer::writeString(std::vector<uint8_t>& buffer, const std::string& str) {
    writeU16(buffer, static_cast<uint16_t>(str.size()));
    for (char c : str) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

uint32_t BinarySerializer::readU32(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 4 > buffer.size()) throw std::runtime_error("Buffer underflow");
    uint32_t val = static_cast<uint32_t>(buffer[offset]) |
                   (static_cast<uint32_t>(buffer[offset + 1]) << 8) |
                   (static_cast<uint32_t>(buffer[offset + 2]) << 16) |
                   (static_cast<uint32_t>(buffer[offset + 3]) << 24);
    offset += 4;
    return val;
}

uint16_t BinarySerializer::readU16(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 2 > buffer.size()) throw std::runtime_error("Buffer underflow");
    uint16_t val = static_cast<uint16_t>(buffer[offset]) |
                   (static_cast<uint16_t>(buffer[offset + 1]) << 8);
    offset += 2;
    return val;
}

std::string BinarySerializer::readString(const std::vector<uint8_t>& buffer, size_t& offset) {
    uint16_t len = readU16(buffer, offset);
    if (offset + len > buffer.size()) throw std::runtime_error("Buffer underflow");
    std::string str(buffer.begin() + offset, buffer.begin() + offset + len);
    offset += len;
    return str;
}

bool BinarySerializer::writeBinary(const EcformGraph& graph, const std::string& filepath) {
    std::vector<uint8_t> buffer;
    
    // Header
    writeU32(buffer, kEcformMagic);
    writeU32(buffer, kEcformVersion);
    
    // Lexeme Table
    writeU32(buffer, static_cast<uint32_t>(graph.lexemes.size()));
    for (const auto& lex : graph.lexemes) {
        writeU32(buffer, lex.id);
        writeString(buffer, lex.symbol);
    }
    
    // Singular Ledger
    writeU32(buffer, static_cast<uint32_t>(graph.singulars.size()));
    for (const auto& sig : graph.singulars) {
        writeString(buffer, sig.entityId);
        writeU32(buffer, sig.kindLexemeId);
        writeU32(buffer, sig.ownerLexemeId);
        writeU32(buffer, sig.zoneLexemeId);
        writeU32(buffer, sig.flags);
    }
    
    // Relation Edge Stream
    writeU32(buffer, static_cast<uint32_t>(graph.relations.size()));
    for (const auto& rel : graph.relations) {
        writeString(buffer, rel.relationId);
        writeString(buffer, rel.fromEntityId);
        writeString(buffer, rel.toEntityId);
        writeU32(buffer, rel.typeLexemeId);
        uint32_t weightBits;
        std::memcpy(&weightBits, &rel.weight, sizeof(float));
        writeU32(buffer, weightBits);
        writeU32(buffer, rel.timestamp);
    }
    
    // Formation Set
    writeU32(buffer, static_cast<uint32_t>(graph.formations.size()));
    for (const auto& form : graph.formations) {
        writeString(buffer, form.rootEntityId);
        writeU32(buffer, static_cast<uint32_t>(form.memberIds.size()));
        for (const auto& mid : form.memberIds) {
            writeString(buffer, mid);
        }
    }
    
    // Property Stream
    writeU32(buffer, static_cast<uint32_t>(graph.properties.size()));
    for (const auto& propStr : graph.properties) {
        writeString(buffer, propStr.entityId);
        writeU32(buffer, static_cast<uint32_t>(propStr.properties.size()));
        for (const auto& p : propStr.properties) {
            writeU32(buffer, p.nameLexemeId);
            buffer.push_back(p.typeTag);
            
            // IntVal
            uint32_t lower = p.intVal & 0xFFFFFFFF;
            uint32_t upper = (p.intVal >> 32) & 0xFFFFFFFF;
            writeU32(buffer, lower);
            writeU32(buffer, upper);
            
            // FloatVal
            uint64_t floatBits;
            std::memcpy(&floatBits, &p.floatVal, sizeof(double));
            writeU32(buffer, floatBits & 0xFFFFFFFF);
            writeU32(buffer, (floatBits >> 32) & 0xFFFFFFFF);
            
            writeU32(buffer, p.strLexemeId);
            writeString(buffer, p.ptrEntityId);
        }
    }
    
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

bool BinarySerializer::readBinary(const std::string& filepath, EcformGraph& outGraph) {
    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in) return false;
    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!in.read(reinterpret_cast<char*>(buffer.data()), size)) return false;
    
    size_t offset = 0;
    try {
        uint32_t magic = readU32(buffer, offset);
        if (magic != kEcformMagic) return false;
        
        uint32_t version = readU32(buffer, offset);
        if (version != kEcformVersion) return false;
        
        // Lexemes
        uint32_t numLexemes = readU32(buffer, offset);
        for (uint32_t i = 0; i < numLexemes; ++i) {
            EcformGraph::LexemeRecord lex;
            lex.id = readU32(buffer, offset);
            lex.symbol = readString(buffer, offset);
            outGraph.lexemes.push_back(lex);
            outGraph.lexemeMap[lex.symbol] = lex.id;
        }
        
        // Singulars
        uint32_t numSingulars = readU32(buffer, offset);
        for (uint32_t i = 0; i < numSingulars; ++i) {
            EcformGraph::SingularRecord sig;
            sig.entityId = readString(buffer, offset);
            sig.kindLexemeId = readU32(buffer, offset);
            sig.ownerLexemeId = readU32(buffer, offset);
            sig.zoneLexemeId = readU32(buffer, offset);
            sig.flags = readU32(buffer, offset);
            outGraph.singulars.push_back(sig);
        }
        
        // Relations
        uint32_t numRelations = readU32(buffer, offset);
        for (uint32_t i = 0; i < numRelations; ++i) {
            EcformGraph::RelationRecord rel;
            rel.relationId = readString(buffer, offset);
            rel.fromEntityId = readString(buffer, offset);
            rel.toEntityId = readString(buffer, offset);
            rel.typeLexemeId = readU32(buffer, offset);
            uint32_t weightBits = readU32(buffer, offset);
            std::memcpy(&rel.weight, &weightBits, sizeof(float));
            rel.timestamp = readU32(buffer, offset);
            outGraph.relations.push_back(rel);
        }
        
        // Formations
        uint32_t numFormations = readU32(buffer, offset);
        for (uint32_t i = 0; i < numFormations; ++i) {
            EcformGraph::FormationRecord form;
            form.rootEntityId = readString(buffer, offset);
            uint32_t numMembers = readU32(buffer, offset);
            for (uint32_t m = 0; m < numMembers; ++m) {
                form.memberIds.push_back(readString(buffer, offset));
            }
            outGraph.formations.push_back(form);
        }
        
        // Properties
        uint32_t numProps = readU32(buffer, offset);
        for (uint32_t i = 0; i < numProps; ++i) {
            EcformGraph::PropertyStreamRecord pStream;
            pStream.entityId = readString(buffer, offset);
            uint32_t numVars = readU32(buffer, offset);
            for (uint32_t v = 0; v < numVars; ++v) {
                EcformGraph::PropertyVariant var;
                var.nameLexemeId = readU32(buffer, offset);
                var.typeTag = buffer[offset++];
                
                uint32_t lower = readU32(buffer, offset);
                uint32_t upper = readU32(buffer, offset);
                var.intVal = (static_cast<uint64_t>(upper) << 32) | lower;
                
                uint32_t fLower = readU32(buffer, offset);
                uint32_t fUpper = readU32(buffer, offset);
                uint64_t floatBits = (static_cast<uint64_t>(fUpper) << 32) | fLower;
                std::memcpy(&var.floatVal, &floatBits, sizeof(double));
                
                var.strLexemeId = readU32(buffer, offset);
                var.ptrEntityId = readString(buffer, offset);
                pStream.properties.push_back(var);
            }
            outGraph.properties.push_back(pStream);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Ecform binary read error: " << e.what() << "\n";
        return false;
    }
    
    return true;
}

} // namespace Storage
} // namespace Earthcall
