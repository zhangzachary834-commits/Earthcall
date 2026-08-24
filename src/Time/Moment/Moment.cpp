//
// Created by Zachary Zhang on 8/23/26.
//

#include "Moment.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <algorithm>
#include <ctime>
#include <sstream>

Moment::Moment(Kind kind, OntoMath::ScalarForm start, OntoMath::ScalarForm end,
               double startCache, double endCache)
    : _kind(kind),
      _start(std::move(start)),
      _end(std::move(end)),
      _startCache(startCache),
      _endCache(endCache) {}

Moment::Moment() : Moment(Kind::Instant,
                          OntoMath::ScalarForm::constant(0.0),
                          OntoMath::ScalarForm::constant(0.0),
                          0.0, 0.0) {}

Moment::Moment(std::time_t unixSeconds)
    : Moment(instant(static_cast<double>(unixSeconds))) {}

Moment::Moment(double seconds)
    : Moment(instant(seconds)) {}

Moment Moment::instant(double seconds) {
    return Moment(Kind::Instant,
                  OntoMath::ScalarForm::constant(seconds),
                  OntoMath::ScalarForm::constant(seconds),
                  seconds, seconds);
}

Moment Moment::interval(double startSeconds, double endSeconds) {
    const double a = std::min(startSeconds, endSeconds);
    const double b = std::max(startSeconds, endSeconds);
    return Moment(Kind::Interval,
                  OntoMath::ScalarForm::constant(a),
                  OntoMath::ScalarForm::constant(b),
                  a, b);
}

Moment Moment::now() {
    return instant(static_cast<double>(std::time(nullptr)));
}

std::string Moment::getIdentifier() const {
    std::ostringstream id;
    id << "moment." << _startCache;
    if (_kind == Kind::Interval) id << "-" << _endCache;
    return id.str();
}

void Moment::setKind(const int& k) {
    if (k == static_cast<int>(Kind::Interval)) {
        _kind = Kind::Interval;
    } else {
        _kind = Kind::Instant;
        _end = _start;
        _endCache = _startCache;
    }
}

void Moment::setStart(const double& t) {
    _start = OntoMath::ScalarForm::constant(t);
    _startCache = t;
    if (_kind == Kind::Instant) {
        _end = _start;
        _endCache = t;
    }
}

void Moment::setEnd(const double& t) {
    _kind = Kind::Interval;
    _end = OntoMath::ScalarForm::constant(t);
    _endCache = t;
}

void Moment::buildProperties() {
    _propertyRegistry.push_back(
        std::make_unique<ComputedProperty<Moment, int>>(
            "kind", this, &Moment::propKind, &Moment::setKind));
    _propertyRegistry.push_back(
        std::make_unique<ComputedProperty<Moment, double>>(
            "start", this, &Moment::propStart, &Moment::setStart));
    _propertyRegistry.push_back(
        std::make_unique<ComputedProperty<Moment, double>>(
            "end", this, &Moment::propEnd, &Moment::setEnd));
    _propertiesBuilt = true;
}
