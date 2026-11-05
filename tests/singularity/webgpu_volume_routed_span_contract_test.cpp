#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>

int main() {
    static_assert(std::is_copy_constructible_v<KernelRoutedRange>);
    static_assert(std::is_copy_constructible_v<KernelRoutedSpanStrip>);
    static_assert(std::is_same_v<
        decltype(KernelRoutedSpanStrip{}.stripIndex), uint32_t>);
    static_assert(std::is_same_v<
        decltype(KernelRoutedSpanStrip{}.startRow), uint32_t>);
    static_assert(std::is_same_v<
        decltype(KernelRoutedSpanStrip{}.rowCount), uint32_t>);
    static_assert(std::is_same_v<
        decltype(KernelRoutedSpanStrip{}.rayCapacity), uint32_t>);

    KernelRoutedSpanStrip strip;
    strip.stripIndex = 3;
    strip.startRow = 144;
    strip.rowCount = 48;
    strip.rayCapacity = 4096;
    strip.window = {reinterpret_cast<WGPUBuffer>(uintptr_t(1)), 768, 48};
    strip.rays = {reinterpret_cast<WGPUBuffer>(uintptr_t(2)), 0, 4096ull * 48ull};
    strip.singles = {
        reinterpret_cast<WGPUBuffer>(uintptr_t(3)), 0,
        4096ull * 64ull * 16ull};
    strip.overlaps = {
        reinterpret_cast<WGPUBuffer>(uintptr_t(4)), 0,
        4096ull * 16ull * 16ull};

    assert(strip.window.size == 48);
    assert(strip.rays.size == uint64_t(strip.rayCapacity) * 48);
    assert(strip.singles.size == uint64_t(strip.rayCapacity) * 64 * 16);
    assert(strip.overlaps.size == uint64_t(strip.rayCapacity) * 16 * 16);

    bool called = false;
    KernelRoutedSpanConsumer consumer =
        [&](WGPUCommandEncoder, const KernelRoutedSpanStrip& borrowed) {
            called = true;
            assert(borrowed.startRow == strip.startRow);
            assert(borrowed.rowCount == strip.rowCount);
            return true;
        };
    assert(static_cast<bool>(consumer));
    assert(consumer(nullptr, strip));
    assert(called);

    std::cout
        << "PASS volume_routed_span_contract_test: strip metadata, "
        << "scratch byte geometry and borrowed callback contract\n";
    return 0;
}
