import sys
with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

impl = """Singular* resolveBeingToken(const std::string& token, Singular& subject) {
    if (token.empty()) return &subject;
    if (token == "@event.subject") {
        return Universe::instance().hasApplicationEvent()
                   ? Universe::instance().applicationEventSubject() : nullptr;
    }
    if (token == "@event.object") {
        return Universe::instance().hasApplicationEvent()
                   ? Universe::instance().applicationEventObject() : nullptr;
    }

    std::string resolvedId = token;
    if (token.length() > 2 && token[0] == '@' && token[1] == '@') {
        PropertyValue val;
        if (lawGetValue(subject, PropertyPath::parse(token.substr(1)), val)) {
            if (auto s = std::get_if<std::string>(&val)) {
                resolvedId = *s;
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

    const std::string id = (resolvedId[0] == '@') ? resolvedId.substr(1) : resolvedId;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}"""

old_impl = """Singular* resolveBeingToken(const std::string& token, Singular& subject) {
    if (token.empty()) return &subject;
    if (token == "@event.subject") {
        return Universe::instance().hasApplicationEvent()
                   ? Universe::instance().applicationEventSubject() : nullptr;
    }
    if (token == "@event.object") {
        return Universe::instance().hasApplicationEvent()
                   ? Universe::instance().applicationEventObject() : nullptr;
    }
    const std::string id = (token[0] == '@') ? token.substr(1) : token;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}"""

text = text.replace(old_impl, impl)
with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
