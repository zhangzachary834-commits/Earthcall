#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Core/StringId.hpp"
#include "json.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace SingularPersistence {

// Session/cohort remapping context to preserve alias topology across save/load.
// Two bearers referencing the same serialized cellKey attach to the exact same
// live CellId upon load. When a cohort finishes hydrating, any cell that was
// only referenced by Weak bindings (with zero Shared/Unique strong owners) is
// immediately expired so Weak reads honestly return PathResult::ExpiredCell.
struct CohortContext {
    std::unordered_map<std::string, CellId> remappedCells;

    void clear() { remappedCells.clear(); }

    void finalize() {
        for (const auto& [key, cellId] : remappedCells) {
            ValueCell* cell = PropertyCellTable::instance().getCell(cellId);
            if (cell && cell->strongCount == 0) {
                cell->generation++;
                if (cell->generation == 0) cell->generation = 1;
                cell->value = PropertyValue{};
                cell->observers.clear();
                cell->derivedEdges.clear();
                cell->isUniqueLocked = false;
            }
        }
    }

    static CohortContext*& activeSlot() {
        static thread_local CohortContext* s_active = nullptr;
        return s_active;
    }

    static CohortContext* active() {
        return activeSlot();
    }
};

// RAII scope that establishes a fresh CohortContext for a multi-being load
// operation (Zone hydration, set-to-set restoration, or cohort deserialization)
// and finalizes orphaned weak-only cells on exit.
struct CohortScope {
    CohortContext context;
    CohortContext* previous = nullptr;
    bool ownsContext = false;

    explicit CohortScope(bool forceNew = false) {
        previous = CohortContext::activeSlot();
        if (!previous || forceNew) {
            CohortContext::activeSlot() = &context;
            ownsContext = true;
        }
    }

    ~CohortScope() {
        if (ownsContext) {
            context.finalize();
            CohortContext::activeSlot() = previous;
        }
    }

    CohortContext& get() {
        return ownsContext ? context : *previous;
    }
};

// Serialize cell bindings for a single Singular.
inline nlohmann::json serializeCellBindings(const Singular& being) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& [propId, cellId] : being.propertyCells()) {
        CellOwnershipMode mode = being.getPropertyCellMode(propId);
        std::string modeStr = "Shared";
        if (mode == CellOwnershipMode::Unique) modeStr = "Unique";
        else if (mode == CellOwnershipMode::Weak) modeStr = "Weak";

        const ValueCell* cell = PropertyCellTable::instance().getCell(cellId);
        if (!cell && mode != CellOwnershipMode::Weak) continue;

        std::string cellKey = std::to_string(cellId.slot) + ":" + std::to_string(cellId.generation);
        std::string propName = Earthcall::StringInterner::resolve(propId);

        nlohmann::json item{
            {"property", propName},
            {"mode", modeStr},
            {"cellKey", cellKey},
            {"value", cell ? propertyValueToJson(cell->value) : nlohmann::json(nullptr)}
        };
        if (!cell) item["expired"] = true;
        arr.push_back(std::move(item));
    }
    return arr;
}

// Deserialize cell bindings for a single Singular using cohort remapping.
// Order-independent: whether a Weak observer or a Shared/Unique strong owner
// is encountered first in the serialized stream, all bearers referencing the
// same cellKey bind to the same CellId, and strongCount reflects the exact
// number of Shared/Unique owners.
inline void deserializeCellBindings(Singular& being,
                                    const nlohmann::json& bindingsJson,
                                    CohortContext& cohort) {
    if (!bindingsJson.is_array()) return;
    for (const auto& item : bindingsJson) {
        if (!item.is_object()) continue;
        std::string propName = item.value("property", "");
        if (propName.empty()) continue;
        std::string modeStr = item.value("mode", "Shared");
        std::string cellKey = item.value("cellKey", "");
        if (cellKey.empty()) continue;

        CellOwnershipMode mode = CellOwnershipMode::Shared;
        if (modeStr == "Unique") mode = CellOwnershipMode::Unique;
        else if (modeStr == "Weak") mode = CellOwnershipMode::Weak;

        PropertyValue cellVal;
        const bool hasValue = item.contains("value") && !item["value"].is_null();
        if (hasValue) {
            cellVal = propertyValueFromJson(item["value"]);
        }

        Earthcall::StringId propId = Earthcall::StringInterner::intern(propName);

        CellId liveId;
        auto it = cohort.remappedCells.find(cellKey);
        if (it != cohort.remappedCells.end()) {
            liveId = it->second;
            ValueCell* cell = PropertyCellTable::instance().getCell(liveId);
            if (cell) {
                if (hasValue && std::holds_alternative<std::monostate>(cell->value) &&
                    !std::holds_alternative<std::monostate>(cellVal)) {
                    cell->value = cellVal;
                }
                // Temporarily ensure strongCount >= 1 if attaching a Weak observer before strong owner
                const uint32_t savedStrong = cell->strongCount;
                if (mode == CellOwnershipMode::Weak && savedStrong == 0) {
                    cell->strongCount = 1;
                }
                being.bindPropertyCell(propId, liveId, mode);
                if (mode == CellOwnershipMode::Weak && savedStrong == 0) {
                    cell->strongCount = 0;
                }
            }
        } else {
            // Allocate as unattached Shared first so bindPropertyCell succeeds even if Weak arrives first
            liveId = PropertyCellTable::instance().allocateCell(
                cellVal, CellOwnershipMode::Shared, nullptr, Earthcall::StringId{});
            if (ValueCell* cell = PropertyCellTable::instance().getCell(liveId)) {
                if (mode == CellOwnershipMode::Weak) {
                    cell->strongCount = 1;
                }
                being.bindPropertyCell(propId, liveId, mode);
                if (mode == CellOwnershipMode::Weak) {
                    cell->strongCount = 0;
                    cell->isUniqueLocked = false;
                } else if (mode == CellOwnershipMode::Unique) {
                    cell->strongCount = 1;
                    cell->isUniqueLocked = true;
                } else {
                    cell->strongCount = 1;
                    cell->isUniqueLocked = false;
                }
            }
            cohort.remappedCells[cellKey] = liveId;
        }

        // Synchronize bearer storage for strong owners without creating shadow dynamic entries for C++ properties
        if (mode != CellOwnershipMode::Weak) {
            if (const ValueCell* cell = PropertyCellTable::instance().getCell(being.getPropertyCell(propId))) {
                Property* prop = being.findProperty(propId);
                if (prop && prop->typeName() != "dynamic") {
                    prop->setValue(cell->value);
                } else {
                    being.syncDynamicPropertyFromCell(propId, cell->value);
                }
            }
        }
    }
}

// Cohort-level serialization helpers
inline nlohmann::json serializeCohort(const std::vector<const Singular*>& beings) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto* being : beings) {
        if (!being) continue;
        nlohmann::json b{
            {"id", being->getIdentifier()},
            {"bindings", serializeCellBindings(*being)}
        };
        j.push_back(std::move(b));
    }
    return j;
}

inline void deserializeCohort(const std::vector<Singular*>& beings,
                              const nlohmann::json& cohortJson,
                              CohortContext& cohort) {
    if (!cohortJson.is_array()) return;
    std::unordered_map<std::string, Singular*> byId;
    for (auto* b : beings) {
        if (b) byId[b->getIdentifier()] = b;
    }
    for (const auto& item : cohortJson) {
        std::string id = item.value("id", "");
        auto it = byId.find(id);
        if (it != byId.end() && item.contains("bindings")) {
            deserializeCellBindings(*it->second, item["bindings"], cohort);
        }
    }
    cohort.finalize();
}

} // namespace SingularPersistence
