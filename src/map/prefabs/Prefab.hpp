/**
 * **************************************************************************
 * @file     Prefab.hpp
 * @brief    Declares reusable prefab and sectional placement helpers
 * **************************************************************************
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

/** Selects deliberate or bounded-random placement of an authored map piece. */
enum class PrefabPlacementMode {
    Random,
    Center,
    Northwest,
    Northeast,
    Southwest,
    Southeast,
};

/** Authored terrain and content markers to overlay on a generated base. */
struct PrefabTemplate {
    std::string name;
    std::vector<std::string> rows;
};

enum class PrefabHorizontalPlacement { Left, Center, Right };

enum class PrefabVerticalPlacement { Top, Center, Bottom };

/** Anchors a prefab to a stable map section for narrative composition. */
struct PrefabSection {
    PrefabTemplate prefab;
    PrefabHorizontalPlacement horizontal = PrefabHorizontalPlacement::Center;
    PrefabVerticalPlacement vertical = PrefabVerticalPlacement::Center;
};

/** Limits a reusable room feature to an appropriate depth range. */
struct PrefabVault {
    PrefabTemplate prefab;
    int minimumDepth = 1;
    int maximumDepth = 999;
};

struct PrefabPlacementResult {
    bool placed = false;
    Point origin{};
    std::string reason;
};

[[nodiscard]] const PrefabTemplate& shrinePrefab();
[[nodiscard]] const PrefabTemplate& armoryVaultPrefab();
[[nodiscard]] PrefabTemplate prefabFromAscii(std::string name,
                                             std::vector<std::string> rows);
[[nodiscard]] PrefabTemplate
loadPrefabFromRexPaint(const std::filesystem::path& path, std::string name);
[[nodiscard]] std::vector<PrefabVault> roomVaultLibrary();
[[nodiscard]] std::vector<PrefabVault> vaultsForDepth(int depth);
[[nodiscard]] int prefabWidth(const PrefabTemplate& prefab);
[[nodiscard]] int prefabHeight(const PrefabTemplate& prefab);

/** Places an anchored piece while protecting start, stairs, and spawn data. */
[[nodiscard]] PrefabPlacementResult placeSection(MapBuildResult& result,
                                                 const PrefabSection& section,
                                                 MapBuildOptions options);

/** Places one authored piece without allowing it to own the whole map style. */
[[nodiscard]] PrefabPlacementResult
placePrefab(MapBuildResult& result,
            const PrefabTemplate& prefab,
            Rng& rng,
            MapBuildOptions options,
            PrefabPlacementMode mode = PrefabPlacementMode::Random,
            int maxAttempts = 80);

} // namespace rl
