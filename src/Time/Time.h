//
// Created by Zachary Zhang on 8/19/26.
//

#ifndef EARTHCALL_TIME_H
#define EARTHCALL_TIME_H

#include "Time/Moment/Moment.hpp"
#include "Time/Event/Event.hpp"

// Event should go here in the future too because Events are really just special Moments with specific properties or Formations of defining Singulars
// ECA should use this instead. We don't want a separate "When" black box apart from Time/Moment/Event
// Person Authored Events should also be here. No black boxes. 
// - Zach
//
// Realized (2026-09-13): Event is elevated to the Time ontology as a "distinguished Moment"
// (class Event : public Moment). ECA aliases Time::Event and laws listen to first-class
// temporal events with full property transparency.

// Placeholder for a first-order vessel of Time in Earthcall.
// First rung written: docs/architecture/ontology/TIME_AND_MOMENT.md.
// Moment (Moment/Moment.hpp) is the being that already answers "when did this
// happen"; this class stays empty until Earthcall's philosophy of time asks
// for something Moment and Universe's world clock do not already provide.

class Time
{
    // Ok so we need a robust philosophy of time. Think: branch of high-level metaphysics that deals with time.

};


#endif //EARTHCALL_TIME_H
