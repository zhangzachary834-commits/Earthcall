#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "Time/Moment/Moment.hpp"
#include <string>
#include <memory>

namespace Core {

// Promoted Event to a Singular so it can exist natively in the physics graph
// and have relations/properties applied to it by the Law system.
class EventEntity : public Singular {
public:
    explicit EventEntity(const std::string& eventType);
    ~EventEntity() override;

    // Singular interface
    std::string getIdentifier() const override;

    const std::string& getEventType() const { return _eventType; }
    void setEventType(const std::string& type) { _eventType = type; }

    const std::string& getSourceId() const { return _sourceId; }
    void setSourceId(const std::string& id) { _sourceId = id; }

    const std::string& getTargetId() const { return _targetId; }
    void setTargetId(const std::string& id) { _targetId = id; }

    const Moment& moment() const { return _moment; }
    void setMoment(const Moment& m) { _moment = m; }
    double propMomentStart() const { return _moment.asSeconds(); }
    void setMomentStart(const double& t) { _moment = Moment::instant(t); }
    double propMomentEnd() const { return _moment.endSeconds().value_or(_moment.asSeconds()); }
    void setMomentEnd(const double& t) {
        _moment = Moment::interval(_moment.asSeconds(), t);
    }

protected:
    void buildProperties() override;

private:
    std::string _id;
    std::string _eventType;
    std::string _sourceId;
    std::string _targetId;
    Moment _moment = Moment::now();
};

} // namespace Core
