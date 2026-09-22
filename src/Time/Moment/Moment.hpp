//
// Created by Zachary Zhang on 8/23/26.
//

#ifndef EARTHCALL_MOMENT_H
#define EARTHCALL_MOMENT_H

#include "ConstructedBeing/Singular/Singular.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <ctime>
#include <optional>
#include <ostream>
#include <string>

// A Moment is a Singular whose substance is time: a discrete instant or a
// closed interval. Each bound is authored OntoMath (exact); a double cache
// is the cheap view channels use when they only need to order two moments.
// Events carry a Moment as their timestamp.
class Moment : public Singular {
public:
    // Serialized as ints if this ever goes on the wire — APPEND-ONLY.
    enum class Kind { Instant = 0, Interval = 1 };

    Moment();
    Moment(std::time_t unixSeconds);
    explicit Moment(double seconds);

    static Moment instant(double seconds);
    static Moment interval(double startSeconds, double endSeconds);
    static Moment now();

    std::string getIdentifier() const override;

    Kind kind() const { return _kind; }
    bool isInstant() const { return _kind == Kind::Instant; }
    bool isInterval() const { return _kind == Kind::Interval; }

    // Numeric view. Instant: the point. Interval: the start.
    double asSeconds() const { return _startCache; }
    std::optional<double> endSeconds() const {
        if (_kind != Kind::Interval) return std::nullopt;
        return _endCache;
    }
    std::time_t unixTime() const { return static_cast<std::time_t>(_startCache); }

    const OntoMath::ScalarForm& startForm() const { return _start; }
    const OntoMath::ScalarForm& endForm() const {
        return _kind == Kind::Interval ? _end : _start;
    }

    int propKind() const { return static_cast<int>(_kind); }
    void setKind(const int& k);
    double propStart() const { return _startCache; }
    void setStart(const double& t);
    double propEnd() const { return _endCache; }
    void setEnd(const double& t);

    explicit operator std::time_t() const { return unixTime(); }
    explicit operator double() const { return asSeconds(); }

    friend std::ostream& operator<<(std::ostream& os, const Moment& m) {
        os << m.asSeconds();
        if (m.isInterval()) os << ".." << *m.endSeconds();
        return os;
    }

    nlohmann::json toJson() const {
        nlohmann::json j{{"kind", static_cast<int>(_kind)}, {"start", _startCache}};
        if (_kind == Kind::Interval) j["end"] = _endCache;
        return j;
    }

protected:
    void buildProperties() override;

private:
    Moment(Kind kind, OntoMath::ScalarForm start, OntoMath::ScalarForm end,
           double startCache, double endCache);

    Kind _kind = Kind::Instant;
    OntoMath::ScalarForm _start = OntoMath::ScalarForm::constant(0.0);
    OntoMath::ScalarForm _end = OntoMath::ScalarForm::constant(0.0);
    double _startCache = 0.0;
    double _endCache = 0.0;
};

inline void to_json(nlohmann::json& j, const Moment& m) { j = m.toJson(); }

#endif //EARTHCALL_MOMENT_H
