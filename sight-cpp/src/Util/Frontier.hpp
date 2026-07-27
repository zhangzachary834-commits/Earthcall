#pragma once

#include <type_traits>
#include <cstdint>
#include <utility>
#include <stdexcept>

namespace Frontier {

/**
 * Trait holding the schema version and previous type in the chain.
 * Specialize this for your versioned structs.
 */
template <typename T>
struct VersionInfo {
    static constexpr uint32_t VERSION = 0;
    using Previous = void;
};

/**
 * Trait indicating if a struct is the frontier (latest) version for its chain.
 */
template <typename T>
struct IsFrontier : std::false_type {};

/**
 * Forward declaration of the user-specialized upgrade function.
 * Users must implement: 
 *   template <> NewT upgrade<OldT, NewT>(OldT&& old_data)
 */
template <typename OldT, typename NewT>
NewT upgrade(OldT&& old_data) {
    static_assert(sizeof(OldT) == 0, "Upgrade path not implemented for these versions!");
    return NewT{};
}

// -------------------------------------------------------------------------
// Metaprogramming to find the "Next" version in a chain
// -------------------------------------------------------------------------

namespace detail {
    template <typename T, typename Old, typename = void>
    struct IsNextOf : std::false_type {};

    template <typename T, typename Old>
    struct IsNextOf<T, Old, std::void_t<typename VersionInfo<T>::Previous>>
        : std::is_same<typename VersionInfo<T>::Previous, Old> {};

    template <typename TargetT, typename OldT, typename CurrentT = TargetT, bool is_direct = IsNextOf<CurrentT, OldT>::value>
    struct FindNext;

    template <typename TargetT, typename OldT, typename CurrentT>
    struct FindNext<TargetT, OldT, CurrentT, true> {
        using Type = CurrentT;
    };

    template <typename TargetT, typename OldT, typename CurrentT>
    struct FindNext<TargetT, OldT, CurrentT, false> {
        using Prev = typename VersionInfo<CurrentT>::Previous;
        // If Prev is void, it means the chain is broken or OldT is not in the chain leading to TargetT
        static_assert(!std::is_void_v<Prev>, "Broken version chain or OldT not in TargetT's ancestry.");
        using Type = typename FindNext<TargetT, OldT, Prev>::Type;
    };
}

/**
 * Retrieves the type that immediately follows OldT in the chain leading up to TargetT.
 */
template <typename TargetT, typename OldT>
using NextVersionT = typename detail::FindNext<TargetT, OldT>::Type;

// -------------------------------------------------------------------------
// Recursive Migration Pipeline
// -------------------------------------------------------------------------

/**
 * Migrates an arbitrary old version into the specified target version.
 */
template <typename TargetT, typename CurrentT>
TargetT migrate_to_target(CurrentT&& data) {
    if constexpr (std::is_same_v<std::decay_t<CurrentT>, TargetT>) {
        return std::forward<CurrentT>(data);
    } else {
        using NextT = NextVersionT<TargetT, std::decay_t<CurrentT>>;
        NextT next_data = upgrade<std::decay_t<CurrentT>, NextT>(std::forward<CurrentT>(data));
        return migrate_to_target<TargetT>(std::move(next_data));
    }
}

/**
 * Migrates an old version all the way to its Frontier version.
 */
template <typename FrontierT, typename CurrentT>
FrontierT migrate_to_frontier(CurrentT&& data) {
    static_assert(IsFrontier<FrontierT>::value, "Target must be a Frontier version!");
    return migrate_to_target<FrontierT>(std::forward<CurrentT>(data));
}

// -------------------------------------------------------------------------
// Helper for Deserialization Dispatch
// -------------------------------------------------------------------------
namespace detail {
    template <typename FrontierT, typename Reader, typename CurrentT>
    FrontierT try_load(Reader& reader, uint32_t version, bool& success) {
        if constexpr (std::is_void_v<CurrentT>) {
            success = false;
            return FrontierT{}; // Base case: version not found
        } else {
            if (version == VersionInfo<CurrentT>::VERSION) {
                CurrentT old_data;
                // Reader must implement `void read(T&)` for all versioned types
                reader.read(old_data);
                success = true;
                return migrate_to_target<FrontierT>(std::move(old_data));
            } else {
                using PrevT = typename VersionInfo<CurrentT>::Previous;
                return try_load<FrontierT, Reader, PrevT>(reader, version, success);
            }
        }
    }
}

/**
 * Given a reader that provides the schema version and knows how to populate
 * versioned structs, this loads the appropriate historical struct and migrates
 * it to the Frontier version.
 */
template <typename FrontierT, typename Reader>
FrontierT load_frontier(Reader& reader) {
    static_assert(IsFrontier<FrontierT>::value, "Target must be a Frontier version!");
    
    uint32_t version = reader.readVersion();
    bool success = false;
    FrontierT result = detail::try_load<FrontierT, Reader, FrontierT>(reader, version, success);
    
    if (!success) {
        throw std::runtime_error("Unknown serialization version encountered.");
    }
    return result;
}

} // namespace Frontier
