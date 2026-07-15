#pragma once
#include <vector>
#include <string>
#include <ctime>
#include "Zone/Zone.hpp"

// Event structures for zone transitions (see Core/EventTypes.hpp).
// Defined in ZoneManager.cpp.
struct ZoneExitedEvent {
    size_t index;
    std::string name;
    std::time_t timestamp;
    ZoneExitedEvent(size_t idx, const std::string& n) : index(idx), name(n), timestamp(std::time(nullptr)) {}
};

struct ZoneEnteredEvent {
    size_t index;
    std::string name;
    std::time_t timestamp;
    ZoneEnteredEvent(size_t idx, const std::string& n) : index(idx), name(n), timestamp(std::time(nullptr)) {}
};

struct ZoneLoadedEvent {
    size_t index;
    std::string name;
    std::time_t timestamp;
    ZoneLoadedEvent(size_t idx, const std::string& n) : index(idx), name(n), timestamp(std::time(nullptr)) {}
};

class ZoneManager {
    std::vector<Zone> _zones;
    size_t _currentIndex = 0;

public:
    void addZone(Zone&& zone) noexcept;           // prefer move for temporaries
    void addZone(const Zone& zone);               // copy retained for compatibility
    void switchTo(size_t index);
    void describeCurrent() const;

    void loadZone();
    void organizeLoad();

    Zone& active();

    // Create a Zone "cross-interaction" system later, so Zones can interact with each other. 
    // Zones can integrate and unite, rather than being siloed.
    // ZoneManager needs to be able to load individual Singulars from their zones at the same time
    // New zones can be birthed from the synthesis of existing ones, use zone creation methods.

    // Accessors to iterate over all zones (needed for serialization)
    std::vector<Zone>& zones();
    const std::vector<Zone>& zones() const;

    // Current active zone index
    size_t currentIndex() const { return _currentIndex; }
};