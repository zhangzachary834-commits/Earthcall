#include "Singularity/Core/Logging/DurableLogger.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using ECA::Logging::DurableLogger;

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string encoded(const std::string& id) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "id-";
    for (unsigned char c : id) {
        out.push_back(hex[c >> 4]);
        out.push_back(hex[c & 15]);
    }
    return out + ".jsonl";
}

struct ScopedSandbox {
    fs::path original = fs::current_path();
    fs::path root;
    ScopedSandbox() {
        const auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int i = 0; i < 100; ++i) {
            auto candidate = fs::temp_directory_path() /
                ("earthcall-law-durable-witness-" + std::to_string(seed) + "-" + std::to_string(i));
            std::error_code ec;
            if (fs::create_directory(candidate, ec)) {
                root = candidate;
                fs::current_path(root);
                return;
            }
            if (ec) throw fs::filesystem_error("create sandbox", candidate, ec);
        }
        throw std::runtime_error("Unable to create unique sandbox");
    }
    ~ScopedSandbox() {
        std::error_code ec;
        fs::current_path(original, ec);
        if (!root.empty()) fs::remove_all(root, ec); // only our newly created temp dir
    }
};

std::vector<nlohmann::json> readEntries(const fs::path& file) {
    std::ifstream in(file);
    check(in.is_open(), "Cannot read log: " + file.string());
    std::vector<nlohmann::json> entries;
    std::string line;
    while (std::getline(in, line)) entries.push_back(nlohmann::json::parse(line));
    return entries;
}
} // namespace

int main() {
    try {
        ScopedSandbox sandbox;
        const auto root = sandbox.root / "direct";
        const std::string idA = "law/a";
        const std::string idB = "law?a";
        const auto pathA = root / "laws" / encoded(idA);
        const auto pathB = root / "laws" / encoded(idB);
        check(pathA != pathB, "Distinct identities alias a filename");

        {
            DurableLogger logger("laws", root);
            check(logger.log(idA, "LAW", "A1"), "A1 append failed");
            check(logger.log(idB, "LAW", "B1"), "B1 append failed");
            check(logger.log(idA, "LAW", "A2"), "A2 append failed");
            // Stress the bounded stream cache and verify append after eviction.
            for (int i = 0; i < 48; ++i) {
                check(logger.log("extra-" + std::to_string(i), "LAW", "extra"),
                      "bounded stream cache append failed");
            }
            check(logger.log(idA, "LAW", "A3"), "reopen after eviction failed");
        }
        {
            DurableLogger reopened("laws", root);
            check(reopened.log(idA, "LAW", "A4"), "append after new instance failed");
        }
        auto a = readEntries(pathA);
        auto b = readEntries(pathB);
        check(a.size() == 4 && b.size() == 1, "Per-identity append counts wrong");
        check(a[0].at("message") == "A1" && a[3].at("message") == "A4",
              "Append order lost");
        for (const auto& entry : a) check(entry.at("identity") == idA, "A identity mixed");
        for (const auto& entry : b) check(entry.at("identity") == idB, "B identity mixed");
        check(!fs::exists(root / "laws" / "laws.jsonl"), "Aggregate filename collision");

        // An invalid log destination must report failure, never silently pass.
        const auto blocked = sandbox.root / "not-a-directory";
        { std::ofstream out(blocked); out << "sentinel"; }
        DurableLogger failing("laws", blocked);
        check(!failing.log("any", "LAW", "nope"), "Unwritable path falsely succeeded");
        // An open failure is transient: the same logger must retry this
        // identity after filesystem permissions are restored.
        const auto recoveryRoot = sandbox.root / "recovery";
        const auto recoveryDir = recoveryRoot / "laws";
        fs::create_directories(recoveryDir);
        const auto originalPermissions = fs::status(recoveryDir).permissions();
        fs::permissions(recoveryDir, fs::perms::owner_read | fs::perms::owner_exec,
                        fs::perm_options::replace);
        DurableLogger recovering("laws", recoveryRoot);
        const bool denied = !recovering.log("retry", "LAW", "denied");
        fs::permissions(recoveryDir, originalPermissions, fs::perm_options::replace);
        if (denied) {
            check(recovering.log("retry", "LAW", "recovered"),
                  "Failed stream was retained after permissions recovered");
            const auto recovered = readEntries(recoveryDir / encoded("retry"));
            check(recovered.size() == 1 && recovered[0].at("message") == "recovered",
                  "Retry did not produce exactly one durable record");
        } else {
            // Privileged environments may ignore directory write permissions.
            std::cout << "Open-failure retry witness skipped: permissions not enforced\n";
        }

        // A full historical file must be preserved, never truncated or extended.
        const auto capRoot = sandbox.root / "capacity";
        fs::create_directories(capRoot / "laws");
        const auto capPath = capRoot / "laws" / encoded("at-cap");
        { std::ofstream out(capPath, std::ios::binary); check(out.good(), "Cannot seed cap"); }
        fs::resize_file(capPath, DurableLogger::kMaxFileBytes);
        DurableLogger capped("laws", capRoot);
        check(!capped.log("at-cap", "LAW", "must be rejected"),
              "Per-identity file budget did not refuse append");
        check(fs::file_size(capPath) == DurableLogger::kMaxFileBytes,
              "Existing log was truncated or exceeded its budget");

        // Exercise the public LawAuditLogger bridge with muted aggregate logging.
        auto& lawLogger = ECA::LawAuditLogger::instance();
        lawLogger.setLevel(ECA::LogLevel::Off);
        check(lawLogger.logForLaw("integration", "LAW", "durable with aggregate Off"),
              "LawAuditLogger did not append with aggregate Off");
        // Synthetic high-frequency ConditionsFailed bridge benchmark. This is
        // a diagnostic measurement, not a machine-independent pass threshold.
        // It includes message/details construction, JSON serialization,
        // the logger mutex, append and synchronous flush, but not Law evaluation.
        constexpr int kHotEvents = 2000;
        const auto hotStart = std::chrono::steady_clock::now();
        for (int i = 0; i < kHotEvents; ++i) {
            const std::string target = "target-" + std::to_string(i % 16);
            const std::string message =
                "Law \"bench-law\" applied to \"" + target + "\" - CONDITIONS FAILED";
            const nlohmann::json details = {
                {"lawId", "bench-law"}, {"targetId", target},
                {"result", "ConditionsFailed"}
            };
            check(lawLogger.logForLaw("bench-law", "LAW", message, details),
                  "High-frequency bridge append failed");
        }
        const auto hotStop = std::chrono::steady_clock::now();
        const double hotMicros = std::chrono::duration<double, std::micro>(
            hotStop - hotStart).count() / kHotEvents;
        std::cout << std::fixed << std::setprecision(2)
                  << "durable logging synthetic ConditionsFailed bridge: "
                  << hotMicros << " us/event over " << kHotEvents
                  << " synchronous append+flush events\n";
        lawLogger.shutdown();
        auto integrated = readEntries(sandbox.root / "logs" / "laws" / encoded("integration"));
        check(readEntries(sandbox.root / "logs" / "laws" / encoded("bench-law")).size() == kHotEvents,
              "High-frequency durable event count mismatch");
        check(integrated.size() == 1 &&
              integrated[0].at("message") == "durable with aggregate Off",
              "LawAuditLogger bridge missing durable event");

        std::cout << "law_durable_logging_test PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "law_durable_logging_test FAILED: " << ex.what() << '\n';
        return 1;
    }
}
