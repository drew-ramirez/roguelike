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
    ShowWorldMap,
    MapGeneration,
    GameOver
};

} // namespace rl
