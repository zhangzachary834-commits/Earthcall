#pragma once
#include "Singularity/Storage/MigrationFramework.hpp"
#include <string>
#include <vector>

namespace Earthcall {
namespace Storage {

class BinarySerializer {
public:
    static const uint32_t kEcformMagic = 0x4543464D; // "ECFM"
    static const uint32_t kEcformVersion = 2;

    // Write an EcformGraph to a binary file
    static bool writeBinary(const EcformGraph& graph, const std::string& filepath);

    // Read an EcformGraph from a binary file
    static bool readBinary(const std::string& filepath, EcformGraph& outGraph);

private:
    static void writeU32(std::vector<uint8_t>& buffer, uint32_t val);
    static void writeU16(std::vector<uint8_t>& buffer, uint16_t val);
    static void writeString(std::vector<uint8_t>& buffer, const std::string& str);

    static uint32_t readU32(const std::vector<uint8_t>& buffer, size_t& offset);
    static uint16_t readU16(const std::vector<uint8_t>& buffer, size_t& offset);
    static std::string readString(const std::vector<uint8_t>& buffer, size_t& offset);
};

} // namespace Storage
} // namespace Earthcall
