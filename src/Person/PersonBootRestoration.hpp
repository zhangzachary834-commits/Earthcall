#pragma once

class Person;

// Result of the same local-profile selection used at real Engine boot.
// This only restores profile data; it never authenticates a keyed Person.
enum class PersonBootRestorationResult {
    NoProfile,
    Restored,
    Ambiguous
};

PersonBootRestorationResult restoreUnambiguousPersonProfileForBoot(Person& person);
