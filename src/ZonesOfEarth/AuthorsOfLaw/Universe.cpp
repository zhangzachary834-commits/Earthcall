#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include "Time/timeline.hpp"

void Universe::setTimeline(Timeline* timelineBeing) {
    if (_timeline == timelineBeing) return;

    // Preserve the last readable clock if authority is explicitly removed.
    // Binding a NEW Timeline does not overwrite that being with stale fallback
    // state: the selected Timeline is authoritative by virtue of being selected.
    if (!timelineBeing && _timeline && _timeline->hasClock()) {
        _now = _timeline->now();
        _dt = _timeline->delta();
        _clockSet = true;
    }

    _timeline = timelineBeing;
}

void Universe::setClock(double nowValue, double deltaValue) {
    if (_timeline) {
        (void)_timeline->setClock(nowValue, deltaValue);
        return;
    }
    _now = nowValue;
    _dt = deltaValue;
    _clockSet = true;
}

bool Universe::hasClock() const {
    return _timeline ? _timeline->hasClock() : _clockSet;
}

double Universe::now() const {
    return _timeline ? _timeline->now() : _now;
}

double Universe::dt() const {
    return _timeline ? _timeline->delta() : _dt;
}
