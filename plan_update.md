1. **Fix `LogLevel::Off` Regression**:
    - Include `<optional>` in `Logger.hpp`.
    - Change `std::array<LogLevel, ...> _categoryLevels` to `std::array<std::optional<LogLevel>, ...> _categoryLevels`.
    - Update `Logger::Logger()` to not initialize with `LogLevel::Off` (they default to `std::nullopt`).
    - Update `setCategoryLevel` to assign `level` to the optional.
    - Update `categoryLevel` to check `.has_value()` and return `.value()` or fall back to `_level.load()`.

2. **Fix Out-of-Bounds Access Risk**:
    - In all places where `LogCategory cat` is cast to `size_t idx` and accessed (e.g. `_streams[idx]`, `_categoryLevels[idx]`), wrap with a bounds check: `if (idx >= static_cast<std::size_t>(LogCategory::Count)) return ...;`.
    - Add bounds check to `categoryLevel` (return `_level.load()` if out of bounds).
    - Add bounds check to `setCategoryLevel` (return early if out of bounds).
    - Add bounds check to `ensureCategoryStreams`.
    - Add bounds check in `log` where EMSCRIPTEN writes directly.
    - Add bounds check in `backgroundWorker` where batch entries are processed.

3. **Re-run tests**: Ensure `logger_test` passes.
4. **Complete pre-commit steps**.
