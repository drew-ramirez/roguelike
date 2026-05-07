#include "spawn/ItemSpawner.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "spawn/SpawnTable.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

Point randomPointInRoom(const Rect& room, Rng& rng)
{
    return Point{
        rng.rangeInclusive(room.x1, room.x2 - 1),
        rng.rangeInclusive(room.y1, room.y2 - 1),
    };
}

[[nodiscard]] bool hasItemAt(const entt::registry& registry, Point point)
{
    return itemAtPosition(registry, point).has_value();
}

[[nodiscard]] bool
canSpawnAt(const entt::registry& registry, Point point, Point playerStart)
{
    return point != playerStart &&
           !blockingEntityAt(registry, point).has_value() &&
           !hasItemAt(registry, point);
}

[[nodiscard]] bool canSpawnAt(const entt::registry& registry,
                              const Map& map,
                              Point point,
                              Point playerStart)
{
    return map.isWalkable(point.x, point.y) &&
           canSpawnAt(registry, point, playerStart);
}

[[nodiscard]] entt::entity
spawnItemById(entt::registry& registry, std::string_view id, Point position)
{
    if (id == "fireball_scroll") {
        return spawnFireballScroll(registry, position);
    }
    if (id == "magic_mapping_scroll") {
        return spawnMagicMappingScroll(registry, position);
    }
    if (id == "food_ration") {
        return spawnFoodRation(registry, position);
    }
    if (id == "dagger") {
        return spawnDagger(registry, position);
    }
    if (id == "sword") {
        return spawnSword(registry, position);
    }
    if (id == "chain_mail") {
        return spawnChainMail(registry, position);
    }
    return spawnHealingPotion(registry, position);
}

} // namespace

entt::entity
spawnHealingPotion(entt::registry& registry, Point position, int amount)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'!', Color{220, 80, 140, 255}, Color::black(), 2});
    registry.emplace<Name>(entity, Name{"Healing Potion"});
    registry.emplace<Item>(entity);
    registry.emplace<Consumable>(entity);
    registry.emplace<ProvidesHealing>(entity, ProvidesHealing{amount});
    return entity;
}

entt::entity spawnFoodRation(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'%', Color{190, 150, 70, 255}, Color::black(), 2});
    registry.emplace<Name>(entity, Name{"Food Ration"});
    registry.emplace<Item>(entity);
    registry.emplace<Consumable>(entity);
    registry.emplace<ProvidesNutrition>(entity, ProvidesNutrition{350});
    return entity;
}

entt::entity spawnFireballScroll(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'?', Color{255, 160, 60, 255}, Color::black(), 2});
    registry.emplace<Name>(entity, Name{"Fireball Scroll"});
    registry.emplace<Item>(entity);
    registry.emplace<Consumable>(entity);
    registry.emplace<Ranged>(entity, Ranged{6});
    registry.emplace<AreaOfEffect>(entity, AreaOfEffect{2});
    registry.emplace<InflictsDamage>(entity, InflictsDamage{12});
    return entity;
}

entt::entity spawnMagicMappingScroll(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'?', Color{90, 180, 255, 255}, Color::black(), 2});
    registry.emplace<Name>(entity, Name{"Magic Mapping Scroll"});
    registry.emplace<Item>(entity);
    registry.emplace<Consumable>(entity);
    registry.emplace<MagicMapper>(entity);
    return entity;
}

entt::entity spawnDagger(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'/', Color{180, 180, 190, 255}, Color::black(), 2});
    registry.emplace<Name>(entity, Name{"Dagger"});
    registry.emplace<Item>(entity);
    registry.emplace<Equippable>(entity, Equippable{EquipmentSlot::Weapon});
    registry.emplace<MeleePowerBonus>(entity, MeleePowerBonus{2});
    return entity;
}

entt::entity spawnSword(entt::registry& registry, Point position)
{
    const auto entity = spawnDagger(registry, position);
    registry.replace<Name>(entity, Name{"Sword"});
    registry.replace<MeleePowerBonus>(entity, MeleePowerBonus{4});
    return entity;
}

entt::entity spawnChainMail(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'[', Color{130, 170, 210, 255}, Color::black(), 2});
    registry.emplace<Name>(entity, Name{"Chain Mail"});
    registry.emplace<Item>(entity);
    registry.emplace<Equippable>(entity, Equippable{EquipmentSlot::Armor});
    registry.emplace<DefenseBonus>(entity, DefenseBonus{2});
    return entity;
}

std::size_t spawnItems(entt::registry& registry,
                       const std::vector<Rect>& rooms,
                       Rng& rng,
                       Point playerStart,
                       int depth)
{
    if (rooms.size() <= 1) {
        return 0;
    }

    std::size_t spawned = 0;
    const auto maximum = static_cast<std::size_t>(maxItemsForDepth(depth));
    for (std::size_t roomIndex = 1;
         roomIndex < rooms.size() && spawned < maximum;
         ++roomIndex) {
        const bool forceEarlyItems = spawned < 2;
        if (!forceEarlyItems && rng.rangeInclusive(0, 3) != 0) {
            continue;
        }

        constexpr int maxAttempts = 12;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            const Point position = randomPointInRoom(rooms[roomIndex], rng);
            if (!canSpawnAt(registry, position, playerStart)) {
                continue;
            }

            const auto id = forceEarlyItems && spawned == 0
                                ? std::string{"healing_potion"}
                                : rollSpawn(itemSpawnTable(), depth, rng);
            [[maybe_unused]] const auto item =
                spawnItemById(registry, id, position);
            ++spawned;
            break;
        }
    }

    return spawned;
}

std::size_t spawnItems(entt::registry& registry,
                       const std::vector<SpawnRegion>& regions,
                       const Map& map,
                       Rng& rng,
                       Point playerStart,
                       int depth)
{
    if (regions.size() <= 1) {
        return 0;
    }

    std::size_t spawned = 0;
    const auto maximum = static_cast<std::size_t>(maxItemsForDepth(depth));
    for (std::size_t regionIndex = 1;
         regionIndex < regions.size() && spawned < maximum;
         ++regionIndex) {
        const bool hallway =
            regions[regionIndex].kind == SpawnRegionKind::Corridor;
        const bool forceEarlyItems = spawned < 2 && !hallway;
        if (!forceEarlyItems && hallway && rng.rangeInclusive(0, 8) != 0) {
            continue;
        }
        if (!forceEarlyItems && !hallway && rng.rangeInclusive(0, 3) != 0) {
            continue;
        }

        constexpr int maxAttempts = 12;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            const auto& tiles = regions[regionIndex].tiles;
            if (tiles.empty()) {
                break;
            }
            const Point position = tiles[static_cast<std::size_t>(
                rng.rangeInclusive(0, static_cast<int>(tiles.size()) - 1))];
            if (!canSpawnAt(registry, map, position, playerStart)) {
                continue;
            }

            const auto id = forceEarlyItems && spawned == 0
                                ? std::string{"healing_potion"}
                                : rollSpawn(itemSpawnTable(), depth, rng);
            [[maybe_unused]] const auto item =
                spawnItemById(registry, id, position);
            ++spawned;
            break;
        }
    }

    return spawned;
}

} // namespace rl
