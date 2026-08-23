// The sixth refusal, held to its word — see docs/architecture/ontology/NO_BLACK_BOX.md.
//
// A field a Person cannot address is a field a Person cannot govern. The
// refusal is that every piece of a being's state is REGISTERED as a property
// path; the only exemption is state beneath the Kernel (GPU handles, mutexes,
// fds), and that must be named in a comment rather than silently omitted.
//
// C++ has no reflection, so no test can diff a class's private members against
// its registry — §4's written reason carries that half. What IS mechanically
// checkable is the four ways the promise has actually broken here:
//
//   A. VOCABULARY      a being whose buildProperties() is empty. The sealed
//                      register below is a DEBT LEDGER, not an allowlist.
//                      Entries are expected to leave it. World left it when
//                      it folded into Zone (2026-08-20).
//
//   B. LAZY-BUILD      a registry built twice. Singular builds lazily and sets
//                      _propertiesBuilt itself; a constructor that also calls
//                      buildProperties() registers everything a second time on
//                      first access, and the authoring picker then offers each
//                      path twice. (ForeignChannel.cpp carries the standing
//                      note on why it does NOT call it; foreign_integration_test
//                      guards that one being. This guards all of them.)
//
//   C. WRITABLE MEANS  a setter that accepts a write and does nothing.
//      IT WRITES       `propSetColor` was an empty function for a month: the
//                      property was registered, the picker offered it, the law
//                      wrote it, the write reported success, nothing changed.
//                      Written through Property::setValue — the same generic
//                      door the Law system uses, not the typed setter.
//
//   D. REGISTERED      a governable property the authoring surface never
//      MEANS REACHABLE offers. This is the INVERSE of channel_paths_test,
//                      which walks advertised -> registered. Both directions
//                      are needed and neither implies the other: a path can be
//                      offered and unregistered (activeShapeKind, the bug that
//                      test exists for) or registered and unoffered (governable
//                      in principle, unreachable in practice — this one).
//
// Failures are REPORTED, not asserted, so one gap does not hide the rest.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Core/EventEntity.hpp"
#include "Singularity/Language/Lexeme.hpp"
#include "Singularity/Screen/LawGraphWindow.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checked = 0;

void fail(const std::string& being, const std::string& what, const std::string& why) {
    ++g_failures;
    std::printf("  FAILED  %-22s %-24s %s\n", being.c_str(), what.c_str(), why.c_str());
}

// ---------------------------------------------------------------------------
// The sealed register — a DEBT LEDGER. Every name here is a being that
// registers nothing, with the reason it is still tolerated. Take names OFF.
// ---------------------------------------------------------------------------
struct Sealed {
    const char* being;
    const char* reason;
};

const Sealed kSealedRegister[] = {
    // World folded into Zone 2026-08-20. Empty name is skipped by isSealed;
    // the slot keeps this a valid array so the walk below still compiles.
    {"", "ledger empty — World folded into Zone"},

    // Perspective is NOT probed: Perspective.cpp is empty, so its constructor is
    // declared and never defined. It is an uninstantiable stub, not a being with
    // hidden state — nothing to register until something implements it.
};

bool isSealed(const std::string& being) {
    if (being.empty()) return false;
    for (const Sealed& s : kSealedRegister) {
        if (!s.being || s.being[0] == '\0') continue;
        if (being == s.being) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Check C cannot mechanically tell "the setter clamped my request to the only
// legal value" from "the setter ignored my request", because it does not know
// each property's legal domain. Where a human has established which it is, the
// property is named here with what was measured. Verified by
// scratch/probes/no_black_box_probe.cpp, not by reading the setter.
// ---------------------------------------------------------------------------
struct WriteExemption {
    const char* being;
    const char* property;      // exact name, or a "prefix*" wildcard
    const char* reason;
};

const WriteExemption kWriteExemptions[] = {
    {"Object", "rotation",
     "WRITES, but round-trips lossily: (1,2,3) degrees reads back as "
     "(1.104, 1.945, 3.036). setRotationEulerDegrees wraps, composes into the "
     "transform, and the getter re-derives Euler angles from the matrix. Not a "
     "black box — a law that writes this path does move the object — but a law "
     "that writes THEN reads gets a different number than it wrote."},
    {"Object", "face.*",
     "activeLayer clamps to [0, layers-1], and a probe object has exactly one "
     "layer, so 0 is the only legal value and no perturbation can round-trip. "
     "Measured: setValue refuses outright (returns false) on a face with no "
     "layers at all, which check C already skips."},
};

// "face.*" matches any face path; anything else is an exact name.
bool isWriteExempt(const std::string& being, const std::string& property) {
    for (const WriteExemption& e : kWriteExemptions) {
        if (being != e.being) continue;
        const std::string pattern = e.property;
        if (pattern.size() >= 2 && pattern.back() == '*') {
            if (property.rfind(pattern.substr(0, pattern.size() - 1), 0) == 0) return true;
        } else if (property == pattern) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Check C — write it, read it back.
//
// Two candidate values are tried and ANY exact round-trip passes. That is
// deliberate: a setter is allowed to CLAMP to a valid range (Law::conditionMode
// folds everything that is not 1 down to All; a shape radius may refuse a
// negative), and clamping is honest behaviour, not a black box. What no
// candidate can survive is a setter that ignores the write entirely — the
// propSetColor failure. Read-only properties return false from setValue and
// are skipped here; read-only is a real answer, not a hidden field.
// ---------------------------------------------------------------------------
std::vector<PropertyValue> candidatesFor(const PropertyValue& current) {
    std::vector<PropertyValue> out;
    std::visit([&](auto&& x) {
        using X = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<X, bool>) {
            out.push_back(PropertyValue(!x));
        } else if constexpr (std::is_same_v<X, std::string>) {
            out.push_back(PropertyValue(x + "-probe"));
            out.push_back(PropertyValue(std::string("probe")));
        } else if constexpr (std::is_same_v<X, glm::vec3>) {
            out.push_back(PropertyValue(x + glm::vec3(1.0f, 2.0f, 3.0f)));
            out.push_back(PropertyValue(glm::vec3(1.0f, 1.0f, 1.0f)));
        } else if constexpr (std::is_same_v<X, glm::mat4>) {
            glm::mat4 m = x;
            m[3][0] += 1.0f;
            out.push_back(PropertyValue(m));
        } else if constexpr (std::is_arithmetic_v<X>) {
            out.push_back(PropertyValue(static_cast<X>(x + static_cast<X>(1))));
            out.push_back(PropertyValue(static_cast<X>(1)));
        }
        // Pointer and shared_ptr alternatives are left alone: writing a raw
        // Singular*/Object* through a probe would hand the being an address
        // this test owns and then free it.
    }, current);
    return out;
}

bool sameValue(const PropertyValue& a, const PropertyValue& b) {
    if (a.index() != b.index()) return false;
    return std::visit([&](auto&& x) {
        using X = std::decay_t<decltype(x)>;
        const X& y = std::get<X>(b);
        if constexpr (std::is_same_v<X, glm::vec3>) {
            return glm::all(glm::epsilonEqual(x, y, 1e-4f));
        } else if constexpr (std::is_same_v<X, glm::mat4>) {
            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r)
                    if (std::abs(x[c][r] - y[c][r]) > 1e-4f) return false;
            return true;
        } else if constexpr (std::is_floating_point_v<X>) {
            return std::abs(static_cast<double>(x) - static_cast<double>(y)) < 1e-4;
        } else if constexpr (std::is_same_v<X, std::monostate>) {
            return true;
        } else if constexpr (std::is_same_v<X, std::string> || std::is_arithmetic_v<X> ||
                             std::is_pointer_v<X>) {
            return x == y;
        } else {
            return true;   // shared_ptr payloads: identity compare is not meaningful here
        }
    }, a);
}

// Walk one being through checks A, B and C.
void audit(const std::string& beingName, Singular& being) {
    const std::vector<Property*> properties = being.listProperties();

    // --- A. vocabulary -----------------------------------------------------
    if (properties.empty()) {
        if (!isSealed(beingName)) {
            fail(beingName, "(vocabulary)",
                 "registers no properties and is not on the sealed register — either "
                 "register its state or name it in kSealedRegister with a reason");
        } else {
            std::printf("  sealed  %-22s registers nothing (see kSealedRegister)\n",
                        beingName.c_str());
        }
        return;
    }
    if (isSealed(beingName)) {
        // `telos` is the universal Singular vocabulary (HIERARCHY_OF_JOYS.md),
        // registered on every being. It does not unseal a being whose own
        // state is still {}.
        bool onlyUniversalTelos = properties.size() == 1
            && properties[0] && properties[0]->name() == "telos";
        if (onlyUniversalTelos) {
            std::printf("  sealed  %-22s only universal `telos`; own state still {}\n",
                        beingName.c_str());
        } else {
            fail(beingName, "(sealed register)",
                 "now registers properties — take it OFF kSealedRegister, the ledger is "
                 "meant to shrink");
        }
    }

    // --- B. lazy-build contract -------------------------------------------
    std::set<std::string> seen;
    for (Property* property : properties) {
        if (!seen.insert(property->name()).second) {
            fail(beingName, property->name(),
                 "registered TWICE — a constructor is calling buildProperties() without "
                 "setting _propertiesBuilt, so Singular's lazy build registers it again");
        }
    }

    // --- C. writable means it writes ---------------------------------------
    for (Property* property : properties) {
        const PropertyValue original = property->value();
        if (std::holds_alternative<std::monostate>(original)) continue;

        bool accepted = false;
        bool roundTripped = false;
        for (const PropertyValue& candidate : candidatesFor(original)) {
            if (!property->setValue(candidate)) continue;   // read-only, or type-refused
            accepted = true;
            if (sameValue(property->value(), candidate)) { roundTripped = true; break; }
        }
        property->setValue(original);   // leave the being as we found it
        ++g_checked;

        if (accepted && !roundTripped) {
            if (isWriteExempt(beingName, property->name())) {
                std::printf("  noted   %-22s %-24s clamped or lossy, see kWriteExemptions\n",
                            beingName.c_str(), property->name().c_str());
            } else {
                fail(beingName, property->name(),
                     "setValue() reported success and the value did not change — a law "
                     "writing this path is silently ignored (the propSetColor failure)");
            }
        }
    }
}

// --- D. registered means reachable ----------------------------------------
//
// Only for beings the picker hand-lists. Object and Law are probed live inside
// knownPathOptions() and so cannot drift by construction; Person and the
// Creation channel are typed out by hand, which is where the gap opens.
void auditReachability(const std::string& beingName, Singular& being,
                       const std::set<std::string>& advertised) {
    for (Property* property : being.listProperties()) {
        if (advertised.count(property->name())) continue;
        fail(beingName, property->name(),
             "is registered and governable but the law-authoring picker never offers "
             "it — reachable only by a Person who already knows the path by heart. "
             "Add it to knownPathOptions() in LawGraphWindow.cpp");
    }
}

}  // namespace

int main() {
    // Mirrors channel_paths_test: a context exists before any being that may
    // touch the renderer is constructed.
    if (!glfwInit()) { std::fprintf(stderr, "no_black_box_test: no GLFW\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "no-black-box", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "no_black_box_test: no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    std::printf("no_black_box_test — the sixth refusal (NO_BLACK_BOX.md)\n\n");

    // ---- beings that must have a vocabulary -------------------------------
    {
        Object object;                                   audit("Object", object);
        Law law("no-black-box-probe");                   audit("Law", law);
        Material material("probe");                      audit("Material", material);
        Relation relation;                               audit("Relation", relation);
        Zone zone("probe-zone", "default");              audit("Zone", zone);
        Home home("probe-home", "default");              audit("Home", home);
        Singularity::Language::Lexeme lexeme("probe");   audit("Lexeme", lexeme);
        Core::EventEntity event("probe-happened");       audit("EventEntity", event);
        geom::FieldNode field("probe-field");            audit("FieldNode", field);
        Singularity::Core::CreationChannel channel;      audit("CreationChannel", channel);
        Singularity::Input::LocomotionChannel locomotion; audit("LocomotionChannel", locomotion);
        audit("TransferPolicy", TransferPolicy::instance());

        Soul soul("Prober");
        Body body("humanoid", "default");
        Person person(std::move(soul), std::move(body), "default");
        audit("Person", person);
    }

    // ---- previously sealed beings, walked so the ledger cannot rot --------
    {
        Formation formation;     audit("Formation", formation);
        Soul soul;               audit("Soul", soul);
        Ourverse ourverse;       audit("Ourverse", ourverse);
    }

    // ---- D. reachability from the authoring surface -----------------------
    std::printf("\n  -- registered means reachable (inverse of channel_paths_test) --\n");
    std::set<std::string> advertised;
    for (const Rendering::PathOption& option : Rendering::knownPathOptions()) {
        advertised.insert(option.path);
    }
    {
        Singularity::Core::CreationChannel channel;
        auditReachability("CreationChannel", channel, advertised);

        Singularity::Input::LocomotionChannel locomotion;
        auditReachability("LocomotionChannel", locomotion, advertised);

        Formation formation;
        auditReachability("Formation", formation, advertised);

        Soul soulProbe;
        auditReachability("Soul", soulProbe, advertised);

        Ourverse ourverseReach;
        auditReachability("Ourverse", ourverseReach, advertised);

        Soul named("Prober");
        Body body("humanoid", "default");
        Person person(std::move(named), std::move(body), "default");
        auditReachability("Person", person, advertised);
    }

    std::printf("\nno_black_box_test: %d property writes probed, %d failures\n",
                g_checked, g_failures);

    glfwDestroyWindow(window);
    glfwTerminate();

    if (g_failures > 0) {
        std::printf("no_black_box_test: FAILED — the substrate is holding state where "
                    "no law can reach it\n");
        return 1;
    }
    std::printf("no_black_box_test: ALL OK\n");
    return 0;
}
