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
#include "ui/ItemDetails.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int panelX = 6;
constexpr int panelY = 7;
constexpr int panelWidth = 68;
constexpr int panelHeight = 26;
constexpr int listWidth = 28;
constexpr int detailX = panelX + listWidth + 4;
constexpr int detailWidth = panelWidth - listWidth - 6;

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
    drawMenuFrame(console, panelX, panelY, panelWidth, panelHeight);

    for (int y = panelY + 2; y < panelY + panelHeight - 1; ++y) {
        console.set(
            detailX - 2, y, U'|', Color{100, 100, 100, 255}, Color::black());
    }
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
                      clippedLabel(std::string{selected ? "> " : "  "} +
                                       displayItemName(registry, items[index]),
                                   listWidth),
                      color);
        ++row;
    }

    const int selectedIndex = std::clamp(
        resources.inventorySelection, 0, static_cast<int>(items.size()) - 1);
    drawItemDetailsPane(console,
                        registry,
                        items[static_cast<std::size_t>(selectedIndex)],
                        detailX,
                        panelY + 3,
                        detailWidth,
                        panelHeight - 5);
}

} // namespace rl
