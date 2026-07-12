#pragma once

namespace rl {

enum class TileType {
    Wall,
    Floor,
    DownStairs,
    UpStairs,
    ClosedDoor,
    OpenDoor,
    Tree,
    DenseReeds,
    Rock
};

struct TileTraits {
    bool blocksMovement = false;
    bool blocksSight = false;
    bool naturalBlocker = false;
    bool connectivityPassable = true;
};

[[nodiscard]] constexpr TileTraits tileTraits(TileType tile) noexcept
{
    switch (tile) {
    case TileType::Wall:
        return TileTraits{true, true, false, false};
    case TileType::ClosedDoor:
        return TileTraits{true, true, false, true};
    case TileType::Tree:
    case TileType::DenseReeds:
    case TileType::Rock:
        return TileTraits{true, true, true, false};
    case TileType::Floor:
    case TileType::DownStairs:
    case TileType::UpStairs:
    case TileType::OpenDoor:
        return TileTraits{};
    }
    return TileTraits{true, true, false, false};
}

[[nodiscard]] constexpr bool isNaturalBlocker(TileType tile) noexcept
{
    return tileTraits(tile).naturalBlocker;
}

} // namespace rl
