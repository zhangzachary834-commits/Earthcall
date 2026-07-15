#pragma once

#include "Form/Singular/Property/PropertyPath.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/Expression.hpp"
#include "json.hpp"

#include <optional>

// Set-to-set with structure carried across (LAW_AND_CREATION_SYSTEM.md §7b):
// read `source` on a source-set member, pass it through `transform`, write
// `target` on the newborn. The new set is mathematically DERIVED from the
// old — modal information transferred, not bytes cloned. "Each new column's
// height = 1.5x its source's; radius = mean of the whole source set."
struct PropertyMapping {
    PropertyPath source;             // read from source-set member(s)
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
    std::optional<double> apply(double x, const Singular* guardSubject = nullptr) const {
        if (hasExact) {
            return exact.evaluate({{exact.inputVariable, x}}, guardSubject);
        }
        return transform.evaluate(x);
    }

    nlohmann::json toJson() const;
    static PropertyMapping fromJson(const nlohmann::json& j);
};
