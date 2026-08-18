#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include <string>

class Person;

// Soul is not a second someone. It is the Person considered as a
// conceptual composite (worship, life-course). Identity is the Person's.
// A construction name may be passed so Person can seed displayName;
// that string is a label, never an identity, and is cleared on bind.
class Soul : public Singular {
public:
    explicit Soul(std::string constructionName = "");
    Soul(const Soul& o);
    Soul& operator=(const Soul& o);
    Soul(Soul&& o) noexcept;
    Soul& operator=(Soul&& o) noexcept;
    ~Soul() override = default;

    void bindPerson(Person* person);
    Person* person() const { return _person; }

    // Construction-only display-name hint. Empty after bindPerson.
    const std::string& constructionName() const { return _constructionName; }

    std::string getIdentifier() const override;

private:
    void buildProperties() override;
    std::string propPerson() const;

    Person* _person = nullptr;
    std::string _constructionName;
};
