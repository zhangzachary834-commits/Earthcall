#include "Singularity/Core/Logging/DurableLogger.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
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

        // Exercise the public LawAuditLogger bridge with muted aggregate logging.
        auto& lawLogger = ECA::LawAuditLogger::instance();
        lawLogger.setLevel(ECA::LogLevel::Off);
        check(lawLogger.logForLaw("integration", "LAW", "durable with aggregate Off"),
              "LawAuditLogger did not append with aggregate Off");
        lawLogger.shutdown();
        auto integrated = readEntries(sandbox.root / "logs" / "laws" / encoded("integration"));
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
