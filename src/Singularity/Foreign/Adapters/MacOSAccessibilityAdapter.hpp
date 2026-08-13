#pragma once

#include "ZonesOfEarth/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawManager.hpp"
#include <string>

// Protocol Adapter for macOS Accessibility API
// 
// This class strictly translates raw OS Accessibility events into Earthcall's shared vocabulary:
// Property writes, ECA::Event dispatches, and Relation assertions.
// It holds NO internal world state and defines NO domain-specific types.
// It is strictly an adapter behind the ForeignChannel modality.
//
// Do NOT include this header anywhere outside of src/Singularity/Foreign/
class MacOSAccessibilityAdapter {
public:
    MacOSAccessibilityAdapter(Universe& universe, LawManager& laws);
    ~MacOSAccessibilityAdapter();

    // SENSE: Poll or listen for events from macOS Accessibility API
    void pollEvents();

    // ACT: Execute native interactions back out to the macOS environment
    void executeClick(const std::string& osElementId);
    void executeMove(const std::string& osElementId, float newX, float newY);

private:
    Universe& _universe;
    LawManager& _laws;

    // Helper functions that convert raw OS data to Earthcall Singulars/Properties
    void injectWindowData(const std::string& windowId, float x, float y, float w, float h);
    void injectButtonData(const std::string& buttonId, const std::string& label, const std::string& parentWindowId);
    void dispatchClickEvent(const std::string& buttonId);
};
