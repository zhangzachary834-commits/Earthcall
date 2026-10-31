#include "Singularity/Core/Logging/DurableLogger.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
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
        // Exercise the real Law::applyTo ConditionsFailed path, not only the
        // direct logging bridge. The result is diagnostic, not a CI timing gate.
        Object refusalAuthor;
        refusalAuthor.setObjectID("durable-refusal-author");
        Object refusalTarget;
        refusalTarget.setObjectID("durable-refusal-target");
        Law refusalLaw("Durable refusal workload", std::vector<Singular*>{&refusalAuthor});
        refusalLaw.setLawIdentifier("durable-refusal-workload");
        refusalLaw.setScope(Law::Scope::Subject);
        refusalLaw.setConditionModel(ConditionNode::identity("never-matching-target"));
        refusalLaw.setActionModel(ActionNode::set("unused", PropertyValue(1.0)));
        constexpr int kRealRefusals = 512;
        const auto realStart = std::chrono::steady_clock::now();
        for (int i = 0; i < kRealRefusals; ++i) {
            check(refusalLaw.applyTo(refusalTarget) == Law::ApplicationResult::ConditionsFailed,
                  "Real Law refusal benchmark did not reject the target");
        }
        const auto realStop = std::chrono::steady_clock::now();
        const double realMicros = std::chrono::duration<double, std::micro>(
            realStop - realStart).count() / kRealRefusals;
        std::cout << std::fixed << std::setprecision(2)
                  << "durable logging real Law::applyTo ConditionsFailed: "
                  << realMicros << " us/refusal over " << kRealRefusals
                  << " repeated failed conditions\n";

        // End-to-end tick diagnostic: a connected WhileTrue law starts with
        // true x > y candidates, then y changes. Rete's x-filter may retain
        // stale candidates; LawManager must recheck and record real refusals.
        // Report observed frequency and wall time, not an invented frame budget.
        std::size_t tickRefusals = 0;
        constexpr int kTickTargets = 32;
        constexpr int kTickSamples = 8;
        double tickMs = 0.0;
        double tickBaselineMs = 0.0;
        {
            Object tickAuthor;
            tickAuthor.setObjectID("tick-refusal-author");
            std::vector<std::unique_ptr<Object>> tickTargets;
            std::vector<Singular*> tickPopulation{&tickAuthor};
            for (int i = 0; i < kTickTargets; ++i) {
                auto target = std::make_unique<Object>();
                target->setObjectID("tick-refusal-target-" + std::to_string(i));
                target->setDynamicProperty("x", PropertyValue(10.0));
                target->setDynamicProperty("y", PropertyValue(0.0));
                tickPopulation.push_back(target.get());
                tickTargets.push_back(std::move(target));
            }
            Universe::instance().setProvider([&](std::vector<Singular*>& out) {
                out.insert(out.end(), tickPopulation.begin(), tickPopulation.end());
            });
            Universe::instance().setClock(100.0, 1.0 / 60.0);
            {
                LawManager manager;
                manager.connectToEventBus();
                auto tickLaw = manager.createLaw(
                    "Durable tick refusal workload", "tick-refusal-workload", {&tickAuthor});
                check(static_cast<bool>(tickLaw), "Tick benchmark Law registration failed");
                tickLaw->setActivation(Law::Activation::WhileTrue);
                tickLaw->setConditionModel(
                    ConditionNode::comparePaths("x", ConditionNode::Op::Gt, "y"));
                tickLaw->setActionModel(
                    ActionNode::set("tick-sentinel", PropertyValue(1.0)));
                for (const auto& target : tickTargets) tickLaw->addTarget(*target);
                manager.tick(); // seed initially true terminal candidates
                // Compare against the same connected Law and population while
                // its condition remains true and no new writes are produced.
                // This is a diagnostic control, not an attribution proof:
                // true and false conditions can take different Rete paths.
                const auto baselineStart = std::chrono::steady_clock::now();
                for (int i = 0; i < kTickSamples; ++i) manager.tick();
                const auto baselineStop = std::chrono::steady_clock::now();
                tickBaselineMs = std::chrono::duration<double, std::milli>(
                    baselineStop - baselineStart).count() / kTickSamples;
                for (const auto& target : tickTargets) {
                    target->setDynamicProperty("y", PropertyValue(20.0));
                }
                const auto tickStart = std::chrono::steady_clock::now();
                for (int i = 0; i < kTickSamples; ++i) manager.tick();
                const auto tickStop = std::chrono::steady_clock::now();
                tickMs = std::chrono::duration<double, std::milli>(
                    tickStop - tickStart).count() / kTickSamples;
            }
            Universe::instance().setProvider(nullptr);
        }
        const auto tickLog = sandbox.root / "logs" / "laws" /
                             encoded("tick-refusal-workload");
        if (fs::exists(tickLog)) {
            for (const auto& entry : readEntries(tickLog)) {
                if (entry.contains("details") &&
                    entry.at("details").value("result", "") == "ConditionsFailed") {
                    ++tickRefusals;
                }
            }
        }
        check(tickRefusals == static_cast<std::size_t>(kTickTargets * kTickSamples),
              "Tick refusal diagnostic did not persist all expected events");
        std::cout << std::fixed << std::setprecision(3)
                  << "durable logging LawManager::tick diagnostic: "
                  << tickBaselineMs << " ms/tick with true condition/no new writes, "
                  << tickMs << " ms/tick with repeated refusals; "
                  << tickRefusals << " persisted ConditionsFailed events across "
                  << kTickSamples << " ticks and " << kTickTargets
                  << " candidate targets (diagnostic, not isolated I/O attribution)\n";
        lawLogger.shutdown();
        auto integrated = readEntries(sandbox.root / "logs" / "laws" / encoded("integration"));
        check(readEntries(sandbox.root / "logs" / "laws" / encoded("bench-law")).size() == kHotEvents,
              "High-frequency durable event count mismatch");
        check(readEntries(sandbox.root / "logs" / "laws" / encoded("durable-refusal-workload")).size() == kRealRefusals,
              "Real Law ConditionsFailed durable count mismatch");
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
