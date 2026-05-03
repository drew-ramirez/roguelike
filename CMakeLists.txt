cmake_minimum_required(VERSION 3.21)

project(roguelike VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ROGUELIKE_ENABLE_CLANG_TIDY "Run clang-tidy during supported CMake builds." OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

set(ROGUELIKE_SOURCES
    src/core/Game.cpp
    src/core/Rng.cpp
    src/main.cpp
    src/map/Map.cpp
    src/map/generators/SimpleMapBuilder.cpp
    src/render/ConsoleBuffer.cpp
    src/render/SdlRenderer.cpp
    src/spawn/Spawner.cpp
    src/systems/InputSystem.cpp
    src/systems/MapRenderSystem.cpp
    src/systems/RenderSystem.cpp
)

set(ROGUELIKE_HEADERS
    src/core/Game.hpp
    src/core/GameResources.hpp
    src/core/Point.hpp
    src/core/Rng.hpp
    src/core/RunState.hpp
    src/ecs/Components.hpp
    src/input/InputEvent.hpp
    src/map/generators/SimpleMapBuilder.hpp
    src/map/Map.hpp
    src/map/MapBuilderTypes.hpp
    src/map/Rect.hpp
    src/map/TileType.hpp
    src/render/Color.hpp
    src/render/ConsoleBuffer.hpp
    src/render/IRenderer.hpp
    src/render/SdlRenderer.hpp
    src/spawn/Spawner.hpp
    src/systems/InputSystem.hpp
    src/systems/MapRenderSystem.hpp
    src/systems/RenderSystem.hpp
)

set(ROGUELIKE_FORMAT_FILES
    ${ROGUELIKE_SOURCES}
    ${ROGUELIKE_HEADERS}
)

add_executable(roguelike ${ROGUELIKE_SOURCES})

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

find_program(CLANG_FORMAT_EXE NAMES clang-format clang-format.exe HINTS ${ROGUELIKE_LLVM_HINTS})
if(CLANG_FORMAT_EXE)
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror --style=file ${ROGUELIKE_FORMAT_FILES}
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

find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy.exe HINTS ${ROGUELIKE_LLVM_HINTS})
if(CLANG_TIDY_EXE AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    add_custom_target(lint
        COMMAND ${CLANG_TIDY_EXE} --quiet -p ${CMAKE_BINARY_DIR} ${ROGUELIKE_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS format-check
        COMMENT "Running clang-tidy"
    )

    if(ROGUELIKE_ENABLE_CLANG_TIDY)
        set_target_properties(roguelike PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
        )
    endif()
elseif(ROGUELIKE_ENABLE_CLANG_TIDY)
    message(FATAL_ERROR "ROGUELIKE_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found on PATH.")
elseif(CLANG_TIDY_EXE)
    add_custom_target(lint
        COMMAND ${CMAKE_COMMAND} -E echo "Use a Ninja or Makefile build directory for linting so compile_commands.json is available."
        COMMAND ${CMAKE_COMMAND} -E echo "Example: cmake -S . -B build-lint -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
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
add_test(NAME roguelike_smoke COMMAND roguelike --smoke)
