import sys
with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

old_lambda = """        case Kind::AddRelation: {
            const std::string srcToken = containerToken;
            const std::string dstToken = elementToken;
            const std::string relType = propertyName;

            return [srcToken, dstToken, relType](const ECA::Event& event, Singular& subject) {
                if (relType.empty()) {"""

new_lambda = """        case Kind::AddRelation: {
            const std::string srcToken = containerToken;
            const std::string dstToken = elementToken;
            const std::string relType = propertyName;

            return [srcToken, dstToken, relType](const ECA::Event& event, Singular& subject) {
                auto resolveDynamicString = [&](const std::string& input) -> std::string {
                    if (input.length() > 2 && input[0] == '@' && input[1] == '@') {
                        PropertyValue val;
                        if (lawGetValue(subject, PropertyPath::parse(input.substr(1)), val)) {
                            if (auto s = std::get_if<std::string>(&val)) return *s;
                        }
                    }
                    return input;
                };
                const std::string resolvedRelType = resolveDynamicString(relType);

                if (resolvedRelType.empty()) {"""

text = text.replace(old_lambda, new_lambda)

# Update `relType` usages inside the lambda
old_rel_usage = """                Singular* a = resolveBeingToken(srcToken, subject);
                Singular* b = resolveBeingToken(dstToken, subject);
                if (!a || !b) {
                    emitEffect("AddRelation", false, "unresolved endpoints for relation");
                    return;
                }
                const bool added = Universe::instance().relate(a, b, relType);"""

new_rel_usage = """                Singular* a = resolveBeingToken(srcToken, subject);
                Singular* b = resolveBeingToken(dstToken, subject);
                if (!a || !b) {
                    emitEffect("AddRelation", false, "unresolved endpoints for relation");
                    return;
                }
                const bool added = Universe::instance().relate(a, b, resolvedRelType);"""

text = text.replace(old_rel_usage, new_rel_usage)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
