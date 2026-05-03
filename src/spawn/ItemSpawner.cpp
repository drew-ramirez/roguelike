#include "spawn/ItemSpawner.hpp"

#include "ecs/Components.hpp"

namespace rl {

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

} // namespace rl
