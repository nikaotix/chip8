cmake_minimum_required(VERSION 4.0)

project(Chip8Ingrassia)

include_directories(inc)

add_executable(Chip8)

set_property(TARGET Chip8 PROPERTY C_STANDARD 23)
set_property(TARGET Chip8 PROPERTY C_STANDARD_REQUIRED TRUE)

target_sources(Chip8
    PRIVATE
    chip8.c
    sound.c
    main.c
)

# Disable AVIF Support
set(SDL3IMAGE_AVIF OFF)

# Enable FetchContent
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG origin/main
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(SDL3)

target_link_libraries(Chip8 PRIVATE SDL3::SDL3)