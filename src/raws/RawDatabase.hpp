/**
 * **************************************************************************
 * @file     RawDatabase.hpp
 * @brief    Declares loading and validation for JSON raw content
 * **************************************************************************
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "raws/RawTypes.hpp"

namespace rl {

class RawDatabase {
  public:
    [[nodiscard]] static RawDatabase fromJson(std::string_view text);
    [[nodiscard]] static RawDatabase
    loadFromFile(const std::filesystem::path& path);
    [[nodiscard]] static RawDatabase loadDefault();

    [[nodiscard]] const RawEntityTemplate*
    entity(std::string_view name) const noexcept;
    [[nodiscard]] const RawSpell* spell(std::string_view id) const noexcept;
    [[nodiscard]] const RawFaction* faction(std::string_view id) const noexcept;
    [[nodiscard]] const RawIngredient*
    ingredient(std::string_view id) const noexcept;
    [[nodiscard]] const RawRecipe* recipe(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<const RawRecipe*> recipes() const;
    [[nodiscard]] const RawBiomeResourceTable*
    biomeResourceTable(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<std::string>
    itemIdsForCategory(std::string_view category) const;

    [[nodiscard]] const std::vector<SpawnEntry>& monsterSpawnTable() const;
    [[nodiscard]] const std::vector<SpawnEntry>& itemSpawnTable() const;
    [[nodiscard]] const std::vector<SpawnEntry>& propSpawnTable() const;
    [[nodiscard]] std::vector<std::string> validate() const;

  private:
    std::unordered_map<std::string, RawEntityTemplate> entities_;
    std::unordered_map<std::string, std::string> entityAliases_;
    std::unordered_map<std::string, RawSpell> spells_;
    std::unordered_map<std::string, RawFaction> factions_;
    std::unordered_map<std::string, RawIngredient> ingredients_;
    std::unordered_map<std::string, RawRecipe> recipes_;
    std::unordered_map<std::string, RawBiomeResourceTable>
        biomeResourceTables_;
    std::vector<SpawnEntry> monsterSpawnTable_;
    std::vector<SpawnEntry> itemSpawnTable_;
    std::vector<SpawnEntry> propSpawnTable_;
};

[[nodiscard]] const RawDatabase& defaultRawDatabase();

} // namespace rl
