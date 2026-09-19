// Dramatic A/B witness for the Law-Direct rung.
//
// This intentionally magnifies the exact historical Chess pathology:
//   * 32 category members (a chess-set-sized bearer population)
//   * many Laws sharing one Related(instance-of, category) conjunct
//   * vocabulary already narrows to the same 32 beings
//   * each member has a dense irrelevant relation neighborhood
//
// SlowAdapter is ON in BOTH arms. The only variable is LawDirect OFF vs ON.
// OFF therefore reproduces the immediately-pre-Direct executor in the same
// binary / process / machine. ON is expected to pay route proof once and then
// evaluate only the residual live property condition.
//
// PRE-DIRECT hot work is O(P * L * M * D) relation-edge examination.
// LAW-DIRECT hot work is O(P * L * M), with D removed from the hot path.
//
// The timing ratio is deliberately not the only oracle. We also count calls to
// the relation graph and the total relation fan-out returned. Correct Direct
// should make the work reduction enormous even on a noisy CI runner.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;

constexpr int kMembers = 32;
constexpr int kNoiseCategories = 128;
constexpr int kLaws = 128;
constexpr int kPulses = 16;
constexpr int kRounds = 3;

struct Counters {
    std::uint64_t relationQueries = 0;
    std::uint64_t relationFanoutReturned = 0;
};

struct Batch {
    double ms = 0.0;
    std::uint64_t queries = 0;
    std::uint64_t fanout = 0;
    std::size_t records = 0;
};

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

void fail(bool condition, const char* message, int& failures) {
    if (condition) {
        std::printf("  ok: %s\n", message);
    } else {
        ++failures;
        std::printf("  FAILED: %s\n", message);
    }
}
}

int main() {
    int failures = 0;

    Object author;
    author.setObjectID("direct-stress-author");
    Object hotCategory;
    hotCategory.setObjectID("category.direct.hot");

    std::vector<std::unique_ptr<Object>> membersOwned;
    std::vector<std::unique_ptr<Object>> noiseOwned;
    std::vector<Singular*> population{&author, &hotCategory};

    membersOwned.reserve(kMembers);
    for (int i = 0; i < kMembers; ++i) {
        auto member = std::make_unique<Object>();
        member->setObjectID("direct.member." + std::to_string(i));
        member->setDynamicProperty("directGate", PropertyValue(1.0));
        population.push_back(member.get());
        membersOwned.push_back(std::move(member));
    }

    noiseOwned.reserve(kNoiseCategories);
    for (int i = 0; i < kNoiseCategories; ++i) {
        auto noise = std::make_unique<Object>();
        noise->setObjectID("category.noise." + std::to_string(i));
        population.push_back(noise.get());
        noiseOwned.push_back(std::move(noise));
    }

    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.insert(out.end(), population.begin(), population.end());
    });

    RelationManager graph;
    // Put the desired category edge LAST in every member's endpoint list so a
    // full Related check must traverse the whole irrelevant neighborhood.
    for (const auto& member : membersOwned) {
        for (const auto& noise : noiseOwned) {
            graph.add(std::make_shared<Relation>(
                "instance-of", *member, *noise, true));
        }
        graph.add(std::make_shared<Relation>(
            "instance-of", *member, hotCategory, true));
    }

    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& relation : graph.getAll()) {
            if (relation) out.push_back(relation.get());
        }
    });

    Counters counters;
    Universe::instance().setRelationsInvolvingProvider(
        [&](const Singular& being, std::vector<Relation*>& out) {
            ++counters.relationQueries;
            graph.relationsInvolving(being, out);
            counters.relationFanoutReturned += out.size();
        });
    Universe::instance().setRelationGenerationProvider(
        [&]() { return graph.generation(); });
    Universe::instance().setClock(100.0, 1.0 / 60.0);

    LawManager manager;
    manager.connectToEventBus();
    manager.setUseSlowAdapter(true);
    manager.setUseLawDirect(false);

    std::vector<std::shared_ptr<Law>> laws;
    laws.reserve(kLaws);
    for (int i = 0; i < kLaws; ++i) {
        auto law = manager.createLaw(
            "direct-stress-law-" + std::to_string(i), {&author});
        law->setActivation(Law::Activation::OnEvent);
        law->setScope(Law::Scope::Everyone);
        law->setConditionModel(ConditionNode::all({
            ConditionNode::related("instance-of", "category.direct.hot"),
            ConditionNode::compare(
                "directGate", ConditionNode::Op::Gt, PropertyValue(0.5))
        }));
        manager.bindTrigger(law->getIdentifier(), "direct-stress-pulse");
        laws.push_back(law);
    }

    // Independent-clock discovery. All 128 Laws share ONE RouteKey.
    double adapterWall = 0.0;
    manager.serviceSlowAdapterClock(adapterWall); // prime
    for (int i = 0; i < 4; ++i) {
        adapterWall += manager.slowAdapterClockPeriodSeconds();
        manager.serviceSlowAdapterClock(adapterWall);
    }
    fail(manager.slowAdapter().roadsKnown() == 1,
         "all stress Laws share one retained category road", failures);
    fail(manager.slowAdapter().ready(laws.front()->getIdentifier()),
         "the shared road is current before measurement", failures);

    const auto forceTiers = [&](bool direct) {
        manager.setUseLawDirect(direct);
        std::size_t directCount = 0, adapterCount = 0, vocabularyCount = 0;
        const auto t0 = Clock::now();
        for (const auto& law : laws) {
            const std::string tier = manager.candidateTierFor(*law);
            if (tier == "law-direct") ++directCount;
            else if (tier == "adapter-road") ++adapterCount;
            else if (tier == "vocabulary") ++vocabularyCount;
        }
        const auto t1 = Clock::now();
        const double promotionMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
        return std::array<double,4>{
            promotionMs,
            static_cast<double>(directCount),
            static_cast<double>(adapterCount),
            static_cast<double>(vocabularyCount)
        };
    };

    const std::size_t expectedRecords =
        static_cast<std::size_t>(kLaws) * kMembers * kPulses;

    const auto runBatch = [&]() {
        counters = {};
        std::size_t records = 0;
        const auto t0 = Clock::now();
        for (int pulse = 0; pulse < kPulses; ++pulse) {
            ECA::Event e{"direct-stress-pulse", &author, nullptr, 0};
            Core::EventBus::instance().publish(e);
            records += manager.tick().size();
        }
        const auto t1 = Clock::now();
        return Batch{
            std::chrono::duration<double, std::milli>(t1 - t0).count(),
            counters.relationQueries,
            counters.relationFanoutReturned,
            records
        };
    };

    // Seed Rete/vocabulary before either timed arm.
    forceTiers(false);
    {
        ECA::Event e{"direct-stress-pulse", &author, nullptr, 0};
        Core::EventBus::instance().publish(e);
        manager.tick();
    }

    std::vector<double> preMs, directMs, promotionMs;
    std::uint64_t preQueries = 0, directQueries = 0;
    std::uint64_t preFanout = 0, directFanout = 0;

    for (int round = 0; round < kRounds; ++round) {
        // Alternate ordering so thermal/frequency drift cannot favor one arm.
        for (int phase = 0; phase < 2; ++phase) {
            const bool direct = ((round + phase) % 2) != 0;
            const auto tiers = forceTiers(direct);

            if (direct) {
                fail(static_cast<int>(tiers[1]) == kLaws,
                     "every stress Law graduates to Law-Direct", failures);
                promotionMs.push_back(tiers[0]);
            } else {
                // Equal-width AdapterRoad is rejected in pre-Direct code, so
                // this reproduces Chess's old Vocabulary=32 / Road=32 case.
                fail(static_cast<int>(tiers[3]) == kLaws,
                     "Direct OFF reproduces the equal-width vocabulary tier", failures);
            }

            // One untimed pulse settles the just-cleared route decision.
            ECA::Event warm{"direct-stress-pulse", &author, nullptr, 0};
            Core::EventBus::instance().publish(warm);
            manager.tick();

            const Batch batch = runBatch();
            fail(batch.records == expectedRecords,
                 "both arms produce exactly the same lawful applications", failures);

            if (direct) {
                directMs.push_back(batch.ms);
                directQueries += batch.queries;
                directFanout += batch.fanout;
            } else {
                preMs.push_back(batch.ms);
                preQueries += batch.queries;
                preFanout += batch.fanout;
            }
        }
    }

    const double preMedian = median(preMs);
    const double directMedian = median(directMs);
    const double speedup = directMedian > 0.0 ? preMedian / directMedian : 0.0;
    const double promotionMedian = median(promotionMs);
    const double savedPerBatch = preMedian - directMedian;
    const double savedPerPulse = savedPerBatch / kPulses;
    const double amortizePulses =
        savedPerPulse > 0.0 ? promotionMedian / savedPerPulse : 1e30;

    std::printf(
        "LAW_DIRECT_STRESS shape=chess-amplified members=%d laws=%d "
        "irrelevant_edges_per_member=%d pulses_per_batch=%d rounds=%d\n",
        kMembers, kLaws, kNoiseCategories, kPulses, kRounds);
    std::printf(
        "LAW_DIRECT_ARM mode=pre-direct median_ms=%.3f relation_queries=%llu "
        "relation_fanout_returned=%llu records_per_batch=%zu\n",
        preMedian,
        static_cast<unsigned long long>(preQueries / kRounds),
        static_cast<unsigned long long>(preFanout / kRounds),
        expectedRecords);
    std::printf(
        "LAW_DIRECT_ARM mode=direct median_ms=%.3f relation_queries=%llu "
        "relation_fanout_returned=%llu records_per_batch=%zu "
        "promotion_median_ms=%.3f\n",
        directMedian,
        static_cast<unsigned long long>(directQueries / kRounds),
        static_cast<unsigned long long>(directFanout / kRounds),
        expectedRecords, promotionMedian);
    std::printf(
        "LAW_DIRECT_COMPARE speedup=%.2fx saved_ms_per_batch=%.3f "
        "saved_ms_per_pulse=%.3f promotion_amortizes_after_pulses=%.2f "
        "theoretical_removed_relation_degree=%dx\n",
        speedup, savedPerBatch, savedPerPulse, amortizePulses,
        kNoiseCategories + 1);

    // Work counters are the deterministic oracle; timing is an intentionally
    // dramatic secondary oracle. These thresholds are loose relative to the
    // constructed asymptotic gap but still fail a ceremonial/no-op Direct tier.
    fail(preQueries > 100000,
         "pre-Direct performs a six-figure number of graph queries", failures);
    fail(directQueries * 50 < preQueries,
         "Law-Direct removes at least 98% of hot relation queries", failures);
    fail(directFanout * 50 < preFanout,
         "Law-Direct removes at least 98% of returned relation fan-out", failures);
    fail(speedup >= 2.0,
         "hostile Chess-shaped workload is at least 2x faster with Law-Direct", failures);
    fail(amortizePulses <= 4.0,
         "one-time direct promotion amortizes within four stress pulses", failures);

    Universe::instance().setRelationsInvolvingProvider(nullptr);
    Universe::instance().setRelationProvider(nullptr);
    Universe::instance().setRelationGenerationProvider(nullptr);
    Universe::instance().setProvider(nullptr);

    std::printf("%s\n", failures ? "law_direct_stress_test: FAILURES"
                                  : "law_direct_stress_test: OK");
    return failures ? 1 : 0;
}
