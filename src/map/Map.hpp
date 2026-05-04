/**
 * **************************************************************************
 * @file     Map.hpp
 * @brief    Declares the tile map, blocking rules, and map memory state
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "map/TileType.hpp"

namespace rl {

class Map {
  public:
    Map(int width, int height, TileType fill = TileType::Floor);

    [[nodiscard]] static Map createStarter(int width = 80, int height = 45);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] int tileCount() const noexcept;
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;
    [[nodiscard]] int xyIdx(int x, int y) const;
    [[nodiscard]] TileType tileAt(int x, int y) const;
    [[nodiscard]] bool blocksMovement(int x, int y) const;
    [[nodiscard]] bool blocksSight(int x, int y) const;
    [[nodiscard]] bool isWalkable(int x, int y) const;
    [[nodiscard]] bool isVisible(int x, int y) const;
    [[nodiscard]] bool isRevealed(int x, int y) const;
    [[nodiscard]] bool hasBloodstain(int x, int y) const;

    void setTile(int x, int y, TileType tile);
    void setBloodstain(int x, int y, bool bloodstained = true);
    void clearVisible();
    void revealAll();
    void setVisible(int x, int y, bool visible = true);
    void setRevealed(int x, int y, bool revealed = true);

  private:
    int width_ = 0;
    int height_ = 0;
    std::vector<TileType> tiles_;
    std::vector<bool> bloodstains_;
    std::vector<bool> visibleTiles_;
    std::vector<bool> revealedTiles_;
};

} // namespace rl
