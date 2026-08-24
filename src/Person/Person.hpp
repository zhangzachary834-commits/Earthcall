#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Body.hpp"
#include <glm/glm.hpp>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "Identity/SingularId.hpp"
#include "Soul/Soul.hpp"
#include <json.hpp>

struct PersonJoinedEvent;
struct PersonLeftZoneEvent;
struct PersonLoginEvent;
struct PersonLogoutEvent;

class Person : public Singular {
public:
    // this should be Lexeme. - Zach
    std::string displayName;

    // WHAT IS THIS DOIGN HERE?!?!?!?! These are supposed to either be Perspective, or be properties not hardcoded fields!!! - Zach
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};

    // Where this Person is looking from — facts about the Person, not the
    // window. The Input modality writes them through LocomotionChannel.
    glm::vec3 cameraPos{0.0f, 0.0f, 0.0f};
    glm::vec3 cameraForward{0.0f, 0.0f, -1.0f};

    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);

    // `foundationSymbol` seeds this Person's Hierarchy of Joys (a Formation
    // of Lexemes, not a stored string). Empty refuses a root.
    // "default"/"strict" seed the first-mover foundation (lexeme.christ).
    Person(Soul soul, Body body, const std::string& foundationSymbol);

    Formation& joys() { return _joys; }
    const Formation& joys() const { return _joys; }
    Soul& soul() { return _soul; }
    const Soul& soul() const { return _soul; }
    bool satisfiesJoyBounds() const { return _joys.satisfiesJoyBounds(); }
    std::string propJoys() const { return _joys.getIdentifier(); }
    void express() const;
    void draw() const;
    void drawNametag() const;

    // Lexeme. Not a mere string.
    const std::string& getDisplayName() const { return displayName; }

    // Rebuild body-part world transforms from position. Joint inheritance is
    // still a temporary parent-name chain; Relations should own it later.
    void updatePose();

    Body& getBody() { return bodies[activeBodyIndex]; }
    const Body& getBody() const { return bodies[activeBodyIndex]; }

    void addBody(Body&& newBody) { bodies.push_back(std::move(newBody)); }
    void setActiveBody(int index) { if(index >= 0 && index < static_cast<int>(bodies.size())) activeBodyIndex = index; }

    // Logging into the device, the signal of when app/computer should interface with Person and when not to
    void login(const std::string& sessionId = "");
    void logout(const std::string& sessionId = "");

    bool isLoggedIn() const { return _isLoggedIn; }
    const std::string& getCurrentSession() const { return _currentSession; }

    // WHY IS THIS DOING THIS BY STRINGS RATHER THAN POINTERSSSSS - Zach
    void joinZone(const std::string& zoneName);
    void leaveZone(const std::string& zoneName);

    const std::vector<std::string>& getJoinedZones() const { return _joinedZones; }

    // No new primitive methods for AI, it needs to be a larger ontology of how Persons Relate to First Movers - Zach
    void requestAIAction(const std::string& context, const std::string& targetObjectId);

    // ------------------------------------------------------------------
    // Identity and personal address.
    //
    // This belongs to Person, not to Soul. Identity is how a Person is
    // addressed, recognised, and held to what they own -- it is a fact about
    // the Person as a whole, and routing it through a member meant the class
    // responsible for a Person's standing in the world did not hold it.
    //
    // Two distinct things, deliberately not one:
    //
    //   personId    what this Person IS. An Ed25519 public key, so holding it
    //               is provable and claiming it is not. Never chosen, never
    //               typed in, never compared against a name.
    //   displayName what this Person is CALLED. Chosen, editable, and free to
    //               collide with anyone else's. It carries no authority --
    //               that is exactly why it is safe to let people pick it.
    // ------------------------------------------------------------------
    const Identity::SingularId& personId() const { return _personId; }
    bool hasIdentity() const { return _personId.canAuthenticate(); }

    // Assigning identity is a distinct act from constructing a Person: a
    // Person may exist in a loaded world before their key is available, and
    // minting one on every construction would hand out a fresh identity to
    // every temporary copy.
    void setPersonId(const Identity::SingularId& id) { _personId = id; }

    // Singular interface implementation. Prefers the cryptographic identity;
    // falls back to the display name only for worlds saved before identities
    // existed, which is what keeps legacy law-author resolution working until
    // migration has run.
    std::string getIdentifier() const override {
        return _personId.canAuthenticate() ? _personId.toString() : displayName;
    }

    // Law-author resolution scans beings for a matching identifier. During
    // migration a saved world may address this Person either way, so accept
    // both -- but never let a display name match a Person who has a real
    // identity, or picking someone's name would again be enough to be them.
    bool matchesIdentifier(const std::string& candidate) const {
        if (_personId.canAuthenticate()) return candidate == _personId.toString();
        return candidate == displayName;
    }

private:
    void buildProperties() override;

    // "prop" name goes against the Person is not an Object principle.
    std::string propName() const { return displayName; }

    // Soul is this Person viewed through the light of their composite experiences, states, and moments, not a second someone.
    // Its identifier is this Person's. See Soul.hpp.
    Soul _soul;
    Formation _joys;
    Identity::SingularId _personId;
    std::vector<Body> bodies;
    int activeBodyIndex = 0;

    bool _isLoggedIn = false;
    std::string _currentSession;
    std::vector<std::string> _joinedZones;
};
