/**
 * **************************************************************************
 * @file     Prefab.hpp
 * @brief    Declares reusable prefab and sectional placement helpers
 * **************************************************************************
 */

#pragma once

#include <string>
#include <vector>

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

enum class PrefabPlacementMode {
    Random,
    Center,
    Northwest,
    Northeast,
    Southwest,
    Southeast,
};

struct PrefabTemplate {
    std::string name;
    std::vector<std::string> rows;
};

enum class PrefabHorizontalPlacement { Left, Center, Right };

enum class PrefabVerticalPlacement { Top, Center, Bottom };

struct PrefabSection {
    PrefabTemplate prefab;
    PrefabHorizontalPlacement horizontal = PrefabHorizontalPlacement::Center;
    PrefabVerticalPlacement vertical = PrefabVerticalPlacement::Center;
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
[[nodiscard]] int prefabWidth(const PrefabTemplate& prefab);
[[nodiscard]] int prefabHeight(const PrefabTemplate& prefab);

[[nodiscard]] PrefabPlacementResult placeSection(MapBuildResult& result,
                                                 const PrefabSection& section,
                                                 MapBuildOptions options);

[[nodiscard]] PrefabPlacementResult
placePrefab(MapBuildResult& result,
            const PrefabTemplate& prefab,
            Rng& rng,
            MapBuildOptions options,
            PrefabPlacementMode mode = PrefabPlacementMode::Random,
            int maxAttempts = 80);

} // namespace rl