# String Interning Implementation Plan

**Related Documents:**
* [Audit: Property Lookup Complexity Audit](../audits/PROPERTY_LOOKUP_COMPLEXITY_AUDIT_2026-09-01.md)
* [Analysis: Property Lookup Complexity Analysis](../analysis/PROPERTY_LOOKUP_COMPLEXITY_ANALYSIS.md)

---

## The Goal
Replace the current pointer-chasing, string-allocating property lookup system with an $O(1)$-equivalent, cache-friendly integer comparison architecture using String Interning and a Structure of Arrays (SoA) layout.

---

## 1. Core Data Structure: `StringId`
Introduce a central string interning registry to convert strings to lightweight integers.

**Files:** `src/Singularity/Core/StringId.hpp` / `.cpp`
* `struct StringId { uint32_t value; }`
* `class StringInterner`:
  * `static StringId intern(const std::string& str)`
  * `static const std::string& resolve(StringId id)`
* Backed by `std::unordered_map<std::string, uint32_t>` and `std::vector<std::string>`.

---

## 2. Property Interface Updates
Properties must cache their interned names at creation so they don't allocate strings.

**Files:** `src/ConstructedBeing/Singular/Property/Property.hpp`, `PropertyRef.hpp`, `ComputedProperty.hpp`
* Add `virtual StringId nameId() const = 0;`.
* `PropertyRef` and `ComputedProperty` constructors will call `StringInterner::intern(_name)` and store the result.

---

## 3. The Lookup Layer (Structure of Arrays)
Optimize the `Singular` property registry to eliminate cache misses and pointer dereferencing during the linear scan.

**Files:** `src/ConstructedBeing/Singular/Singular.hpp`, `Singular.cpp`
* **Registered Properties:** Replace `std::vector<std::unique_ptr<Property>> _propertyRegistry` with a two-vector SoA layout:
  ```cpp
  std::vector<StringId> _propertyNames;
  std::vector<std::unique_ptr<Property>> _propertyPointers;
  ```
  `findProperty(StringId)` scans `_propertyNames` (which perfectly utilizes CPU cache) and uses the found index to access `_propertyPointers`.
* **Dynamic Properties:** Change `_dynamicProperties` to `std::unordered_map<StringId, PropertyValue>` or a similar flat vector scan for authored properties.

---

## 4. PropertyPath Pre-Calculation
Remove all runtime string concatenation from Law evaluations.

**Files:** `src/ConstructedBeing/Singular/Property/PropertyPath.hpp`, `PropertyPath.cpp`
* `PropertyPath::parse` will pre-calculate and intern all joined sub-path combinations (e.g., `shape`, `shape.color`, `shape.color.r`) at parse time.
* `PropertyPath::resolve` will query `Singular` using only these pre-calculated `StringId` arrays.

---

## Verification Plan
1. **Automated Tests:** Verify `no_black_box_test`, `channel_paths_test`, and `object_roundtrip_test` pass to ensure property semantics and serialization hold.
2. **Performance:** Run `frame_lag_test` and `webgpu_micro_mastery_lag_test` to confirm identical or improved frame timing.
3. **Manual Validation:** Ensure authored laws involving deeply nested properties (`@shape.material.reflectivity`) resolve cleanly in the engine.
