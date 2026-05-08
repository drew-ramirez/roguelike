#include "ui/Hud.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "progression/ExperienceSystem.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int defaultUiHeight = 7;
constexpr int hpBarWidth = 16;
constexpr int xpBarWidth = 10;
constexpr std::size_t visibleMessages = 3;

[[nodiscard]] std::string entityName(const entt::registry& registry,
                                     entt::entity entity)
{
    if (registry.valid(entity) && registry.all_of<Name>(entity)) {
        return registry.get<Name>(entity).value;
    }
    return "None";
}

[[nodiscard]] std::optional<entt::entity> equippedInSlot(
    const entt::registry& registry, entt::entity owner, EquipmentSlot slot)
{
    const auto view = registry.view<const Equipped>();
    for (const auto item : view) {
        const auto& equipped = view.get<const Equipped>(item);
        if (equipped.owner == owner && equipped.slot == slot) {
            return item;
        }
    }
    return std::nullopt;
}

[[nodiscard]] int carriedWeight(const entt::registry& registry,
                                entt::entity owner)
{
    int total = 0;
    for (const auto item : inventoryItems(registry, owner)) {
        if (registry.all_of<ItemStats>(item)) {
            total += registry.get<ItemStats>(item).weight;
        }
    }
    return total;
}

[[nodiscard]] Pools displayPools(const entt::registry& registry,
                                 entt::entity player)
{
    if (registry.all_of<Pools>(player)) {
        return registry.get<Pools>(player);
    }
    if (registry.all_of<CombatStats>(player)) {
        const auto& stats = registry.get<CombatStats>(player);
        return Pools{stats.hp, stats.maxHp, 0, 0, 1, 0, 0};
    }
    return {};
}

[[nodiscard]] Attributes displayAttributes(const entt::registry& registry,
                                           entt::entity player)
{
    if (registry.all_of<Attributes>(player)) {
        return registry.get<Attributes>(player);
    }
    return {};
}

} // namespace

void drawHud(ConsoleBuffer& console,
             const GameResources& resources,
             const entt::registry& registry,
             RunState state)
{
    const int uiHeight = std::min(defaultUiHeight, console.height());
    const int uiTop = std::max(0, console.height() - uiHeight);

    drawPanel(console, uiTop, uiHeight);

    const bool hasPlayer =
        resources.player != entt::null && registry.valid(resources.player);
    if (hasPlayer) {
        const auto pools = displayPools(registry, resources.player);
        const auto attributes = displayAttributes(registry, resources.player);
        console.print(1, uiTop + 1, "HP", Color::white());
        drawBar(console,
                4,
                uiTop + 1,
                hpBarWidth,
                pools.hitPoints,
                pools.maxHitPoints,
                Color{220, 60, 60, 255},
                Color{70, 30, 30, 255});
        console.print(26,
                      uiTop + 1,
                      std::to_string(pools.hitPoints) + "/" +
                          std::to_string(pools.maxHitPoints),
                      Color::white());
        console.print(34,
                      uiTop + 1,
                      "MP " + std::to_string(pools.mana) + "/" +
                          std::to_string(pools.maxMana),
                      Color{120, 180, 255, 255});
        const int xpThreshold = xpThresholdForLevel(pools.level);
        console.print(44,
                      uiTop + 1,
                      "Lv " + std::to_string(pools.level),
                      Color{180, 180, 180, 255});
        console.print(50, uiTop + 1, "XP", Color{180, 180, 180, 255});
        drawBar(console,
                53,
                uiTop + 1,
                xpBarWidth,
                pools.xp,
                xpThreshold,
                Color{220, 200, 90, 255},
                Color{70, 60, 30, 255});
        console.print(64,
                      uiTop + 1,
                      std::to_string(pools.xp) + "/" +
                          std::to_string(xpThreshold),
                      Color{180, 180, 180, 255});

        console.print(1,
                      uiTop + 2,
                      "Mgt " + std::to_string(attributes.might) + " Fit " +
                          std::to_string(attributes.fitness) + " Qck " +
                          std::to_string(attributes.quickness) + " Int " +
                          std::to_string(attributes.intelligence),
                      Color{180, 180, 180, 255});
        console.print(
            36,
            uiTop + 2,
            "Atk " +
                std::to_string(
                    meleePowerWithEquipment(registry, resources.player)) +
                " Def " +
                std::to_string(
                    defenseWithEquipment(registry, resources.player)) +
                " Wt " +
                std::to_string(carriedWeight(registry, resources.player)),
            Color{180, 180, 180, 255});
        console.print(56,
                      uiTop + 2,
                      "Gold " + std::to_string(pools.gold),
                      Color{220, 200, 120, 255});

        const auto weapon =
            equippedInSlot(registry, resources.player, EquipmentSlot::Weapon);
        const auto armor =
            equippedInSlot(registry, resources.player, EquipmentSlot::Armor);
        console.print(
            1,
            uiTop + 3,
            "W: " + entityName(registry, weapon.value_or(entt::null)) +
                "  A: " + entityName(registry, armor.value_or(entt::null)),
            Color{200, 200, 200, 255});
    }

    console.print(1,
                  uiTop + 4,
                  "Depth " + std::to_string(resources.depth),
                  Color{180, 180, 180, 255});
    if (state == RunState::GameOver) {
        console.print(14, uiTop + 4, "GAME OVER", Color{255, 80, 80, 255});
    } else {
        console.print(14, uiTop + 4, "g/i/>/F5", Color{130, 130, 130, 255});
    }

    const auto messages = resources.log.recent(visibleMessages);
    int row = uiTop + 4;
    for (const auto message : messages) {
        console.print(28, row, message, Color{190, 190, 190, 255});
        ++row;
    }
}

} // namespace rl
