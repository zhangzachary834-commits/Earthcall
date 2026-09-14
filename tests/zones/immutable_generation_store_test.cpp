#include "Identity/SingularId.hpp"
#include "Singularity/Storage/ImmutableGeneration.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr "\n"; return EXIT_FAILURE; } } while (false)

int main() {
    const auto root = std::filesystem::temp_directory_path() /
                      ("earthcall_generations_" + Identity::SingularId::mintOpaque().abbreviated());
    std::filesystem::remove_all(root);
    const Identity::SingularId zoneId = Identity::SingularId::mintOpaque();
    Singularity::Storage::ImmutableGenerationStore store(root);

    const auto first = store.writeJson("zone", zoneId, {{"slug", "Go"}, {"value", 1}});
    CHECK(first.ok && !first.alreadyExisted);
    const auto duplicate = store.writeJson("zone", zoneId, {{"slug", "Go"}, {"value", 1}});
    CHECK(duplicate.ok && duplicate.alreadyExisted);
    CHECK(duplicate.ref.generation == first.ref.generation);

    nlohmann::json loaded;
    std::string error;
    CHECK(store.readVerifiedJson(first.ref, loaded, error));
    CHECK(loaded.at("slug") == "Go");
    CHECK(store.publishHead("zone", zoneId, "", first.ref, "transaction-1", error));

    const auto second = store.writeJson("zone", zoneId, {{"slug", "Go"}, {"value", 2}});
    CHECK(second.ok && second.ref.generation != first.ref.generation);
    CHECK(!store.publishHead("zone", zoneId, "stale-head", second.ref, "transaction-2", error));
    CHECK(store.publishHead("zone", zoneId, first.ref.generation, second.ref,
                            "transaction-2", error));
    CHECK(std::filesystem::exists(store.headPath("zone", zoneId).parent_path() /
                                  "head.previous.json"));

    // Corruption is refused and cannot advance a head.
    std::ofstream corrupt(root / second.ref.relativePath, std::ios::app);
    corrupt << 'x'; corrupt.close();
    CHECK(!store.readVerifiedJson(second.ref, loaded, error));

    std::filesystem::remove_all(root);
    std::cout << "immutable_generation_store_test: all checks passed\n";
    return EXIT_SUCCESS;
}
