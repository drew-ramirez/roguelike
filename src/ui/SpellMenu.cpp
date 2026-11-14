#include "ui/SpellMenu.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "systems/combat/SpellSystem.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int panelX = 16;
constexpr int panelY = 8;
constexpr int panelWidth = 48;
constexpr int panelHeight = 20;

[[nodiscard]] std::vector<std::string_view>
spellIds(const entt::registry& registry, const GameResources& resources)
{
    return knownSpellIds(registry, resources.player);
}

} // namespace

SpellInputResult handleSpellInput(entt::registry& registry,
                                  GameResources& resources,
                                  const Map& map,
                                  const std::vector<InputEvent>& events)
{
    auto spells = spellIds(registry, resources);
    if (spells.empty()) {
        resources.spellSelection = 0;
    } else {
        resources.spellSelection = std::clamp(
            resources.spellSelection, 0, static_cast<int>(spells.size()) - 1);
    }

    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape || event.key == Key::C) {
            return SpellInputResult::Close;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && !spells.empty() && movement->y != 0) {
            resources.spellSelection =
                std::clamp(resources.spellSelection + movement->y,
                           0,
                           static_cast<int>(spells.size()) - 1);
            return SpellInputResult::None;
        }

        if (event.key == Key::Enter && !spells.empty()) {
            const auto spellId =
                spells[static_cast<std::size_t>(resources.spellSelection)];
            const auto* spell = defaultRawDatabase().spell(spellId);
            if (spell == nullptr) {
                resources.log.add("That spell is missing.");
                return SpellInputResult::None;
            }
            if (spell->range.has_value()) {
                const auto& position = registry.get<Position>(resources.player);
                resources.pendingTargeting =
                    PendingTargeting{TargetingMode::Spell,
                                     entt::null,
                                     std::string{spellId},
                                     spell->manaCost,
                                     *spell->range,
                                     std::nullopt,
                                     Point{position.x, position.y}};
                resources.log.add("Choose a target.");
                return SpellInputResult::StartedTargeting;
            }

            registry.emplace_or_replace<WantsToCastSpell>(
                resources.player,
                WantsToCastSpell{std::string{spellId}, std::nullopt});
            (void)map;
            return SpellInputResult::TookTurn;
        }
    }
    return SpellInputResult::None;
}

void drawSpellMenu(ConsoleBuffer& console,
                   const entt::registry& registry,
                   const GameResources& resources)
{
    drawMenuFrame(console, panelX, panelY, panelWidth, panelHeight);
    console.print(panelX + 2, panelY + 1, "Spells", Color::yellow());

    const auto spells = spellIds(registry, resources);
    if (spells.empty()) {
        console.print(panelX + 2,
                      panelY + 3,
                      "You know no spells.",
                      Color{170, 170, 170, 255});
        return;
    }

    int row = panelY + 3;
    for (std::size_t index = 0;
         index < spells.size() && row < panelY + panelHeight - 2;
         ++index) {
        const auto* spell = defaultRawDatabase().spell(spells[index]);
        if (spell == nullptr) {
            continue;
        }
        const bool selected =
            static_cast<int>(index) == resources.spellSelection;
        const Color color =
            selected ? Color::yellow() : Color{200, 200, 200, 255};
        console.print(panelX + 2,
                      row,
                      std::string{selected ? "> " : "  "} + spell->name + " (" +
                          std::to_string(spell->manaCost) + " MP)",
                      color);
        ++row;
    }
}

} // namespace rl
