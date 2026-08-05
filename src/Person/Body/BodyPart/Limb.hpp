#pragma once
#include "BodyPart.hpp"

class Limb : public BodyPart {
public:
    Limb(const std::string& name,
         Type type,
         const Form& form);
};
