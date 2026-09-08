1.  **Analyze the Optimization Opportunity**:
    *   The `Logger` class currently uses `std::unordered_map` for both `_categoryLevels` and `_streams`.
    *   The keys for these maps are `LogCategory`, which is a simple enum with 6 values (`Laws`, `System`, `Person`, `State`, `Language`, `Audio`).
    *   In a high-throughput logging scenario, repeatedly querying the `std::unordered_map` involves hashing and pointer chasing which is computationally more expensive than a simple array lookup. The benchmark shows a ~7x speedup for array lookup vs unordered map lookup (0.17s vs 1.20s for 100M iterations).
    *   `LogCategory` can be used directly as an index into a `std::array`. We can add a `Count` value to the enum to define the size of the array, or use a hardcoded constant. Since `Count` is standard, we'll add it.

2.  **Implementation**:
    *   **In `src/Singularity/Core/Logger.hpp`**:
        *   Add `Count` to `enum class LogCategory`.
        *   Replace `std::unordered_map<LogCategory, LogLevel> _categoryLevels` with `std::array<LogLevel, static_cast<size_t>(LogCategory::Count)> _categoryLevels`.
        *   Replace `std::unordered_map<LogCategory, CategoryStreams> _streams` with `std::array<std::unique_ptr<CategoryStreams>, static_cast<size_t>(LogCategory::Count)> _streams`.
            Wait, `CategoryStreams` has `std::ofstream`, which is non-copyable, so `std::array<CategoryStreams, static_cast<size_t>(LogCategory::Count)>` is fine but requires default constructibility.
            Actually, let's look at `CategoryStreams`. `std::ofstream` is default constructible, so `std::array<CategoryStreams, 6>` is completely fine and doesn't even need `std::unique_ptr`.
            Even better, we can just use `std::array<CategoryStreams, static_cast<size_t>(LogCategory::Count)> _streams;`.
            Let's check if we want an active flag or just check if `logFile.is_open()`.
    *   **In `src/Singularity/Core/Logger.cpp`**:
        *   Update all places where `_categoryLevels` is used.
        *   Update all places where `_streams` is accessed:
            *   Change `auto it = _streams.find(cat); if (it != _streams.end()) { ... }` to access by index.
            *   Update loops iterating over `_streams` to iterate over the array.
    *   Ensure thread-safety is maintained (it looks like `_categoryLevels` is protected by `_categoryLevelMutex` and `_streams` is used in the worker thread or protected by logic).

3.  **Refining the Array Idea**:
    *   Let's check if `std::array` is okay. `CategoryStreams` will be default constructed for all 6 categories.
    *   Currently, the constructor calls `ensureCategoryStreams` for all standard categories anyway:
        ```cpp
        // Initialize all standard category subdirectories and streams
        ensureCategoryStreams(LogCategory::Laws);
        ensureCategoryStreams(LogCategory::System);
        ensureCategoryStreams(LogCategory::Person);
        ensureCategoryStreams(LogCategory::State);
        ensureCategoryStreams(LogCategory::Language);
        ensureCategoryStreams(LogCategory::Audio);
        ```
    *   So an array is perfectly suited here. All 6 categories are populated at startup.
    *   There's an `ensureCategoryStreams` function that does:
        ```cpp
        if (_streams.find(cat) != _streams.end()) return;
        ```
        If we use an array, this just becomes checking if `_streams[static_cast<size_t>(cat)].logFile.is_open()`.

4.  **Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.**

5.  **Create a PR** to document the change and its performance benefits.
