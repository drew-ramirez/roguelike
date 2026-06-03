#include "ui/InventoryMenu.hpp"

#include <algorithm>
#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"

namespace rl {
namespace {

constexpr int panelX = 14;
constexpr int panelY = 8;
constexpr int panelWidth = 52;
constexpr int panelHeight = 24;

std::string itemLabel(const entt::registry& registry, entt::entity item)
{
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

void drawBox(ConsoleBuffer& console)
{
    for (int y = panelY; y < panelY + panelHeight; ++y) {
        for (int x = panelX; x < panelX + panelWidth; ++x) {
            const bool border = x == panelX || x == panelX + panelWidth - 1 ||
                                y == panelY || y == panelY + panelHeight - 1;
            console.set(x,
                        y,
                        border ? U'#' : U' ',
                        Color{180, 180, 180, 255},
                        Color::black());
        }
    }
}

} // namespace

InventoryInputResult handleInventoryInput(entt::registry& registry,
                                          GameResources& resources,
                                          Map& map,
                                          const std::vector<InputEvent>& events)
{
    auto items = inventoryItems(registry, resources.player);
    if (items.empty()) {
        resources.inventorySelection = 0;
    } else {
        resources.inventorySelection =
            std::clamp(resources.inventorySelection,
                       0,
                       static_cast<int>(items.size()) - 1);
    }

    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        if (event.key == Key::Escape || event.key == Key::I) {
            return InventoryInputResult::Close;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && !items.empty() && movement->y != 0) {
            resources.inventorySelection =
                std::clamp(resources.inventorySelection + movement->y,
                           0,
                           static_cast<int>(items.size()) - 1);
            return InventoryInputResult::None;
        }

        if (items.empty()) {
            continue;
        }

        const auto selected =
            items[static_cast<std::size_t>(resources.inventorySelection)];
        if (event.key == Key::D) {
            return dropItemAtOwner(
                       registry, resources, resources.player, selected)
                       ? InventoryInputResult::TookTurn
                       : InventoryInputResult::None;
        }
        if (event.key == Key::E) {
            return equipOrUnequipItem(
                       registry, resources.log, resources.player, selected)
                       ? InventoryInputResult::TookTurn
                       : InventoryInputResult::None;
        }
        if (event.key == Key::U || event.key == Key::Enter) {
            if (registry.all_of<ProvidesIdentify>(selected) ||
                registry.all_of<ProvidesRemoveCurse>(selected)) {
                const auto targets =
                    itemEffectTargets(registry, resources.player, selected);
                if (targets.empty()) {
                    resources.log.add(
                        registry.all_of<ProvidesIdentify>(selected)
                            ? "You have nothing unidentified."
                            : "No curses respond.");
                    return InventoryInputResult::None;
                }
                resources.pendingItemSelection = PendingItemSelection{
                    registry.all_of<ProvidesIdentify>(selected)
                        ? ItemSelectionMode::Identify
                        : ItemSelectionMode::RemoveCurse,
                    selected};
                resources.itemSelection = 0;
                return InventoryInputResult::StartedItemSelection;
            }
            if (!registry.all_of<Ranged>(selected) &&
                registry.all_of<TownPortal, Item>(selected)) {
                registry.emplace_or_replace<WantsToUseItem>(
                    resources.player, WantsToUseItem{selected});
                return InventoryInputResult::TookTurn;
            }
            const bool consumedTurn = useInventoryItem(
                registry, resources, map, resources.player, selected);
            if (resources.pendingTargeting.has_value()) {
                return InventoryInputResult::StartedTargeting;
            }
            return consumedTurn ? InventoryInputResult::TookTurn
                                : InventoryInputResult::None;
        }
    }

    return InventoryInputResult::None;
}

void drawInventoryMenu(ConsoleBuffer& console,
                       const entt::registry& registry,
                       const GameResources& resources)
{
    drawBox(console);
    console.print(panelX + 2, panelY + 1, "Inventory", Color::yellow());

    const auto items = inventoryItems(registry, resources.player);
    if (items.empty()) {
        console.print(panelX + 2,
                      panelY + 3,
                      "Your backpack is empty.",
                      Color{170, 170, 170, 255});
        return;
    }

    int row = panelY + 3;
    for (std::size_t index = 0;
         index < items.size() && row < panelY + panelHeight - 2;
         ++index) {
        const bool selected =
            static_cast<int>(index) == resources.inventorySelection;
        const Color color =
            selected ? Color::yellow() : Color{200, 200, 200, 255};
        console.print(panelX + 2,
                      row,
                      std::string{selected ? "> " : "  "} +
                          itemLabel(registry, items[index]),
                      color);
        ++row;
    }
}

} // namespace rl
