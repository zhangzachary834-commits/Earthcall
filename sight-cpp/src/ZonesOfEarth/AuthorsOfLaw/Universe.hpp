#pragma once

#include <functional>
#include <utility>
#include <vector>

class Singular;

// The universe of beings that CONTINUOUS laws watch and quantified conditions
// (ForAny / ForAll) range over.
//
// The ECA loop is edge-triggered: conditions are checked at the discrete
// moments events fire. But some laws are level-triggered — their condition
// phase must monitor the program at all times ("whenever y sinks below the
// ground", with no event announcing it), and quantified conditions ("if ANY
// object...", "ALL instances except...") need a domain to range over. Both
// need the same thing: the set of beings currently in the world.
//
// The engine supplies the provider (the active world's objects, the
// registered laws, the player); tests supply their own. No provider = empty
// universe: continuous untargeted laws watch nothing, ForAny is false,
// ForAll is vacuously true.
class Universe {
public:
    static Universe& instance() {
        static Universe universe;
        return universe;
    }

    using Provider = std::function<void(std::vector<Singular*>&)>;

    void setProvider(Provider provider) { _provider = std::move(provider); }
    bool hasProvider() const { return static_cast<bool>(_provider); }

    std::vector<Singular*> beings() const {
        std::vector<Singular*> out;
        if (_provider) _provider(out);
        return out;
    }

private:
    Universe() = default;
    Universe(const Universe&) = delete;
    Universe& operator=(const Universe&) = delete;

    Provider _provider;
};
