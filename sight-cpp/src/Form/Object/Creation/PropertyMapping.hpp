#pragma once

#include "Form/Singular/Property/PropertyPath.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "json.hpp"

// Set-to-set with structure carried across (LAW_AND_CREATION_SYSTEM.md §7b):
// read `source` on a source-set member, pass it through `transform`, write
// `target` on the newborn. The new set is mathematically DERIVED from the
// old — modal information transferred, not bytes cloned. "Each new column's
// height = 1.5x its source's; radius = mean of the whole source set."
struct PropertyMapping {
    PropertyPath source;             // read from source-set member(s)
    CurveModel transform;            // y = f(x); identity = polynomial {0, 1}
    PropertyPath target;             // written on the newborn

    // PerMember pairs newborn i with source i (mod source count); the
    // aggregates fold the whole source set into one domain value.
    enum class Aggregate { PerMember = 0, Mean = 1, Sum = 2, Max = 3 };
    Aggregate agg = Aggregate::PerMember;

    nlohmann::json toJson() const;
    static PropertyMapping fromJson(const nlohmann::json& j);
};
