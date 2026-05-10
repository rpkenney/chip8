#pragma once

#include <curses.h>

#include <string>
#include <vector>

#include "framebuffer.h"

#include "tui_support.h"

class Chip8CPU;
class Chip8Debugger;
class Chip8Memory;

namespace chip8_tui {

struct TuiFrame {
    WINDOW* game = nullptr;
    WINDOW* status = nullptr;
    WINDOW* log = nullptr;
    WINDOW* cmd = nullptr;
    int scale = 1;
    bool paused = false;
    bool log_pager_active = false;
    bool blink_on = false;
};

void drawAllPanels(const TuiFrame& frame, const Chip8FrameBuffer& fb,
                   const Chip8Debugger& dbg, const Chip8CPU& cpu, const Chip8Memory& mem,
                   const std::vector<std::string>& log_lines, const std::string& cmd_line,
                   TuiPagerState& pager);

}  // namespace chip8_tui
