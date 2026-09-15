#include "Singularity/Physical/Adapters/SerialAdapter.hpp"
#include <cstdio>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::printf("FAILED: %s\n", message.c_str());
    } else {
        std::printf("  ok: %s\n", message.c_str());
    }
}
} // namespace

int main() {
    std::printf("Running SerialAdapter tests...\n");

    Singularity::Physical::Adapters::SerialAdapter adapter;

    // As per the source, methods currently just cast to void or are empty.
    // The main guarantee is they are safe to call in sequence.
    adapter.connect("/dev/dummy");
    check(true, "connect() handles a string path");

    adapter.update();
    check(true, "update() does not crash after connection");

    adapter.disconnect();
    check(true, "disconnect() unbinds without crashing");

    adapter.update();
    check(true, "update() handles disconnected state safely");

    adapter.connect(nullptr);
    adapter.disconnect();
    check(true, "null port is handled without fault");

    if (failures != 0) {
        std::printf("serial_adapter_test: %d failure(s)\n", failures);
        return 1;
    }

    std::printf("serial_adapter_test: ALL OK\n");
    return 0;
}
