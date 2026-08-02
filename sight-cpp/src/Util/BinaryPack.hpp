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
