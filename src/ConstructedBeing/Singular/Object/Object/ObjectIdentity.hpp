#pragma once

// ============================================================================
// ObjectIdentity.hpp - Identity management for Object
//
// This header contains identity-related functionality for Object, including
// the global object ID counter and ID management utilities.
// ============================================================================

#include <atomic>
#include <cstdint>   // uint64_t
#include <cstdlib>   // std::strtoull
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

// Volatile object ID tracking and summary logging
inline std::atomic<uint64_t>& getVolatileObjectCount() {
    static std::atomic<uint64_t> g_volatileCount{0};
    return g_volatileCount;
}

inline bool& getVerboseVolatileLogging() {
    static bool g_verbose = false;
    return g_verbose;
}

inline void setVerboseVolatileLogging(bool verbose) {
    getVerboseVolatileLogging() = verbose;
}

inline void recordVolatileIdentifier(const std::string& id) {
    getVolatileObjectCount().fetch_add(1);
    if (getVerboseVolatileLogging()) {
        printf("WARNING: Object initialized without a stable string identifier. Assigned volatile ID '%s'. This object should not be reliably targeted by Law text.\n", id.c_str());
    }
}

inline uint64_t reportAndResetVolatileCount() {
    return getVolatileObjectCount().exchange(0);
}

// The slug a concept's newborn is named by.
//
// A concept names its members by SLOT ("member-0" is the first thing in the
// recipe), and a slot is not an identity: instantiating the same concept
// twice produces two beings, and naming them both after the slot gives the
// world two things answering to one name. Every identifier lookup in the
// engine is first-match -- World::removeObject, Formation::hasMember, the
// save reattachment in Serialization.cpp -- so a repeated slug does not
// merely read badly, it resolves to an arbitrary sibling.
//
// The birth number comes from the SAME counter as generateObjectId(), so it
// is unique across concepts, and claimIdentifierAtLeast below recognises this
// form as well -- a save full of these advances the counter past all of them,
// exactly as it does for "object-N".
inline std::string conceptMemberIdPrefix() { return ".birth-"; }

inline std::string generateConceptMemberId(const std::string& conceptId, std::size_t member) {
    return conceptId + conceptMemberIdPrefix() +
           std::to_string(getNextObjectId().fetch_add(1)) +
           ".member-" + std::to_string(member);
}

// Ensure a restored object ID advances the counter past itself
// This prevents ID collisions between loaded objects and new objects
inline void claimIdentifierAtLeast(const std::string& identifier) {
    // Two forms carry a number drawn from the shared counter: "object-N" and
    // "<conceptId>.birth-N.member-M". A restored id of EITHER form has to push
    // the counter past itself, or the next spawn reissues an identity that is
    // already alive in the world.
    std::size_t numberStart = std::string::npos;

    const std::string prefix = "object-";
    if (identifier.rfind(prefix, 0) == 0) {
        numberStart = prefix.size();
    } else {
        const std::string birth = conceptMemberIdPrefix();
        const std::size_t at = identifier.find(birth);
        if (at != std::string::npos) numberStart = at + birth.size();
    }
    if (numberStart == std::string::npos) return;   // not a counter-borne id

    const uint64_t n = std::strtoull(identifier.c_str() + numberStart, nullptr, 10);
    uint64_t current = getNextObjectId().load();

    while (n + 1 > current &&
           !getNextObjectId().compare_exchange_weak(current, n + 1)) {
        // Keep trying until we successfully advance past n
    }
}

} // namespace ObjectIdentity
