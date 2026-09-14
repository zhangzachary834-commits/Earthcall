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
    Singular* subject = nullptr;  // The primary being undergoing transition
    Singular* object = nullptr;   // The secondary being/relatum/target (optional)
    std::string author;           // The author/First Mover of intention (optional)

    Event();
    Event(std::string verb, Singular* subject = nullptr, Singular* object = nullptr,
          const Moment& when = Moment::now(), std::string author = "");
    Event(std::string verb, Singular* subject, Singular* object,
          std::time_t unixSeconds, std::string author = "");

    // Distinguishing accessors
    const std::string& verb() const { return type; }
    void setVerb(const std::string& v) { type = v; }

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

    nlohmann::json toJson() const;

protected:
    void buildProperties() override;
};

inline void to_json(nlohmann::json& j, const Event& e) { j = e.toJson(); }

#endif // EARTHCALL_EVENT_H
