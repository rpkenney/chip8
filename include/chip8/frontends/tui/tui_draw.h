#pragma once

#include <curses.h>

#include <string>
#include <vector>

#include <chip8/machine/framebuffer.h>

#include <chip8/frontends/tui/tui_support.h>

class Chip8CPU;
class Chip8Debugger;
class Chip8Memory;

namespace chip8::debug_map {
class DebugMap;
}

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
                   TuiPagerState& pager, const chip8::debug_map::DebugMap* debug_map);

}  // namespace chip8_tui
