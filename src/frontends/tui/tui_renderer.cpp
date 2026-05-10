#include "tui_renderer.h"

#include <curses.h>
#include <chrono>

#include "cpu.h"
#include "debugger.h"
#include "framebuffer.h"
#include "memory.h"

namespace chip8_tui {

TuiRenderer::TuiRenderer(TuiWindows& win)
    : window(win),
      last_blink_check(std::chrono::steady_clock::now()) {
}

void TuiRenderer::renderFrame(const TuiState& state, const Chip8FrameBuffer& fb,
                              const Chip8Debugger& dbg, const Chip8CPU& cpu, const Chip8Memory& mem,
                              bool paused) {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_blink_check).count();
    if (elapsed_ms > 0) {
        last_blink_check = now;
        const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        blink_on = (total_ms / 400) % 2 == 0;
    }

    TuiFrame frame{};
    frame.game = window.game;
    frame.status = window.status;
    frame.log = window.log;
    frame.cmd = window.cmd;
    frame.scale = window.scale;
    frame.paused = paused;
    frame.log_pager_active = state.pager.active;
    frame.blink_on = blink_on;

    drawAllPanels(frame, fb, dbg, cpu, mem, state.log_lines, state.cmd_line,
                  const_cast<TuiPagerState&>(state.pager));
}

void TuiRenderer::flushToTerminal() const {
    wnoutrefresh(window.game);
    wnoutrefresh(window.game_frame);
    wnoutrefresh(window.status);
    wnoutrefresh(window.log);
    wnoutrefresh(window.cmd);
    doupdate();
}

}  // namespace chip8_tui

