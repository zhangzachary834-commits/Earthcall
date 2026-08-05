#include "Util/Frontier.hpp"
#include <cassert>
#include <cstdio>
#include <string>

// ---------------------------------------------------------
// Example Schemas and Version chains
// ---------------------------------------------------------

struct SaveData_V1 {
    int old_field = 10;
};

struct SaveData_V2 {
    float new_field = 20.0f;
};

struct SaveData_V3 {
    std::string text = "thirty";
    float new_field = 0.0f;
};

// ---------------------------------------------------------
// Setup VersionInfo
// ---------------------------------------------------------

namespace Frontier {

template <>
struct VersionInfo<SaveData_V1> {
    static constexpr uint32_t VERSION = 1;
    using Previous = void; // Root of the chain
};

template <>
struct VersionInfo<SaveData_V2> {
    static constexpr uint32_t VERSION = 2;
    using Previous = SaveData_V1;
};

template <>
struct VersionInfo<SaveData_V3> {
    static constexpr uint32_t VERSION = 3;
    using Previous = SaveData_V2;
};

template <>
struct IsFrontier<SaveData_V3> : std::true_type {};

// ---------------------------------------------------------
// Upgrade functions
// ---------------------------------------------------------

template <>
SaveData_V2 upgrade<SaveData_V1, SaveData_V2>(SaveData_V1&& old) {
    SaveData_V2 result;
    result.new_field = static_cast<float>(old.old_field) * 2.0f;
    return result;
}

template <>
SaveData_V3 upgrade<SaveData_V2, SaveData_V3>(SaveData_V2&& old) {
    SaveData_V3 result;
    result.new_field = old.new_field;
    result.text = "migrated";
    return result;
}

} // namespace Frontier

// ---------------------------------------------------------
// Mock Reader for Test
// ---------------------------------------------------------
class MockReader {
    uint32_t m_version;
    int m_v1_payload;
    
public:
    MockReader(uint32_t version, int payload) : m_version(version), m_v1_payload(payload) {}
    
    uint32_t readVersion() const { return m_version; }
    
    // Simulate reading different version payloads
    void read(SaveData_V1& out) {
        out.old_field = m_v1_payload;
    }
    void read(SaveData_V2& out) {
        out.new_field = static_cast<float>(m_v1_payload);
    }
    void read(SaveData_V3& out) {
        out.new_field = static_cast<float>(m_v1_payload);
        out.text = "read_v3";
    }
};


int main() {
    std::puts("frontier_test: Starting...");

    // Test 1: Direct compile-time migration
    SaveData_V1 v1;
    v1.old_field = 42;
    
    SaveData_V3 migrated = Frontier::migrate_to_frontier<SaveData_V3>(std::move(v1));
    assert(migrated.new_field == 84.0f);
    assert(migrated.text == "migrated");
    std::puts("frontier_test: compile-time migration OK");

    // Test 2: Runtime load via Frontier::load_frontier from V1
    MockReader v1_reader(1, 100);
    SaveData_V3 loaded_from_v1 = Frontier::load_frontier<SaveData_V3>(v1_reader);
    assert(loaded_from_v1.new_field == 200.0f); // 100 * 2.0f
    assert(loaded_from_v1.text == "migrated");
    std::puts("frontier_test: runtime load from V1 OK");

    // Test 3: Runtime load via Frontier::load_frontier from V2
    MockReader v2_reader(2, 50); // read(SaveData_V2&) will set new_field to 50.0f
    SaveData_V3 loaded_from_v2 = Frontier::load_frontier<SaveData_V3>(v2_reader);
    assert(loaded_from_v2.new_field == 50.0f);
    assert(loaded_from_v2.text == "migrated");
    std::puts("frontier_test: runtime load from V2 OK");

    // Test 4: Runtime load via Frontier::load_frontier from V3
    MockReader v3_reader(3, 25); // read(SaveData_V3&) will set new_field to 25.0f and text to "read_v3"
    SaveData_V3 loaded_from_v3 = Frontier::load_frontier<SaveData_V3>(v3_reader);
    assert(loaded_from_v3.new_field == 25.0f);
    assert(loaded_from_v3.text == "read_v3");
    std::puts("frontier_test: runtime load from V3 OK");

    std::puts("frontier_test: ALL OK");
    return 0;
}
