#include "Time/timeline.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace {
std::atomic<unsigned long long> s_nextTimelineId{1};
}

std::vector<Timeline*>& Timeline::registry() {
    static std::vector<Timeline*> timelines;
    return timelines;
}

const std::vector<Timeline*>& Timeline::all() {
    return registry();
}

bool Timeline::identifierInUse(const std::string& identifier) {
    for (const Timeline* timeline : registry()) {
        if (timeline && timeline->getIdentifier() == identifier) return true;
    }
    return false;
}

std::string Timeline::nextTimelineId() {
    for (;;) {
        const std::string candidate =
            "timeline-" + std::to_string(s_nextTimelineId.fetch_add(1));
        if (!identifierInUse(candidate)) return candidate;
    }
}

Timeline::Timeline()
    : Timeline(nextTimelineId()) {}

Timeline::Timeline(std::string identifier)
    : _identifier(std::move(identifier)) {
    if (_identifier.empty()) {
        throw std::invalid_argument("Timeline identifier must not be empty");
    }
    if (identifierInUse(_identifier)) {
        throw std::invalid_argument(
            "Timeline identifier already exists: " + _identifier);
    }
    registry().push_back(this);
}

Timeline::~Timeline() {
    auto& timelines = registry();
    timelines.erase(
        std::remove(timelines.begin(), timelines.end(), this),
        timelines.end());
}

bool Timeline::setClock(double nowValue, double deltaValue) {
    if (!std::isfinite(nowValue) || !std::isfinite(deltaValue)) {
        std::fprintf(stderr,
            "Timeline '%s': REFUSED non-finite clock value now=%g delta=%g\n",
            _identifier.c_str(), nowValue, deltaValue);
        return false;
    }

    const double oldNow = _now;
    const double oldDelta = _delta;
    const bool oldClockSet = _clockSet;

    _now = nowValue;
    _delta = deltaValue;
    _clockSet = true;
    announceClockChange(oldNow, oldDelta, oldClockSet);
    return true;
}

bool Timeline::advanceBy(double deltaValue) {
    if (!std::isfinite(deltaValue)) {
        std::fprintf(stderr,
            "Timeline '%s': REFUSED non-finite advance %g\n",
            _identifier.c_str(), deltaValue);
        return false;
    }
    const double start = _clockSet ? _now : 0.0;
    return setClock(start + deltaValue, deltaValue);
}

bool Timeline::addMoment(std::shared_ptr<Moment> moment) {
    if (!moment) return false;
    for (const auto& existing : _moments) {
        if (existing.get() == moment.get()) return false;
    }
    _moments.push_back(std::move(moment));
    announceMomentChange();
    return true;
}

bool Timeline::removeMoment(const Moment* moment) {
    if (!moment) return false;
    const auto oldSize = _moments.size();
    _moments.erase(
        std::remove_if(_moments.begin(), _moments.end(),
            [moment](const std::shared_ptr<Moment>& candidate) {
                return candidate.get() == moment;
            }),
        _moments.end());
    if (_moments.size() == oldSize) return false;
    announceMomentChange();
    return true;
}

void Timeline::clearMoments() {
    if (_moments.empty()) return;
    _moments.clear();
    announceMomentChange();
}

std::vector<std::shared_ptr<Moment>> Timeline::orderedMoments() const {
    auto ordered = _moments;
    std::stable_sort(ordered.begin(), ordered.end(),
        [](const std::shared_ptr<Moment>& a,
           const std::shared_ptr<Moment>& b) {
            if (!a) return false;
            if (!b) return true;
            return a->asSeconds() < b->asSeconds();
        });
    return ordered;
}

std::shared_ptr<Moment> Timeline::latestMoment() const {
    std::shared_ptr<Moment> latest;
    for (const auto& moment : _moments) {
        if (!moment) continue;
        if (!latest || moment->asSeconds() > latest->asSeconds()) {
            latest = moment;
        }
    }
    return latest;
}

std::shared_ptr<PropertyList> Timeline::propMoments() const {
    auto list = std::make_shared<PropertyList>();
    for (const auto& moment : orderedMoments()) {
        if (moment) list->elements.emplace_back(moment->getIdentifier());
    }
    return list;
}

std::string Timeline::propLatestMoment() const {
    const auto latest = latestMoment();
    return latest ? latest->getIdentifier() : std::string{};
}

void Timeline::announceClockChange(double oldNow, double oldDelta,
                                   bool oldClockSet) {
    if (!oldClockSet) {
        notifyPropertyChanged(this, "hasClock");
    }
    if (!oldClockSet || oldNow != _now) {
        notifyPropertyChanged(this, "now");
    }
    if (!oldClockSet || oldDelta != _delta) {
        notifyPropertyChanged(this, "delta");
    }
}

void Timeline::announceMomentChange() {
    notifyPropertyChanged(this, "moments");
    notifyPropertyChanged(this, "momentCount");
    notifyPropertyChanged(this, "latestMoment");
}

void Timeline::buildProperties() {
    registerProperty(
        std::make_unique<ComputedProperty<Timeline, bool>>(
            "hasClock", this, &Timeline::hasClock, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Timeline, double>>(
            "now", this, &Timeline::now, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Timeline, double>>(
            "delta", this, &Timeline::delta, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Timeline, int>>(
            "momentCount", this, &Timeline::propMomentCount, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Timeline, std::shared_ptr<PropertyList>>>(
            "moments", this, &Timeline::propMoments, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Timeline, std::string>>(
            "latestMoment", this, &Timeline::propLatestMoment, nullptr));
    _propertiesBuilt = true;
}
