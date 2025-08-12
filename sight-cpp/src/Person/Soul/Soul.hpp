#pragma once

#include "Form/Singular.hpp"
#include <string>

class Soul : public Singular {
public:
    Soul(std::string identity = "");
    ~Soul();

    std::string getIdentifier() const;

private: 
    std::string _identity;
};

// How would I represent an intangible thing like a soul in code?
// Ah—a channel that tries to "fit" over the real world user's soul. If the body is what physically interact and manifests the soul, then the body must haev the soul. Preference, relational attitude, memory, etc.