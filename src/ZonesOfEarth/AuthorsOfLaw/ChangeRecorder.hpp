#pragma once

#include "ActionModel.hpp"
#include "ConstructedBeing/Singular/Object/Automation/Automation.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"

#include <string>
#include <utility>
#include <vector>

// Authoring by demonstration (LAW_AND_CREATION_SYSTEM.md §4):
//   "the Action is essentially recording the change of designated variables
//    over time and modeling that change."
//
// Watch PropertyPaths on a subject, sample them per frame while the Person
// works with the ordinary tools (Pottery, Rotate, gizmos), then FIT the
// traces — constant (no change: no node), linear (polynomial Drive), sinusoid
// (zero-crossing frequency estimate + least squares). The demonstration is
// the source code; the fitted curve preserves the SHAPE of what the Person
// did (entire-process capture, not output matching).
class ChangeRecorder {
public:
    struct Trace {
        PropertyPath path;
        std::vector<std::pair<float, double>> samples;   // (seconds, value)
    };

    // Declare which variables the recording is about. May be called before
    // or between recordings; watching is not recording.
    void watch(const std::string& dottedPath);
    void clearWatches();

    // Start recording against a subject: clears prior samples, zeroes the clock.
    void begin(Singular& subject);
    bool recording() const { return _subject != nullptr; }
    void end() { _subject = nullptr; }

    // Advance the clock and sample every watched path. Call once per frame
    // while the Person demonstrates.
    void sample(float dt);

    const std::vector<Trace>& traces() const { return _traces; }
    float duration() const { return _t; }

    // Traces whose value actually changed become Drive nodes (one child per
    // changed trace; a single change is the root itself; nothing changed
    // yields an empty Sequence). The Drive input is left unbound — the author
    // binds the domain at law-assembly time; for time-periodic motion use
    // toClip() instead (Automation is the native time-playback engine).
    ActionModel fit() const;

    // Convert sinusoid fits on transform-channel paths (position.*/rotation.*)
    // into an additive Automation::Clip relative to the rest pose: bias is
    // expressed against the first sample, since the rest pose is captured at
    // clip-add time (= where the demonstration started).
    Automation::Clip toClip(const std::string& clipName) const;

    // Fit one raw series. Chooses constant / polynomial{b,m} / sinusoid by
    // least-squares error. Exposed for tests and for the synthesis path
    // (higher laws re-fit cumulative traces).
    static CurveModel fitSeries(const std::vector<std::pair<float, double>>& samples);

private:
    std::vector<Trace> _traces;
    Singular* _subject = nullptr;
    float _t = 0.0f;
};
