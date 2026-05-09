# Refuse to configure with an older CMake than this project was written for.
cmake_minimum_required(VERSION 3.16)

# Defines the project name and which languages CMake should enable for targets.
project(chip8 LANGUAGES C CXX)

# Use ISO C++17 for .cpp files (Chip8CPU, IO, etc.).
set(CMAKE_CXX_STANDARD 17)
# glad.c is C; match a modern C standard for consistency with the toolchain.
set(CMAKE_C_STANDARD 17)
# Emit compile_commands.json in the build dir (useful for clangd and other tools).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Main executable and every translation unit that links into it.
add_executable(chip8
    src/main.cpp
    src/runner.cpp
    src/debug.cpp
    src/memory.cpp
    src/io_glfw.cpp
    src/cpu.cpp
    src/glad.c
)

# Header search paths: repo-root include/ (glad, KHR) and src/ for local headers.
# PRIVATE: only this target uses these includes, not dependents.
target_include_directories(chip8 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link GLFW (window, context, input) and GL (OpenGL on Unix; CMake's FindOpenGL module names it GL).
target_link_libraries(chip8 PRIVATE glfw GL)
