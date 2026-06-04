#include "save/SaveArchive.hpp"

#include <utility>

#include "save/SaveDungeon.hpp"
#include "save/SaveMap.hpp"
#include "save/SaveResources.hpp"

namespace rl::save {

Json archiveToJson(const entt::registry& registry,
                   const GameResources& resources,
                   const Map& map,
                   Json entities)
{
    return Json{{"version", currentSaveVersion},
                {"map", mapToJson(map)},
                {"resources", resourcesToJson(registry, resources)},
                {"dungeon", dungeonToJson(resources.dungeon)},
                {"entities", std::move(entities)}};
}

bool isSupportedArchiveVersion(const Json& root)
{
    const int version = root.value("version", 0);
    return version >= 1 && version <= currentSaveVersion;
}

} // namespace rl::save
