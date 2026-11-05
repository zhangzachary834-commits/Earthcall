#pragma once

#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "Singularity/Core/StringId.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

class Singular;

struct CellId {
    uint32_t slot = 0;
    uint32_t generation = 0;

    bool valid() const noexcept { return generation != 0; }
    bool operator==(const CellId& o) const noexcept {
        return slot == o.slot && generation == o.generation;
    }
    bool operator!=(const CellId& o) const noexcept {
        return !(*this == o);
    }
};

enum class CellOwnershipMode : uint8_t {
    InlineValue = 0, // Default fast path: unshared inline value on Singular
    Shared      = 1, // Ref-counted strong shared ownership
    Unique      = 2, // Single strong owner; sharing refuses until converted
    Weak        = 3  // Generation-checked non-owning observation
};

struct CellObserver {
    Singular* bearer = nullptr;
    Earthcall::StringId rootPropertyId;
    CellOwnershipMode mode = CellOwnershipMode::InlineValue;

    bool operator==(const CellObserver& o) const noexcept {
        return bearer == o.bearer &&
               rootPropertyId == o.rootPropertyId &&
               mode == o.mode;
    }
};

struct DerivedEdge {
    CellId sourceCellId;
    Singular* destBearer = nullptr;
    Earthcall::StringId destPropId;
    Singular* subjectBearer = nullptr;
    const Singular* lawBearer = nullptr;
    std::function<void()> recompute;
};

struct ValueCell {
    CellId id;
    PropertyValue value;
    uint32_t generation = 1;
    uint32_t strongCount = 0;
    bool isUniqueLocked = false;
    uint64_t revision = 0;
    std::vector<CellObserver> observers;
    std::vector<DerivedEdge> derivedEdges;
};

class PropertyCellTable {
public:
    static PropertyCellTable& instance();

    // Allocate a new cell slot with initial value and ownership mode.
    CellId allocateCell(PropertyValue initialValue,
                        CellOwnershipMode mode,
                        Singular* bearer,
                        Earthcall::StringId rootPropertyId);

    // Look up a live cell. Returns nullptr if slot is invalid or generation does not match.
    ValueCell* getCell(CellId id);
    const ValueCell* getCell(CellId id) const;

    bool isValid(CellId id) const;
    bool isExpired(CellId id) const;

    // Attach an observer to an existing cell.
    // If mode == Shared and cell.isUniqueLocked, returns false.
    // If mode == Unique and cell.strongCount > 0, returns false.
    bool attachObserver(CellId id,
                        Singular* bearer,
                        Earthcall::StringId rootPropertyId,
                        CellOwnershipMode mode);

    // Detach an observer from a cell. If strongCount reaches 0, cell is reclaimed.
    void detachObserver(CellId id,
                        Singular* bearer,
                        Earthcall::StringId rootPropertyId);

    // Explicitly unlock and convert a Unique cell to Shared mode.
    bool convertUniqueToShared(CellId id,
                               Singular* owner,
                               Earthcall::StringId rootPropertyId);

    // Monotonically bump revision and notify all observers via Singular::notifyPropertyChanged.
    void notifyMutation(CellId id);

    // Reactive derived edge management (BindingMode::Derive)
    void registerDerivedEdge(CellId sourceCellId,
                             Singular* destBearer,
                             Earthcall::StringId destPropId,
                             std::function<void()> recompute,
                             Singular* subjectBearer = nullptr,
                             const Singular* lawBearer = nullptr);
    void removeDerivedEdgesForBearer(const Singular* bearer);

    // Observer retargeting on Singular move
    void retargetBearer(Singular* oldBearer, Singular* newBearer);

    // Detach all observer edges referencing bearer across all active cells (called during ~Singular()).
    void detachAllForBearer(Singular* bearer);

    // Deep copy helper for CopyValue mode.
    static PropertyValue deepCopy(const PropertyValue& src);

    // Reset table for test fixtures if needed.
    void clear();

private:
    PropertyCellTable() = default;
    std::vector<ValueCell> _cells;
    std::vector<uint32_t> _freeSlots;
};
