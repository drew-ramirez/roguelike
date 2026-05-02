cmake_minimum_required(VERSION 3.21)

project(roguelike VERSION 0.1.0 LANGUAGES CXX)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

add_executable(roguelike
    src/core/Game.cpp
    src/main.cpp
    src/map/Map.cpp
    src/render/ConsoleBuffer.cpp
    src/render/SdlRenderer.cpp
    src/spawn/Spawner.cpp
    src/systems/MapRenderSystem.cpp
    src/systems/RenderSystem.cpp
)

target_compile_features(roguelike PRIVATE cxx_std_20)
set_target_properties(roguelike PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(roguelike PRIVATE
    src
)

target_link_libraries(roguelike PRIVATE
    EnTT::EnTT
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
)

if(MSVC)
    target_compile_options(roguelike PRIVATE /W4 /permissive-)
else()
    target_compile_options(roguelike PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME roguelike_smoke COMMAND roguelike --smoke)