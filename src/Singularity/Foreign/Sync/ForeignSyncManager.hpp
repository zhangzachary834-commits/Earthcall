#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"   // LawManager is declared here
#include <string>

// 
// ForeignSyncManager
// 
// Listens to Earthcall's native ECA event system and property changes.
// When a native change occurs on an Object that maps to a foreign app 
// (e.g. a Person clicking a reconstructed UI button), this manager intercepts it
// and routes it to the appropriate Adapter for execution on the host OS.
//
class ForeignSyncManager {
public:
    ForeignSyncManager(Universe& universe, LawManager& laws);
    ~ForeignSyncManager();

    void initialize();
    void update();

private:
    Universe& _universe;
    LawManager& _laws;

    // Callbacks for native Earthcall events
    void onNativeEventEmitted(const std::string& targetId, const std::string& eventType);
};
