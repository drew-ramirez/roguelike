cmake_minimum_required(VERSION 3.21)

project(roguelike VERSION 0.1.0 LANGUAGES CXX)

add_executable(roguelike
    src/main.cpp
)

target_compile_features(roguelike PRIVATE cxx_std_20)
set_target_properties(roguelike PROPERTIES
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(roguelike PRIVATE /W4 /permissive-)
else()
    target_compile_options(roguelike PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME roguelike_smoke COMMAND roguelike)
