/**
 * **************************************************************************
 * @file     WorldGeneration.hpp
 * @brief    Declares deterministic macro-world generation helpers
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Rng.hpp"
#include "world/WorldTypes.hpp"

namespace rl {

[[nodiscard]] bool worldPointInBounds(Point point) noexcept;
[[nodiscard]] bool localScreenInBounds(Point point) noexcept;
[[nodiscard]] std::size_t worldCellIndex(Point point);
[[nodiscard]] std::string worldCellKey(Point point);

[[nodiscard]] WorldCellDescriptor* worldCellAt(WorldMapState& world,
                                               Point point);
[[nodiscard]] const WorldCellDescriptor* worldCellAt(const WorldMapState& world,
                                                     Point point);
[[nodiscard]] WorldRegionDescriptor* worldRegionById(WorldMapState& world,
                                                     std::string_view id);
[[nodiscard]] const WorldRegionDescriptor*
worldRegionById(const WorldMapState& world, std::string_view id);
[[nodiscard]] WorldRegionDescriptor* currentWorldRegion(WorldMapState& world);
[[nodiscard]] const WorldRegionDescriptor*
currentWorldRegion(const WorldMapState& world);
void updateCurrentWorldRegion(WorldMapState& world);
[[nodiscard]] const RouteLink*
findDiscoveredRoute(const WorldRegionDescriptor& region,
                    std::string_view destinationRegionId);
[[nodiscard]] std::vector<RouteLink>
discoveredRoutes(const WorldRegionDescriptor& region);
bool unlockCurrentLandmarkFastTravel(WorldMapState& world);

[[nodiscard]] double valueNoise2d(Rng::Seed seed, double x, double y) noexcept;
[[nodiscard]] double
fbmNoise2d(Rng::Seed seed, double x, double y, int octaves = 4) noexcept;

[[nodiscard]] BiomeVariant deriveBiomeVariant(const WorldCellFields& fields);
[[nodiscard]] std::string_view biomeVariantId(BiomeVariant biome) noexcept;
[[nodiscard]] BiomeVariant biomeVariantFromId(std::string_view id) noexcept;
[[nodiscard]] std::string_view biomeDisplayName(BiomeVariant biome) noexcept;
[[nodiscard]] char32_t biomeGlyph(BiomeVariant biome) noexcept;
[[nodiscard]] Color biomeColor(BiomeVariant biome) noexcept;

[[nodiscard]] std::string_view localScreenRoleId(LocalScreenRole role) noexcept;
[[nodiscard]] LocalScreenRole
localScreenRoleFromId(std::string_view id) noexcept;

[[nodiscard]] WorldMapState generateWorldMap(Rng::Seed seed);
[[nodiscard]] LocalScreenDescriptor
describeLocalScreen(const WorldMapState& world,
                    Point macro,
                    Point screen,
                    bool dangerEncounter = false);

[[nodiscard]] Point greenholtWorldPosition() noexcept;
[[nodiscard]] Point reedfenWorldPosition() noexcept;
[[nodiscard]] Point limestoneWorldPosition() noexcept;

} // namespace rl
