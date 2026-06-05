/**
 * **************************************************************************
 * @file     MasterDungeonMap.cpp
 * @brief    Captures and restores depth-local map/entity state
 * **************************************************************************
 */

#include "map/MasterDungeonMap.hpp"

#include <utility>

namespace rl {
namespace {

[[nodiscard]] StoredEntity captureEntity(const entt::registry& registry,
                                         entt::entity entity)
{
    StoredEntity stored;
    if (registry.all_of<RawIdentity>(entity)) {
        stored.rawIdentity = registry.get<RawIdentity>(entity);
    }
    if (registry.all_of<Position>(entity)) {
        stored.position = registry.get<Position>(entity);
    }
    if (registry.all_of<Renderable>(entity)) {
        stored.renderable = registry.get<Renderable>(entity);
    }
    if (registry.all_of<Name>(entity)) {
        stored.name = registry.get<Name>(entity);
    }
    stored.monster = registry.all_of<Monster>(entity);
    stored.item = registry.all_of<Item>(entity);
    stored.blocksTile = registry.all_of<BlocksTile>(entity);
    stored.hidden = registry.all_of<Hidden>(entity);
    stored.consumable = registry.all_of<Consumable>(entity);
    stored.magicMapper = registry.all_of<MagicMapper>(entity);
    stored.neutral = registry.all_of<Neutral>(entity);
    stored.bystander = registry.all_of<Bystander>(entity);
    stored.prop = registry.all_of<Prop>(entity);
    stored.boss = registry.all_of<Boss>(entity);
    stored.grantsWin = registry.all_of<GrantsWin>(entity);

    if (registry.all_of<Viewshed>(entity)) {
        const auto& viewshed = registry.get<Viewshed>(entity);
        stored.viewshed = Viewshed{{}, viewshed.range, true};
    }
    if (registry.all_of<CombatStats>(entity)) {
        stored.combatStats = registry.get<CombatStats>(entity);
    }
    if (registry.all_of<Attributes>(entity)) {
        stored.attributes = registry.get<Attributes>(entity);
    }
    if (registry.all_of<Pools>(entity)) {
        stored.pools = registry.get<Pools>(entity);
    }
    if (registry.all_of<ProvidesHealing>(entity)) {
        stored.providesHealing = registry.get<ProvidesHealing>(entity);
    }
    if (registry.all_of<ProvidesNutrition>(entity)) {
        stored.providesNutrition = registry.get<ProvidesNutrition>(entity);
    }
    if (registry.all_of<Ingredient>(entity)) {
        stored.ingredient = registry.get<Ingredient>(entity);
    }
    if (registry.all_of<Meal>(entity)) {
        stored.meal = registry.get<Meal>(entity);
    }
    stored.providesIdentify = registry.all_of<ProvidesIdentify>(entity);
    stored.providesRemoveCurse = registry.all_of<ProvidesRemoveCurse>(entity);
    if (registry.all_of<Ranged>(entity)) {
        stored.ranged = registry.get<Ranged>(entity);
    }
    if (registry.all_of<AreaOfEffect>(entity)) {
        stored.areaOfEffect = registry.get<AreaOfEffect>(entity);
    }
    if (registry.all_of<InflictsDamage>(entity)) {
        stored.inflictsDamage = registry.get<InflictsDamage>(entity);
    }
    if (registry.all_of<Equippable>(entity)) {
        stored.equippable = registry.get<Equippable>(entity);
    }
    if (registry.all_of<MeleePowerBonus>(entity)) {
        stored.meleePowerBonus = registry.get<MeleePowerBonus>(entity);
    }
    if (registry.all_of<DefenseBonus>(entity)) {
        stored.defenseBonus = registry.get<DefenseBonus>(entity);
    }
    if (registry.all_of<AttributeBonus>(entity)) {
        stored.attributeBonus = registry.get<AttributeBonus>(entity);
    }
    if (registry.all_of<Trap>(entity)) {
        stored.trap = registry.get<Trap>(entity);
    }
    if (registry.all_of<XpReward>(entity)) {
        stored.xpReward = registry.get<XpReward>(entity);
    }
    if (registry.all_of<ItemStats>(entity)) {
        stored.itemStats = registry.get<ItemStats>(entity);
    }
    if (registry.all_of<WeaponStats>(entity)) {
        stored.weaponStats = registry.get<WeaponStats>(entity);
    }
    if (registry.all_of<RangedWeapon>(entity)) {
        stored.rangedWeapon = registry.get<RangedWeapon>(entity);
    }
    if (registry.all_of<MagicItem>(entity)) {
        stored.magicItem = registry.get<MagicItem>(entity);
    }
    stored.identifiedItem = registry.all_of<IdentifiedItem>(entity);
    stored.cursedItem = registry.all_of<CursedItem>(entity);
    if (registry.all_of<Vendor>(entity)) {
        stored.vendor = registry.get<Vendor>(entity);
    }
    if (registry.all_of<CampSite>(entity)) {
        stored.campSite = registry.get<CampSite>(entity);
    }
    if (registry.all_of<CookingStation>(entity)) {
        stored.cookingStation = registry.get<CookingStation>(entity);
    }
    if (registry.all_of<Faction>(entity)) {
        stored.faction = registry.get<Faction>(entity);
    }
    stored.townPortal = registry.all_of<TownPortal>(entity);
    if (registry.all_of<Confusion>(entity)) {
        stored.confusion = registry.get<Confusion>(entity);
    }
    if (registry.all_of<KnownSpells>(entity)) {
        stored.knownSpells = registry.get<KnownSpells>(entity);
    }
    if (registry.all_of<Spellbook>(entity)) {
        stored.spellbook = registry.get<Spellbook>(entity);
    }
    if (registry.all_of<ManaCost>(entity)) {
        stored.manaCost = registry.get<ManaCost>(entity);
    }
    if (registry.all_of<BreathWeapon>(entity)) {
        stored.breathWeapon = registry.get<BreathWeapon>(entity);
    }
    return stored;
}

void restoreEntity(entt::registry& registry, const StoredEntity& stored)
{
    const auto entity = registry.create();
    if (stored.rawIdentity.has_value()) {
        registry.emplace<RawIdentity>(entity, *stored.rawIdentity);
    }
    if (stored.position.has_value()) {
        registry.emplace<Position>(entity, *stored.position);
    }
    if (stored.renderable.has_value()) {
        registry.emplace<Renderable>(entity, *stored.renderable);
    }
    if (stored.name.has_value()) {
        registry.emplace<Name>(entity, *stored.name);
    }
    if (stored.monster) {
        registry.emplace<Monster>(entity);
    }
    if (stored.item) {
        registry.emplace<Item>(entity);
    }
    if (stored.blocksTile) {
        registry.emplace<BlocksTile>(entity);
    }
    if (stored.hidden) {
        registry.emplace<Hidden>(entity);
    }
    if (stored.consumable) {
        registry.emplace<Consumable>(entity);
    }
    if (stored.magicMapper) {
        registry.emplace<MagicMapper>(entity);
    }
    if (stored.neutral) {
        registry.emplace<Neutral>(entity);
    }
    if (stored.bystander) {
        registry.emplace<Bystander>(entity);
    }
    if (stored.prop) {
        registry.emplace<Prop>(entity);
    }
    if (stored.boss) {
        registry.emplace<Boss>(entity);
    }
    if (stored.grantsWin) {
        registry.emplace<GrantsWin>(entity);
    }
    if (stored.viewshed.has_value()) {
        registry.emplace<Viewshed>(entity, *stored.viewshed);
    }
    if (stored.combatStats.has_value()) {
        registry.emplace<CombatStats>(entity, *stored.combatStats);
    }
    if (stored.attributes.has_value()) {
        registry.emplace<Attributes>(entity, *stored.attributes);
    }
    if (stored.pools.has_value()) {
        registry.emplace<Pools>(entity, *stored.pools);
    }
    if (stored.providesHealing.has_value()) {
        registry.emplace<ProvidesHealing>(entity, *stored.providesHealing);
    }
    if (stored.providesNutrition.has_value()) {
        registry.emplace<ProvidesNutrition>(entity, *stored.providesNutrition);
    }
    if (stored.ingredient.has_value()) {
        registry.emplace<Ingredient>(entity, *stored.ingredient);
    }
    if (stored.meal.has_value()) {
        registry.emplace<Meal>(entity, *stored.meal);
    }
    if (stored.providesIdentify) {
        registry.emplace<ProvidesIdentify>(entity);
    }
    if (stored.providesRemoveCurse) {
        registry.emplace<ProvidesRemoveCurse>(entity);
    }
    if (stored.ranged.has_value()) {
        registry.emplace<Ranged>(entity, *stored.ranged);
    }
    if (stored.areaOfEffect.has_value()) {
        registry.emplace<AreaOfEffect>(entity, *stored.areaOfEffect);
    }
    if (stored.inflictsDamage.has_value()) {
        registry.emplace<InflictsDamage>(entity, *stored.inflictsDamage);
    }
    if (stored.equippable.has_value()) {
        registry.emplace<Equippable>(entity, *stored.equippable);
    }
    if (stored.meleePowerBonus.has_value()) {
        registry.emplace<MeleePowerBonus>(entity, *stored.meleePowerBonus);
    }
    if (stored.defenseBonus.has_value()) {
        registry.emplace<DefenseBonus>(entity, *stored.defenseBonus);
    }
    if (stored.attributeBonus.has_value()) {
        registry.emplace<AttributeBonus>(entity, *stored.attributeBonus);
    }
    if (stored.trap.has_value()) {
        registry.emplace<Trap>(entity, *stored.trap);
    }
    if (stored.xpReward.has_value()) {
        registry.emplace<XpReward>(entity, *stored.xpReward);
    }
    if (stored.itemStats.has_value()) {
        registry.emplace<ItemStats>(entity, *stored.itemStats);
    }
    if (stored.weaponStats.has_value()) {
        registry.emplace<WeaponStats>(entity, *stored.weaponStats);
    }
    if (stored.rangedWeapon.has_value()) {
        registry.emplace<RangedWeapon>(entity, *stored.rangedWeapon);
    }
    if (stored.magicItem.has_value()) {
        registry.emplace<MagicItem>(entity, *stored.magicItem);
    }
    if (stored.identifiedItem) {
        registry.emplace<IdentifiedItem>(entity);
    }
    if (stored.cursedItem) {
        registry.emplace<CursedItem>(entity);
    }
    if (stored.vendor.has_value()) {
        registry.emplace<Vendor>(entity, *stored.vendor);
    }
    if (stored.campSite.has_value()) {
        registry.emplace<CampSite>(entity, *stored.campSite);
    }
    if (stored.cookingStation.has_value()) {
        registry.emplace<CookingStation>(entity, *stored.cookingStation);
    }
    if (stored.faction.has_value()) {
        registry.emplace<Faction>(entity, *stored.faction);
    }
    if (stored.townPortal) {
        registry.emplace<TownPortal>(entity);
    }
    if (stored.confusion.has_value()) {
        registry.emplace<Confusion>(entity, *stored.confusion);
    }
    if (stored.knownSpells.has_value()) {
        registry.emplace<KnownSpells>(entity, *stored.knownSpells);
    }
    if (stored.spellbook.has_value()) {
        registry.emplace<Spellbook>(entity, *stored.spellbook);
    }
    if (stored.manaCost.has_value()) {
        registry.emplace<ManaCost>(entity, *stored.manaCost);
    }
    if (stored.breathWeapon.has_value()) {
        registry.emplace<BreathWeapon>(entity, *stored.breathWeapon);
    }
}

} // namespace

bool isPlayerPersistentEntity(const entt::registry& registry,
                              entt::entity player,
                              entt::entity entity)
{
    if (entity == player) {
        return true;
    }
    if (registry.all_of<InBackpack>(entity) &&
        registry.get<InBackpack>(entity).owner == player) {
        return true;
    }
    return registry.all_of<Equipped>(entity) &&
           registry.get<Equipped>(entity).owner == player;
}

std::vector<StoredEntity>
captureLevelLocalEntities(const entt::registry& registry, entt::entity player)
{
    std::vector<StoredEntity> entities;
    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        if (!isPlayerPersistentEntity(registry, player, entity)) {
            entities.push_back(captureEntity(registry, entity));
        }
    }
    return entities;
}

void restoreStoredEntities(entt::registry& registry,
                           const std::vector<StoredEntity>& entities)
{
    for (const auto& entity : entities) {
        restoreEntity(registry, entity);
    }
}

void destroyLevelLocalEntities(entt::registry& registry, entt::entity player)
{
    std::vector<entt::entity> doomed;
    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        if (!isPlayerPersistentEntity(registry, player, entity)) {
            doomed.push_back(entity);
        }
    }

    for (const auto entity : doomed) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
}

void MasterDungeonMap::storeLevel(int depth,
                                  std::string builderId,
                                  const Map& map,
                                  const entt::registry& registry,
                                  entt::entity player)
{
    DungeonLevelState state;
    state.depth = depth;
    state.builderId = std::move(builderId);
    state.map = map;
    state.entities = captureLevelLocalEntities(registry, player);

    levels_[depth] = std::move(state);
}

bool MasterDungeonMap::hasLevel(int depth) const
{
    return levels_.contains(depth);
}

const DungeonLevelState* MasterDungeonMap::level(int depth) const
{
    const auto found = levels_.find(depth);
    return found == levels_.end() ? nullptr : &found->second;
}

void MasterDungeonMap::restoreLevel(int depth,
                                    Map& map,
                                    entt::registry& registry) const
{
    const auto* state = level(depth);
    if (state == nullptr) {
        return;
    }

    map = state->map;
    restoreStoredEntities(registry, state->entities);
}

void MasterDungeonMap::clear()
{
    levels_.clear();
}

const std::map<int, DungeonLevelState>& MasterDungeonMap::levels() const
{
    return levels_;
}

void MasterDungeonMap::replaceLevels(std::map<int, DungeonLevelState> levels)
{
    levels_ = std::move(levels);
}

} // namespace rl
