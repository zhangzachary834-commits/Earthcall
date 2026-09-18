// The Slow Adapter's maintenance clock must be independent of frame cadence.
//
// Zach, 2026-09-17: Moment is first-class so there is no rigid universal
// timeline; the adapter's slow work should not advance merely because a frame
// happened. This witness polls the same LawManager at several render-like rates
// over the same one-second interval and requires the maintenance cadence to be
// the same.
//
// Same thread is intentional for this rung. Independence here means temporal
// authority, not concurrency: serviceSlowAdapterClock() may be POLLED every
// frame, but only its own wall-time deadline causes maintenance.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>

namespace {
int failures = 0;

void check(bool ok, const char* what) {
    if (ok) std::printf("  ok: %s\n", what);
    else { ++failures; std::printf("  FAILED: %s\n", what); }
}

std::uint64_t runAtHz(double hz) {
    LawManager mgr; // adapter is ON by default
    const double dt = 1.0 / hz;
    double t = 0.0;
    mgr.serviceSlowAdapterClock(t); // prime; must not run maintenance
    while (t + 1e-12 < 1.0) {
        t = std::min(1.0, t + dt);
        mgr.serviceSlowAdapterClock(t);
    }
    return mgr.slowAdapterMaintenanceRuns();
}
}

int main() {
    LawManager priming;
    check(priming.usesSlowAdapter(), "Slow Adapter ships enabled");
    check(priming.serviceSlowAdapterClock(10.0) == 0,
          "first poll only establishes the independent clock deadline");
    check(priming.slowAdapterMaintenanceRuns() == 0,
          "priming does not count as maintenance");
    priming.serviceSlowAdapterClock(10.05);
    check(priming.slowAdapterMaintenanceRuns() == 0,
          "a frame before the deadline cannot advance the adapter");
    priming.serviceSlowAdapterClock(10.10);
    check(priming.slowAdapterMaintenanceRuns() == 1,
          "the adapter advances when its own deadline arrives");

    // A long stall gets ONE admitted slice, not a catch-up burst.
    priming.serviceSlowAdapterClock(20.0);
    check(priming.slowAdapterMaintenanceRuns() == 2,
          "missed periods are not replayed as a post-stall burst");

    const auto at60  = runAtHz(60.0);
    const auto at144 = runAtHz(144.0);
    const auto at240 = runAtHz(240.0);
    std::printf("  maintenance runs over 1s: 60Hz=%llu 144Hz=%llu 240Hz=%llu\n",
                static_cast<unsigned long long>(at60),
                static_cast<unsigned long long>(at144),
                static_cast<unsigned long long>(at240));

    check(at60 >= 9 && at60 <= 10, "10 Hz bootstrap cadence is approximately ten runs/second");
    check(at60 == at144 && at144 == at240,
          "render polling frequency does not multiply Slow Adapter maintenance");

    std::printf("%s\n", failures ? "slow_adapter_clock_test: FAILURES"
                                  : "slow_adapter_clock_test: OK");
    return failures ? 1 : 0;
}
