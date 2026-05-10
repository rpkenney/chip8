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

# Dear ImGui: vendored via FetchContent. ImGui ships no CMakeLists.txt, so we
# pull the sources and add them to the chip8 target directly.
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4-docking
)
FetchContent_MakeAvailable(imgui)

set(CURSES_NEED_WIDE TRUE)
find_package(Curses QUIET)

# Main executable and every translation unit that links into it.
add_executable(chip8
    src/main.cpp
    src/app/emulator.cpp
    src/app/runner.cpp
    src/debug/breakpoints_loader.cpp
    src/debug/debugger.cpp
    src/debug/disassemble_chip8.cpp
    src/machine/framebuffer.cpp
    src/machine/keypad_state.cpp
    src/machine/memory.cpp
    src/machine/rom_loader.cpp
    src/frontends/glfw/glfw_frontend.cpp
    src/frontends/glfw/imgui_panels.cpp
    src/frontends/tui/tui_frontend.cpp
    src/frontends/tui/tui_support.cpp
    src/frontends/tui/tui_commands.cpp
    src/frontends/tui/tui_draw.cpp
    src/frontends/tui/tui_windows.cpp
    src/frontends/tui/tui_input.cpp
    src/frontends/tui/tui_renderer.cpp
    src/machine/cpu.cpp
    src/glad.c

    # Dear ImGui core + GLFW/OpenGL3 backends.
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Per-module include roots (do not add src/ itself: a src/debug dir would shadow
# libstdc++ headers such as <debug/safe_iterator.h> when -I src is used).
target_include_directories(chip8 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/machine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/glfw
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/tui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debug
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

if(CURSES_FOUND)
    target_sources(chip8 PRIVATE
        src/frontends/tui/tui_draw.cpp
        src/frontends/tui/tui_input.cpp
        src/frontends/tui/tui_windows.cpp
    )
    target_compile_definitions(chip8 PRIVATE CHIP8_HAVE_NCURSES=1)
    target_link_libraries(chip8 PRIVATE ${CURSES_LIBRARIES} glfw GL)
    if(CURSES_INCLUDE_DIR)
        target_include_directories(chip8 PRIVATE ${CURSES_INCLUDE_DIR})
    elseif(CURSES_INCLUDE_PATH)
        target_include_directories(chip8 PRIVATE ${CURSES_INCLUDE_PATH})
    endif()
else()
    target_compile_definitions(chip8 PRIVATE CHIP8_HAVE_NCURSES=0)
    target_link_libraries(chip8 PRIVATE glfw GL)
    message(WARNING "Wide ncurses not found (e.g. libncursesw5-dev). "
                    "Windowed `chip8` still builds; `chip8 --tui` will fail until ncurses is available.")
endif()
