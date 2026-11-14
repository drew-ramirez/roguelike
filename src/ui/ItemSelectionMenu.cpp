#include "ui/ItemSelectionMenu.hpp"

#include <algorithm>
#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "ui/ItemDetails.hpp"

namespace rl {
namespace {

constexpr int panelX = 6;
constexpr int panelY = 8;
constexpr int panelWidth = 68;
constexpr int panelHeight = 22;
constexpr int listWidth = 28;
constexpr int detailX = panelX + listWidth + 4;
constexpr int detailWidth = panelWidth - listWidth - 6;

[[nodiscard]] std::string clippedLabel(std::string text, int width)
{
    if (width <= 0) {
        return {};
    }
    const auto maximum = static_cast<std::size_t>(width);
    if (text.size() <= maximum) {
        return text;
    }
    if (maximum <= 3U) {
        text.resize(maximum);
        return text;
    }
    text.resize(maximum - 3U);
    text += "...";
    return text;
}

[[nodiscard]] std::vector<entt::entity>
selectionTargets(const entt::registry& registry, const GameResources& resources)
{
    if (!resources.pendingItemSelection.has_value()) {
        return {};
    }
    return itemEffectTargets(
        registry, resources.player, resources.pendingItemSelection->source);
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

    for (int y = panelY + 3; y < panelY + panelHeight - 1; ++y) {
        console.set(
            detailX - 2, y, U'|', Color{100, 100, 100, 255}, Color::black());
    }
}

} // namespace

ItemSelectionInputResult
handleItemSelectionInput(entt::registry& registry,
                         GameResources& resources,
                         Map& map,
                         const std::vector<InputEvent>& events)
{
    if (!resources.pendingItemSelection.has_value()) {
        return ItemSelectionInputResult::Close;
    }

    auto targets = selectionTargets(registry, resources);
    if (targets.empty()) {
        resources.pendingItemSelection.reset();
        resources.itemSelection = 0;
        return ItemSelectionInputResult::Close;
    }
    resources.itemSelection = std::clamp(
        resources.itemSelection, 0, static_cast<int>(targets.size()) - 1);

    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape) {
            resources.pendingItemSelection.reset();
            resources.itemSelection = 0;
            resources.log.add("Selection cancelled.");
            return ItemSelectionInputResult::Close;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && movement->y != 0) {
            resources.itemSelection =
                std::clamp(resources.itemSelection + movement->y,
                           0,
                           static_cast<int>(targets.size()) - 1);
            return ItemSelectionInputResult::None;
        }

        if (event.key == Key::Enter) {
            const auto source = resources.pendingItemSelection->source;
            const auto target =
                targets[static_cast<std::size_t>(resources.itemSelection)];
            const bool used = useInventoryItemOnTargetItem(
                registry, resources, map, resources.player, source, target);
            if (used) {
                resources.pendingItemSelection.reset();
                resources.itemSelection = 0;
                return ItemSelectionInputResult::TookTurn;
            }
            return ItemSelectionInputResult::None;
        }
    }

    return ItemSelectionInputResult::None;
}

void drawItemSelectionMenu(ConsoleBuffer& console,
                           const entt::registry& registry,
                           const GameResources& resources)
{
    drawBox(console);
    const bool identify =
        resources.pendingItemSelection.has_value() &&
        resources.pendingItemSelection->mode == ItemSelectionMode::Identify;
    console.print(panelX + 2,
                  panelY + 1,
                  identify ? "Identify Item" : "Remove Curse",
                  Color::yellow());
    console.print(panelX + 2,
                  panelY + 2,
                  "Enter confirm  Esc cancel",
                  Color{140, 140, 140, 255});

    const auto targets = selectionTargets(registry, resources);
    if (targets.empty()) {
        console.print(panelX + 2,
                      panelY + 4,
                      "No valid targets.",
                      Color{170, 170, 170, 255});
        return;
    }

    int row = panelY + 4;
    for (std::size_t index = 0;
         index < targets.size() && row < panelY + panelHeight - 2;
         ++index) {
        const bool selected =
            static_cast<int>(index) == resources.itemSelection;
        const Color color =
            selected ? Color::yellow() : Color{200, 200, 200, 255};
        console.print(
            panelX + 2,
            row,
            clippedLabel(std::string{selected ? "> " : "  "} +
                             displayItemName(registry, targets[index]),
                         listWidth),
            color);
        ++row;
    }

    const int selectedIndex = std::clamp(
        resources.itemSelection, 0, static_cast<int>(targets.size()) - 1);
    drawItemDetailsPane(console,
                        registry,
                        targets[static_cast<std::size_t>(selectedIndex)],
                        detailX,
                        panelY + 4,
                        detailWidth,
                        panelHeight - 6);
}

} // namespace rl
