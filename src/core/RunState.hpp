#pragma once

namespace rl {

enum class RunState {
    MainMenu,
    AwaitingInput,
    PlayerTurn,
    MonsterTurn,
    ShowInventory,
    ShowItemSelection,
    ShowDropItem,
    ShowTargeting,
    MapGeneration,
    GameOver
};

} // namespace rl
