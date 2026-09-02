#include "StringId.hpp"
#include <cassert>

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

// ----------------------------------------------------------------------------
// StringInterner::intern — map a string to its unique ID
//
// Algorithm:
// 1. Check if the string is already interned (O(1) hash map lookup)
// 2. If yes, return the existing ID
// 3. If no, assign a new ID (next available index), store bidirectional mapping
//
// ID assignment starts at 1 (0 is reserved for "invalid").
// ----------------------------------------------------------------------------

StringId StringInterner::intern(const std::string& str) {
    auto& strToId = stringToId();
    auto& idToStr = idToString();

    // Check if already interned
    auto it = strToId.find(str);
    if (it != strToId.end()) {
        return StringId(it->second);
    }

    // Assign new ID (next index in the vector)
    uint32_t newId = static_cast<uint32_t>(idToStr.size());

    // Sanity check: we're using uint32_t, so we can intern up to ~4 billion
    // unique strings. If we somehow exceed that, fail loudly rather than
    // silently wrapping around (which would cause ID collisions).
    assert(newId > 0 && "StringId overflow: exceeded 4 billion unique strings");

    // Store bidirectional mapping
    strToId[str] = newId;
    idToStr.push_back(str);

    return StringId(newId);
}

// ----------------------------------------------------------------------------
// StringInterner::resolve — recover the original string from its ID
//
// Returns empty string if ID is invalid (0, or out of bounds).
// ----------------------------------------------------------------------------

const std::string& StringInterner::resolve(StringId id) {
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
    // Subtract 1 for the sentinel at index 0
    return idToString().size() - 1;
}

void StringInterner::clear() {
    stringToId().clear();
    idToString().clear();
    idToString().push_back("");  // Restore sentinel
}

} // namespace Earthcall
