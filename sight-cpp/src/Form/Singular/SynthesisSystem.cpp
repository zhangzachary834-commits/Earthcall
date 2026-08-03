#include "Form/Singular/SynthesisSystem.hpp"
#include "Singularity/Language/Lexeme.hpp"
#include "Form/Object/Object.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Core/EventEntity.hpp"
#include <iostream>

nlohmann::json SynthesisMapping::toJson() const {
    nlohmann::json j;
    j["source"] = source.toString();
    j["target"] = target.toString();
    j["hasExact"] = hasExact;
    if (hasExact) {
        j["exact"] = exact.toJson();
    }
    return j;
}

SynthesisMapping SynthesisMapping::fromJson(const nlohmann::json& j) {
    SynthesisMapping m;
    if (j.contains("source")) m.source = PropertyPath::parse(j["source"]);
    if (j.contains("target")) m.target = PropertyPath::parse(j["target"]);
    m.hasExact = j.value("hasExact", false);
    if (m.hasExact && j.contains("exact")) {
        m.exact = OntoMath::Piecewise::fromJson(j["exact"]);
    }
    return m;
}

std::optional<PropertyValue> SynthesisMapping::apply(double x, const std::map<std::string, double>& numericVars, 
                                                     const std::map<std::string, std::string>& stringVars, 
                                                     const Singular* guardSubject) const {
    if (hasExact) {
        std::map<std::string, PropertyValue> evalVars;
        for (const auto& [k, v] : numericVars) {
            evalVars[k] = PropertyValue(v);
        }
        for (const auto& [k, v] : stringVars) {
            evalVars[k] = PropertyValue(v);
        }
        // x is the NUMERIC reading of the source, and is 0.0 when the source
        // was not a number. The caller binds a string source under this very
        // name in stringVars, so assigning unconditionally would clobber it
        // and evaluate the mapping against 0.0 instead of the text.
        if (evalVars.find(exact.inputVariable) == evalVars.end()) {
            evalVars[exact.inputVariable] = PropertyValue(x);
        }

        auto res = exact.evaluate(evalVars, guardSubject);
        if (res) {
            if (std::holds_alternative<double>(*res)) {
                return PropertyValue(std::get<double>(*res));
            } else if (std::holds_alternative<std::string>(*res)) {
                return PropertyValue(std::get<std::string>(*res));
            } else if (std::holds_alternative<bool>(*res)) {
                return PropertyValue(std::get<bool>(*res));
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}


SynthesisSystem& SynthesisSystem::instance() {
    static SynthesisSystem inst;
    return inst;
}

std::shared_ptr<Singular> SynthesisSystem::instantiateClass(
    const std::string& classType,
    const std::map<std::string, PropertyValue>& initialProperties) {
    const auto initialString = [&](const std::string& key) -> std::string {
        auto it = initialProperties.find(key);
        if (it == initialProperties.end()) return {};
        const auto* s = std::get_if<std::string>(&it->second);
        return s ? *s : std::string{};
    };

    if (classType == "Lexeme") {
        return std::make_shared<Singularity::Language::Lexeme>("");
    } else if (classType == "Object") {
        return std::make_shared<Object>();
    } else if (classType == "Zone") {
        // A Zone's name is registered read-only, so it can only be given at
        // birth -- setting it from initialProperties below would no-op and
        // leave the zone permanently unnamed.
        return std::make_shared<Zone>(initialString("name"), "");
    } else if (classType == "Event") {
        return std::make_shared<Core::EventEntity>("CustomEvent");
    }
    // Fallback or dynamic instantiation can be extended here
    return nullptr;
}

std::vector<std::shared_ptr<Singular>> SynthesisSystem::synthesize(
    const std::vector<Singular*>& inputs, 
    const Concept& blueprint,
    const std::vector<SynthesisMapping>& mappings,
    const class Law* activeLaw,
    const Singular* author) 
{
    std::vector<std::shared_ptr<Singular>> outputs;
    
    // Check Kernel Bounds (placeholder)
    for (const auto* in : inputs) {
        if (!in->satisfiesKernelBounds()) {
            std::cerr << "Synthesis failed: Input " << in->getIdentifier() << " failed kernel bounds check.\n";
            return outputs;
        }
    }

    for (const auto& tmpl : blueprint.members()) {
        auto spawned = instantiateClass(tmpl.classType, tmpl.initialProperties);
        if (!spawned) continue;

        // Apply initial properties from template
        for (const auto& [propName, propVal] : tmpl.initialProperties) {
            if (auto* prop = spawned->findProperty(propName)) {
                if (prop->isAccessibleForSynthesis(activeLaw, author)) {
                    prop->setValue(propVal);
                }
            } else {
                spawned->setDynamicProperty(propName, propVal);
            }
        }
        
        // Execute mappings from inputs to this spawned Singular
        for (const auto& mapping : mappings) {
            // Very simplified: just grab the first input's source property if valid
            if (inputs.empty()) continue;
            Singular* primarySource = inputs[0];

            if (auto* sourceProp = primarySource->findProperty(mapping.source.toString())) {
                if (!sourceProp->isAccessibleForSynthesis(activeLaw, author)) continue;
                
                auto sourceVal = sourceProp->value();
                std::map<std::string, double> numVars;
                std::map<std::string, std::string> strVars;
                double x = 0.0;
                double num;
                if (propertyValueToNumber(sourceVal, num)) {
                    x = num;
                } else if (auto strPtr = std::get_if<std::string>(&sourceVal)) {
                    strVars[mapping.exact.inputVariable] = *strPtr;
                }
                
                auto mappedVal = mapping.apply(x, numVars, strVars, primarySource);
                if (mappedVal) {
                    if (auto* targetProp = spawned->findProperty(mapping.target.toString())) {
                        if (targetProp->isAccessibleForSynthesis(activeLaw, author)) {
                            targetProp->setValue(*mappedVal);
                        }
                    } else {
                        spawned->setDynamicProperty(mapping.target.toString(), *mappedVal);
                    }
                }
            }
        }
        
        outputs.push_back(spawned);
    }
    
    return outputs;
}
