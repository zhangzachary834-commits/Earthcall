#pragma once

#include <array>
#include <string_view>

// Central registry of every event type published on Core::EventBus.
//
// EventBus itself is templated (subscribe<T>/publish<T>) and doesn't need
// this enum to function - it dispatches on std::type_index. This registry
// exists purely so contributors have one place to see what events exist
// instead of grepping the whole tree for "struct ...Event". When you add a
// new event struct, add a matching entry here too.
//
// The comment next to each entry names the struct and the file it lives in.
namespace Core {

enum class EventType {
    // Person/ (src/Person/Person.cpp)
    PersonCreated,      // PersonCreatedEvent
    PersonJoined,       // PersonJoinedEvent
    PersonLogin,        // PersonLoginEvent
    PersonLogout,       // PersonLogoutEvent

    // Form/Object/ (src/Form/Object/Object.cpp)
    ObjectHover,        // ObjectHoverEvent
    ObjectHoverEnter,   // ObjectHoverEnterEvent
    ObjectHoverExit,    // ObjectHoverExitEvent

    // ZonesOfEarth/Physics/ (src/ZonesOfEarth/Physics/Physics.hpp)
    PhysicsCollision,   // Physics::PhysicsCollisionEvent

    // Relation/ (src/Relation/RelationManager.hpp)
    RelationCreated,    // RelationCreatedEvent

    // ZonesOfEarth/Ourverse/ (src/ZonesOfEarth/Ourverse/Ourverse.hpp)
    Interaction,        // InteractionEvent

    // Integration/SecurityManager (src/Integration/SecurityManager.hpp)
    Security,           // SecurityEvent

    // Core/Game save/load (src/Core/GameSaveLoad.cpp)
    SaveStarted,        // SaveStartedEvent
    SaveCompleted,      // SaveCompletedEvent
    LoadStarted,        // LoadStartedEvent
    LoadCompleted,      // LoadCompletedEvent

    // ZonesOfEarth/ZoneManager (src/ZonesOfEarth/ZoneManager.hpp)
    ZoneEntered,        // ZoneEnteredEvent
    ZoneExited,         // ZoneExitedEvent
    ZoneLoaded,         // ZoneLoadedEvent

    // Perspective/KeyboardHandler (src/Perspective/KeyboardHandler.hpp)
    KeyPressed,         // KeyPressedEvent

    // Perspective/MouseHandler (src/Perspective/MouseHandler.hpp)
    MouseClicked,       // MouseClickedEvent
    MouseMoved,         // MouseMovedEvent

    // Core/Game toolbar (src/Core/Game.hpp)
    ToolSelected,       // ToolSelectedEvent

    // Form/Object/Formation/Menu (src/Form/Object/Formation/Menu/Menu.hpp)
    MenuOpened,         // MenuOpenedEvent
    MenuClosed,         // MenuClosedEvent

    // Integration/WebIntegration (src/Integration/WebIntegration.hpp)
    WebPageLoaded,      // WebPageLoadedEvent

    // Integration/EarthcallAPI (src/Integration/EarthcallAPI.hpp)
    APICallCompleted,   // APICallCompletedEvent

    Count // sentinel, not a real event
};

inline constexpr std::string_view eventTypeName(EventType type) {
    switch (type) {
        case EventType::PersonCreated:      return "PersonCreated";
        case EventType::PersonJoined:       return "PersonJoined";
        case EventType::PersonLogin:        return "PersonLogin";
        case EventType::PersonLogout:       return "PersonLogout";
        case EventType::ObjectHover:        return "ObjectHover";
        case EventType::ObjectHoverEnter:   return "ObjectHoverEnter";
        case EventType::ObjectHoverExit:    return "ObjectHoverExit";
        case EventType::PhysicsCollision:   return "PhysicsCollision";
        case EventType::RelationCreated:    return "RelationCreated";
        case EventType::Interaction:        return "Interaction";
        case EventType::Security:           return "Security";
        case EventType::SaveStarted:        return "SaveStarted";
        case EventType::SaveCompleted:      return "SaveCompleted";
        case EventType::LoadStarted:        return "LoadStarted";
        case EventType::LoadCompleted:      return "LoadCompleted";
        case EventType::ZoneEntered:        return "ZoneEntered";
        case EventType::ZoneExited:         return "ZoneExited";
        case EventType::ZoneLoaded:         return "ZoneLoaded";
        case EventType::KeyPressed:         return "KeyPressed";
        case EventType::MouseClicked:       return "MouseClicked";
        case EventType::MouseMoved:         return "MouseMoved";
        case EventType::ToolSelected:       return "ToolSelected";
        case EventType::MenuOpened:         return "MenuOpened";
        case EventType::MenuClosed:         return "MenuClosed";
        case EventType::WebPageLoaded:      return "WebPageLoaded";
        case EventType::APICallCompleted:   return "APICallCompleted";
        case EventType::Count:              return "Count";
    }
    return "Unknown";
}

} // namespace Core
