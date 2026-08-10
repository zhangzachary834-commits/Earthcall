#pragma once

// ============================================================================
// ObjectIdentity.hpp - Identity management for Object
//
// This header contains identity-related functionality for Object, including
// the global object ID counter and ID management utilities.
// ============================================================================

#include <atomic>
#include <string>

// ============================================================================
// Global Object ID Management
// ============================================================================

namespace ObjectIdentity {

// Global atomic counter for generating unique object IDs
// Objects are assigned IDs of the form "object-{N}" where N is this counter
inline std::atomic<uint64_t>& getNextObjectId() {
    static std::atomic<uint64_t> g_nextObjectId{1};
    return g_nextObjectId;
}

// Generate a new unique object ID
inline std::string generateObjectId() {
    return "object-" + std::to_string(getNextObjectId().fetch_add(1));
}

// Ensure a restored object ID advances the counter past itself
// This prevents ID collisions between loaded objects and new objects
inline void claimIdentifierAtLeast(const std::string& id) {
    const std::string prefix = "object-";
    if (id.rfind(prefix, 0) != 0) return; // Doesn't start with "object-"
    
    const uint64_t n = std::strtoull(id.c_str() + prefix.size(), nullptr, 10);
    uint64_t current = getNextObjectId().load();
    
    while (n + 1 > current &&
           !getNextObjectId().compare_exchange_weak(current, n + 1)) {
        // Keep trying until we successfully advance past n
    }
}

} // namespace ObjectIdentity
