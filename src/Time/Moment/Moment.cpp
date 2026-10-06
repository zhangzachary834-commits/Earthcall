//
// Created by Zachary Zhang on 8/23/26.
//

#include "Moment.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <algorithm>
#include <ctime>
#include <sstream>
#include <stdexcept>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#elif defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif

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

Moment::Moment(std::string identifier, const Moment& prototype)
    : Moment(prototype) { _authoredIdentifier = std::move(identifier); }

Moment Moment::fromJson(const nlohmann::json& state) {
    const int kind = state.at("kind").get<int>();
    if (kind != 0 && kind != 1) throw std::runtime_error("unsupported Moment kind");
    const double start = state.at("start").get<double>();
    const double end = state.value("end", start);
    auto out = Moment(static_cast<Kind>(kind),
                      state.contains("startForm") ? OntoMath::ScalarForm::fromJson(state["startForm"]) : OntoMath::ScalarForm::constant(start),
                      state.contains("endForm") ? OntoMath::ScalarForm::fromJson(state["endForm"]) : OntoMath::ScalarForm::constant(end),
                      start, end);
    out._authoredIdentifier = state.value("id", std::string{});
    return out;
}

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
    if (!_authoredIdentifier.empty()) return _authoredIdentifier;
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

long Moment::propCpuClockCycle() const {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    return static_cast<long>(__rdtsc());
#elif defined(__i386__) || defined(__x86_64__)
    return static_cast<long>(__rdtsc());
#elif defined(__aarch64__)
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return static_cast<long>(val);
#elif defined(__has_builtin)
#if __has_builtin(__builtin_readcyclecounter)
    return static_cast<long>(__builtin_readcyclecounter());
#else
    return 0;
#endif
#else
    return 0;
#endif
}

void Moment::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<Moment, std::string>>(
        "identifier", this, &Moment::propIdentifier));
    registerProperty(
        std::make_unique<ComputedProperty<Moment, int>>(
            "kind", this, &Moment::propKind, &Moment::setKind));
    registerProperty(
        std::make_unique<ComputedProperty<Moment, double>>(
            "start", this, &Moment::propStart, &Moment::setStart));
    registerProperty(
        std::make_unique<ComputedProperty<Moment, double>>(
            "end", this, &Moment::propEnd, &Moment::setEnd));
    registerProperty(
        std::make_unique<ComputedProperty<Moment, long>>(
            "cpuClockCycle", this, &Moment::propCpuClockCycle));
    _propertiesBuilt = true;
}
