#include "Singularity/Core/Logging/DurableLogger.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
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
        // JSONL budgets use physical bytes. Text-mode CRLF translation on
        // Windows would make file_size exceed the serialized record lengths.
        std::uintmax_t expectedRawBytes = 0;
        for (const auto& entry : a) expectedRawBytes += entry.dump().size() + 1;
        check(fs::file_size(pathA) == expectedRawBytes,
              "JSONL disk bytes differ from serialized bytes plus LF delimiters");
        check(a[0].at("message") == "A1" && a[3].at("message") == "A4",
              "Append order lost");
        // The fast timestamp formatter must preserve the existing UTC JSONL schema.
        const std::string timestamp = a[0].at("timestamp").get<std::string>();
        check(timestamp.size() == 20 && timestamp[4] == '-' && timestamp[7] == '-' &&
              timestamp[10] == 'T' && timestamp[13] == ':' && timestamp[16] == ':' &&
              timestamp[19] == 'Z', "Durable timestamp format changed");
        for (const auto& entry : a) check(entry.at("identity") == idA, "A identity mixed");
        for (const auto& entry : b) check(entry.at("identity") == idB, "B identity mixed");
        check(!fs::exists(root / "laws" / "laws.jsonl"), "Aggregate filename collision");

        // Parallel callers must not interleave JSONL fragments, duplicate
        // records, or route one Law's history into another Law's file.
        // Check every message, not merely the final file length.
        constexpr int kConcurrentWorkers = 4;
        constexpr int kConcurrentEventsPerWorker = 64;
        const auto concurrentRoot = sandbox.root / "concurrent";
        std::atomic<bool> concurrentAppendFailed{false};
        {
            DurableLogger concurrentLogger("laws", concurrentRoot);
            std::vector<std::thread> workers;
            workers.reserve(kConcurrentWorkers);
            for (int worker = 0; worker < kConcurrentWorkers; ++worker) {
                workers.emplace_back([&, worker] {
                    const std::string identity = "concurrent-law-" + std::to_string(worker % 2);
                    for (int event = 0; event < kConcurrentEventsPerWorker; ++event) {
                        const std::string message =
                            "worker-" + std::to_string(worker) + "-event-" + std::to_string(event);
                        if (!concurrentLogger.log(identity, "LAW", message))
                            concurrentAppendFailed.store(true);
                    }
                });
            }
            for (auto& worker : workers) worker.join();
            check(!concurrentAppendFailed.load(), "Concurrent durable append failed");
        }
        for (int identityIndex = 0; identityIndex < 2; ++identityIndex) {
            const std::string identity = "concurrent-law-" + std::to_string(identityIndex);
            const auto entries = readEntries(concurrentRoot / "laws" / encoded(identity));
            check(entries.size() == kConcurrentWorkers * kConcurrentEventsPerWorker / 2,
                  "Concurrent append count wrong");
            std::set<std::string> messages;
            for (const auto& entry : entries) {
                check(entry.at("identity") == identity, "Concurrent histories mixed identities");
                check(messages.insert(entry.at("message").get<std::string>()).second,
                      "Concurrent history contains duplicate records");
            }
            for (int worker = identityIndex; worker < kConcurrentWorkers; worker += 2) {
                for (int event = 0; event < kConcurrentEventsPerWorker; ++event) {
                    const std::string expected =
                        "worker-" + std::to_string(worker) + "-event-" + std::to_string(event);
                    check(messages.count(expected) == 1, "Concurrent history lost a record");
                }
            }
        }

        // More distinct Laws than the open-stream cap must still preserve
        // every identity's append history when streams are evicted/reopened.
        // This is an ordinary world scale, not a stress-only scenario.
        constexpr int kEvictionLawCount = 40;
        const auto evictionRoot = sandbox.root / "stream-eviction";
        {
            DurableLogger evictionLogger("laws", evictionRoot);
            for (int i = 0; i < kEvictionLawCount; ++i) {
                const auto id = "eviction-law-" + std::to_string(i);
                check(evictionLogger.log(id, "LAW", "first"),
                      "Initial multi-Law append failed");
            }
            for (int i = 0; i < kEvictionLawCount; ++i) {
                const auto id = "eviction-law-" + std::to_string(i);
                check(evictionLogger.log(id, "LAW", "second"),
                      "Multi-Law append after stream eviction failed");
            }
        }
        for (int i = 0; i < kEvictionLawCount; ++i) {
            const auto id = "eviction-law-" + std::to_string(i);
            const auto entries = readEntries(evictionRoot / "laws" / encoded(id));
            check(entries.size() == 2 && entries[0].at("message") == "first" &&
                  entries[1].at("message") == "second" &&
                  entries[0].at("identity") == id && entries[1].at("identity") == id,
                  "Stream eviction lost, truncated, reordered or misrouted Law history");
        }

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

        // Deterministic obstruction/recovery witness, including environments
        // where permission-based denial cannot be enforced (e.g. root).
        // A file occupying the intended parent directory must not poison the
        // logger: removing it must allow a later append to the same identity.
        const auto obstructedRoot = sandbox.root / "obstructed-recovery";
        fs::create_directories(obstructedRoot);
        const auto obstructedParent = obstructedRoot / "laws";
        { std::ofstream blocker(obstructedParent); check(blocker.good(), "Cannot seed obstruction"); }
        DurableLogger obstructionLogger("laws", obstructedRoot);
        check(!obstructionLogger.log("retry", "LAW", "blocked"),
              "Parent-directory obstruction falsely succeeded");
        fs::remove(obstructedParent);
        check(obstructionLogger.log("retry", "LAW", "recovered"),
              "Same-identity append did not recover after parent obstruction cleared");
        const auto obstructionEntries = readEntries(obstructedParent / encoded("retry"));
        check(obstructionEntries.size() == 1 &&
              obstructionEntries[0].at("message") == "recovered",
              "Obstruction recovery lost or duplicated durable records");

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

        // One remaining byte must not permit a multi-byte JSONL record.
        // Refusal must preserve the original file length, not write a fragment.
        fs::resize_file(capPath, DurableLogger::kMaxFileBytes - 1);
        DurableLogger nearlyCapped("laws", capRoot);
        check(!nearlyCapped.log("at-cap", "LAW", "record cannot fit"),
              "Near-cap file accepted a record larger than its remaining space");
        check(fs::file_size(capPath) == DurableLogger::kMaxFileBytes - 1,
              "Near-cap refusal modified existing history");

        // A malicious domain cannot escape the logging root, and an identity
        // longer than the encoded filename limit must be rejected, not aliased.
        const auto safetyRoot = sandbox.root / "path-safety";
        DurableLogger invalidDomain("../escaped", safetyRoot);
        check(!invalidDomain.log("safe", "LAW", "must be rejected"),
              "Path-traversing durable logging domain was accepted");
        check(!fs::exists(sandbox.root / "escaped"),
              "Path-traversing domain created a directory outside its root");
        DurableLogger boundedIdentity("laws", safetyRoot);
        const std::string maxIdentity(110, 'x');
        check(boundedIdentity.log(maxIdentity, "LAW", "valid boundary identity"),
              "Maximum permitted identity length was rejected");
        check(!boundedIdentity.log(maxIdentity + "x", "LAW", "invalid boundary identity"),
              "Oversized identity was accepted or truncated");
        check(readEntries(safetyRoot / "laws" / encoded(maxIdentity)).size() == 1,
              "Oversized identity altered the maximum-length identity history");

        // Reach the exact file cap through an already-open stream, then
        // confirm that subsequent appends are rejected without modifying it.
        // A same-identity probe gives the precise serialized record length;
        // timestamps have a fixed 20-character UTC representation.
        const auto probeRoot = sandbox.root / "capacity-probe";
        DurableLogger capProbe("laws", probeRoot);
        check(capProbe.log("exact-cap", "LAW", "last fitting record"),
              "Cannot measure exact-cap record size");
        const auto fittingBytes = fs::file_size(probeRoot / "laws" / encoded("exact-cap"));
        check(fittingBytes < DurableLogger::kMaxFileBytes,
              "Exact-cap record unexpectedly exceeds file budget");
        const auto openCapRoot = sandbox.root / "capacity-open";
        fs::create_directories(openCapRoot / "laws");
        const auto openCapPath = openCapRoot / "laws" / encoded("exact-cap");
        { std::ofstream out(openCapPath, std::ios::binary);
          check(out.good(), "Cannot seed open-stream cap"); }
        fs::resize_file(openCapPath, DurableLogger::kMaxFileBytes - fittingBytes);
        DurableLogger openCapped("laws", openCapRoot);
        check(openCapped.log("exact-cap", "LAW", "last fitting record"),
              "Last record did not fit exactly to file cap");
        check(fs::file_size(openCapPath) == DurableLogger::kMaxFileBytes,
              "Last fitting record did not reach exact cap");
        check(!openCapped.log("exact-cap", "LAW", "must be rejected"),
              "Open stream accepted record after reaching exact cap");
        check(fs::file_size(openCapPath) == DurableLogger::kMaxFileBytes,
              "Rejected open-stream append modified capped history");

        // Identity contract before routing migration: author-facing Law IDs
        // are reusable, but independent Laws must not share a history lineage.
        Law lineageA("History lineage A");
        Law lineageB("History lineage B");
        lineageA.setLawIdentifier("same-author-facing-id");
        lineageB.setLawIdentifier("same-author-facing-id");
        const std::string lineageId = lineageA.durableHistoryLineage();
        check(!lineageId.empty() && lineageId != lineageB.durableHistoryLineage(),
              "Independent Laws with identical authored IDs share a history lineage");
        const auto serializedLineage = lineageA.toJson();
        check(serializedLineage.at("durableHistoryLineage") == lineageId,
              "Law save omitted its durable-history lineage");
        auto restoredLineage = Law::fromJson(serializedLineage);
        check(restoredLineage->durableHistoryLineage() == lineageId,
              "Law load changed a persisted history lineage");
        lineageA.setLawIdentifier("renamed-author-facing-id");
        check(lineageA.durableHistoryLineage() == lineageId,
              "Renaming a Law split its durable-history lineage");
        auto forkedLineage = Law::fromJson(serializedLineage);
        forkedLineage->forkDurableHistoryLineage();
        check(forkedLineage->durableHistoryLineage() != lineageId &&
              forkedLineage->getIdentifier() == "same-author-facing-id",
              "Explicit Law fork retained its source history or changed authored ID");
        check(forkedLineage->toJson().at("durableHistoryLineage") ==
                  forkedLineage->durableHistoryLineage(),
              "Explicitly forked lineage did not persist");
        auto legacySave = serializedLineage;
        legacySave.erase("durableHistoryLineage");
        auto legacyRestored = Law::fromJson(legacySave);
        check(legacyRestored->durableHistoryLineage() != lineageId,
              "Legacy save silently claimed an existing lineage");
        check(legacyRestored->toJson().contains("durableHistoryLineage"),
              "Legacy save did not acquire a persistable new lineage");
        auto malformedLineage = serializedLineage;
        malformedLineage["durableHistoryLineage"] = "../untrusted";
        bool rejectedMalformed = false;
        try { (void)Law::fromJson(malformedLineage); }
        catch (const std::invalid_argument&) { rejectedMalformed = true; }
        check(rejectedMalformed, "Malformed persisted lineage was accepted");

        // Engine-owned First Movers are intentionally NOT in the authored
        // "laws" array. Their history identity still must survive a world
        // save/load; restoring it must not deserialize engine authority.
        {
            LawManager engineManager;
            auto engineLaw = std::make_shared<FirstMoverLaw>("Engine Law");
            engineLaw->setLawIdentifier("engine-stable-law");
            engineManager.add(engineLaw);
            const std::string engineLineage = engineLaw->durableHistoryLineage();
            const auto savedEngine = engineManager.toJson();
            check(savedEngine.at("laws").empty() &&
                  savedEngine.at("firstMoverHistoryLineages").at("engine-stable-law") ==
                      engineLineage,
                  "Engine-owned First Mover history was not saved separately");
            engineLaw->forkDurableHistoryLineage();
            check(engineLaw->durableHistoryLineage() != engineLineage,
                  "First Mover replay setup failed to change lineage");
            engineManager.loadFromJson(savedEngine);
            check(engineLaw->durableHistoryLineage() == engineLineage &&
                  engineLaw->isFirstMover(),
                  "First Mover load fragmented history or lost engine ownership");
            auto invalidEngine = savedEngine;
            invalidEngine["firstMoverHistoryLineages"]["engine-stable-law"] =
                "../invalid";
            bool rejectedEngine = false;
            try { engineManager.loadFromJson(invalidEngine); }
            catch (const std::invalid_argument&) { rejectedEngine = true; }
            check(rejectedEngine && engineLaw->durableHistoryLineage() == engineLineage,
                  "Malformed First Mover lineage changed live world before rejection");

            // The engine-owned Law object survives world changes. A legacy
            // world has no saved lineage: it must not inherit the previous
            // world's history merely because the same engine Law survives.
            auto legacyEngine = savedEngine;
            legacyEngine.erase("firstMoverHistoryLineages");
            engineManager.loadFromJson(legacyEngine);
            const std::string legacyLineage = engineLaw->durableHistoryLineage();
            check(legacyLineage != engineLineage && engineLaw->isFirstMover(),
                  "Legacy world inherited the previous First Mover history");
            check(engineManager.toJson().at("firstMoverHistoryLineages")
                      .at("engine-stable-law") == legacyLineage,
                  "Fresh legacy First Mover lineage did not persist on save");
            engineManager.loadFromJson(legacyEngine);
            check(engineLaw->durableHistoryLineage() != legacyLineage,
                  "Two independent legacy loads reused an unpersisted history");

            // Partial maps must not preserve stale lineages for absent Laws.
            auto missingEngine = savedEngine;
            missingEngine["firstMoverHistoryLineages"].erase("engine-stable-law");
            const std::string previousLineage = engineLaw->durableHistoryLineage();
            engineManager.loadFromJson(missingEngine);
            check(engineLaw->durableHistoryLineage() != previousLineage,
                  "Missing First Mover lineage entry inherited stale history");

            // The previous world's live First Mover lineage is not a member
            // of the incoming world's persisted history namespace. It must be
            // rebound before authored Law registration checks uniqueness.
            const std::string staleLiveFirstMoverLineage =
                engineLaw->durableHistoryLineage();
            check(staleLiveFirstMoverLineage != engineLineage,
                  "Cross-world registration witness needs distinct histories");
            auto incomingWorld = savedEngine;
            Law incomingAuthored("Incoming world authored Law");
            incomingAuthored.setLawIdentifier("incoming-world-authored");
            auto incomingLawJson = incomingAuthored.toJson();
            incomingLawJson["durableHistoryLineage"] = staleLiveFirstMoverLineage;
            incomingWorld["laws"].push_back(incomingLawJson);
            engineManager.loadFromJson(incomingWorld);
            check(engineLaw->durableHistoryLineage() == engineLineage &&
                  engineManager.find("incoming-world-authored") != nullptr &&
                  engineManager.find("incoming-world-authored")->durableHistoryLineage() ==
                      staleLiveFirstMoverLineage,
                  "Stale live First Mover lineage blocked a valid incoming world");
        }

        // Reject collisions across authored Laws and engine-owned First Movers
        // before replace-all can discard any live Law or change its history.
        {
            LawManager collisionManager;
            auto authored = std::make_shared<Law>("Persisted collision witness");
            authored->setLawIdentifier("collision-authored");
            collisionManager.add(authored);
            auto engine = std::make_shared<FirstMoverLaw>("Engine collision witness");
            engine->setLawIdentifier("collision-engine");
            collisionManager.add(engine);
            const auto original = collisionManager.toJson();
            check(original.at("laws").size() == 1,
                  "Collision witness did not serialize its authored Law");
            const std::string authoredHistory = authored->durableHistoryLineage();
            const std::string engineHistory = engine->durableHistoryLineage();
            const auto expectRejectedWithoutMutation = [&](const nlohmann::json& candidate,
                                                           const std::string& why) {
                bool rejected = false;
                try { collisionManager.loadFromJson(candidate); }
                catch (const std::invalid_argument&) { rejected = true; }
                check(rejected && collisionManager.find("collision-authored") == authored.get() &&
                      collisionManager.find("collision-engine") == engine.get() &&
                      authored->durableHistoryLineage() == authoredHistory &&
                      engine->durableHistoryLineage() == engineHistory,
                      why);
            };
            auto duplicateAuthored = original;
            auto copied = duplicateAuthored["laws"][0];
            copied["id"] = "collision-authored-copy";
            duplicateAuthored["laws"].push_back(copied);
            expectRejectedWithoutMutation(duplicateAuthored,
                "Duplicate authored Law histories were accepted or mutated live state");
            auto crossKind = original;
            crossKind["laws"][0]["durableHistoryLineage"] = engineHistory;
            expectRejectedWithoutMutation(crossKind,
                "Authored Law claimed engine First Mover history or mutated live state");
            auto malformedAuthored = original;
            malformedAuthored["laws"][0]["durableHistoryLineage"] = "../invalid";
            expectRejectedWithoutMutation(malformedAuthored,
                "Malformed authored history was rejected only after live mutation");

            // Distinct histories are not enough if two authored Laws claim
            // the same authored identifier: add() would silently drop one.
            auto duplicateIdWorld = original;
            auto independentLaw = std::make_shared<Law>("Independent history, duplicate id");
            auto duplicateIdLaw = independentLaw->toJson();
            duplicateIdLaw["id"] = "collision-authored";
            duplicateIdWorld["laws"].push_back(duplicateIdLaw);
            expectRejectedWithoutMutation(duplicateIdWorld,
                "Duplicate authored identifier silently discarded a Law");

            // Authored JSON cannot shadow an engine-owned First Mover even
            // when its lineage is different from the First Mover's.
            auto shadowedFirstMover = original;
            shadowedFirstMover["laws"][0]["id"] = "collision-engine";
            expectRejectedWithoutMutation(shadowedFirstMover,
                "Authored identifier shadowed an engine-owned First Mover");

            auto malformedAuthoredId = original;
            malformedAuthoredId["laws"][0]["id"] = 42;
            expectRejectedWithoutMutation(malformedAuthoredId,
                "Malformed authored identifier mutated live world before rejection");

            // Persisted preflight alone is insufficient: a caller can restore
            // one Law, rename its authored ID, then register it beside its
            // source without passing through loadFromJson(). Fail closed and
            // require an explicit history fork before admitting the copy.
            auto liveCopy = Law::fromJson(authored->toJson());
            liveCopy->setLawIdentifier("collision-live-copy");
            bool rejectedLiveCopy = false;
            try { collisionManager.add(liveCopy); }
            catch (const std::invalid_argument&) { rejectedLiveCopy = true; }
            check(rejectedLiveCopy && collisionManager.find("collision-live-copy") == nullptr &&
                  collisionManager.find("collision-authored") == authored.get() &&
                  authored->durableHistoryLineage() == authoredHistory,
                  "Live registration silently admitted a copied Law history");
            liveCopy->forkDurableHistoryLineage();
            collisionManager.add(liveCopy);
            check(collisionManager.find("collision-live-copy") == liveCopy.get() &&
                  liveCopy->durableHistoryLineage() != authoredHistory,
                  "Explicit history fork did not permit independent registration");
        }

        // Exercise the public LawAuditLogger bridge with muted aggregate logging.
        auto& lawLogger = ECA::LawAuditLogger::instance();
        lawLogger.setLevel(ECA::LogLevel::Off);
        check(lawLogger.logForLaw("integration", "LAW", "durable with aggregate Off"),
              "LawAuditLogger did not append with aggregate Off");
        // The Applied path shares one JSON payload between aggregate and
        // per-Law logging. Exercise the real Law action with aggregate Off,
        // then verify the durable record retains its result and node trace.
        Object appliedAuthor;
        appliedAuthor.setObjectID("durable-applied-author");
        Object appliedTarget;
        appliedTarget.setObjectID("durable-applied-target");
        appliedTarget.setDynamicProperty("x", PropertyValue(10.0));
        appliedTarget.setDynamicProperty("y", PropertyValue(0.0));
        Law appliedLaw("Durable applied payload", std::vector<Singular*>{&appliedAuthor});
        appliedLaw.setLawIdentifier("durable-applied-payload");
        appliedLaw.setScope(Law::Scope::Subject);
        appliedLaw.setConditionModel(
            ConditionNode::comparePaths("x", ConditionNode::Op::Gt, "y"));
        appliedLaw.setActionModel(
            ActionNode::set("durable-applied-sentinel", PropertyValue(1.0)));
        check(appliedLaw.applyTo(appliedTarget) == Law::ApplicationResult::Applied,
              "Real Law Applied witness did not execute");
        const auto appliedPath = sandbox.root / "logs" / "laws" /
                                 encoded(appliedLaw.durableHistoryLineage());
        const auto appliedEntries = readEntries(appliedPath);
        check(appliedEntries.size() == 1,
              "Applied path did not persist exactly one per-Law event");
        const auto& appliedEntry = appliedEntries.front();
        check(appliedEntry.at("identity") == appliedLaw.durableHistoryLineage() &&
              appliedEntry.at("type") == "LAW" &&
              appliedEntry.at("message").get<std::string>().find("SUCCESS") != std::string::npos,
              "Applied durable event identity/type/success message changed");
        const auto& appliedDetails = appliedEntry.at("details");
        check(appliedDetails.at("lawId") == "durable-applied-payload" &&
              appliedDetails.at("targetId") == "durable-applied-target" &&
              appliedDetails.at("result") == "Applied" &&
              appliedDetails.at("changed") == true &&
              appliedDetails.at("nodes").is_array() &&
              !appliedDetails.at("nodes").empty() &&
              appliedDetails.at("nodes")[0].at("wrote") == true,
              "Applied durable event lost its shared node-level payload");
        // A second Law with the same authored ID must write a different file.
        // Renaming the first Law must continue its original lineage.
        Law otherApplied("Independent same-ID Law", std::vector<Singular*>{&appliedAuthor});
        otherApplied.setLawIdentifier("durable-applied-payload");
        otherApplied.setScope(Law::Scope::Subject);
        otherApplied.setConditionModel(
            ConditionNode::comparePaths("x", ConditionNode::Op::Gt, "y"));
        otherApplied.setActionModel(
            ActionNode::set("other-durable-sentinel", PropertyValue(2.0)));
        check(otherApplied.applyTo(appliedTarget) == Law::ApplicationResult::Applied,
              "Independent same-ID Law did not apply");
        const auto otherPath = sandbox.root / "logs" / "laws" /
                               encoded(otherApplied.durableHistoryLineage());
        check(otherPath != appliedPath && readEntries(otherPath).size() == 1,
              "Same authored Law ID mixed two distinct durable histories");
        appliedLaw.setLawIdentifier("durable-applied-renamed");
        check(appliedLaw.applyTo(appliedTarget) == Law::ApplicationResult::Applied,
              "Renamed Law did not apply");
        const auto renamedEntries = readEntries(appliedPath);
        check(renamedEntries.size() == 2 &&
              renamedEntries[1].at("identity") == appliedLaw.durableHistoryLineage() &&
              renamedEntries[1].at("details").at("lawId") == "durable-applied-renamed",
              "Law rename fragmented its durable history");
        // Existing identifier-keyed files are legacy evidence. Never silently
        // append to, truncate, or claim them as lineage-scoped history.
        const auto legacyPath = sandbox.root / "logs" / "laws" /
                                encoded("durable-applied-payload");
        check(!fs::exists(legacyPath),
              "Lineage routing unexpectedly created a legacy identifier file");
        { DurableLogger legacy("laws", sandbox.root / "logs");
          check(legacy.log("durable-applied-payload", "LAW", "legacy sentinel"),
                "Cannot seed legacy history witness"); }
        check(readEntries(legacyPath).size() == 1 &&
              readEntries(appliedPath).size() == 2,
              "Legacy history was silently attributed to a new lineage");

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
        const std::string refusalLineage = refusalLaw.durableHistoryLineage();
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

        // Controlled same-Law diagnostic: compare successful append+flush above
        // with an otherwise identical refusal whose per-Law file is already
        // at its non-destructive 8 MiB cap. The cap is deliberately exercised
        // ONLY in this test; rejected appends are not a production optimization.
        // This is not a pure I/O subtraction: the capped path still checks the
        // filesystem and returns before JSON serialization, timestamp and flush.
        const std::string noAppendId = "durable-refusal-noappend";
        Law cappedRefusalLaw("Capped refusal control", std::vector<Singular*>{&refusalAuthor});
        cappedRefusalLaw.setLawIdentifier(noAppendId);
        cappedRefusalLaw.setScope(Law::Scope::Subject);
        cappedRefusalLaw.setConditionModel(ConditionNode::identity("never-matching-target"));
        cappedRefusalLaw.setActionModel(ActionNode::set("unused", PropertyValue(1.0)));
        const auto noAppendPath = sandbox.root / "logs" / "laws" /
                                  encoded(cappedRefusalLaw.durableHistoryLineage());
        fs::create_directories(noAppendPath.parent_path());
        {
            std::ofstream seed(noAppendPath, std::ios::binary);
            check(seed.good(), "Cannot seed refusal no-append diagnostic");
        }
        fs::resize_file(noAppendPath, DurableLogger::kMaxFileBytes);
        const auto unconfirmedBefore = lawLogger.unconfirmedDurableAppendCount();
        // Prime the first failure warning outside the timed region.
        check(cappedRefusalLaw.applyTo(refusalTarget) == Law::ApplicationResult::ConditionsFailed,
              "Capped-file control did not exercise the same Law refusal");
        const auto noAppendStart = std::chrono::steady_clock::now();
        for (int i = 0; i < kRealRefusals; ++i) {
            check(cappedRefusalLaw.applyTo(refusalTarget) == Law::ApplicationResult::ConditionsFailed,
                  "Capped-file control unexpectedly changed Law result");
        }
        const auto noAppendStop = std::chrono::steady_clock::now();
        check(lawLogger.unconfirmedDurableAppendCount() ==
                  unconfirmedBefore + kRealRefusals + 1,
              "Capped Law refusals did not increment unconfirmed-append telemetry");
        const double noAppendMicros = std::chrono::duration<double, std::micro>(
            noAppendStop - noAppendStart).count() / kRealRefusals;
        check(fs::file_size(noAppendPath) == DurableLogger::kMaxFileBytes,
              "Capped-file diagnostic modified historical bytes");
        std::cout << std::fixed << std::setprecision(2)
                  << "durable logging same-Law append-cost diagnostic: "
                  << realMicros << " us/refusal with confirmed append+flush, "
                  << noAppendMicros << " us/refusal with pre-capped file ("
                  << kRealRefusals << " calls each; test-only control, not pure I/O attribution)\n";

        // End-to-end tick diagnostic: a connected WhileTrue law starts with
        // true x > y candidates, then y changes. Rete's x-filter may retain
        // stale candidates; LawManager must recheck and record real refusals.
        // Report observed frequency and wall time, not an invented frame budget.
        std::size_t tickRefusals = 0;
        std::string tickLineage;
        constexpr int kTickTargets = 32;
        constexpr int kTickSamples = 8;
        double tickMs = 0.0;
        double tickBaselineMs = 0.0;
        double tickCappedMs = 0.0;
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
                tickLineage = tickLaw->durableHistoryLineage();
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
            // Replay the same connected false-condition path with an already
            // capped per-Law file. This is a measurement control ONLY:
            // all 256 appends are intentionally unconfirmed and must be
            // observable. It is not an acceptable production suppression mode.
            const std::string cappedTickId = "tick-refusal-noappend";
            fs::path cappedTickPath;
            for (const auto& target : tickTargets) {
                target->setDynamicProperty("y", PropertyValue(0.0));
                target->setDynamicProperty("tick-sentinel", PropertyValue(0.0));
            }
            {
                LawManager cappedManager;
                cappedManager.connectToEventBus();
                auto cappedLaw = cappedManager.createLaw(
                    "Durable tick capped replay", cappedTickId, {&tickAuthor});
                check(static_cast<bool>(cappedLaw), "Capped tick Law registration failed");
                cappedTickPath = sandbox.root / "logs" / "laws" /
                                 encoded(cappedLaw->durableHistoryLineage());
                fs::create_directories(cappedTickPath.parent_path());
                { std::ofstream seed(cappedTickPath, std::ios::binary);
                  check(seed.good(), "Cannot seed capped tick replay"); }
                fs::resize_file(cappedTickPath, DurableLogger::kMaxFileBytes);
                cappedLaw->setActivation(Law::Activation::WhileTrue);
                cappedLaw->setConditionModel(
                    ConditionNode::comparePaths("x", ConditionNode::Op::Gt, "y"));
                cappedLaw->setActionModel(
                    ActionNode::set("tick-sentinel", PropertyValue(1.0)));
                for (const auto& target : tickTargets) cappedLaw->addTarget(*target);
                cappedManager.tick(); // seed the same initially true candidates
                for (const auto& target : tickTargets)
                    target->setDynamicProperty("y", PropertyValue(20.0));
                const auto unconfirmedBeforeTick = lawLogger.unconfirmedDurableAppendCount();
                const auto cappedTickStart = std::chrono::steady_clock::now();
                for (int i = 0; i < kTickSamples; ++i) cappedManager.tick();
                const auto cappedTickStop = std::chrono::steady_clock::now();
                tickCappedMs = std::chrono::duration<double, std::milli>(
                    cappedTickStop - cappedTickStart).count() / kTickSamples;
                check(lawLogger.unconfirmedDurableAppendCount() ==
                          unconfirmedBeforeTick + kTickTargets * kTickSamples,
                      "Capped tick replay did not account for every unconfirmed refusal");
            }
            check(fs::file_size(cappedTickPath) == DurableLogger::kMaxFileBytes,
                  "Capped tick replay modified historical log bytes");
            Universe::instance().setProvider(nullptr);
        }
        const auto tickLog = sandbox.root / "logs" / "laws" /
                             encoded(tickLineage);
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
        std::cout << std::fixed << std::setprecision(3)
                  << "durable logging replayed false-condition LawManager::tick diagnostic: "
                  << tickMs << " ms/tick confirmed append+flush vs "
                  << tickCappedMs << " ms/tick pre-capped unconfirmed ("
                  << kTickSamples << " ticks x " << kTickTargets
                  << " candidates; separate manager replay, not pure I/O attribution)\n";
        lawLogger.shutdown();
        auto integrated = readEntries(sandbox.root / "logs" / "laws" / encoded("integration"));
        check(readEntries(sandbox.root / "logs" / "laws" / encoded("bench-law")).size() == kHotEvents,
              "High-frequency durable event count mismatch");
        check(readEntries(sandbox.root / "logs" / "laws" / encoded(refusalLineage)).size() == kRealRefusals,
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
