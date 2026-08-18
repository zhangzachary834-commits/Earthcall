#pragma once

#include <ctime>
#include <string>

class Person;

/*
 * Person-level automation triggers carried on Core::EventBus.
 *
 * LocomotionChanged decouples "the vessel is moving at speed X" from the
 * walk/idle clips that react to it. Producers (LocomotionChannel, later AI
 * steering) publish it; LocomotionChannel::installRouting dispatches to the
 * named Person. The event carries the Person* so we need only one subscription
 * — Core::EventBus has no unsubscribe.
 */
struct LocomotionChanged {
    Person* person = nullptr;
    bool    moving = false;
    float   speed  = 0.0f;  // metres/second of horizontal travel
};

/*
 * Session and zone-membership triggers. Each also echoes as a string-typed
 * ECA::Event ("person-logged-in" etc.) so Person-authored laws can bind to
 * them — the typed struct here is what a C++ listener would subscribe<>()
 * to; it lives in this header (not Person.cpp) specifically so code outside
 * Person.cpp can see the type and actually do that.
 */
struct PersonJoinedEvent {
    const Person& person;
    std::string   zoneName;
    std::time_t   timestamp;

    PersonJoinedEvent(const Person& p, const std::string& zone)
        : person(p), zoneName(zone), timestamp(std::time(nullptr)) {}
};

struct PersonLeftZoneEvent {
    const Person& person;
    std::string   zoneName;
    std::time_t   timestamp;

    PersonLeftZoneEvent(const Person& p, const std::string& zone)
        : person(p), zoneName(zone), timestamp(std::time(nullptr)) {}
};

struct PersonLoginEvent {
    const Person& person;
    std::string   sessionId;
    std::time_t   timestamp;

    PersonLoginEvent(const Person& p, const std::string& session = "")
        : person(p), sessionId(session), timestamp(std::time(nullptr)) {}
};

struct PersonLogoutEvent {
    const Person& person;
    std::string   sessionId;
    std::time_t   timestamp;

    PersonLogoutEvent(const Person& p, const std::string& session = "")
        : person(p), sessionId(session), timestamp(std::time(nullptr)) {}
};
