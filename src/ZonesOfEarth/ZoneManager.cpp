#include "ZoneManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <ctime>
#include <iostream>

void ZoneManager::addZone(Zone&& zone) noexcept
{
    // Avoid copy of potentially large internal members by moving in
    _zones.push_back(std::move(zone));
}

void ZoneManager::addZone(const Zone& zone)
{
    _zones.emplace_back(zone); // invokes Zone copy-ctor
}

void ZoneManager::switchTo(size_t index)
{
    if (index < _zones.size())
    {
        _currentIndex = index;
        std::cout << "🔀 Switching to zone [" << index << "]..." << std::endl;
        
        // Repopulate active zone's world with global objects that belong to it or its parents
        std::vector<std::string> activeZones;
        std::string currentZoneId = _zones[_currentIndex].getIdentifier();
        while (!currentZoneId.empty()) {
            activeZones.push_back(currentZoneId);
            std::string parent = "";
            for (const auto& z : _zones) {
                if (z.getIdentifier() == currentZoneId) {
                    parent = z.getParentZone();
                    break;
                }
            }
            if (parent == currentZoneId || parent.empty()) break;
            currentZoneId = parent;
        }

        auto& worldObjs = _zones[_currentIndex].world().getOwnedObjectsMutable();
        worldObjs.clear();
        for (const auto& obj : globalObjects) {
            bool matches = false;
            for (const auto& az : activeZones) {
                if (obj->belongsToZone(az)) {
                    matches = true;
                    break;
                }
            }
            if (matches) {
                worldObjs.push_back(obj);
            }
        }

        try { _zones[_currentIndex].load(); } catch (...) { std::cerr << "⚠️  Zone load failed." << std::endl; }
        describeCurrent();
        // The zone is a being: laws hear arrival (subject: the zone itself).
        Core::EventBus::instance().publish(
            ECA::Event{"zone-entered", &_zones[_currentIndex], nullptr, std::time(nullptr)});
    }
    else
    {
        std::cerr << "⚠️ Invalid zone index!" << std::endl;
    }
}

void ZoneManager::describeCurrent() const
{
    if (!_zones.empty())
    {
        _zones[_currentIndex].describe();
    }
    else
    {
        std::cout << "⚠️ No zones available." << std::endl;
    }
}

void ZoneManager::loadZone()
{
    if (_currentIndex < _zones.size())
    {
        // Unload previous zone if necessary
        _zones[_currentIndex].load();
    }
    else
    {
        std::cerr << "⚠️ Cannot load zone: index out of bounds!" << std::endl;
    }
}

Zone& ZoneManager::active() { return ZoneManager::_zones[ZoneManager::_currentIndex]; }

std::vector<Zone>& ZoneManager::zones() { return _zones; }

const std::vector<Zone>& ZoneManager::zones() const { return _zones; }
