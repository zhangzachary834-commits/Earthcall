#pragma once
#include <vector>
#include <string>
#include "Zone/Zone.hpp"

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