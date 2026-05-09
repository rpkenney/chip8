#ifndef CHIP8_GLFW_FRONTEND_H
#define CHIP8_GLFW_FRONTEND_H

namespace glfw_frontend {

struct Options {
    bool trace = false;
    bool step = false;
    const char* rom_path = nullptr;
    const char* breakpoints_path = nullptr;
};

/// Boots the GLFW + GL window, constructs the machine, and drives the main
/// loop until the window closes. Returns a process exit code.
int run(const Options& opts);

}  // namespace glfw_frontend

#endif
