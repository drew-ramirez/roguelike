#include "save/SaveGame.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "map/MasterDungeonMap.hpp"
#include "map/TileType.hpp"
#include "save/SaveDungeon.hpp"
#include "save/SaveEntities.hpp"
#include "save/SaveMap.hpp"
#include "save/SaveResources.hpp"
#include "save/SaveTypes.hpp"

namespace rl {
namespace {

using save::colorFromJson;
using save::colorToJson;
using save::currentSaveVersion;
using save::dungeonFromJson;
using save::dungeonToJson;
using save::ensureSaveIds;
using save::EntityId;
using save::entityId;
using save::entityToJson;
using save::hungerStateToString;
using save::Json;
using save::loadEntityImmediateComponents;
using save::loadEntityReferences;
using save::loadResources;
using save::mapFromJson;
using save::mapToJson;
using save::pointToJson;
using save::resourcesToJson;
using save::slotToString;
} // namespace

std::filesystem::path defaultSavePath()
{
    return std::filesystem::path{"savegame.json"};
}

std::string serializeGame(entt::registry& registry,
                          const GameResources& resources,
                          const Map& map)
{
    ensureSaveIds(registry);

    Json entities = Json::array();
    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        entities.push_back(entityToJson(registry, entity));
    }

    Json root{{"version", currentSaveVersion},
              {"map", mapToJson(map)},
              {"resources", resourcesToJson(registry, resources)},
              {"dungeon", dungeonToJson(resources.dungeon)},
              {"entities", std::move(entities)}};
    return root.dump(2);
}

bool deserializeGame(std::string_view text,
                     entt::registry& registry,
                     GameResources& resources,
                     Map& map)
{
    try {
        const Json root = Json::parse(text);
        const int version = root.value("version", 0);
        if (version < 1 || version > currentSaveVersion) {
            return false;
        }

        Map loadedMap = mapFromJson(root.at("map"));
        entt::registry loadedRegistry;
        std::unordered_map<EntityId, entt::entity> entityMap;
        std::vector<std::pair<entt::entity, Json>> pendingReferences;

        for (const auto& saved : root.at("entities")) {
            const auto id = saved.at("id").get<EntityId>();
            if (id == 0 || entityMap.contains(id)) {
                return false;
            }
            const auto entity = loadedRegistry.create();
            const auto& components = saved.at("components");
            loadEntityImmediateComponents(
                loadedRegistry, entity, id, components);
            pendingReferences.emplace_back(entity, components);
            entityMap.emplace(id, entity);
        }

        for (const auto& [entity, components] : pendingReferences) {
            loadEntityReferences(loadedRegistry, entity, components, entityMap);
        }

        GameResources loadedResources;
        loadResources(root.at("resources"), entityMap, loadedResources);
        loadedResources.dungeon.replaceLevels(
            dungeonFromJson(root.value("dungeon", Json::array())));

        map = std::move(loadedMap);
        registry = std::move(loadedRegistry);
        resources = std::move(loadedResources);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void saveGame(const std::filesystem::path& path,
              entt::registry& registry,
              const GameResources& resources,
              const Map& map)
{
    std::ofstream output{path};
    if (!output) {
        throw std::runtime_error("Unable to open save file for writing");
    }
    output << serializeGame(registry, resources, map);
}

bool loadGame(const std::filesystem::path& path,
              entt::registry& registry,
              GameResources& resources,
              Map& map)
{
    std::ifstream input{path};
    if (!input) {
        return false;
    }

    const std::string text{std::istreambuf_iterator<char>{input},
                           std::istreambuf_iterator<char>{}};
    return deserializeGame(text, registry, resources, map);
}

} // namespace rl
