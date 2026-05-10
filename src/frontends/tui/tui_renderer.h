#pragma once

#include <chrono>

#include "tui_draw.h"
#include "tui_state.h"
#include "tui_windows.h"

class Chip8CPU;
class Chip8Debugger;
class Chip8FrameBuffer;
class Chip8Memory;

namespace chip8_tui {

class TuiRenderer {
public:
    explicit TuiRenderer(TuiWindows& win);

    void renderFrame(const TuiState& state, const Chip8FrameBuffer& fb,
                     const Chip8Debugger& dbg, const Chip8CPU& cpu, const Chip8Memory& mem,
                     bool paused);

    void flushToTerminal() const;

private:
    TuiWindows& window;
    bool blink_on = false;
    std::chrono::steady_clock::time_point last_blink_check;
};

}  // namespace chip8_tui
