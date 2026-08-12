#pragma once

#include "Law.hpp"
#include "ChangeRecorder.hpp"
#include "ConstructedBeing/Object/Automation/Automation.hpp"

#include <memory>
#include <string>
#include <vector>

// How separate laws synthesize into higher laws (LAW_AND_CREATION_SYSTEM.md
// §6; manifesto "How separate laws can be synthesized into higher laws").
// Both paths produce an ordinary Law — authored, serializable, governable —
// with synthesized-from provenance to its constituents.
namespace LawSynthesis {

// The interpretive path: tree algebra. Conditions join under All/Any; actions
// join under Sequence/Parallel. Cheap, exact, and the composition structure
// stays visible in the higher law's text — the vector-composition homology.
// `into` is where the higher law takes its place in the world: registered with
// the manager and bound to the UNION of what wakes its constituents. Omit it
// and you get the algebra alone — a correct law that nothing has been told to
// listen for, which is useful for testing the trees and useless in a world.
std::shared_ptr<Law> compose(const std::string& name,
                             const Law& a, const Law& b,
                             const std::vector<Singular*>& authors,
                             bool allConditions = true,       // false = Any
                             bool sequentialActions = true,   // false = Parallel
                             LawManager* into = nullptr);

// The native path: run both constituent laws on the same referent while the
// ChangeRecorder watches the designated paths, then FIT one model from the
// cumulative trace. This is entire-process capture — the higher law owns a
// single fused model of what the constituents jointly did, with no
// interpretive delegation at runtime. Conditions still join by algebra
// (a demonstration shows change, not criteria).
std::shared_ptr<Law> synthesizeByDemonstration(
    const std::string& name,
    Law& a, Law& b,
    Singular& referent,
    const std::vector<std::string>& watchPaths,
    int steps, float dt,
    const std::vector<Singular*>& authors,
    LawManager* into = nullptr);

// Translate an Automation Clip into a Law with Drive actions.
// Used for migrating legacy animation clips into the native Law system.
// The authors are the first movers answering for the migration: an unauthored
// law cannot fire, so a clip migrated without them arrives inert.
std::shared_ptr<Law> fromAutomationClip(const std::string& name,
                                        const Automation::Clip& clip,
                                        const std::vector<Singular*>& authors);

} // namespace LawSynthesis
