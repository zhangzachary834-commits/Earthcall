#include "ZoneManager.hpp"
#include "Core/EventBus.hpp"
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
        if (!_zones.empty() && _currentIndex < _zones.size() && _currentIndex != index) {
            Core::EventBus::instance().publish(ZoneExitedEvent(_currentIndex, _zones[_currentIndex].name()));
        }

        _currentIndex = index;
        std::cout << "🔀 Switching to zone [" << index << "]..." << std::endl;
        try {
            _zones[_currentIndex].load();
            Core::EventBus::instance().publish(ZoneLoadedEvent(_currentIndex, _zones[_currentIndex].name()));
        } catch (...) { std::cerr << "⚠️  Zone load failed." << std::endl; }
        describeCurrent();

        Core::EventBus::instance().publish(ZoneEnteredEvent(_currentIndex, _zones[_currentIndex].name()));
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
