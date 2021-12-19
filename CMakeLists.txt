cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(chip8 C CXX)

file(GLOB_RECURSE SOURCES src/*.cc src/*.hh)

add_executable(chip8 ${SOURCES})

set_property(TARGET chip8 PROPERTY CXX_STANDARD 23)
set_property(TARGET chip8 PROPERTY ENABLE_EXPORTS 1)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE /std:c++latest")
    set_property(TARGET chip8 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_compile_options("/MP")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b -lstdc++fs")
endif()

find_package(SDL2 CONFIG REQUIRED)
find_library(SDL2_LIBRARY SDL2)
find_path(SDL_INCLUDE_DIR SDL2/SDL.h)

target_include_directories(chip8 PUBLIC src)
target_include_directories(chip8 SYSTEM PRIVATE ${SDL_INCLUDE_DIR})

target_link_libraries(
        chip8
        ${SDL2_LIBRARY}
)
