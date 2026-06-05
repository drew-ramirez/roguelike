/**
 * **************************************************************************
 * @file     RawSpawner.cpp
 * @brief    Builds ECS entities from raw content templates
 * **************************************************************************
 */

#include "spawn/RawSpawner.hpp"

#include <stdexcept>

#include "ecs/Components.hpp"

namespace rl {
namespace {

void applyKind(entt::registry& registry,
               entt::entity entity,
               const RawEntityTemplate& raw)
{
    if (raw.kind == RawEntityKind::Mob) {
        registry.emplace<Monster>(entity);
    } else if (raw.kind == RawEntityKind::Item) {
        registry.emplace<Item>(entity);
    } else {
        registry.emplace<Prop>(entity);
    }
}

void applyCombatBridge(entt::registry& registry,
                       entt::entity entity,
                       const RawEntityTemplate& raw)
{
    if (!raw.combat.has_value()) {
        return;
    }

    const auto& combat = *raw.combat;
    registry.emplace<CombatStats>(
        entity,
        CombatStats{combat.maxHp, combat.hp, combat.defense, combat.power});

    const RawPools pools =
        raw.pools.value_or(RawPools{combat.hp, combat.maxHp, 0, 0, 1, 0, 0});
    registry.emplace<Pools>(entity,
                            Pools{pools.hitPoints,
                                  pools.maxHitPoints,
                                  pools.mana,
                                  pools.maxMana,
                                  pools.level,
                                  pools.xp,
                                  pools.gold});
}

void applyEffects(entt::registry& registry,
                  entt::entity entity,
                  const RawEntityTemplate& raw)
{
    const auto& effects = raw.effects;
    if (effects.providesHealing.has_value()) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<ProvidesHealing>(
            entity, ProvidesHealing{*effects.providesHealing});
    }
    if (effects.providesNutrition.has_value()) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<ProvidesNutrition>(
            entity, ProvidesNutrition{*effects.providesNutrition});
    }
    if (effects.magicMapper) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<MagicMapper>(entity);
    }
    if (effects.inflictsDamage.has_value()) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<InflictsDamage>(
            entity, InflictsDamage{*effects.inflictsDamage});
    }
    if (effects.areaOfEffect.has_value()) {
        registry.emplace<AreaOfEffect>(entity,
                                       AreaOfEffect{*effects.areaOfEffect});
    }
    if (effects.townPortal) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<TownPortal>(entity);
    }
    if (effects.identify) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<ProvidesIdentify>(entity);
    }
    if (effects.removeCurse) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<ProvidesRemoveCurse>(entity);
    }
    if (effects.confusionTurns.has_value()) {
        registry.emplace_or_replace<Consumable>(entity);
        registry.emplace<Confusion>(entity, Confusion{*effects.confusionTurns});
    }
}

void applyEquipment(entt::registry& registry,
                    entt::entity entity,
                    const RawEntityTemplate& raw)
{
    if (raw.equippable.has_value()) {
        registry.emplace<Equippable>(entity, Equippable{*raw.equippable});
    }
    if (raw.meleePowerBonus.has_value()) {
        registry.emplace<MeleePowerBonus>(
            entity, MeleePowerBonus{*raw.meleePowerBonus});
    }
    if (raw.defenseBonus.has_value()) {
        registry.emplace<DefenseBonus>(entity, DefenseBonus{*raw.defenseBonus});
    }
    if (raw.attributeBonus.has_value()) {
        const auto& bonus = *raw.attributeBonus;
        registry.emplace<AttributeBonus>(entity,
                                         AttributeBonus{bonus.might,
                                                        bonus.fitness,
                                                        bonus.quickness,
                                                        bonus.intelligence});
    }
    if (raw.itemStats.has_value()) {
        const auto& stats = *raw.itemStats;
        registry.emplace<ItemStats>(entity,
                                    ItemStats{stats.weight,
                                              stats.baseValue,
                                              stats.initiativePenalty,
                                              stats.category});
    }
    if (raw.weapon.has_value()) {
        const auto& weapon = *raw.weapon;
        registry.emplace<WeaponStats>(
            entity,
            WeaponStats{weapon.damage, weapon.hitBonus, weapon.attribute});
    }
    if (raw.rangedWeapon.has_value()) {
        const auto& ranged = *raw.rangedWeapon;
        registry.emplace<RangedWeapon>(entity,
                                       RangedWeapon{ranged.range,
                                                    ranged.damage,
                                                    ranged.hitBonus,
                                                    ranged.attribute});
        registry.emplace<Ranged>(entity, Ranged{ranged.range});
    }
    if (raw.magic.has_value()) {
        const auto& magic = *raw.magic;
        registry.emplace<MagicItem>(
            entity,
            MagicItem{magic.tier, magic.unidentifiedName, magic.identified});
        if (magic.identified) {
            registry.emplace<IdentifiedItem>(entity);
        }
    }
    if (raw.cursed) {
        registry.emplace<CursedItem>(entity);
    }
}

void applyRoles(entt::registry& registry,
                entt::entity entity,
                const RawEntityTemplate& raw,
                const RawSpawnOptions& options)
{
    if (raw.blocksTile) {
        registry.emplace<BlocksTile>(entity);
    }
    if (raw.neutral) {
        registry.emplace<Neutral>(entity);
    }
    if (raw.bystander) {
        registry.emplace<Bystander>(entity);
    }
    if (raw.vendor.has_value()) {
        const auto& vendor = *raw.vendor;
        registry.emplace<Vendor>(entity, Vendor{vendor.category, vendor.gold});
        registry.emplace_or_replace<Neutral>(entity);
    }
    if (raw.campSite.has_value()) {
        const auto& camp = *raw.campSite;
        registry.emplace<CampSite>(entity,
                                   CampSite{camp.campType, camp.safetyBonus});
    }
    if (raw.settlementService.has_value()) {
        const auto& service = *raw.settlementService;
        registry.emplace<SettlementService>(
            entity, SettlementService{service.serviceId, service.recipeId});
        registry.emplace_or_replace<Neutral>(entity);
    }
    if (raw.cookingStation.has_value()) {
        const auto& station = *raw.cookingStation;
        registry.emplace<CookingStation>(entity,
                                         CookingStation{station.stationId,
                                                        station.methods,
                                                        station.placeTags});
    }
    if (!raw.faction.empty()) {
        registry.emplace<Faction>(entity, Faction{raw.faction});
    }
    if (raw.xpReward.has_value()) {
        registry.emplace<XpReward>(entity, XpReward{*raw.xpReward});
    }
    if (raw.viewshedRange.has_value()) {
        registry.emplace<Viewshed>(entity,
                                   Viewshed{{}, *raw.viewshedRange, true});
    }
    if (raw.boss) {
        registry.emplace<Boss>(entity);
    }
    if (raw.grantsWin) {
        registry.emplace<GrantsWin>(entity);
    }
    if (raw.breathDamage.has_value() || raw.breathRange.has_value()) {
        registry.emplace<BreathWeapon>(
            entity,
            BreathWeapon{
                raw.breathRange.value_or(0), raw.breathDamage.value_or(0), 0});
    }
    const bool hidden = options.hidden.value_or(raw.hidden);
    if (hidden) {
        registry.emplace<Hidden>(entity);
    }
}

void applySpellData(entt::registry& registry,
                    entt::entity entity,
                    const RawEntityTemplate& raw)
{
    if (!raw.knownSpells.empty()) {
        registry.emplace<KnownSpells>(entity, KnownSpells{raw.knownSpells});
    }
    if (raw.spellbook.has_value()) {
        registry.emplace<Spellbook>(entity, Spellbook{*raw.spellbook});
    }
    if (raw.manaCost.has_value()) {
        registry.emplace<ManaCost>(entity, ManaCost{*raw.manaCost});
    }
}

void applyIngredientData(entt::registry& registry,
                         entt::entity entity,
                         const RawDatabase& raws,
                         const RawEntityTemplate& raw)
{
    const auto* ingredient = raws.ingredient(raw.id);
    if (ingredient == nullptr || !ingredient->item.has_value() ||
        *ingredient->item != raw.id) {
        return;
    }

    registry.emplace<Ingredient>(entity,
                                 Ingredient{ingredient->tags, {}, {}, 1, 0});
}

} // namespace

std::optional<entt::entity> spawnRawEntity(entt::registry& registry,
                                           const RawDatabase& raws,
                                           std::string_view name,
                                           Point position,
                                           RawSpawnOptions options)
{
    const auto* raw = raws.entity(name);
    if (raw == nullptr) {
        return std::nullopt;
    }

    const auto entity = registry.create();
    registry.emplace<RawIdentity>(entity, RawIdentity{raw->id});
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(entity,
                                 Renderable{raw->renderable.glyph,
                                            raw->renderable.foreground,
                                            raw->renderable.background,
                                            raw->renderable.order});
    registry.emplace<Name>(entity, Name{raw->name});
    applyKind(registry, entity, *raw);
    applyCombatBridge(registry, entity, *raw);
    if (raw->attributes.has_value()) {
        const auto& attributes = *raw->attributes;
        registry.emplace<Attributes>(entity,
                                     Attributes{attributes.might,
                                                attributes.fitness,
                                                attributes.quickness,
                                                attributes.intelligence});
    }
    if (raw->pools.has_value() && !registry.all_of<Pools>(entity)) {
        const auto& pools = *raw->pools;
        registry.emplace<Pools>(entity,
                                Pools{pools.hitPoints,
                                      pools.maxHitPoints,
                                      pools.mana,
                                      pools.maxMana,
                                      pools.level,
                                      pools.xp,
                                      pools.gold});
    }
    applyEffects(registry, entity, *raw);
    applyEquipment(registry, entity, *raw);
    applyRoles(registry, entity, *raw, options);
    applySpellData(registry, entity, *raw);
    applyIngredientData(registry, entity, raws, *raw);

    return entity;
}

entt::entity spawnNamedEntity(entt::registry& registry,
                              const RawDatabase& raws,
                              std::string_view name,
                              Point position,
                              RawSpawnOptions options)
{
    const auto entity = spawnRawEntity(registry, raws, name, position, options);
    if (!entity.has_value()) {
        throw std::runtime_error("Unknown raw entity: " + std::string{name});
    }
    return *entity;
}

} // namespace rl
