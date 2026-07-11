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

    // ------------------------------------------------------------------
    // The world clock. Singularity owns time: the engine sets it once per
    // frame (accumulated seconds since the world began + that frame's dt),
    // tests set it by hand, and laws read it through the reserved paths
    // "time" / "time.delta" / "time.sinceApplied" (MathBinding.hpp). No law
    // writes time.
    // ------------------------------------------------------------------
    void setClock(double now, double dt) {
        _now = now;
        _dt = dt;
        _clockSet = true;
    }
    bool hasClock() const { return _clockSet; }
    double now() const { return _now; }
    double dt() const { return _dt; }

    // The application context: while a law's actions execute, this holds the
    // world time at which that law began holding for the current subject, so
    // "time.sinceApplied" (= now - onset) resolves inside compiled closures.
    // Law::applyTo sets and clears it (RAII); outside an application the
    // path is undefined — a law never reads another law's onset.
    void setApplicationOnset(double onset) {
        _onset = onset;
        _onsetSet = true;
    }
    void clearApplicationOnset() { _onsetSet = false; }
    bool hasApplicationOnset() const { return _onsetSet; }
    double applicationOnset() const { return _onset; }

private:
    Universe() = default;
    Universe(const Universe&) = delete;
    Universe& operator=(const Universe&) = delete;

    Provider _provider;

    double _now = 0.0;
    double _dt = 0.0;
    bool _clockSet = false;

    double _onset = 0.0;
    bool _onsetSet = false;
};
