#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include "json.hpp"

class Singular;
class Zone;

// One creation algebra for every birthable Singular.
//
// A "concept" is not a special C++ noun. Any ordinary Singular may be used as
// the prototype/source of another Singular. Set-to-set creation is therefore
// the universal operation:
//
//      source Singular set + authored mappings/bindings -> newborn Singular set
//
// Concrete C++ types still need a mechanical persistence/constructor adapter
// (C++ has no runtime constructor reflection), but those adapters are NOT new
// authoring operations, ActionKinds, or semantic creation systems. They only
// teach this ONE operation how an already-existing Singular kind crosses its
// storage boundary.
namespace SingularSetToSetCreation {

struct Request {
    // The prototype is itself an ordinary Singular. Its runtime kind determines
    // the newborn's runtime kind; it need not be wrapped in an ObjectConcept.
    Singular& prototype;

    // Full input set available to authored mappings. The prototype need not be
    // repeated here. Today the first Law-Forge rung uses author/target directly;
    // later the generic PropertyMapping surface can range over this set.
    std::vector<Singular*> sources;

    Singular* author = nullptr;
    Singular* target = nullptr;
    Zone* destinationZone = nullptr;

    std::string newbornId;    // empty => <prototype>.branch-N
    std::string newbornName;  // optional display/name surface

    // Explicit model-parameter substitutions. No guessing: if `$TARGET` is
    // present and no target was supplied, the caller should refuse.
    std::unordered_map<std::string, std::string> textBindings;
    // Relations require authored endpoints, not an inferred clone of the
    // enduring Relation between the prototype's original participants.
    Singular* endpointA = nullptr;
    Singular* endpointB = nullptr;
};

// Borrowed, operation-local author context beneath the Kernel. A Law arms
// this from its actual authors; nested applications restore the outer context.
// It is not an authority grant or an alternate permission system.
class AuthorScope {
public:
    explicit AuthorScope(const std::vector<Singular*>& authors);
    ~AuthorScope();
    AuthorScope(const AuthorScope&) = delete;
    AuthorScope& operator=(const AuthorScope&) = delete;
private:
    const std::vector<Singular*>* previous;
};

struct Result {
    Singular* newborn = nullptr; // lifetime is retained by the destination root/register
    std::string refusal;

    explicit operator bool() const { return newborn != nullptr; }
};

// Universal creation entrypoint. Kernel policies (Person is not synthesizable,
// Relation requires actual participants, etc.) constrain this operation rather
// than spawning parallel CreatePerson/CreateRelation/CreateLaw mechanisms.
Result derive(const Request& request);

// Mechanical codec envelopes for already-existing concrete C++ storage.
// Codec tags select constructors; they are not authored category identities.
using Resolver = std::function<Singular*(const std::string&)>;
nlohmann::json storedToJson(const Singular& being);
std::shared_ptr<Singular> storedFromJson(const nlohmann::json& record, const Resolver& resolve);
void restoreStored(Zone& zone, const nlohmann::json& records, bool replace);

} // namespace SingularSetToSetCreation
