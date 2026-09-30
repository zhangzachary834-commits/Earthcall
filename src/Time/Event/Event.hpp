//
// Created for Earthcall: Elevating Event to Time ontology.
// An Event is a distinguished Moment: an occurrence in time carrying transition edges,
// participants, and authorial intention.
//

#ifndef EARTHCALL_EVENT_H
#define EARTHCALL_EVENT_H

#include "Time/Moment/Moment.hpp"

#include <string>
#include <vector>

class Event : public Moment {
public:
    std::string type;             // The semantic verb/transition slug (e.g. "jump-started", "zone-entered")
    Singular* subject = nullptr;  // Legacy participant A; defining Relations are a later rung
    Singular* object = nullptr;   // Legacy participant B (optional), not a Law-wide subject
    std::string author;           // The author/First Mover of intention (optional)

    Event();
    Event(std::string verb, Singular* subject = nullptr, Singular* object = nullptr,
          const Moment& when = Moment::now(), std::string author = "");
    Event(std::string verb, Singular* subject, Singular* object,
          std::time_t unixSeconds, std::string author = "");

    // Distinguishing accessors
    const std::string& verb() const { return type; }
    void setVerb(const std::string& v) { type = v; }
    const std::string& occurrenceId() const { return _occurrenceId; }

    // Ontological truth: An Event IS the Moment.
    // timestamp() provides seamless backward compatibility and explicit temporal projection.
    const Moment& timestamp() const { return *this; }
    Moment& timestamp() { return *this; }
    void setTimestamp(const Moment& m) { static_cast<Moment&>(*this) = m; }

    std::string getIdentifier() const override;

    // Property reflection (Refusal #6: No black box)
    std::string propVerb() const { return type; }
    void setPropVerb(const std::string& v) { type = v; }
    std::string propSubject() const;
    std::string propObject() const;
    std::string propAuthor() const { return author; }
    void setPropAuthor(const std::string& a) { author = a; }
    std::string propOccurrenceId() const { return _occurrenceId; }

    nlohmann::json toJson() const;

protected:
    void buildProperties() override;

private:
    // Identity is minted once and copied with the Event. Verb, participants,
    // and even timestamp can coincide for two real transition edges.
    std::string _occurrenceId;
};

inline void to_json(nlohmann::json& j, const Event& e) { j = e.toJson(); }

#endif // EARTHCALL_EVENT_H
