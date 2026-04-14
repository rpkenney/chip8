cmake_minimum_required(VERSION 3.16)
project(chip8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(chip8
    src/main.cpp
    src/memory.cpp
    src/io_glfw.cpp
    src/cpu.cpp
    src/glad.c
)

target_include_directories(chip8 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(chip8 PRIVATE glfw GL)
