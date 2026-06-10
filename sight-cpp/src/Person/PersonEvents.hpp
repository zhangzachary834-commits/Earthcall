#pragma once

class Person;

/*
 * Person-level automation triggers carried on Core::EventBus.
 *
 * LocomotionChanged decouples "the player/NPC is moving at speed X" from the
 * automation that reacts to it (walk vs idle clips). Producers (input handling,
 * AI steering, scripted paths) publish it; a single router subscription
 * dispatches to the named Person. The event carries the Person* so we need only
 * one subscription for everyone — important because Core::EventBus has no
 * unsubscribe, so per-Person subscriptions would dangle when a Person dies.
 */
struct LocomotionChanged {
    Person* person = nullptr;
    bool    moving = false;
    float   speed  = 0.0f;  // metres/second of horizontal travel
};
