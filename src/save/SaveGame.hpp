/**
 * **************************************************************************
 * @file     SaveGame.hpp
 * @brief    Declares durable JSON save/load for map, resources, and ECS data
 * **************************************************************************
 */

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class Map;

[[nodiscard]] std::filesystem::path defaultSavePath();

[[nodiscard]] std::string serializeGame(entt::registry& registry,
                                        const GameResources& resources,
                                        const Map& map);

[[nodiscard]] bool deserializeGame(std::string_view text,
                                   entt::registry& registry,
                                   GameResources& resources,
                                   Map& map);

void saveGame(const std::filesystem::path& path,
              entt::registry& registry,
              const GameResources& resources,
              const Map& map);

[[nodiscard]] bool loadGame(const std::filesystem::path& path,
                            entt::registry& registry,
                            GameResources& resources,
                            Map& map);

} // namespace rl
