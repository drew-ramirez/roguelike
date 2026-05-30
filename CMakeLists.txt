cmake_minimum_required(VERSION 3.21)

project(roguelike VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ROGUELIKE_ENABLE_CLANG_TIDY
       "Run clang-tidy during supported CMake builds." OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

set(ROGUELIKE_CORE_SOURCES
    src/assets/RexPaintLoader.cpp
    src/core/Achievements.cpp
    src/core/Dice.cpp
    src/core/GameLog.cpp
    src/core/Morgue.cpp
    src/core/Rng.cpp
    src/debug/MapgenPlayback.cpp
    src/debug/MapgenViewer.cpp
    src/fov/ShadowcastingFov.cpp
    src/map/Map.cpp
    src/map/MapBuilderCommon.cpp
    src/map/MapBrush.cpp
    src/map/MapBuilderRegistry.cpp
    src/map/MapConnectivity.cpp
    src/map/DungeonRegionRouter.cpp
    src/map/MapGeometry.cpp
    src/map/MasterDungeonMap.cpp
    src/map/MapValidation.cpp
    src/map/generators/BspDungeonBuilder.cpp
    src/map/generators/BspInteriorBuilder.cpp
    src/map/generators/BuilderChain.cpp
    src/map/generators/CellularAutomataBuilder.cpp
    src/map/generators/DlaBuilder.cpp
    src/map/generators/DrunkardsWalkBuilder.cpp
    src/map/generators/MazeBuilder.cpp
    src/map/generators/RandomWalk.cpp
    src/map/generators/Section4Builders.cpp
    src/map/generators/SimpleMapBuilder.cpp
    src/map/generators/VoronoiBuilder.cpp
    src/map/generators/WfcBuilder.cpp
    src/map/corridors/CorridorBuilder.cpp
    src/map/prefabs/Prefab.cpp
    src/map/rooms/RoomCarver.cpp
    src/progression/ExperienceSystem.cpp
    src/raws/RawDatabase.cpp
    src/render/Camera.cpp
    src/render/ConsoleBuffer.cpp
    src/render/MapTheme.cpp
    src/render/WallGlyph.cpp
    src/save/SaveGame.cpp
    src/spawn/ItemSpawner.cpp
    src/spawn/MonsterSpawner.cpp
    src/spawn/RawSpawner.cpp
    src/spawn/SpawnTable.cpp
    src/spawn/Spawner.cpp
    src/spawn/TrapSpawner.cpp
    src/systems/BystanderAI.cpp
    src/systems/DamageSystem.cpp
    src/systems/EquipmentSystem.cpp
    src/systems/HungerSystem.cpp
    src/systems/InputSystem.cpp
    src/systems/ItemSystem.cpp
    src/systems/MapRenderSystem.cpp
    src/systems/MapTransitionSystem.cpp
    src/systems/MeleeCombatSystem.cpp
    src/systems/MonsterAI.cpp
    src/systems/ParticleSystem.cpp
    src/systems/RenderSystem.cpp
    src/systems/SpatialIndexSystem.cpp
    src/systems/TrapSystem.cpp
    src/systems/VendorSystem.cpp
    src/systems/VisibilitySystem.cpp
    src/ui/Hud.cpp
    src/ui/InventoryMenu.cpp
    src/ui/MainMenu.cpp
    src/ui/Targeting.cpp
    src/ui/Widgets.cpp
)

set(ROGUELIKE_APP_SOURCES
    src/core/Game.cpp
    src/main.cpp
    src/render/SdlRenderer.cpp
)

set(ROGUELIKE_TEST_SOURCES
    tests/asset_tests.cpp
    tests/ai_tests.cpp
    tests/core_tests.cpp
    tests/descent_spawn_tests.cpp
    tests/dice_tests.cpp
    tests/equipment_tests.cpp
    tests/input_tests.cpp
    tests/inventory_tests.cpp
    tests/level_snapshot_tests.cpp
    tests/log_tests.cpp
    tests/mapgen_tests.cpp
    tests/polish_pressure_tests.cpp
    tests/progression_tests.cpp
    tests/raws_validation_tests.cpp
    tests/region_routing_tests.cpp
    tests/save_load_tests.cpp
    tests/spatial_tests.cpp
    tests/targeting_tests.cpp
    tests/ui_tests.cpp
    tests/vendor_tests.cpp
)

set(ROGUELIKE_TEST_HEADERS
    tests/TestSuites.hpp
    tests/TestSupport.hpp
)

set(ROGUELIKE_HEADERS
    src/assets/RexPaintLoader.hpp
    src/core/Achievements.hpp
    src/core/Dice.hpp
    src/core/Game.hpp
    src/core/GameLog.hpp
    src/core/GameResources.hpp
    src/core/Morgue.hpp
    src/core/Point.hpp
    src/core/Rng.hpp
    src/core/RunState.hpp
    src/debug/MapgenPlayback.hpp
    src/debug/MapgenViewer.hpp
    src/ecs/Components.hpp
    src/fov/IFovProvider.hpp
    src/fov/ShadowcastingFov.hpp
    src/input/InputEvent.hpp
    src/map/generators/BspDungeonBuilder.hpp
    src/map/generators/BspInteriorBuilder.hpp
    src/map/generators/BuilderChain.hpp
    src/map/generators/CellularAutomataBuilder.hpp
    src/map/generators/DlaBuilder.hpp
    src/map/generators/DrunkardsWalkBuilder.hpp
    src/map/generators/MazeBuilder.hpp
    src/map/generators/RandomWalk.hpp
    src/map/generators/Section4Builders.hpp
    src/map/generators/SimpleMapBuilder.hpp
    src/map/generators/VoronoiBuilder.hpp
    src/map/generators/WfcBuilder.hpp
    src/map/corridors/CorridorBuilder.hpp
    src/map/prefabs/Prefab.hpp
    src/map/rooms/RoomCarver.hpp
    src/map/MapBuilderCommon.hpp
    src/map/MapBrush.hpp
    src/map/MapConnectivity.hpp
    src/map/DungeonRegionRouter.hpp
    src/map/MapGeometry.hpp
    src/map/Map.hpp
    src/map/MapBuilder.hpp
    src/map/MapBuilderRegistry.hpp
    src/map/MapBuilderTypes.hpp
    src/map/MapValidation.hpp
    src/map/MasterDungeonMap.hpp
    src/map/Rect.hpp
    src/map/TileType.hpp
    src/progression/ExperienceSystem.hpp
    src/raws/RawDatabase.hpp
    src/raws/RawTypes.hpp
    src/render/Color.hpp
    src/render/Camera.hpp
    src/render/ConsoleBuffer.hpp
    src/render/IRenderer.hpp
    src/render/MapTheme.hpp
    src/render/SdlRenderer.hpp
    src/render/WallGlyph.hpp
    src/save/SaveGame.hpp
    src/spawn/ItemSpawner.hpp
    src/spawn/MonsterSpawner.hpp
    src/spawn/RawSpawner.hpp
    src/spawn/SpawnTable.hpp
    src/spawn/Spawner.hpp
    src/spawn/TrapSpawner.hpp
    src/systems/BystanderAI.hpp
    src/systems/DamageSystem.hpp
    src/systems/EquipmentSystem.hpp
    src/systems/HungerSystem.hpp
    src/systems/InputSystem.hpp
    src/systems/ItemSystem.hpp
    src/systems/MapRenderSystem.hpp
    src/systems/MapTransitionSystem.hpp
    src/systems/MeleeCombatSystem.hpp
    src/systems/MonsterAI.hpp
    src/systems/ParticleSystem.hpp
    src/systems/RenderSystem.hpp
    src/systems/SpatialIndexSystem.hpp
    src/systems/TrapSystem.hpp
    src/systems/VendorSystem.hpp
    src/systems/VisibilitySystem.hpp
    src/ui/Hud.hpp
    src/ui/InventoryMenu.hpp
    src/ui/MainMenu.hpp
    src/ui/MenuResult.hpp
    src/ui/Targeting.hpp
    src/ui/Widgets.hpp
)

set(ROGUELIKE_FORMAT_FILES
    ${ROGUELIKE_CORE_SOURCES}
    ${ROGUELIKE_APP_SOURCES}
    ${ROGUELIKE_HEADERS}
    ${ROGUELIKE_TEST_SOURCES}
    ${ROGUELIKE_TEST_HEADERS}
)

add_library(roguelike_core STATIC
    ${ROGUELIKE_CORE_SOURCES}
    ${ROGUELIKE_HEADERS}
)

target_compile_features(roguelike_core PUBLIC cxx_std_20)
set_target_properties(roguelike_core PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(roguelike_core PUBLIC src)

target_link_libraries(roguelike_core PUBLIC
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    ZLIB::ZLIB
)

add_executable(roguelike ${ROGUELIKE_APP_SOURCES})

target_compile_features(roguelike PRIVATE cxx_std_20)
set_target_properties(roguelike PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(roguelike PRIVATE
    roguelike_core
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
)

add_executable(roguelike_core_tests
    ${ROGUELIKE_TEST_SOURCES}
    ${ROGUELIKE_TEST_HEADERS}
)
target_compile_features(roguelike_core_tests PRIVATE cxx_std_20)
set_target_properties(roguelike_core_tests PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(roguelike_core_tests PRIVATE roguelike_core)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying runtime assets"
)
add_custom_target(copy_data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/data
    COMMENT "Copying runtime data"
)
add_dependencies(roguelike copy_assets)
add_dependencies(roguelike_core_tests copy_assets)
add_dependencies(roguelike copy_data)
add_dependencies(roguelike_core_tests copy_data)

foreach(target_name roguelike_core roguelike roguelike_core_tests)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

if(WIN32)
    list(APPEND ROGUELIKE_LLVM_HINTS
        "C:/Program Files/LLVM/bin"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/Llvm/x64/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/Llvm/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/Llvm/x64/bin"
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/Llvm/bin"
    )
endif()

find_program(CLANG_FORMAT_EXE NAMES clang-format clang-format.exe
             HINTS ${ROGUELIKE_LLVM_HINTS})
if(CLANG_FORMAT_EXE)
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror --style=file
                ${ROGUELIKE_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking clang-format"
    )

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${ROGUELIKE_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format"
    )
else()
    add_custom_target(format-check
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format was not found."
        COMMAND ${CMAKE_COMMAND} -E false
    )

    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format was not found."
        COMMAND ${CMAKE_COMMAND} -E false
    )
endif()

find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy.exe
             HINTS ${ROGUELIKE_LLVM_HINTS})
if(CLANG_TIDY_EXE AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    add_custom_target(lint
        COMMAND ${CLANG_TIDY_EXE} --quiet -p ${CMAKE_BINARY_DIR}
                ${ROGUELIKE_CORE_SOURCES} ${ROGUELIKE_APP_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS format-check
        COMMENT "Running clang-tidy"
    )

    if(ROGUELIKE_ENABLE_CLANG_TIDY)
        set_target_properties(roguelike_core roguelike PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
        )
    endif()
elseif(ROGUELIKE_ENABLE_CLANG_TIDY)
    message(FATAL_ERROR
            "ROGUELIKE_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found on PATH.")
elseif(CLANG_TIDY_EXE)
    add_custom_target(lint
        COMMAND ${CMAKE_COMMAND} -E echo
                "Use a Ninja or Makefile build directory for linting so compile_commands.json is available."
        COMMAND ${CMAKE_COMMAND} -E echo
                "Example: cmake -S . -B build-lint -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        COMMAND ${CMAKE_COMMAND} -E false
        DEPENDS format-check
    )
else()
    add_custom_target(lint
        COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy was not found on PATH."
        COMMAND ${CMAKE_COMMAND} -E false
        DEPENDS format-check
    )
endif()

enable_testing()
add_test(NAME roguelike_core_tests COMMAND roguelike_core_tests)
add_test(NAME roguelike_smoke COMMAND roguelike --smoke)
