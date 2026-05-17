#ifndef CHIP8_GLFW_FRONTEND_H
#define CHIP8_GLFW_FRONTEND_H

class Chip8Emulator;

namespace glfw_frontend {

/// Boots the GLFW + GL window and drives the main loop until the window
/// closes. The emulator (RAM, CPU, runner, …) must be constructed by the
/// caller so other frontends can share the same composition root.
int run(Chip8Emulator& emulator);

}  // namespace glfw_frontend

#endif
