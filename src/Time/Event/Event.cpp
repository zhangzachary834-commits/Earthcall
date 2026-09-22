//
// Created for Earthcall: Elevating Event to Time ontology.
// An Event is a distinguished Moment: an occurrence in time carrying transition edges,
// participants, and authorial intention.
//

#include "Event.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <sstream>

Event::Event()
    : Moment(),
      type(""),
      subject(nullptr),
      object(nullptr),
      author("") {}

Event::Event(std::string verb, Singular* subj, Singular* obj,
             const Moment& when, std::string auth)
    : Moment(when),
      type(std::move(verb)),
      subject(subj),
      object(obj),
      author(std::move(auth)) {}

Event::Event(std::string verb, Singular* subj, Singular* obj,
             std::time_t unixSeconds, std::string auth)
    : Moment(unixSeconds),
      type(std::move(verb)),
      subject(subj),
      object(obj),
      author(std::move(auth)) {}

std::string Event::getIdentifier() const {
    std::ostringstream id;
    id << "event." << (type.empty() ? "untyped" : type) << ".";
    if (subject) {
        id << subject->getIdentifier();
    } else {
        id << "none";
    }
    id << "." << asSeconds();
    return id.str();
}

std::string Event::propSubject() const {
    return subject ? subject->getIdentifier() : "";
}

std::string Event::propObject() const {
    return object ? object->getIdentifier() : "";
}

void Event::buildProperties() {
    // Base Moment properties
    registerProperty(
        std::make_unique<ComputedProperty<Event, int>>(
            "kind", this, &Event::propKind, &Event::setKind));
    registerProperty(
        std::make_unique<ComputedProperty<Event, double>>(
            "start", this, &Event::propStart, &Event::setStart));
    registerProperty(
        std::make_unique<ComputedProperty<Event, double>>(
            "end", this, &Event::propEnd, &Event::setEnd));

    // Event distinguishing properties
    registerProperty(
        std::make_unique<ComputedProperty<Event, std::string>>(
            "verb", this, &Event::propVerb, &Event::setPropVerb));
    registerProperty(
        std::make_unique<ComputedProperty<Event, std::string>>(
            "type", this, &Event::propVerb, &Event::setPropVerb));
    registerProperty(
        std::make_unique<ComputedProperty<Event, std::string>>(
            "subject", this, &Event::propSubject, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Event, std::string>>(
            "object", this, &Event::propObject, nullptr));
    registerProperty(
        std::make_unique<ComputedProperty<Event, std::string>>(
            "author", this, &Event::propAuthor, &Event::setPropAuthor));

    _propertiesBuilt = true;
}

nlohmann::json Event::toJson() const {
    nlohmann::json j = Moment::toJson();
    j["type"] = type;
    j["verb"] = type;
    j["subject"] = subject ? subject->getIdentifier() : "";
    j["object"] = object ? object->getIdentifier() : "";
    if (!author.empty()) {
        j["author"] = author;
    }
    return j;
}
