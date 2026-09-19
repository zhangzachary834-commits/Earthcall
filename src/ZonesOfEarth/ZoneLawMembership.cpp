#include "ZonesOfEarth/ZoneManager.hpp"

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <iostream>

bool ZoneManager::adoptLawIntoActiveZone(const std::string& lawId) {
    if (lawId.empty()) {
        std::cerr << "[zones] REFUSED authored Law adoption: empty Law identity.\n";
        return false;
    }
    if (_zones.empty() || _currentIndex >= _zones.size() || !_zones[_currentIndex]) {
        std::cerr << "[zones] REFUSED authored Law adoption for '" << lawId
                  << "': no active Zone.\n";
        return false;
    }
    if (!_lawManager) {
        std::cerr << "[zones] REFUSED authored Law adoption for '" << lawId
                  << "': no LawManager is bound.\n";
        return false;
    }

    Law* law = _lawManager->find(lawId);
    if (!law) {
        std::cerr << "[zones] REFUSED authored Law adoption for '" << lawId
                  << "': Law is not in the running register.\n";
        return false;
    }
    if (law->isFirstMover()) {
        std::cerr << "[zones] REFUSED authored Law adoption for '" << lawId
                  << "': First Movers are engine substrate, not Zone-authored roots.\n";
        return false;
    }

    // This set is already the runtime image of the active identity's lawRefs.
    // Adding here is therefore an authored mutation OF THAT CLOSURE. Save Zone
    // records it; switchTo later releases it when the Person leaves this Zone.
    _activeZoneLawIds.insert(lawId);
    return true;
}
