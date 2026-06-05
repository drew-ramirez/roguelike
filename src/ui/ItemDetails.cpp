/**
 * **************************************************************************
 * @file     ItemDetails.cpp
 * @brief    Formats item metadata for inventory-style UI detail panes
 * **************************************************************************
 */

#include "ui/ItemDetails.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"

namespace rl {
namespace {

[[nodiscard]] std::string signedAmount(int amount)
{
    return amount >= 0 ? "+" + std::to_string(amount) : std::to_string(amount);
}

[[nodiscard]] std::string slotName(EquipmentSlot slot)
{
    switch (slot) {
    case EquipmentSlot::Weapon:
        return "weapon";
    case EquipmentSlot::Armor:
        return "armor";
    }
    return "unknown";
}

[[nodiscard]] std::string join(const std::vector<std::string>& values,
                               std::string_view separator)
{
    std::string text;
    for (const auto& value : values) {
        if (!text.empty()) {
            text += separator;
        }
        text += value;
    }
    return text;
}

void appendEquipmentLines(std::vector<std::string>& lines,
                          const entt::registry& registry,
                          entt::entity item)
{
    if (registry.all_of<Equippable>(item)) {
        lines.push_back("Slot: " +
                        slotName(registry.get<Equippable>(item).slot));
    }
    if (registry.all_of<Equipped>(item)) {
        lines.push_back("State: equipped");
    }
    if (registry.all_of<WeaponStats>(item)) {
        const auto& weapon = registry.get<WeaponStats>(item);
        lines.push_back("Melee: " + weapon.damage + ", " +
                        signedAmount(weapon.hitBonus) + " hit, " +
                        weapon.attribute);
    }
    if (registry.all_of<RangedWeapon>(item)) {
        const auto& ranged = registry.get<RangedWeapon>(item);
        lines.push_back("Ranged: " + ranged.damage + ", range " +
                        std::to_string(ranged.range) + ", " +
                        signedAmount(ranged.hitBonus) + " hit, " +
                        ranged.attribute);
    } else if (registry.all_of<Ranged>(item)) {
        lines.push_back("Range: " +
                        std::to_string(registry.get<Ranged>(item).range));
    }
    if (registry.all_of<MeleePowerBonus>(item)) {
        lines.push_back(
            "Power bonus: " +
            signedAmount(registry.get<MeleePowerBonus>(item).amount));
    }
    if (registry.all_of<DefenseBonus>(item)) {
        lines.push_back("Defense bonus: " +
                        signedAmount(registry.get<DefenseBonus>(item).amount));
    }
    if (registry.all_of<AttributeBonus>(item)) {
        const auto& bonus = registry.get<AttributeBonus>(item);
        std::vector<std::string> parts;
        if (bonus.might != 0) {
            parts.push_back("Mgt " + signedAmount(bonus.might));
        }
        if (bonus.fitness != 0) {
            parts.push_back("Fit " + signedAmount(bonus.fitness));
        }
        if (bonus.quickness != 0) {
            parts.push_back("Qck " + signedAmount(bonus.quickness));
        }
        if (bonus.intelligence != 0) {
            parts.push_back("Int " + signedAmount(bonus.intelligence));
        }
        if (!parts.empty()) {
            lines.push_back("Attributes: " + join(parts, " "));
        }
    }
}

void appendEffectLines(std::vector<std::string>& lines,
                       const entt::registry& registry,
                       entt::entity item)
{
    if (registry.all_of<ProvidesHealing>(item)) {
        lines.push_back(
            "Heals " +
            std::to_string(registry.get<ProvidesHealing>(item).amount) + " HP");
    }
    if (registry.all_of<ProvidesNutrition>(item)) {
        lines.push_back(
            "Nutrition: " +
            std::to_string(registry.get<ProvidesNutrition>(item).amount));
    }
    if (registry.all_of<InflictsDamage>(item)) {
        std::string line =
            "Effect: " +
            std::to_string(registry.get<InflictsDamage>(item).amount) +
            " damage";
        if (registry.all_of<AreaOfEffect>(item)) {
            line += " in radius " +
                    std::to_string(registry.get<AreaOfEffect>(item).radius);
        }
        lines.push_back(std::move(line));
    }
    if (registry.all_of<MagicMapper>(item)) {
        lines.push_back("Effect: reveals the map");
    }
    if (registry.all_of<TownPortal>(item)) {
        lines.push_back("Effect: opens a town portal");
    }
    if (registry.all_of<ProvidesIdentify>(item)) {
        lines.push_back("Effect: identify an item");
    }
    if (registry.all_of<ProvidesRemoveCurse>(item)) {
        lines.push_back("Effect: remove a curse");
    }
    if (registry.all_of<Confusion>(item)) {
        lines.push_back("Effect: confusion for " +
                        std::to_string(registry.get<Confusion>(item).turns) +
                        " turns");
    }
    if (registry.all_of<Spellbook>(item)) {
        lines.push_back("Spellbook: teaches " +
                        registry.get<Spellbook>(item).spellId);
    }
}

void appendFoodLines(std::vector<std::string>& lines,
                     const entt::registry& registry,
                     entt::entity item)
{
    if (registry.all_of<Ingredient>(item)) {
        const auto& ingredient = registry.get<Ingredient>(item);
        if (!ingredient.tags.empty()) {
            lines.push_back("Ingredient tags: " + join(ingredient.tags, ", "));
        }
        if (!ingredient.originBiome.empty()) {
            lines.push_back("Origin biome: " + ingredient.originBiome);
        }
        if (!ingredient.originRegionId.empty()) {
            lines.push_back("Origin region: " + ingredient.originRegionId);
        }
        lines.push_back("Potency: " + std::to_string(ingredient.potency));
        if (ingredient.risk != 0) {
            lines.push_back("Risk: " + std::to_string(ingredient.risk));
        }
    }
    if (registry.all_of<Meal>(item)) {
        lines.push_back("Meal recipe: " + registry.get<Meal>(item).recipeId);
    }
}

void appendMagicLines(std::vector<std::string>& lines,
                      const entt::registry& registry,
                      entt::entity item)
{
    const bool cursed = registry.all_of<CursedItem>(item);
    if (!registry.all_of<MagicItem>(item)) {
        if (cursed) {
            lines.push_back("Curse: cursed");
        }
        return;
    }

    const auto& magic = registry.get<MagicItem>(item);
    lines.push_back("Magic: " + magic.tier +
                    (magic.identified ? " (identified)" : " (unidentified)"));
    if (!magic.unidentifiedName.empty()) {
        lines.push_back("Unidentified name: " + magic.unidentifiedName);
    }
    if (!magic.identified) {
        lines.push_back("Curse: unknown");
    } else {
        lines.push_back(cursed ? "Curse: cursed" : "Curse: none known");
    }
}

void appendActionLine(std::vector<std::string>& lines,
                      const entt::registry& registry,
                      entt::entity item)
{
    const bool carried =
        registry.all_of<InBackpack>(item) || registry.all_of<Equipped>(item);
    if (!carried) {
        return;
    }

    std::vector<std::string> actions{"D drop"};
    if (registry.all_of<Consumable, Item>(item) ||
        registry.all_of<Spellbook>(item) || registry.all_of<TownPortal>(item) ||
        registry.all_of<Ranged>(item)) {
        actions.push_back("U/Enter use");
    }
    if (registry.all_of<Equippable>(item)) {
        actions.push_back(registry.all_of<Equipped>(item) ? "E remove"
                                                          : "E equip");
    }
    lines.push_back("Actions: " + join(actions, ", "));
}

} // namespace

std::string displayItemName(const entt::registry& registry, entt::entity item)
{
    if (item == entt::null || !registry.valid(item)) {
        return "No item";
    }

    std::string label = registry.all_of<Name>(item)
                            ? registry.get<Name>(item).value
                            : "Unnamed item";
    if (registry.all_of<MagicItem>(item)) {
        const auto& magic = registry.get<MagicItem>(item);
        if (!magic.identified && !magic.unidentifiedName.empty()) {
            label = magic.unidentifiedName;
        }
    }
    if (registry.all_of<Equipped>(item)) {
        label += " (equipped)";
    }
    return label;
}

std::vector<std::string> itemDetailLines(const entt::registry& registry,
                                         entt::entity item)
{
    if (item == entt::null || !registry.valid(item)) {
        return {"No item selected."};
    }

    std::vector<std::string> lines{displayItemName(registry, item)};
    if (registry.all_of<ItemStats>(item)) {
        const auto& stats = registry.get<ItemStats>(item);
        if (!stats.category.empty()) {
            lines.push_back("Category: " + stats.category);
        }
        lines.push_back("Weight: " + std::to_string(stats.weight));
        lines.push_back("Value: " + std::to_string(stats.baseValue) + "g");
        if (stats.initiativePenalty != 0) {
            lines.push_back("Initiative penalty: " +
                            std::to_string(stats.initiativePenalty));
        }
    }

    appendEquipmentLines(lines, registry, item);
    appendEffectLines(lines, registry, item);
    appendFoodLines(lines, registry, item);
    appendMagicLines(lines, registry, item);
    appendActionLine(lines, registry, item);
    return lines;
}

void drawWrappedText(ConsoleBuffer& console,
                     int x,
                     int y,
                     int width,
                     int height,
                     const std::vector<std::string>& lines,
                     Color color)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    int row = y;
    for (const auto& line : lines) {
        if (row >= y + height) {
            return;
        }
        if (line.empty()) {
            ++row;
            continue;
        }

        std::string_view remaining{line};
        while (!remaining.empty() && row < y + height) {
            const auto limit = std::min<std::size_t>(
                remaining.size(), static_cast<std::size_t>(width));
            std::size_t chunk = limit;
            if (limit < remaining.size()) {
                const auto space = remaining.substr(0, limit).find_last_of(' ');
                if (space != std::string_view::npos && space > 0U) {
                    chunk = space;
                }
            }

            console.print(x, row, remaining.substr(0, chunk), color);
            remaining.remove_prefix(chunk);
            while (!remaining.empty() && remaining.front() == ' ') {
                remaining.remove_prefix(1);
            }
            ++row;
        }
    }
}

void drawItemDetailsPane(ConsoleBuffer& console,
                         const entt::registry& registry,
                         entt::entity item,
                         int x,
                         int y,
                         int width,
                         int height)
{
    const auto lines = itemDetailLines(registry, item);
    if (lines.empty() || width <= 0 || height <= 0) {
        return;
    }

    drawWrappedText(console, x, y, width, 1, {lines.front()}, Color::yellow());
    if (lines.size() == 1U || height <= 2) {
        return;
    }

    const std::vector<std::string> detailLines(lines.begin() + 1, lines.end());
    drawWrappedText(console,
                    x,
                    y + 2,
                    width,
                    height - 2,
                    detailLines,
                    Color{200, 200, 200, 255});
}

} // namespace rl
