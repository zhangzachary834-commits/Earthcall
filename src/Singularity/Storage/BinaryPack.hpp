#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <stdexcept>
#include <glm/glm.hpp>
#include "json.hpp"

namespace BinaryPack {

class Writer {
public:
    std::vector<uint8_t> buffer;

    template<typename T>
    void write(const T& val) {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        size_t offset = buffer.size();
        buffer.resize(offset + sizeof(T));
        std::memcpy(buffer.data() + offset, &val, sizeof(T));
    }

    template<typename T>
    void writeArray(const std::vector<T>& vec) {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        uint32_t size = static_cast<uint32_t>(vec.size());
        write(size);
        if (size > 0) {
            size_t offset = buffer.size();
            size_t bytes = size * sizeof(T);
            buffer.resize(offset + bytes);
            std::memcpy(buffer.data() + offset, vec.data(), bytes);
        }
    }
    
    nlohmann::json::binary_t toBinaryJson() const {
        return nlohmann::json::binary(buffer);
    }
};

// A binary pack, read back from wherever it was stored.
//
// nlohmann has TWO on-disk shapes for the same binary value. A save packed as
// msgpack/CBOR carries a real binary node, and `get_binary()` returns it. A save
// written as TEXT JSON cannot carry one -- `dump()` degrades it to the object
// {"bytes":[...],"subtype":...} -- and re-parsing that yields an object, on
// which `get_binary()` THROWS. Every save under saves/ is text JSON, so every
// polyhedron, Bezier patch and convex hull written with a binary pack became a
// file that could be written and never read: Earthcall terminated on
// `[json.exception.type_error.302] type must be binary, but is object` before
// reaching a window. Reading both shapes is what makes the two halves of the
// format agree; nothing on disk has to change, and nothing already written is
// lost.
inline std::vector<uint8_t> bytesFrom(const nlohmann::json& j) {
    if (j.is_binary()) {
        const auto& b = j.get_binary();
        return std::vector<uint8_t>(b.begin(), b.end());
    }
    if (j.is_object() && j.contains("bytes") && j["bytes"].is_array()) {
        std::vector<uint8_t> out;
        out.reserve(j["bytes"].size());
        for (const auto& e : j["bytes"]) out.push_back(e.get<uint8_t>());
        return out;
    }
    if (j.is_array()) {
        std::vector<uint8_t> out;
        out.reserve(j.size());
        for (const auto& e : j) out.push_back(e.get<uint8_t>());
        return out;
    }
    throw std::runtime_error("BinaryPack: value is neither a binary node nor a dumped byte array");
}

class Reader {
private:
    const uint8_t* data;
    size_t size;
    size_t offset;

public:
    Reader(const std::vector<uint8_t>& buf) : data(buf.data()), size(buf.size()), offset(0) {}
    Reader(const std::vector<uint8_t>&&) = delete;
    Reader(const nlohmann::json::binary_t& bin) : data(bin.data()), size(bin.size()), offset(0) {}
    Reader(const nlohmann::json::binary_t&&) = delete;

    template<typename T>
    T read() {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        if (offset + sizeof(T) > size) throw std::runtime_error("Binary buffer underflow in read");
        T val;
        std::memcpy(&val, data + offset, sizeof(T));
        offset += sizeof(T);
        return val;
    }

    template<typename T>
    void readArray(std::vector<T>& vec) {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        uint32_t count = read<uint32_t>();
        if (count > 0) {
            uint64_t bytes = static_cast<uint64_t>(count) * sizeof(T);
            if (offset + bytes > size) throw std::runtime_error("Binary buffer underflow in readArray");
            vec.resize(count);
            std::memcpy(vec.data(), data + offset, static_cast<size_t>(bytes));
            offset += static_cast<size_t>(bytes);
        } else {
            vec.resize(0);
        }
    }
};

} // namespace BinaryPack
