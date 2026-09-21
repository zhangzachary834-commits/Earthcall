#pragma once

#include <cstdint>

namespace OntoMath { struct Piecewise; }

namespace Rendering {

// Renderer-facing projection of participating-medium density.
//
// This is deliberately not ontology: a FieldNode remains the authored being.
// The projection exists so the compiler never has to guess whether a null
// pointer means "no medium" or "please reinterpret the legacy generic field."
struct VolumeDensityBinding {
    enum class Kind : std::uint8_t {
        LegacyField, // compatibility: consume the historical FieldNode scalar field
        None,        // resolved truth: this draw has no participating medium
        Authored     // consume densityExpr as D(p,t)
    };

    Kind kind = Kind::LegacyField;
    const OntoMath::Piecewise* densityExpr = nullptr;
    std::uint64_t revision = 0;
    double temporalCoordinate = 0.0;
    double temporalDelta = 0.0;

    static VolumeDensityBinding legacy() {
        return {};
    }

    static VolumeDensityBinding none() {
        VolumeDensityBinding b;
        b.kind = Kind::None;
         return b;
    }

    static VolumeDensityBinding authored(const OntoMath::Piecewise* expr,
                                         std::uint64_t contentRevision,
                                         double t = 0.0,
                                         double dt = 0.0) {
        VolumeDensityBinding b;
        b.kind = Kind::Authored;
        b.densityExpr = expr;
        b.revision = contentRevision;
        b.temporalCoordinate = t;
        b.temporalDelta = dt;
         return b;
    }
};

} // namespace Rendering
