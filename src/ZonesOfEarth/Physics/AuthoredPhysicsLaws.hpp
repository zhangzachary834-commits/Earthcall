#pragma once

#include <vector>
#include <memory>
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

class LawManager;
class Person;

namespace Physics {

// Creates the suite of person-authored laws governing rotational kinematics,
// center-of-mass gravitational tilt, rolling surface coupling, and angular damping.
// These are authored laws (isFirstMover() == false), fully inspectable and
// editable by Persons in the Law Authoring system.
std::vector<std::shared_ptr<Law>> createAuthoredRotationalLaws();

// Shared boot contract used by production Engine and its focused test harness:
// every authored rotational Law is signed by a real Person before registration.
void registerAuthoredRotationalLaws(LawManager& manager, Person& author);

} // namespace Physics
