#ifndef CHIP8_TUI_FRONTEND_H
#define CHIP8_TUI_FRONTEND_H

class Chip8Emulator;

namespace tui_frontend {

/// Terminal (ncurses) frontend. Same contract as `glfw_frontend::run`.
int run(Chip8Emulator& emulator);

}  // namespace tui_frontend

#endif
