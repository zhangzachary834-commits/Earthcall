#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include <algorithm>

PropertyCellTable& PropertyCellTable::instance() {
    static PropertyCellTable s_table;
    return s_table;
}

CellId PropertyCellTable::allocateCell(PropertyValue initialValue,
                                       CellOwnershipMode mode,
                                       Singular* bearer,
                                       Earthcall::StringId rootPropertyId) {
    uint32_t slot = 0;
    if (!_freeSlots.empty()) {
        slot = _freeSlots.back();
        _freeSlots.pop_back();
    } else {
        slot = static_cast<uint32_t>(_cells.size());
        _cells.emplace_back();
    }

    ValueCell& cell = _cells[slot];
    if (cell.generation == 0) cell.generation = 1;
    cell.id = CellId{slot, cell.generation};
    cell.value = std::move(initialValue);
    cell.revision = 0;
    cell.observers.clear();
    cell.derivedEdges.clear();

    if (mode == CellOwnershipMode::Unique) {
        cell.strongCount = 1;
        cell.isUniqueLocked = true;
    } else if (mode == CellOwnershipMode::Shared) {
        cell.strongCount = 1;
        cell.isUniqueLocked = false;
    } else if (mode == CellOwnershipMode::Weak) {
        cell.strongCount = 0;
        cell.isUniqueLocked = false;
    } else {
        cell.strongCount = 0;
        cell.isUniqueLocked = false;
    }

    if (bearer && rootPropertyId.isValid()) {
        cell.observers.push_back({bearer, rootPropertyId, mode});
    }

    return CellId{slot, cell.generation};
}

ValueCell* PropertyCellTable::getCell(CellId id) {
    if (!id.valid() || id.slot >= _cells.size()) return nullptr;
    ValueCell& cell = _cells[id.slot];
    if (cell.generation != id.generation) return nullptr;
    return &cell;
}

const ValueCell* PropertyCellTable::getCell(CellId id) const {
    if (!id.valid() || id.slot >= _cells.size()) return nullptr;
    const ValueCell& cell = _cells[id.slot];
    if (cell.generation != id.generation) return nullptr;
    return &cell;
}

bool PropertyCellTable::isValid(CellId id) const {
    return getCell(id) != nullptr;
}

bool PropertyCellTable::isExpired(CellId id) const {
    return !isValid(id);
}

bool PropertyCellTable::attachObserver(CellId id,
                                       Singular* bearer,
                                       Earthcall::StringId rootPropertyId,
                                       CellOwnershipMode mode) {
    if (!id.valid() || id.slot >= _cells.size()) return false;
    ValueCell& cell = _cells[id.slot];
    if (cell.generation != id.generation) return false;

    // If already observing this cell, update mode if compatible and return true
    for (auto& obs : cell.observers) {
        if (obs.bearer == bearer && obs.rootPropertyId == rootPropertyId) {
            if (obs.mode == mode) return true;
            if (mode == CellOwnershipMode::Unique) {
                if (cell.strongCount > 1 || (cell.strongCount == 1 && obs.mode == CellOwnershipMode::Weak)) {
                    return false;
                }
                if (obs.mode == CellOwnershipMode::Weak) ++cell.strongCount;
                cell.isUniqueLocked = true;
                obs.mode = CellOwnershipMode::Unique;
                return true;
            }
            if (mode == CellOwnershipMode::Shared) {
                if (obs.mode == CellOwnershipMode::Weak) {
                    if (cell.isUniqueLocked) return false;
                    ++cell.strongCount;
                } else if (obs.mode == CellOwnershipMode::Unique) {
                    cell.isUniqueLocked = false;
                }
                obs.mode = CellOwnershipMode::Shared;
                return true;
            }
            if (mode == CellOwnershipMode::Weak) {
                if (obs.mode == CellOwnershipMode::Unique || obs.mode == CellOwnershipMode::Shared) {
                    if (cell.strongCount <= 1) return false;
                    --cell.strongCount;
                    if (obs.mode == CellOwnershipMode::Unique) cell.isUniqueLocked = false;
                }
                obs.mode = CellOwnershipMode::Weak;
                return true;
            }
            obs.mode = mode;
            return true;
        }
    }

    if (mode == CellOwnershipMode::Shared) {
        if (cell.isUniqueLocked) {
            return false; // UniqueOwnershipRefused
        }
        ++cell.strongCount;
    } else if (mode == CellOwnershipMode::Unique) {
        if (cell.strongCount > 0) {
            return false;
        }
        cell.strongCount = 1;
        cell.isUniqueLocked = true;
    } else if (mode == CellOwnershipMode::Weak) {
        if (cell.strongCount == 0) {
            return false;
        }
    }

    cell.observers.push_back({bearer, rootPropertyId, mode});
    return true;
}

void PropertyCellTable::detachObserver(CellId id,
                                       Singular* bearer,
                                       Earthcall::StringId rootPropertyId) {
    if (!id.valid() || id.slot >= _cells.size()) return;
    ValueCell& cell = _cells[id.slot];
    if (cell.generation != id.generation) return;

    bool found = false;
    CellOwnershipMode detachedMode = CellOwnershipMode::InlineValue;
    for (auto it = cell.observers.begin(); it != cell.observers.end(); ++it) {
        if (it->bearer == bearer && it->rootPropertyId == rootPropertyId) {
            detachedMode = it->mode;
            cell.observers.erase(it);
            found = true;
            break;
        }
    }

    if (!found) return;

    if (detachedMode == CellOwnershipMode::Unique || detachedMode == CellOwnershipMode::Shared) {
        if (cell.strongCount > 0) {
            --cell.strongCount;
        }
        if (detachedMode == CellOwnershipMode::Unique) {
            cell.isUniqueLocked = false;
        }
    }

    if (cell.strongCount == 0) {
        cell.generation++;
        if (cell.generation == 0) cell.generation = 1;
        cell.value = PropertyValue{};
        cell.observers.clear();
        cell.derivedEdges.clear();
        cell.isUniqueLocked = false;
        _freeSlots.push_back(id.slot);
    }
}

bool PropertyCellTable::convertUniqueToShared(CellId id,
                                              Singular* owner,
                                              Earthcall::StringId rootPropertyId) {
    if (!id.valid() || id.slot >= _cells.size()) return false;
    ValueCell& cell = _cells[id.slot];
    if (cell.generation != id.generation) return false;
    if (!cell.isUniqueLocked) return true;

    bool isOwner = false;
    for (auto& obs : cell.observers) {
        if (obs.bearer == owner && obs.rootPropertyId == rootPropertyId && obs.mode == CellOwnershipMode::Unique) {
            obs.mode = CellOwnershipMode::Shared;
            isOwner = true;
            break;
        }
    }
    if (!isOwner) return false;

    cell.isUniqueLocked = false;
    return true;
}

void PropertyCellTable::notifyMutation(CellId id) {
    if (!id.valid() || id.slot >= _cells.size()) return;
    ValueCell& cell = _cells[id.slot];
    if (cell.generation != id.generation) return;

    // Re-entrancy and cyclic-derivation guard: prevent infinite recursion if
    // two derived properties depend on one another in a cycle.
    static thread_local std::vector<CellId> s_activeNotifications;
    for (const auto& active : s_activeNotifications) {
        if (active == id) return;
    }
    struct NotificationGuard {
        NotificationGuard(CellId c) { s_activeNotifications.push_back(c); }
        ~NotificationGuard() { s_activeNotifications.pop_back(); }
    } guard(id);

    ++cell.revision;
    auto observersCopy = cell.observers;
    for (const auto& obs : observersCopy) {
        if (obs.bearer && obs.rootPropertyId.isValid()) {
            if (obs.mode == CellOwnershipMode::Shared || obs.mode == CellOwnershipMode::Unique) {
                obs.bearer->syncDynamicPropertyFromCell(obs.rootPropertyId, cell.value);
            }
            Property* p = obs.bearer->findProperty(obs.rootPropertyId);
            if (p && p->typeName() != "dynamic") {
                p->setValue(cell.value);
            }
            Singular::notifyPropertyChanged(obs.bearer, Earthcall::StringInterner::resolve(obs.rootPropertyId));
        }
    }

    if (id.slot >= _cells.size() || _cells[id.slot].generation != id.generation) return;
    auto derivedCopy = _cells[id.slot].derivedEdges;
    for (const auto& edge : derivedCopy) {
        if (edge.recompute) {
            edge.recompute();
        }
    }
}

void PropertyCellTable::registerDerivedEdge(CellId sourceCellId,
                                            Singular* destBearer,
                                            Earthcall::StringId destPropId,
                                            std::function<void()> recompute,
                                            Singular* subjectBearer,
                                            const Singular* lawBearer) {
    ValueCell* cell = getCell(sourceCellId);
    if (!cell) return;
    for (auto& edge : cell->derivedEdges) {
        if (edge.destBearer == destBearer && edge.destPropId == destPropId) {
            edge.subjectBearer = subjectBearer;
            edge.lawBearer = lawBearer;
            edge.recompute = std::move(recompute);
            return;
        }
    }
    cell->derivedEdges.push_back(DerivedEdge{
        sourceCellId, destBearer, destPropId, subjectBearer, lawBearer, std::move(recompute)});
}

void PropertyCellTable::removeDerivedEdgesForBearer(const Singular* bearer) {
    if (!bearer) return;
    for (auto& cell : _cells) {
        cell.derivedEdges.erase(
            std::remove_if(cell.derivedEdges.begin(), cell.derivedEdges.end(),
                           [bearer](const DerivedEdge& edge) {
                               return edge.destBearer == bearer ||
                                      edge.subjectBearer == bearer ||
                                      edge.lawBearer == bearer;
                           }),
            cell.derivedEdges.end());
    }
}

void PropertyCellTable::retargetBearer(Singular* oldBearer, Singular* newBearer) {
    if (!oldBearer || oldBearer == newBearer) return;
    for (auto& cell : _cells) {
        for (auto& obs : cell.observers) {
            if (obs.bearer == oldBearer) {
                obs.bearer = newBearer;
            }
        }
        for (auto& edge : cell.derivedEdges) {
            if (edge.destBearer == oldBearer) {
                edge.destBearer = newBearer;
            }
            if (edge.subjectBearer == oldBearer) {
                edge.subjectBearer = newBearer;
            }
            if (edge.lawBearer == oldBearer) {
                edge.lawBearer = newBearer;
            }
        }
    }
}

void PropertyCellTable::detachAllForBearer(Singular* bearer) {
    if (!bearer) return;
    removeDerivedEdgesForBearer(bearer);
    for (uint32_t slot = 0; slot < _cells.size(); ++slot) {
        ValueCell& cell = _cells[slot];
        if (cell.generation == 0) continue;
        for (std::size_t i = 0; i < cell.observers.size(); ) {
            if (cell.observers[i].bearer == bearer) {
                auto mode = cell.observers[i].mode;
                cell.observers.erase(cell.observers.begin() + i);
                if (mode == CellOwnershipMode::Unique || mode == CellOwnershipMode::Shared) {
                    if (cell.strongCount > 0) --cell.strongCount;
                    if (mode == CellOwnershipMode::Unique) cell.isUniqueLocked = false;
                }
                if (cell.strongCount == 0) {
                    cell.generation++;
                    if (cell.generation == 0) cell.generation = 1;
                    cell.value = PropertyValue{};
                    cell.observers.clear();
                    cell.derivedEdges.clear();
                    cell.isUniqueLocked = false;
                    _freeSlots.push_back(slot);
                    break;
                }
            } else {
                ++i;
            }
        }
    }
}

PropertyValue PropertyCellTable::deepCopy(const PropertyValue& src) {
    return std::visit([](const auto& x) -> PropertyValue {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<PropertyList>>) {
            if (!x) return std::shared_ptr<PropertyList>{};
            auto copy = std::make_shared<PropertyList>();
            copy->elements.reserve(x->elements.size());
            for (const auto& el : x->elements) {
                copy->elements.push_back(PropertyCellTable::deepCopy(el));
            }
            return copy;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<PropertyDict>>) {
            if (!x) return std::shared_ptr<PropertyDict>{};
            auto copy = std::make_shared<PropertyDict>();
            for (const auto& [k, v] : x->elements) {
                copy->elements[k] = PropertyCellTable::deepCopy(v);
            }
            return copy;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<OntoMath::ScalarField>>) {
            if (!x) return std::shared_ptr<OntoMath::ScalarField>{};
            return std::make_shared<OntoMath::ScalarField>(*x);
        } else if constexpr (std::is_same_v<T, std::shared_ptr<OntoMath::VectorField>>) {
            if (!x) return std::shared_ptr<OntoMath::VectorField>{};
            return std::make_shared<OntoMath::VectorField>(*x);
        } else {
            return x;
        }
    }, src);
}

void PropertyCellTable::clear() {
    _cells.clear();
    _freeSlots.clear();
}
