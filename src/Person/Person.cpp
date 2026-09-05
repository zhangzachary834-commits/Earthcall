#include "Person.hpp"
#include "Singularity/Language/JoyHierarchy.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include <ctime>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include "Singularity/Screen/GL/GluCompat.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Relation/Formation/Menu/stb_easy_font.h"
#include "Singularity/Storage/Serialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "PersonEvents.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Core/Logger.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// PersonJoinedEvent/PersonLeftZoneEvent/PersonLoginEvent/PersonLogoutEvent
// are defined in PersonEvents.hpp (not here), so external code can
// subscribe<>() to them.

// Need to load and save Persons based on data saved in txt and json files.
// If its "logging in," a Person should be created in the memory via loading.
// If its "singing up", new Person data should first be added to the new txt and json files, and only then should Person created.
void Person::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "position", this, &Person::_position));
    registerProperty(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "velocity", this, &Person::_velocity));
    registerProperty(std::make_unique<ComputedProperty<Person, std::string>>(
        "name", this, &Person::propName));   // read-only: identity is not a slot
    // --- Law System Perception Properties ---
    registerProperty(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cameraPos", this, &Person::cameraPos));
    registerProperty(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cameraForward", this, &Person::cameraForward));
    registerProperty(std::make_unique<ComputedProperty<Person, std::string>>(
        "joys", this, &Person::propJoys, nullptr));
}



Person::Person(Soul soul, Body body, const std::string& foundationSymbol) : _soul(std::move(soul)) {
    bodies.push_back(std::move(body));
    // Soul("Zach") is a display-name hint, not an identity. Identity is
    // this Person's (personId / called Lexeme). Binding clears the hint so
    // Soul cannot keep a second name.
    std::string seedName = _soul.constructionName();
    if (seedName.empty()) seedName = "Person";
    setDisplayName(seedName);
    _soul.bindPerson(this);

    _joys.setIdentifier("person-joys");
    Singularity::Language::Lexeme* foundation = nullptr;
    if (!foundationSymbol.empty()) {
        auto& lang = Singularity::Language::LanguageSystem::instance();
        foundation = (foundationSymbol == "default" || foundationSymbol == "strict")
            ? lang.foundation().get()
            : lang.resolve(foundationSymbol).get();
    }
    Singularity::Language::seedJoyHierarchy(_joys, foundation);
    if (_joys.root()) setTelosId(_joys.root()->getIdentifier());
}

const std::string& Person::getDisplayName() const {
    static const std::string kEmpty;
    return _called ? _called->getSymbol() : kEmpty;
}

void Person::setDisplayName(const std::string& name) {
    _called = Singularity::Language::LanguageSystem::instance().resolve(
        name.empty() ? "Person" : name);
}

nlohmann::json Person::serialize() const {
    return personToJson(*this);
}

void Person::deserialize(const nlohmann::json& j) {
    personFromJson(j, *this);
}




void Person::express() const {
    std::cout << "\n✨ Person: " << getDisplayName() << std::endl;
    getBody().describe();
}


void Person::draw() const {
    // Body parts already carry their absolute transforms (updated via updatePose),
    // so an extra translation would double-apply position and make the avatar
    // appear to move faster than the camera. Simply draw the body parts.
    getBody().draw();
}

// ---------------------------------------------------------------------------------
//  Render a simple nametag above the Person's head using stb_easy_font
// ---------------------------------------------------------------------------------
void Person::drawNametag() const {
    // Offset above the head where the nametag should appear (world space)
    const float tagHeight = getBody().getNametagHeight();

    // Read the camera off the renderer rather than out of the GL matrix stack —
    // portable, and independent of what the stack happens to hold right now.
    const Renderer& r = currentRenderer();
    const glm::ivec4& vp = r.viewport();
    double model[16], proj[16];
    int viewport[4] = {vp.x, vp.y, vp.z, vp.w};
    for (int i = 0; i < 16; ++i) {
        model[i] = static_cast<double>(glm::value_ptr(r.view())[i]);
        proj[i]  = static_cast<double>(glm::value_ptr(r.proj())[i]);
    }

    // Project 3D position (above head) to 2D window coordinates
    double winX, winY, winZ;
    ecgl::project(_position.x, _position.y + tagHeight, _position.z,
                  model, proj, viewport, &winX, &winY, &winZ);

    // Skip if projected behind camera
    if (winZ < 0.0 || winZ > 1.0) return;

    // Convert Y to top-left origin expected by stb_easy_font
    winY = viewport[3] - winY;

    // Prepare string buffer
    char buf[6000];
    std::string line = getDisplayName();
    int quads = stb_easy_font_print(static_cast<float>(winX), static_cast<float>(winY),
                                    const_cast<char*>(line.c_str()), nullptr,
                                    buf, sizeof(buf));

    // Render in 2D overlay
    Renderer& rw = currentRenderer();
    rw.begin2D(static_cast<uint32_t>(viewport[2]), static_cast<uint32_t>(viewport[3]));
    rw.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    rw.end2D();
}

// -----------------------------------------------------------------------------
void Person::updatePose() {
    glm::mat4 base = glm::translate(glm::mat4(1.0f), _position);

    std::unordered_map<std::string, BodyPart*> partsByName;
    partsByName.reserve(getBody().parts.size());
    for (auto* part : getBody().parts) {
        if (part) partsByName[part->getName()] = part;
    }

    // Temporary rig resolver: this hard-coded parent chain gives body parts
    // immediate inherited motion, but the real "stickiness" of joints should
    // eventually be expressed through Relation/Bonds. As SpatialKind grows from
    // whole shapes into explicit points, edges, anchors, and surfaces, limbs
    // should connect by relational constraints between those spatial features
    // instead of by a separate bespoke skeleton system.
    auto parentNameFor = [](const std::string& name) -> const char* {
        if (name == "Head") return "Neck";
        if (name == "Neck") return "Torso";
        if (name == "Chest" || name == "Stomach") return "Torso";
        if (name == "LowerTorso") return "Stomach";

        if (name == "LeftShoulder" || name == "RightShoulder") return "Chest";
        if (name == "LeftArm") return "LeftShoulder";
        if (name == "RightArm") return "RightShoulder";
        if (name == "LeftForeArm") return "LeftArm";
        if (name == "RightForeArm") return "RightArm";
        if (name == "LeftHand") return "LeftForeArm";
        if (name == "RightHand") return "RightForeArm";

        if (name == "LeftLeg" || name == "RightLeg") return "LowerTorso";
        if (name == "LeftForeLeg") return "LeftLeg";
        if (name == "RightForeLeg") return "RightLeg";
        if (name == "LeftFoot") return "LeftForeLeg";
        if (name == "RightFoot") return "RightForeLeg";

        return nullptr;
    };

    std::unordered_map<BodyPart*, glm::mat4> resolvedWorld;
    resolvedWorld.reserve(getBody().parts.size());

    std::function<glm::mat4(BodyPart*)> resolvePart = [&](BodyPart* part) -> glm::mat4 {
        if (!part) return base;
        auto cached = resolvedWorld.find(part);
        if (cached != resolvedWorld.end()) return cached->second;

        const glm::mat4 restLocal = part->localTransform();
        const glm::mat4 animatedLocal = part->getPrimaryObject()->hasAutomations()
            ? part->getPrimaryObject()->sampleAutomations(restLocal)
            : restLocal;

        glm::mat4 worldT = base * animatedLocal;
        if (const char* parentName = parentNameFor(part->getName())) {
            auto parentIt = partsByName.find(parentName);
            if (parentIt != partsByName.end() && parentIt->second && parentIt->second != part) {
                BodyPart* parent = parentIt->second;
                const glm::mat4 parentWorld = resolvePart(parent);
                const glm::mat4 childFromParentRest = glm::inverse(parent->localTransform()) * animatedLocal;
                worldT = parentWorld * childFromParentRest;
            }
        }

        resolvedWorld[part] = worldT;
        return worldT;
    };

    for (auto* part : getBody().parts) {
        if (part) part->setTransform(resolvePart(part));
    }
}

void Person::login(const std::string& sessionId) {
    if (!_isLoggedIn) {
        _isLoggedIn = true;
        _currentSession = sessionId.empty() ? "session_" + std::to_string(std::time(nullptr)) : sessionId;
        
        // Trigger PersonLoginEvent
        PersonLoginEvent event(*this, _currentSession);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-logged-in", this, nullptr, Moment::now()});

        ECA::Logger::instance().log(ECA::LogCategory::Person, "LOGIN", getDisplayName() + " logged in (Session: " + _currentSession + ")", nlohmann::json{{"person", getDisplayName()}, {"session", _currentSession}});
        std::cout << "👤 " << getDisplayName() << " logged in (Session: " << _currentSession << ")" << std::endl;
    }
}

void Person::logout(const std::string& sessionId) {
    if (_isLoggedIn) {
        std::string session = sessionId.empty() ? _currentSession : sessionId;
        
        // Trigger PersonLogoutEvent
        PersonLogoutEvent event(*this, session);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-logged-out", this, nullptr, Moment::now()});

        _isLoggedIn = false;
        _currentSession.clear();
        
        ECA::Logger::instance().log(ECA::LogCategory::Person, "LOGOUT", getDisplayName() + " logged out (Session: " + session + ")", nlohmann::json{{"person", getDisplayName()}, {"session", session}});
        std::cout << "👤 " << getDisplayName() << " logged out (Session: " << session << ")" << std::endl;
    }
}

void Person::joinZone(Zone& zone) {
    auto it = std::find(_joinedZones.begin(), _joinedZones.end(), &zone);
    if (it == _joinedZones.end()) {
        _joinedZones.push_back(&zone);

        PersonJoinedEvent event(*this, zone);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-joined-zone", this, &zone, Moment::now()});

        ECA::Logger::instance().log(ECA::LogCategory::Person, "ZONE_JOIN", getDisplayName() + " joined zone: " + zone.name(), nlohmann::json{{"person", getDisplayName()}, {"zone", zone.name()}});
        std::cout << "👤 " << getDisplayName() << " joined zone: " << zone.name() << std::endl;
    }
}

void Person::leaveZone(Zone& zone) {
    auto it = std::find(_joinedZones.begin(), _joinedZones.end(), &zone);
    if (it != _joinedZones.end()) {
        _joinedZones.erase(it);

        PersonLeftZoneEvent event(*this, zone);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-left-zone", this, &zone, Moment::now()});

        ECA::Logger::instance().log(ECA::LogCategory::Person, "ZONE_LEAVE", getDisplayName() + " left zone: " + zone.name(), nlohmann::json{{"person", getDisplayName()}, {"zone", zone.name()}});
        std::cout << "👤 " << getDisplayName() << " left zone: " << zone.name() << std::endl;
    }
}
