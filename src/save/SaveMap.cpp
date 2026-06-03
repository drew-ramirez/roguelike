#include "save/SaveMap.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace rl::save {

[[nodiscard]] Json mapToJson(const Map& map)
{
    Json tiles = Json::array();
    Json revealed = Json::array();
    Json blood = Json::array();
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            tiles.push_back(tileToString(map.tileAt(x, y)));
            revealed.push_back(map.isRevealed(x, y));
            blood.push_back(map.hasBloodstain(x, y));
        }
    }

    return Json{{"width", map.width()},
                {"height", map.height()},
                {"tiles", std::move(tiles)},
                {"blood", std::move(blood)},
                {"revealed", std::move(revealed)}};
}

[[nodiscard]] Map mapFromJson(const Json& json)
{
    const int width = json.at("width").get<int>();
    const int height = json.at("height").get<int>();
    if (width <= 0 || height <= 0 ||
        width > std::numeric_limits<int>::max() / height) {
        throw std::runtime_error("Invalid saved map dimensions");
    }

    const auto& tiles = json.at("tiles");
    const auto& revealed = json.at("revealed");
    const auto blood = json.value("blood", Json::array());
    const auto expected = static_cast<std::size_t>(width * height);
    if (!tiles.is_array() || tiles.size() != expected || !revealed.is_array() ||
        revealed.size() != expected) {
        throw std::runtime_error("Invalid saved map tile data");
    }
    if (!blood.empty() && (!blood.is_array() || blood.size() != expected)) {
        throw std::runtime_error("Invalid saved map blood data");
    }

    Map map{width, height, TileType::Wall};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto index = static_cast<std::size_t>(y * width + x);
            map.setTile(
                x, y, tileFromString(tiles.at(index).get<std::string>()));
            map.setRevealed(x, y, revealed.at(index).get<bool>());
            if (!blood.empty()) {
                map.setBloodstain(x, y, blood.at(index).get<bool>());
            }
        }
    }
    return map;
}

} // namespace rl::save