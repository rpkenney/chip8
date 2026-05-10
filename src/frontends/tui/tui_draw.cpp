#include "tui_draw.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "debug_frame.h"
#include "debugger.h"

#include "tui_constants.h"

namespace chip8_tui {

void drawAllPanels(const TuiFrame& frame, const Chip8FrameBuffer& fb,
                   const Chip8Debugger& dbg, const Chip8CPU& cpu, const Chip8Memory& mem,
                   const std::vector<std::string>& log_lines, const std::string& cmd_line,
                   TuiPagerState& pager) {
    const bool colors = hasColors();

    // Render game framebuffer
    {
        WINDOW* w = frame.game;
        werase(w);
        const std::uint8_t* px = fb.pixels();
        const bool colors = hasColors();

        if (frame.scale == 1) {
            // Scale 1: Each terminal row encodes 2 CHIP-8 rows via half-block glyphs
            for (int row = 0; row < kHalfRows; ++row) {
                for (int col = 0; col < kFbW; ++col) {
                    const bool top = px[static_cast<std::size_t>((row * 2) * kFbW + col)] != 0;
                    const bool bot = px[static_cast<std::size_t>((row * 2 + 1) * kFbW + col)] != 0;
                    wchar_t wc[2] = {L' ', 0};
                    if (top && bot)       wc[0] = L'\u2588';  // Full block
                    else if (top)         wc[0] = L'\u2580';  // Upper half
                    else if (bot)         wc[0] = L'\u2584';  // Lower half
                    const bool lit = top || bot;
                    if (lit && colors) wattron(w, COLOR_PAIR(kPairLit));
                    mvwaddnwstr(w, row, col, wc, 1);
                    if (lit && colors) wattroff(w, COLOR_PAIR(kPairLit));
                }
            }
        } else {
            // Scale > 1: Each CHIP-8 pixel becomes scale×scale terminal cells using full blocks
            const int win_rows = kHalfRows * frame.scale;
            const int win_cols = kFbW * frame.scale;
            for (int ty = 0; ty < win_rows; ++ty) {
                const int chip8_row = ty * kFbH / win_rows;  // Map terminal row to CHIP-8 row
                for (int tx = 0; tx < win_cols; ++tx) {
                    const int chip8_col = tx / frame.scale;
                    const bool on = px[static_cast<std::size_t>(chip8_row * kFbW + chip8_col)] != 0;
                    wchar_t wc[2] = {on ? L'\u2588' : L' ', 0};  // Full block or space
                    if (on && colors) wattron(w, COLOR_PAIR(kPairLit));
                    mvwaddnwstr(w, ty, tx, wc, 1);
                    if (on && colors) wattroff(w, COLOR_PAIR(kPairLit));
                }
            }
        }
    }

    // Render status line
    {
        WINDOW* w = frame.status;
        werase(w);
        int cols = getmaxx(w);
        if (cols < 1) cols = 1;
        const Chip8DebugFrame debug_frame = dbg.captureFrame(cpu, mem);
        char mn[28] = {};
        std::strncpy(mn, debug_frame.mnemonic.c_str(), sizeof(mn) - 1);
        char line[320];
        std::snprintf(line, sizeof(line), "==== PC=%04X %04X %s I=%03X SP%u D%02X S%02X ====",
                      debug_frame.cpu.pc, debug_frame.opcode, mn, debug_frame.cpu.I,
                      static_cast<unsigned>(debug_frame.cpu.sp),
                      static_cast<unsigned>(debug_frame.cpu.dt),
                      static_cast<unsigned>(debug_frame.cpu.st));
        const int clip = (std::min)(cols, static_cast<int>(sizeof(line)) - 1);
        if (static_cast<int>(std::strlen(line)) > clip) {
            line[static_cast<std::size_t>(clip)] = '\0';
        }
        mvwprintw(w, 0, 0, "%s", line);
    }

    // Render log
    {
        WINDOW* w = frame.log;
        werase(w);
        const int max_rows = getmaxy(w);
        const int max_cols = getmaxx(w);
        if (max_rows > 0 && max_cols > 0) {
            const int n = static_cast<int>(log_lines.size());
            clampPagerFirst(pager, max_rows, n);
            int start = 0;
            if (!pager.active) {
                start = logMaxFirstIndex(n, max_rows);
            } else {
                start = pager.first_visible;
            }
            const int n_show = std::min(max_rows, n - start);
            for (int i = 0; i < n_show; ++i) {
                const std::string& raw = log_lines[static_cast<std::size_t>(start + i)];
                std::string clip = raw;
                if (static_cast<int>(clip.size()) > max_cols) {
                    clip.resize(static_cast<std::size_t>(max_cols));
                }
                mvwprintw(w, i, 0, "%s", clip.c_str());
            }
        }
    }

    // Render command line
    {
        WINDOW* w = frame.cmd;
        curs_set(0);
        werase(w);
        int rows = 0, cols = 0;
        getmaxyx(w, rows, cols);
        (void)rows;

        if (!frame.paused) {
            if (cols > 4) {
                const char* msg = "[running] F1=pause | keys -> CHIP-8";
                std::string clip(msg);
                if (static_cast<int>(clip.size()) > cols - 1) {
                    clip.resize(static_cast<std::size_t>(cols - 1));
                }
                mvwprintw(w, 0, 0, "%s", clip.c_str());
            }
        } else if (frame.log_pager_active) {
            const char* msg = "PAGER  q/Esc  PgUp/PgDn  j/k  g/G";
            std::string clip(msg);
            if (static_cast<int>(clip.size()) > cols - 1) {
                clip.resize(static_cast<std::size_t>(cols - 1));
            }
            mvwprintw(w, 0, 0, "%s", clip.c_str());
        } else {
            const char* prefix = ":";
            const int prefix_len = 1;
            const int budget = cols - prefix_len;
            if (budget <= 0) {
                mvwprintw(w, 0, 0, "%s", prefix);
            } else {
                std::string tail = cmd_line;
                if (static_cast<int>(tail.size()) > budget) {
                    tail.erase(0, tail.size() - static_cast<std::size_t>(budget));
                }
                mvwprintw(w, 0, 0, "%s%s", prefix, tail.c_str());
                const int cx = (std::min)(cols - 1, prefix_len + static_cast<int>(tail.size()));
                wmove(w, 0, cx);
                curs_set(frame.blink_on ? 1 : 0);
            }
        }
    }
}

}  // namespace chip8_tui
