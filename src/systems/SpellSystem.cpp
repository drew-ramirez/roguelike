/**
 * **************************************************************************
 * @file     SpellSystem.cpp
 * @brief    Resolves known spell casts through the generic effect system
 * **************************************************************************
 */

#include "systems/SpellSystem.hpp"

#include <algorithm>
#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "systems/EffectSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] std::optional<Point>
entityPosition(const entt::registry& registry, entt::entity entity)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return std::nullopt;
    }
    const auto& position = registry.get<Position>(entity);
    return Point{position.x, position.y};
}

[[nodiscard]] int distanceSquared(Point left, Point right) noexcept
{
    const int dx = left.x - right.x;
    const int dy = left.y - right.y;
    return (dx * dx) + (dy * dy);
}

[[nodiscard]] bool knowsSpell(const entt::registry& registry,
                              entt::entity caster,
                              std::string_view spellId)
{
    if (!registry.valid(caster) || !registry.all_of<KnownSpells>(caster)) {
        return false;
    }
    const auto& known = registry.get<KnownSpells>(caster).ids;
    return std::find(known.begin(), known.end(), spellId) != known.end();
}

} // namespace

std::vector<std::string_view> knownSpellIds(const entt::registry& registry,
                                            entt::entity caster)
{
    std::vector<std::string_view> ids;
    if (!registry.valid(caster) || !registry.all_of<KnownSpells>(caster)) {
        return ids;
    }
    const auto& known = registry.get<KnownSpells>(caster).ids;
    ids.reserve(known.size());
    for (const auto& id : known) {
        ids.push_back(id);
    }
    return ids;
}

bool castKnownSpell(entt::registry& registry,
                    GameResources& resources,
                    Map& map,
                    entt::entity caster,
                    std::string_view spellId,
                    std::optional<Point> target,
                    IFovProvider* fovProvider)
{
    if (!knowsSpell(registry, caster, spellId)) {
        resources.log.add("You do not know that spell.");
        return false;
    }

    const auto* spell = defaultRawDatabase().spell(spellId);
    if (spell == nullptr) {
        resources.log.add("The spell formula is missing.");
        return false;
    }
    if (!registry.all_of<Pools>(caster)) {
        return false;
    }
    auto& pools = registry.get<Pools>(caster);
    if (pools.mana < spell->manaCost) {
        resources.log.add("You do not have enough mana.");
        return false;
    }

    if (spell->range.has_value()) {
        if (!target.has_value()) {
            resources.log.add("That spell needs a target.");
            return false;
        }
        const auto casterPosition = entityPosition(registry, caster);
        if (!casterPosition.has_value() ||
            !map.inBounds(target->x, target->y) ||
            !map.isVisible(target->x, target->y) ||
            distanceSquared(*casterPosition, *target) >
                *spell->range * *spell->range) {
            resources.log.add("You cannot target there.");
            return false;
        }
    }

    const auto requests =
        effectRequestsForRawEffects(spell->effects, caster, entt::null, target);
    (void)fovProvider;
    const bool resolved =
        resolveEffectRequests(registry, resources, map, requests);
    if (!resolved) {
        return false;
    }

    pools.mana -= spell->manaCost;
    resources.log.add("You cast " + spell->name + ".");
    return true;
}

bool processSpellCasts(entt::registry& registry,
                       GameResources& resources,
                       Map& map,
                       IFovProvider* fovProvider)
{
    bool cast = false;
    const auto view = registry.view<const WantsToCastSpell>();
    for (const auto caster : view) {
        const auto& intent = view.get<const WantsToCastSpell>(caster);
        cast = castKnownSpell(registry,
                              resources,
                              map,
                              caster,
                              intent.spellId,
                              intent.target,
                              fovProvider) ||
               cast;
    }
    registry.clear<WantsToCastSpell>();
    return cast;
}

} // namespace rl
