#include "save/SaveGame.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "save/SaveArchive.hpp"
#include "save/SaveDungeon.hpp"
#include "save/SaveEntities.hpp"
#include "save/SaveMap.hpp"
#include "save/SaveResources.hpp"
#include "save/SaveTypes.hpp"

namespace rl {

std::filesystem::path defaultSavePath()
{
    return std::filesystem::path{"savegame.json"};
}

std::string serializeGame(entt::registry& registry,
                          const GameResources& resources,
                          const Map& map)
{
    save::ensureSaveIds(registry);

    save::Json entities = save::Json::array();
    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        entities.push_back(save::entityToJson(registry, entity));
    }

    return save::archiveToJson(registry, resources, map, std::move(entities))
        .dump(2);
}

bool deserializeGame(std::string_view text,
                     entt::registry& registry,
                     GameResources& resources,
                     Map& map)
{
    try {
        const save::Json root = save::Json::parse(text);
        if (!save::isSupportedArchiveVersion(root)) {
            return false;
        }

        Map loadedMap = save::mapFromJson(root.at("map"));
        entt::registry loadedRegistry;
        std::unordered_map<save::EntityId, entt::entity> entityMap;
        std::vector<std::pair<entt::entity, save::Json>> pendingReferences;

        for (const auto& saved : root.at("entities")) {
            const auto id = saved.at("id").get<save::EntityId>();
            if (id == 0 || entityMap.contains(id)) {
                return false;
            }
            const auto entity = loadedRegistry.create();
            const auto& components = saved.at("components");
            save::loadEntityImmediateComponents(
                loadedRegistry, entity, id, components);
            pendingReferences.emplace_back(entity, components);
            entityMap.emplace(id, entity);
        }

        for (const auto& [entity, components] : pendingReferences) {
            save::loadEntityReferences(
                loadedRegistry, entity, components, entityMap);
        }

        GameResources loadedResources;
        save::loadResources(root.at("resources"), entityMap, loadedResources);
        loadedResources.dungeon.replaceLevels(
            save::dungeonFromJson(root.value("dungeon", save::Json::array())));

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
