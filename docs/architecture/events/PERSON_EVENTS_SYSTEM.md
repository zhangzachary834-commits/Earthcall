# Person Events System

This document explains the event system for Person-related activities including creation, login/logout, and zone management.

## What are Person Events?

The Person Events system automatically triggers events when important Person-related activities happen in your system. This allows other parts of your code to react when:

- A new Person is created
- A Person logs in or out
- A Person joins or leaves a zone

## Available Events

### 1. PersonCreatedEvent
**Triggered when:** A new Person is created
**Contains:**
- `person`: Reference to the newly created Person
- `timestamp`: When the event was triggered

### 2. PersonLoginEvent
**Triggered when:** A Person logs in
**Contains:**
- `person`: Reference to the Person who logged in
- `sessionId`: The session identifier
- `timestamp`: When the event was triggered

### 3. PersonLogoutEvent
**Triggered when:** A Person logs out
**Contains:**
- `person`: Reference to the Person who logged out
- `sessionId`: The session identifier
- `timestamp`: When the event was triggered

### 4. PersonJoinedEvent
**Triggered when:** A Person joins a zone
**Contains:**
- `person`: Reference to the Person who joined
- `zoneName`: The name of the zone they joined
- `timestamp`: When the event was triggered

## How to use it

### 1. Subscribe to events

```cpp
#include "Singularity/Core/EventBus.hpp"
#include "Person/Person.hpp"
#include "Person/PersonEvents.hpp"

// Create handlers for each event type
void handleNewPerson(const PersonCreatedEvent& event) {
    std::cout << "New person created: " << event.person.getDisplayName() << std::endl;
}

void handleLogin(const PersonLoginEvent& event) {
    std::cout << event.person.getDisplayName() << " logged in with session: " << event.sessionId << std::endl;
}

void handleLogout(const PersonLogoutEvent& event) {
    std::cout << event.person.getDisplayName() << " logged out from session: " << event.sessionId << std::endl;
}

void handleZoneJoin(const PersonJoinedEvent& event) {
    std::cout << event.person.getDisplayName() << " joined zone: " << event.zoneName << std::endl;
}

// Subscribe to all events
Core::EventBus::instance().subscribe<PersonCreatedEvent>(handleNewPerson);
Core::EventBus::instance().subscribe<PersonLoginEvent>(handleLogin);
Core::EventBus::instance().subscribe<PersonLogoutEvent>(handleLogout);
Core::EventBus::instance().subscribe<PersonJoinedEvent>(handleZoneJoin);
```

### 2. Events trigger automatically

The events are triggered automatically when you use the Person methods:

```cpp
// Create Person with Soul, Body, and joy ordering
Soul soul("Alice");
Body body;
Person alice(soul, std::move(body), "Christ-first");
alice.displayName = "Alice";

// Session and Zone actions trigger events
alice.login("session_001");
alice.joinZone("Main Plaza");
alice.logout("session_001");
```

## Person Management Methods

### Session Management
- `login(sessionId)`: Log in a person (triggers `PersonLoginEvent`)
- `logout(sessionId)`: Log out a person (triggers `PersonLogoutEvent`)
- `isLoggedIn()`: Check if person is currently logged in
- `getCurrentSession()`: Get the current session ID

### Zone Management
- `joinZone(zoneName)`: Join a zone (triggers `PersonJoinedEvent`)
- `leaveZone(zoneName)`: Leave a zone
- `getJoinedZones()`: Get list of zones the person has joined

## Important Notes

- **Automatic triggering**: Events are triggered automatically when you use the Person methods
- **Session IDs**: If you don't provide a session ID, one will be auto-generated
- **Zone tracking**: The system prevents duplicate zone joins
- **Thread-safe**: The EventBus system handles concurrent access safely
- **Synchronous delivery**: Events are processed immediately when triggered
