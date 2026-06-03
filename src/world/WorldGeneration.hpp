/**
 * **************************************************************************
 * @file     WorldGeneration.hpp
 * @brief    Declares deterministic macro-world generation helpers
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <string_view>

#include "core/Rng.hpp"
#include "world/WorldTypes.hpp"

namespace rl {

[[nodiscard]] bool worldPointInBounds(Point point) noexcept;
[[nodiscard]] bool localScreenInBounds(Point point) noexcept;
[[nodiscard]] std::size_t worldCellIndex(Point point);

[[nodiscard]] double valueNoise2d(Rng::Seed seed, double x, double y) noexcept;
[[nodiscard]] double fbmNoise2d(Rng::Seed seed,
                                double x,
                                double y,
                                int octaves = 4) noexcept;

[[nodiscard]] BiomeVariant deriveBiomeVariant(const WorldCellFields& fields);
[[nodiscard]] std::string_view biomeVariantId(BiomeVariant biome) noexcept;
[[nodiscard]] BiomeVariant biomeVariantFromId(std::string_view id) noexcept;

} // namespace rl