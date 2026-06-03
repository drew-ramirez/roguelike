#include "systems/InputSystem.hpp"

#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] std::optional<entt::entity>
equippedRangedWeapon(const entt::registry& registry, entt::entity owner)
{
    const auto view = registry.view<const Equipped, const RangedWeapon>();
    for (const auto item : view) {
        const auto& equipped = view.get<const Equipped>(item);
        if (equipped.owner == owner && equipped.slot == EquipmentSlot::Weapon) {
            return item;
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<Point> movementDeltaForKey(Key key)
{
    switch (key) {
    case Key::Up:
    case Key::Numpad8:
    case Key::W:
        return Point{0, -1};
    case Key::Down:
    case Key::Numpad2:
    case Key::S:
        return Point{0, 1};
    case Key::Left:
    case Key::Numpad4:
    case Key::A:
        return Point{-1, 0};
    case Key::Right:
    case Key::Numpad6:
    case Key::D:
        return Point{1, 0};
    case Key::Numpad7:
        return Point{-1, -1};
    case Key::Numpad9:
        return Point{1, -1};
    case Key::Numpad1:
        return Point{-1, 1};
    case Key::Numpad3:
        return Point{1, 1};
    case Key::B:
    case Key::C:
    case Key::E:
    case Key::F:
    case Key::G:
    case Key::H:
    case Key::I:
    case Key::J:
    case Key::K:
    case Key::L:
    case Key::M:
    case Key::N:
    case Key::O:
    case Key::P:
    case Key::Q:
    case Key::R:
    case Key::T:
    case Key::U:
    case Key::V:
    case Key::X:
    case Key::Y:
    case Key::Z:
    case Key::Escape:
    case Key::Enter:
    case Key::Space:
    case Key::F5:
    case Key::Descend:
    case Key::Ascend:
    case Key::Unknown:
        return std::nullopt;
    }

    return std::nullopt;
}

bool tryMoveEntity(entt::registry& registry,
                   entt::entity entity,
                   const Map& map,
                   Point delta)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return false;
    }

    auto& position = registry.get<Position>(entity);
    const Point target{position.x + delta.x, position.y + delta.y};
    if (isBlocked(map, registry, target, entity)) {
        return false;
    }

    position.x = target.x;
    position.y = target.y;
    if (registry.all_of<Viewshed>(entity)) {
        // Moving changes what the entity can see, so queue a refresh
        registry.patch<Viewshed>(
            entity, [](Viewshed& viewshed) { viewshed.dirty = true; });
    }
    return true;
}

PlayerInputResult handleMapInput(entt::registry& registry,
                                 GameResources& resources,
                                 Map& map,
                                 const std::vector<InputEvent>& events)
{
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        if (event.key == Key::I) {
            resources.inventorySelection = 0;
            return PlayerInputResult::OpenInventory;
        }
        if (event.key == Key::C) {
            resources.spellSelection = 0;
            return PlayerInputResult::OpenSpellMenu;
        }
        if (event.key == Key::Enter) {
            return PlayerInputResult::UsePortal;
        }
        if (event.key == Key::F) {
            const auto weapon =
                equippedRangedWeapon(registry, resources.player);
            if (!weapon.has_value()) {
                resources.log.add("You have no ranged weapon equipped.");
                return PlayerInputResult::None;
            }
            const auto ownerPosition =
                registry.all_of<Position>(resources.player)
                    ? std::optional<Point>{Point{
                          registry.get<Position>(resources.player).x,
                          registry.get<Position>(resources.player).y}}
                    : std::nullopt;
            if (!ownerPosition.has_value()) {
                return PlayerInputResult::None;
            }
            const auto& ranged = registry.get<RangedWeapon>(*weapon);
            resources.pendingTargeting = PendingTargeting{TargetingMode::Shoot,
                                                          *weapon,
                                                          {},
                                                          0,
                                                          ranged.range,
                                                          std::nullopt,
                                                          *ownerPosition};
            resources.log.add("Choose a target.");
            return PlayerInputResult::StartedTargeting;
        }
        if (event.key == Key::G) {
            return queuePickupItemAtPlayer(registry, resources)
                       ? PlayerInputResult::TookTurn
                       : PlayerInputResult::None;
        }
        if (event.key == Key::F5) {
            return PlayerInputResult::Save;
        }
        if (event.key == Key::Descend) {
            return PlayerInputResult::Descend;
        }
        if (event.key == Key::Ascend) {
            return PlayerInputResult::Ascend;
        }

        const auto delta = movementDeltaForKey(event.key);
        if (delta.has_value()) {
            if (resources.player == entt::null ||
                !registry.valid(resources.player) ||
                !registry.all_of<Position>(resources.player)) {
                return PlayerInputResult::None;
            }

            const auto& position = registry.get<Position>(resources.player);
            const Point target{position.x + delta->x, position.y + delta->y};
            if (map.inBounds(target.x, target.y) &&
                map.tileAt(target.x, target.y) == TileType::ClosedDoor) {
                map.setTile(target.x, target.y, TileType::OpenDoor);
                resources.log.add("You open the door.");
                if (registry.all_of<Viewshed>(resources.player)) {
                    registry.patch<Viewshed>(
                        resources.player,
                        [](Viewshed& viewshed) { viewshed.dirty = true; });
                }
                return PlayerInputResult::TookTurn;
            }

            const auto targetContent =
                tileContentAt(registry, target, resources.player);
            if (targetContent.blocker.has_value()) {
                const auto blocker = *targetContent.blocker;
                if (registry.all_of<Vendor>(blocker)) {
                    resources.pendingVendor =
                        PendingVendor{blocker, VendorMode::Buy};
                    resources.vendorSelection = 0;
                    return PlayerInputResult::OpenVendor;
                }
                if (registry.all_of<Neutral>(blocker)) {
                    const auto name = registry.all_of<Name>(blocker)
                                          ? registry.get<Name>(blocker).value
                                          : std::string{"Someone"};
                    resources.log.add(name + " has nothing urgent to say.");
                    return PlayerInputResult::TookTurn;
                }
                if (registry.all_of<Prop>(blocker)) {
                    resources.log.add("Something blocks the way.");
                    return PlayerInputResult::None;
                }
                registry.emplace_or_replace<WantsToMelee>(
                    resources.player, WantsToMelee{blocker});
                return PlayerInputResult::TookTurn;
            }

            return tryMoveEntity(registry, resources.player, map, *delta)
                       ? PlayerInputResult::TookTurn
                       : PlayerInputResult::None;
        }
    }

    return PlayerInputResult::None;
}

bool handlePlayerInput(entt::registry& registry,
                       const GameResources& resources,
                       const Map& map,
                       const std::vector<InputEvent>& events)
{
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        const auto delta = movementDeltaForKey(event.key);
        if (delta.has_value()) {
            if (resources.player == entt::null ||
                !registry.valid(resources.player) ||
                !registry.all_of<Position>(resources.player)) {
                return false;
            }

            const auto& position = registry.get<Position>(resources.player);
            const Point target{position.x + delta->x, position.y + delta->y};
            const auto targetContent =
                tileContentAt(registry, target, resources.player);
            if (targetContent.blocker.has_value()) {
                registry.emplace_or_replace<WantsToMelee>(
                    resources.player, WantsToMelee{*targetContent.blocker});
                return true;
            }

            return tryMoveEntity(registry, resources.player, map, *delta);
        }
    }

    return false;
}

} // namespace rl
