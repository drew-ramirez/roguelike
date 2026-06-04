#pragma once

namespace rl {

enum class RunState {
    MainMenu,
    AwaitingInput,
    PlayerTurn,
    MonsterTurn,
    ShowInventory,
    ShowVendor,
    ShowSpellMenu,
    ShowItemSelection,
    ShowDropItem,
    ShowTargeting,
    MapGeneration,
    GameOver
};

} // namespace rl
