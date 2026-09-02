#include "StringId.hpp"
#include <cassert>
#include <mutex>

namespace Earthcall {

// ----------------------------------------------------------------------------
// Static initialization helpers — the map and vector live in function-local
// statics to avoid static initialization order fiasco. First call to intern()
// or resolve() constructs them.
// ----------------------------------------------------------------------------

std::unordered_map<std::string, uint32_t>& StringInterner::stringToId() {
    static std::unordered_map<std::string, uint32_t> instance;
    return instance;
}

std::vector<std::string>& StringInterner::idToString() {
    // Index 0 is reserved for "invalid" (StringId default constructor).
    // The first real string gets ID 1, stored at idToString()[1].
    static std::vector<std::string> instance{""};  // Sentinel at index 0
    return instance;
}

static std::mutex& getInternerMutex() {
    static std::mutex instance;
    return instance;
}

// ----------------------------------------------------------------------------
// StringInterner::intern — map a string to its unique ID
// ----------------------------------------------------------------------------

StringId StringInterner::intern(const std::string& str) {
    std::lock_guard<std::mutex> lock(getInternerMutex());
    auto& strToId = stringToId();
    auto& idToStr = idToString();

    // Check if already interned
    auto it = strToId.find(str);
    if (it != strToId.end()) {
        return StringId(it->second);
    }

    // Assign new ID (next index in the vector)
    uint32_t newId = static_cast<uint32_t>(idToStr.size());
    assert(newId > 0 && "StringId overflow: exceeded 4 billion unique strings");

    // Store bidirectional mapping
    strToId[str] = newId;
    idToStr.push_back(str);

    return StringId(newId);
}

// ----------------------------------------------------------------------------
// StringInterner::resolve — recover the original string from its ID
// ----------------------------------------------------------------------------

const std::string& StringInterner::resolve(StringId id) {
    std::lock_guard<std::mutex> lock(getInternerMutex());
    auto& idToStr = idToString();

    // ID 0 is invalid (default-constructed StringId)
    if (id.value == 0 || id.value >= idToStr.size()) {
        return idToStr[0];  // Return the sentinel empty string
    }

    return idToStr[id.value];
}

// ----------------------------------------------------------------------------
// Introspection / debugging
// ----------------------------------------------------------------------------

size_t StringInterner::internedCount() {
    std::lock_guard<std::mutex> lock(getInternerMutex());
    return idToString().size() - 1;
}

void StringInterner::clear() {
    std::lock_guard<std::mutex> lock(getInternerMutex());
    stringToId().clear();
    idToString().clear();
    idToString().push_back("");  // Restore sentinel
}

} // namespace Earthcall
