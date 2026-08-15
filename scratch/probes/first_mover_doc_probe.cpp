// Probe: does the law JSON printed verbatim in
// docs/architecture/FIRST_MOVER_AUTHORING.md §4d actually load into a working
// law? Not a permanent test — a check that the onboarding doc is true.
//
// Build (from repo root, after the ordinary cmake configure):
//   c++ -std=c++17 -UNDEBUG -Isrc -Iimgui -Ithird_party/flatbuffers/include \
//       -Ilocal_deps/include -Ibuild/_deps/asio-src/asio/include \
//       -Ibuild/_deps/websocketpp-src -DASIO_STANDALONE \
//       scratch/probes/first_mover_doc_probe.cpp \
//       $(find build/CMakeFiles/earthcall_core.dir -name '*.o') -o scratch/probes/fm_probe

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "json.hpp"

#include <cassert>
#include <cstdio>

int main() {
    // ---- verbatim from the doc, §4d (comments stripped; JSON has none) ----
    const char* kDocLaw = R"JSON({
      "id": "law-1",
      "name": "New Law",
      "enabled": true,
      "authority": 0,
      "activation": 0,
      "scope": 0,
      "drives": false,
      "retrigger": 0,
      "conditionMode": "all",
      "authors": ["Player"],
      "conditionSubjects": [],
      "targets": [],
      "conditionModel": {
        "kind": 0,
        "path": "position.y",
        "op": 2,
        "operand": {"t": "double", "v": 0.0}
      },
      "actionModel": {
        "kind": 0,
        "path": "position.y",
        "operand": {"t": "double", "v": 20.0}
      },
      "conditionDescriptions": ["position.y < 0.000000"],
      "actionDescriptions": ["set position.y"],
      "provenance": [
        {"type": "authored-by", "entityA": "law-1", "entityB": "Player",
         "directed": true, "weight": 1.0,
         "events": [{"description": "authored-by", "deltaWeight": 1.0,
                     "timestamp": 1783903356}]}
      ],
      "applicationLog": []
    })JSON";

    const auto j = nlohmann::json::parse(kDocLaw);
    auto law = Law::fromJson(j);
    assert(law && "doc law failed to parse");

    // The doc's claims about what survives fromJson.
    assert(law->getIdentifier() == "law-1");
    assert(law->isEnabled());
    assert(law->activation() == Law::Activation::OnEvent);       // "activation": 0
    assert(law->scope() == Law::Scope::Subject);                 // "scope": 0
    assert(law->retrigger() == Law::Retrigger::Absorb);          // "retrigger": 0
    assert(!law->drives());
    assert(law->conditionModel() && "conditionModel dropped");
    assert(law->actionModel() && "actionModel dropped");
    assert(law->conditionModel()->kind == ConditionNode::Kind::Compare);
    assert(law->conditionModel()->op == ConditionNode::Op::Lt);
    assert(law->actionModel()->kind == ActionNode::Kind::Set);

    // §2b: authors are NOT attached by fromJson — LawManager::loadFromJson
    // reattaches them by identifier. A law loaded alone is Unauthored.
    assert(!law->isAuthored() && "doc §2b wrong: fromJson attached an author");

    // §2a: the authority clamp. kAuthoredCeiling == 0.
    auto forged = j;
    forged["authority"] = 9999;
    forged["id"] = "law-2";
    auto forgedLaw = Law::fromJson(forged);
    assert(forgedLaw->authorityLevel() == 0 && "doc §2a wrong: authority not clamped");

    // §6c: claimLawIdAtLeast only recognises the "law-" prefix, so a
    // non-conforming id does not advance the fresh-id counter.
    auto odd = j;
    odd["id"] = "my-cool-law";
    auto oddLaw = Law::fromJson(odd);
    assert(oddLaw->getIdentifier() == "my-cool-law");
    Law fresh("fresh");
    std::printf("  fresh law minted after 'my-cool-law': %s\n",
                fresh.getIdentifier().c_str());

    std::printf("OK  doc §4d law parses; §2a clamp holds; §2b unauthored holds\n");
    return 0;
}
