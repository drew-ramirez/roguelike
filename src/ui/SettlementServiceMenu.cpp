/**
 * **************************************************************************
 * @file     SettlementServiceMenu.cpp
 * @brief    Handles settlement service menu input and rendering
 * **************************************************************************
 */

#include "ui/SettlementServiceMenu.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "systems/services/SettlementServiceSystem.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int panelX = 14;
constexpr int panelY = 8;
constexpr int panelWidth = 54;
constexpr int panelHeight = 12;
constexpr std::array<std::string_view, 2> actions{"Learn recipe", "Leave"};

[[nodiscard]] std::string serviceName(const entt::registry& registry,
                                      entt::entity provider)
{
    return registry.valid(provider) && registry.all_of<Name>(provider)
               ? registry.get<Name>(provider).value
               : std::string{"Settlement Service"};
}

[[nodiscard]] std::string recipeLine(const entt::registry& registry,
                                     entt::entity provider)
{
    if (!registry.valid(provider) ||
        !registry.all_of<SettlementService>(provider)) {
        return "No lesson available.";
    }

    const auto& service = registry.get<SettlementService>(provider);
    const auto* recipe = defaultRawDatabase().recipe(service.recipeId);
    return recipe == nullptr ? "No lesson available."
                             : "Recipe: " + recipe->name;
}

} // namespace

SettlementServiceInputResult
handleSettlementServiceInput(entt::registry& registry,
                             GameResources& resources,
                             const std::vector<InputEvent>& events)
{
    if (!resources.pendingSettlementService.has_value() ||
        !settlementServiceAvailable(
            resources,
            registry,
            resources.pendingSettlementService->provider)) {
        resources.pendingSettlementService.reset();
        return SettlementServiceInputResult::Close;
    }

    resources.settlementServiceSelection =
        std::clamp(resources.settlementServiceSelection,
                   0,
                   static_cast<int>(actions.size()) - 1);
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape) {
            resources.pendingSettlementService.reset();
            return SettlementServiceInputResult::Close;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && movement->y != 0) {
            resources.settlementServiceSelection =
                std::clamp(resources.settlementServiceSelection + movement->y,
                           0,
                           static_cast<int>(actions.size()) - 1);
            return SettlementServiceInputResult::None;
        }

        if (event.key == Key::Enter) {
            if (resources.settlementServiceSelection == 0) {
                return useSettlementService(
                           registry,
                           resources,
                           resources.pendingSettlementService->provider)
                           ? SettlementServiceInputResult::TookTurn
                           : SettlementServiceInputResult::None;
            }
            resources.pendingSettlementService.reset();
            return SettlementServiceInputResult::Close;
        }
    }

    return SettlementServiceInputResult::None;
}

void drawSettlementServiceMenu(ConsoleBuffer& console,
                               const entt::registry& registry,
                               const GameResources& resources)
{
    drawMenuFrame(console, panelX, panelY, panelWidth, panelHeight);
    const auto provider = resources.pendingSettlementService.has_value()
                              ? resources.pendingSettlementService->provider
                              : entt::null;

    console.print(panelX + 2,
                  panelY + 1,
                  serviceName(registry, provider),
                  Color::yellow());
    console.print(
        panelX + 2, panelY + 2, recipeLine(registry, provider), Color::white());

    int row = panelY + 4;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        const bool selected =
            static_cast<int>(index) == resources.settlementServiceSelection;
        console.print(panelX + 2,
                      row,
                      std::string{selected ? "> " : "  "} +
                          std::string{actions[index]},
                      selected ? Color::yellow() : Color{200, 200, 200, 255});
        ++row;
    }
}

} // namespace rl
