import sys
with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

old_lambda = """                return [source, id, name, endpointA, endpointB, parent, placement, hasLegacyOverrides, runs](const ECA::Event& event, Singular& subject) {
                    std::size_t start = 0;"""

new_lambda = """                return [source, id, name, endpointA, endpointB, parent, placement, hasLegacyOverrides, runs](const ECA::Event& event, Singular& subject) {
                    auto resolveDynamicString = [&](const std::string& input) -> std::string {
                        if (input.length() > 2 && input[0] == '@' && input[1] == '@') {
                            PropertyValue val;
                            if (lawGetValue(subject, PropertyPath::parse(input.substr(1)), val)) {
                                if (auto s = std::get_if<std::string>(&val)) return *s;
                            }
                        }
                        return input;
                    };
                    std::string resolvedId = resolveDynamicString(id);
                    std::string resolvedName = resolveDynamicString(name);
                    std::string resolvedEndpointA = resolveDynamicString(endpointA);
                    std::string resolvedEndpointB = resolveDynamicString(endpointB);

                    std::size_t start = 0;"""
text = text.replace(old_lambda, new_lambda)

old_req = "SingularSetToSetCreation::Request request{*prototype, {prototype, &subject}, nullptr, nullptr, destination, id, name, {}};"
new_req = "SingularSetToSetCreation::Request request{*prototype, {prototype, &subject}, nullptr, nullptr, destination, resolvedId, resolvedName, {}};"
text = text.replace(old_req, new_req)

old_endpoints = """                    request.endpointA = endpoint(endpointA);
                    request.endpointB = endpoint(endpointB);"""
new_endpoints = """                    request.endpointA = endpoint(resolvedEndpointA);
                    request.endpointB = endpoint(resolvedEndpointB);"""
text = text.replace(old_endpoints, new_endpoints)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
