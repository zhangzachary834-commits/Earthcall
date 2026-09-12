#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

int main() {
    std::cout << "Starting ReteNetwork unit test..." << std::endl;
    ReteNetwork rete;

    // 1. Assert a state fact
    auto fact1 = std::make_shared<ReteFact>();
    fact1->id = "fact-1";
    fact1->subjectId = "obj-1";
    fact1->attribute = "color";
    fact1->isState = true;
    fact1->value = "red";
    fact1->dirty = false;

    rete.assertFact(fact1);
    assert(rete.facts().size() == 1);
    std::cout << "Assert fact ok." << std::endl;

    // 2. markFactDirty
    bool marked = rete.markFactDirty("obj-1", "color");
    assert(marked);
    assert(rete.hasDirtyFacts());
    std::cout << "markFactDirty ok." << std::endl;

    // 3. Non-existent fact dirty
    bool marked2 = rete.markFactDirty("obj-2", "color");
    assert(!marked2);

    // 4. Retract fact
    bool retracted = rete.retractFact("fact-1");
    assert(retracted);
    assert(rete.facts().empty());
    std::cout << "retractFact ok." << std::endl;

    // 5. Verify markFactDirty after retract
    bool marked3 = rete.markFactDirty("obj-1", "color");
    assert(!marked3);

    // 6. Stress test: 10,000 facts
    std::cout << "Stress testing 10,000 facts assertion and dirty marking..." << std::endl;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        auto f = std::make_shared<ReteFact>();
        f->id = "fact-" + std::to_string(i + 10);
        f->subjectId = "obj-" + std::to_string(i);
        f->attribute = "position";
        f->isState = true;
        f->value = 1.0;
        f->dirty = false;
        rete.assertFact(f);
    }
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "Assert 10,000 facts took: "
              << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms" << std::endl;

    // 7. Mark dirty 1,000 facts
    auto t2 = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        bool m = rete.markFactDirty("obj-" + std::to_string(i), "position");
        assert(m);
    }
    auto t3 = std::chrono::steady_clock::now();
    std::cout << "markFactDirty 1,000 times took: "
              << std::chrono::duration<double, std::milli>(t3 - t2).count() << " ms" << std::endl;

    // 8. Retract 1,000 facts
    auto t4 = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        bool r = rete.retractFact("fact-" + std::to_string(i + 10));
        assert(r);
    }
    auto t5 = std::chrono::steady_clock::now();
    std::cout << "retractFact 1,000 times took: "
              << std::chrono::duration<double, std::milli>(t5 - t4).count() << " ms" << std::endl;

    std::cout << "All Rete tests PASSED successfully!" << std::endl;
    return 0;
}
