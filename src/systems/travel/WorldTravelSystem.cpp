/**
 * **************************************************************************
 * @file     WorldTravelSystem.cpp
 * @brief    Handles surface macro travel and local screen transitions
 * **************************************************************************
 */

#include "systems/travel/WorldTravelSystem.hpp"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "map/MapConnectivity.hpp"
#include "systems/spatial/VisibilitySystem.hpp"
#include "systems/survival/HungerSystem.hpp"
#include "systems/travel/MapTransitionSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

constexpr int maximumDangerChance = 65;

[[nodiscard]] int biomeTravelModifier(BiomeVariant biome) noexcept
{
    switch (biome) {
    case BiomeVariant::ReedMarsh:
    case BiomeVariant::Sinklands:
        return 6;
    case BiomeVariant::LimestoneRise:
        return 4;
    case BiomeVariant::DangerousWilds:
        return 10;
    case BiomeVariant::DryWoods:
    case BiomeVariant::WetWoods:
    case BiomeVariant::DeepGrove:
    case BiomeVariant::RuinedWoodland:
        return 0;
    }
    return 0;
}

[[nodiscard]] int dangerChanceForCell(const WorldCellDescriptor& cell) noexcept
{
    const int chance = (cell.dangerTier * 10) +
                       static_cast<int>(cell.fields.dangerPressure * 25.0);
    return std::clamp(chance, 0, maximumDangerChance);
}

[[nodiscard]] Point targetMacro(Point cursor, Point direction) noexcept
{
    return Point{cursor.x + direction.x, cursor.y + direction.y};
}

[[nodiscard]] std::optional<std::pair<Point, Point>>
shiftedSurfaceScreen(Point macro, Point screen, Point direction) noexcept
{
    Point targetMacro = macro;
    Point targetScreen{screen.x + direction.x, screen.y + direction.y};

    if (targetScreen.x < 0) {
        --targetMacro.x;
        targetScreen.x = localScreenGridSize - 1;
    } else if (targetScreen.x >= localScreenGridSize) {
        ++targetMacro.x;
        targetScreen.x = 0;
    }

    if (targetScreen.y < 0) {
        --targetMacro.y;
        targetScreen.y = localScreenGridSize - 1;
    } else if (targetScreen.y >= localScreenGridSize) {
        ++targetMacro.y;
        targetScreen.y = 0;
    }

    if (!worldPointInBounds(targetMacro) ||
        !localScreenInBounds(targetScreen)) {
        return std::nullopt;
    }
    return std::pair{targetMacro, targetScreen};
}

[[nodiscard]] Point firstWalkablePoint(const Map& map)
{
    for (int y = 1; y < map.height() - 1; ++y) {
        for (int x = 1; x < map.width() - 1; ++x) {
            if (map.isWalkable(x, y)) {
                return Point{x, y};
            }
        }
    }
    return Point{map.width() / 2, map.height() / 2};
}

[[nodiscard]] Point firstWalkableNear(const Map& map, Point target)
{
    if (map.inBounds(target.x, target.y) &&
        map.isWalkable(target.x, target.y)) {
        return target;
    }
    if (const auto closest = closestWalkableFloorTo(map, target)) {
        return *closest;
    }
    return firstWalkablePoint(map);
}

[[nodiscard]] Point edgeArrivalPosition(const Map& map, Point direction)
{
    Point target{map.width() / 2, map.height() / 2};
    if (direction.x > 0) {
        target = Point{1, map.height() / 2};
    } else if (direction.x < 0) {
        target = Point{map.width() - 2, map.height() / 2};
    } else if (direction.y > 0) {
        target = Point{map.width() / 2, 1};
    } else if (direction.y < 0) {
        target = Point{map.width() / 2, map.height() - 2};
    }
    return firstWalkableNear(map, target);
}

[[nodiscard]] bool
landmarkUnlockedForRegion(const WorldMapState& world,
                          const WorldRegionDescriptor& region)
{
    const auto* cell = worldCellAt(world, region.macro);
    return cell != nullptr && cell->landmark.has_value() &&
           cell->landmark->fastTravelUnlocked;
}

[[nodiscard]] RouteLink routeToRegion(const WorldMapState& world,
                                      const WorldRegionDescriptor& target)
{
    if (const auto* current = currentWorldRegion(world); current != nullptr) {
        if (const auto* route = findDiscoveredRoute(*current, target.id);
            route != nullptr) {
            return *route;
        }
    }

    const int distance = std::abs(world.currentMacro.x - target.macro.x) +
                         std::abs(world.currentMacro.y - target.macro.y);
    return RouteLink{
        target.id, target.name, "landmark", std::max(10, distance * 12), true};
}

[[nodiscard]] bool setMapBuilderToSurfaceScreen(GameResources& resources,
                                                Point macro,
                                                Point screen,
                                                bool dangerEncounter)
{
    if (!resources.world.active || !worldPointInBounds(macro) ||
        !localScreenInBounds(screen)) {
        return false;
    }

    const auto descriptor =
        describeLocalScreen(resources.world, macro, screen, dangerEncounter);
    resources.mapBuilderId = descriptor.builderId;
    return true;
}

} // namespace

std::vector<RouteLink> availableTravelRoutes(const GameResources& resources)
{
    std::vector<RouteLink> routes;
    if (!resources.world.active) {
        return routes;
    }

    for (const auto& [id, region] : resources.world.regions) {
        (void)id;
        if (!region.fastTravelLandmark || !region.visited ||
            region.id == resources.world.currentRegionId ||
            !landmarkUnlockedForRegion(resources.world, region)) {
            continue;
        }
        routes.push_back(routeToRegion(resources.world, region));
    }

    std::sort(routes.begin(),
              routes.end(),
              [](const RouteLink& left, const RouteLink& right) {
                  if (left.hungerCost == right.hungerCost) {
                      return left.name < right.name;
                  }
                  return left.hungerCost < right.hungerCost;
              });
    return routes;
}

int macroTravelHungerCost(const WorldCellDescriptor& cell)
{
    return std::max(
        1, 8 + (cell.dangerTier * 4) + biomeTravelModifier(cell.biome));
}

int localTravelHungerCost(const WorldCellDescriptor& cell)
{
    return std::max(1, macroTravelHungerCost(cell) / 2);
}

int travelHungerCost(const RouteLink& route) noexcept
{
    return std::max(0, route.hungerCost);
}

bool moveWorldMapCursor(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider,
                        Point direction)
{
    if (!resources.world.active || direction == Point{0, 0}) {
        return false;
    }

    const Point target = targetMacro(resources.world.cursor, direction);
    if (!worldPointInBounds(target)) {
        resources.log.add("Your map ends there.");
        return false;
    }

    auto* targetCell = worldCellAt(resources.world, target);
    if (targetCell == nullptr) {
        resources.log.add("That region is missing.");
        return false;
    }

    resources.world.cursor = target;
    consumeHunger(registry, resources, macroTravelHungerCost(*targetCell));
    targetCell->discovered = true;

    const int dangerChance = dangerChanceForCell(*targetCell);
    if (dangerChance > 0 &&
        resources.rng.rangeInclusive(1, 100) <= dangerChance) {
        resources.log.add("The wilds close in around you.");
        return enterWorldMapSelection(
            registry, resources, map, fovProvider, true);
    }

    resources.log.add("You travel toward " + targetCell->name + ".");
    return false;
}

bool enterWorldMapSelection(entt::registry& registry,
                            GameResources& resources,
                            Map& map,
                            IFovProvider& fovProvider,
                            bool dangerEncounter)
{
    if (!resources.world.active) {
        resources.log.add("The world map is not available.");
        return false;
    }

    const Point macro = resources.world.cursor;
    const auto* targetCell = worldCellAt(resources.world, macro);
    if (targetCell == nullptr || !targetCell->discovered) {
        resources.log.add("Travel there before entering the wilds.");
        return false;
    }

    const Point screen{1, 1};
    const bool entered = tryEnterSurfaceScreen(
        registry, resources, map, fovProvider, macro, screen, dangerEncounter);
    if (entered) {
        (void)setMapBuilderToSurfaceScreen(
            resources, macro, screen, dangerEncounter);
    }
    return entered;
}

bool tryTravelByEdge(entt::registry& registry,
                     GameResources& resources,
                     Map& map,
                     IFovProvider& fovProvider,
                     Point direction)
{
    if (!resources.world.active || !resources.world.onSurface ||
        direction == Point{0, 0}) {
        return false;
    }

    const auto shifted = shiftedSurfaceScreen(
        resources.world.currentMacro, resources.world.currentScreen, direction);
    if (!shifted.has_value()) {
        resources.log.add("You cannot find a way farther in that direction.");
        return false;
    }

    const auto [macro, screen] = *shifted;
    const auto* targetCell = worldCellAt(resources.world, macro);
    if (targetCell == nullptr) {
        resources.log.add("That region is missing.");
        return false;
    }

    consumeHunger(registry, resources, localTravelHungerCost(*targetCell));
    const bool entered = tryEnterSurfaceScreen(
        registry, resources, map, fovProvider, macro, screen, false);
    if (!entered) {
        return false;
    }

    moveEntityTo(
        registry, resources.player, edgeArrivalPosition(map, direction));
    (void)setMapBuilderToSurfaceScreen(resources, macro, screen, false);
    updateVisibility(registry, map, resources.player, fovProvider);
    return true;
}

bool travelToRegion(entt::registry& registry,
                    GameResources& resources,
                    Map& map,
                    IFovProvider& fovProvider,
                    const std::string& destinationRegionId)
{
    auto* destination = worldRegionById(resources.world, destinationRegionId);
    if (destination == nullptr || !destination->fastTravelLandmark ||
        !destination->visited ||
        !landmarkUnlockedForRegion(resources.world, *destination)) {
        resources.log.add("That landmark is not known.");
        return false;
    }

    const auto routes = availableTravelRoutes(resources);
    const auto route = std::find_if(
        routes.begin(), routes.end(), [&](const RouteLink& candidate) {
            return candidate.destinationRegionId == destinationRegionId;
        });
    if (route == routes.end()) {
        resources.log.add("You are already there.");
        return false;
    }

    consumeHunger(registry, resources, travelHungerCost(*route));
    resources.world.cursor = destination->macro;
    const bool entered = tryEnterSurfaceScreen(registry,
                                               resources,
                                               map,
                                               fovProvider,
                                               destination->macro,
                                               destination->landmarkScreen,
                                               false);
    if (!entered) {
        return false;
    }

    destination->visited = true;
    destination->discovered = true;
    (void)setMapBuilderToSurfaceScreen(
        resources, destination->macro, destination->landmarkScreen, false);
    resources.log.add("You travel to " + destination->name + ".");
    return true;
}

} // namespace rl
