#pragma once

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "json.hpp"
#include <map>
#include <string>

#include <optional>

// Set-to-set with structure carried across (LAW_AND_CREATION_SYSTEM.md §7b):
// read `source` on a source-set member, pass it through `transform`, write
// `target` on the newborn. The new set is mathematically DERIVED from the
// old — modal information transferred, not bytes cloned. "Each new column's
// height = 1.5x its source's; radius = mean of the whole source set."
struct PropertyMapping {
    PropertyPath source;             // legacy single variable (often 'x')
    MathBindings bindings;           // multivariable bindings
    CurveModel transform;            // y = f(x); identity = polynomial {0, 1}
    PropertyPath target;             // written on the newborn

    // When authored, the EXACT transform supersedes the curve: the full
    // OntoMath piecewise algebra (bounded domains, transcendentals). The
    // input variable is exact.inputVariable; undefined math (outside every
    // piece) transfers NOTHING — a derivation never manifests undefined
    // values.
    bool hasExact = false;
    OntoMath::Piecewise exact;

    // PerMember pairs newborn i with source i (mod source count); the
    // aggregates fold the whole source set into one domain value.
    enum class Aggregate { PerMember = 0, Mean = 1, Sum = 2, Max = 3 };
    Aggregate agg = Aggregate::PerMember;

    // The transform applied: exact when authored (nullopt outside its
    // domain), otherwise the curve (total). The guard subject lets
    // expression-guarded pieces testify (the source member, per-member).
    std::optional<double> apply(double x, const std::map<std::string, double>& vars, const Singular* guardSubject = nullptr) const {
        if (hasExact) {
            std::map<std::string, PropertyValue> evalVars;
            for (const auto& [k, v] : vars) {
                evalVars[k] = PropertyValue(v);
            }
            evalVars[exact.inputVariable] = PropertyValue(x); // Ensure legacy input variable is set
            auto res = exact.evaluate(evalVars, guardSubject);
            if (res && std::holds_alternative<double>(*res)) {
                return std::get<double>(*res);
            }
            return std::nullopt;
        }
        return transform.evaluate(x);
    }

    nlohmann::json toJson() const;
    static PropertyMapping fromJson(const nlohmann::json& j);
};
