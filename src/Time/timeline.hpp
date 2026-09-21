#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "Time/Moment/Moment.hpp"

#include <memory>
#include <string>
#include <vector>

// A Timeline is a first-class RELATIVE temporal domain: an identifiable
// Singular that contains Moments and carries a current temporal head when the
// domain is actively advancing.
//
// Any Singular may own a Timeline through ordinary Relations ("I own my own
// clock"). A Timeline can be as broad as the Ourverse/world clock or as local
// as one Object, Field, Material, Person, Relation, Zone, etc. Storage location
// never decides ownership; authored Relation truth does.
//
// Timeline is deliberately NOT an enum of engine clocks. "world", "local",
// "animation", "maintenance", or any future use are ordinary Timeline instances
// distinguished by authored identity/Relations, not new C++ kinds.
//
// Moments are owned through shared_ptr so an Event (which IS a Moment) can live
// on a Timeline without slicing. The storage order is not temporal doctrine;
// orderedMoments() derives chronological order from the Moments themselves.
class Timeline : public Singular {
public:
    Timeline();
    explicit Timeline(std::string identifier);
    ~Timeline() override;

    Timeline(const Timeline&) = delete;
    Timeline& operator=(const Timeline&) = delete;
    Timeline(Timeline&&) = delete;
    Timeline& operator=(Timeline&&) = delete;

    std::string getIdentifier() const override { return _identifier; }

    // Generic reachability registry. It records Timeline BEINGS, not timeline
    // kinds: constructing a new Timeline is sufficient for the engine's generic
    // Universe provider to expose it without another hardcoded branch.
    static const std::vector<Timeline*>& all();

    bool setClock(double now, double delta);
    bool advanceBy(double delta);
    bool hasClock() const { return _clockSet; }
    double now() const { return _now; }
    double delta() const { return _delta; }

    // Compatibility seam for legacy save/load code that still carries a
    // double*. New code should use setClock()/advanceBy() so change
    // notification remains explicit.
    double* nowPtr() { return &_now; }

    bool addMoment(std::shared_ptr<Moment> moment);
    bool removeMoment(const Moment* moment);
    void clearMoments();

    const std::vector<std::shared_ptr<Moment>>& moments() const { return _moments; }
    std::vector<std::shared_ptr<Moment>> orderedMoments() const;
    std::shared_ptr<Moment> latestMoment() const;

protected:
    void buildProperties() override;

private:
    static std::vector<Timeline*>& registry();
    static std::string nextTimelineId();
    static bool identifierInUse(const std::string& identifier);

    int propMomentCount() const { return static_cast<int>(_moments.size()); }
    std::shared_ptr<PropertyList> propMoments() const;
    std::string propLatestMoment() const;

    void announceClockChange(double oldNow, double oldDelta, bool oldClockSet);
    void announceMomentChange();

    std::string _identifier;
    std::vector<std::shared_ptr<Moment>> _moments;
    double _now = 0.0;
    double _delta = 0.0;
    bool _clockSet = false;
};
