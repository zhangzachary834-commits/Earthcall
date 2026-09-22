#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// String Interning — cache-optimal property lookup via lightweight integer IDs
//
// **The Problem:**
// Property lookups iterate std::vector<std::unique_ptr<Property>>, dereferencing
// pointers and calling virtual name() on every iteration. Each name() call
// allocates/copies a std::string, then strcmp walks the characters. At 21
// properties (CreationChannel), every lookup pays: 21 cache misses + 21 vtable
// lookups + 21 string allocations + ~20 failed strcmp calls.
//
// **The Solution:**
// Map every string to a lightweight 4-byte integer (StringId) exactly once at
// parse/load time. Property lookups then scan a contiguous std::vector<StringId>
// via pure integer comparisons — zero allocations, perfect L1 cache utilization.
// A 64-byte cache line holds 16 StringIds, so most property scans complete
// without leaving the CPU cache.
//
// **Related Docs:**
// - docs/audits/PROPERTY_LOOKUP_COMPLEXITY_AUDIT_2026-09-01.md
// - docs/plans/STRING_INTERNING_IMPLEMENTATION_PLAN.md
// - docs/architecture/law/LAW_EXECUTION_FRONTIER.md
// ============================================================================

namespace Earthcall {

// ----------------------------------------------------------------------------
// StringId — a lightweight 4-byte handle representing an interned string.
//
// Two StringIds are equal if and only if they were interned from the same
// string. Comparing StringIds is a single integer comparison (1 cycle);
// comparing the original strings is O(L) character walks.
//
// NOT serialized — save files write the original strings. StringIds are
// runtime optimization state, rebuilt on every load. IDs assigned to the
// same string MAY differ between runs (hash map iteration order), so never
// persist a StringId to disk.
// ----------------------------------------------------------------------------
struct StringId {
    uint32_t value;

    // Default to invalid ID (zero is reserved as "not interned")
    StringId() : value(0) {}
    explicit StringId(uint32_t v) : value(v) {}

    bool isValid() const { return value != 0; }

    bool operator==(const StringId& other) const { return value == other.value; }
    bool operator!=(const StringId& other) const { return value != other.value; }
    bool operator<(const StringId& other) const { return value < other.value; }
};

} // namespace Earthcall

// Hash function for StringId (so it can be used in std::unordered_map)
namespace std {
    template<>
    struct hash<Earthcall::StringId> {
        size_t operator()(const Earthcall::StringId& id) const {
            return std::hash<uint32_t>{}(id.value);
        }
    };
}

namespace Earthcall {

// ----------------------------------------------------------------------------
// StringInterner — global registry mapping strings to StringIds and back.
//
// **Thread Safety:**
// Fully thread-safe. A std::mutex protects internal maps against concurrent
// access from the WebSocket server and the main tick thread.
//
// **Lifecycle:**
// The interner is a static singleton that lives for the program's lifetime.
// Calling intern() repeatedly with the same string returns the same ID.
// Strings are never evicted (IDs remain valid until shutdown).
//
// **Performance:**
// - intern():   O(L) hash + O(1) map lookup/insert, paid once per unique string
// - resolve():  O(1) vector index, costs ~3 cycles
// - compare():  1 cycle integer comparison (vs. O(L) strcmp)
// ----------------------------------------------------------------------------
class StringInterner {
public:
    // Intern a string, returning its unique ID. Calling intern("shape") multiple
    // times always returns the same StringId. The interner keeps the original
    // string alive, so resolve() can recover it.
    static StringId intern(const std::string& str);

    // Recover the original string from its ID. Returns empty string if the ID
    // is invalid (never interned, or id.value == 0). The returned reference is
    // stable for the program's lifetime — the interner never evicts strings.
    static const std::string& resolve(StringId id);

    // Debugging / introspection
    static size_t internedCount();  // How many unique strings are interned
    static void clear();  // Reset the interner (for tests only, never in production)

private:
    StringInterner() = delete;  // Static-only class, never instantiate

    // The interner's state lives in static locals inside intern() to guarantee
    // initialization order (first call constructs them). Returning references
    // from functions ensures they're constructed before use.
    static std::unordered_map<std::string, uint32_t>& stringToId();
    static std::vector<std::string>& idToString();
};

} // namespace Earthcall
