#include <chip8/frontends/tui/tui_frontend.h>

#include <chip8/machine/cpu.h>
#include <chip8/debug/debugger.h>
#include <chip8/app/emulator.h>
#include <chip8/debug_map/debug_map.h>
#include <chip8/machine/framebuffer.h>
#include <chip8/machine/keypad_state.h>
#include <chip8/machine/memory.h>
#include <chip8/app/runner.h>

#if CHIP8_HAVE_NCURSES
#include <curses.h>

#include <chrono>
#include <clocale>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

#include <chip8/frontends/tui/tui_constants.h>
#include <chip8/frontends/tui/tui_draw.h>
#include <chip8/frontends/tui/tui_input.h>
#include <chip8/frontends/tui/tui_support.h>
#include <chip8/frontends/tui/tui_windows.h>
#endif

namespace tui_frontend {

int run(Chip8Emulator& emulator) {
#if CHIP8_HAVE_NCURSES
    using namespace chip8_tui;

    std::setlocale(LC_ALL, "");
    if (initscr() == nullptr) {
        return 1;
    }
    if (has_colors() == TRUE) {
        start_color();
        if (use_default_colors() == OK) {
            init_pair(kPairLit, COLOR_CYAN, -1);
        } else {
            init_pair(kPairLit, COLOR_CYAN, COLOR_BLACK);
        }
    }
    initColors();  // Cache color capability
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(Chip8Runner::FRAME_INTERVAL_MS);

    Chip8Memory& memory = emulator.memory();
    Chip8FrameBuffer& fb = emulator.framebuffer();
    Chip8KeypadState& kp = emulator.keypad();
    Chip8CPU& cpu = emulator.cpu();
    Chip8Debugger& debugger = emulator.debugger();
    Chip8Runner& runner = emulator.runner();

    std::vector<std::string> log_lines;
    std::string cmd_line;
    TuiPagerState pager;
    bool quit = false;

    {
        TuiWindows ui{};
        if (!ui.layout()) {
            if (!waitForUsableTerminal(ui)) {
                endwin();
                return 0;
            }
        }

        std::vector<int> pending;
        pending.reserve(32);

        bool layout_ok = true;
        while (!quit) {
            wtimeout(stdscr, layout_ok ? Chip8Runner::FRAME_INTERVAL_MS : -1);
            gatherWgetch(pending);
            bool resized_ok = false;
            std::vector<bool> skip_chip8(pending.size(), false);
            const int log_h = layout_ok ? getmaxy(ui.log) : 0;
            const int log_sz_before = static_cast<int>(log_lines.size());

            for (std::size_t i = 0; i < pending.size(); ++i) {
                const int ch = pending[i];
                if (ch == KEY_RESIZE) {
#if defined(NCURSES_VERSION)
                    (void)resize_term(0, 0);
#endif
                    if (!ui.layout()) {
                        layout_ok = false;
                    } else {
                        layout_ok = true;
                        resized_ok = true;
                    }
                    skip_chip8[i] = true;
                    continue;
                }
                if (!layout_ok) {
                    skip_chip8[i] = true;
                    continue;
                }
                const bool running = debugger.pacing() == PausePacing::Auto;
                if (running) {
                    if (ch == 'p' || ch == 'P') {
                        debugger.requestPause();
                        skip_chip8[i] = true;
                    }
                    continue;
                }
                const auto& dm_opt = emulator.debugMap();
                const chip8::debug_map::DebugMap* const dm_ptr =
                    dm_opt.has_value() ? &*dm_opt : nullptr;
                if (handlePausedKey(ch, cmd_line, log_lines, quit, pager, log_h, debugger, cpu,
                                    memory, dm_ptr)) {
                    skip_chip8[i] = true;
                }
            }

            if (!layout_ok) {
                werase(stdscr);
                printw("Terminal too small.\n"
                       "Resize the window.\n");
                refresh();
                runner.tick();
                continue;
            }

            if (resized_ok) {
                werase(stdscr);
                clearok(stdscr, TRUE);
                if (ui.game_frame != nullptr) {
                    touchwin(ui.game_frame);
                }
                if (ui.game != nullptr) {
                    touchwin(ui.game);
                }
                if (ui.status != nullptr) {
                    touchwin(ui.status);
                }
                if (ui.log != nullptr) {
                    touchwin(ui.log);
                }
                if (ui.cmd != nullptr) {
                    touchwin(ui.cmd);
                }
            }

            pagerFollowTailIfAtBottom(pager, log_h, log_sz_before, static_cast<int>(log_lines.size()));

            kp.clear();
            for (std::size_t i = 0; i < pending.size(); ++i) {
                if (skip_chip8[i]) {
                    continue;
                }
                const int ch = pending[i];
                if (ch == ERR) {
                    continue;
                }
                syncChip8Keys(ch, kp);
            }

            const bool redraw = runner.tick();
            if (ui.game_frame != nullptr) {
                box(ui.game_frame, 0, 0);
            }
            const bool paused = debugger.pacing() == PausePacing::Paused;
            const auto blink_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now().time_since_epoch())
                                      .count();
            const bool blink_on = (blink_ms / 400) % 2 == 0;

            TuiFrame frame{};
            frame.game = ui.game;
            frame.status = ui.status;
            frame.log = ui.log;
            frame.cmd = ui.cmd;
            frame.scale = ui.scale;
            frame.paused = paused;
            frame.log_pager_active = pager.active;
            frame.blink_on = blink_on;
            const auto& dm_opt = emulator.debugMap();
            const chip8::debug_map::DebugMap* const dm_ptr =
                dm_opt.has_value() ? &*dm_opt : nullptr;
            drawAllPanels(frame, fb, debugger, cpu, memory, log_lines, cmd_line, pager, dm_ptr);

            if (resized_ok) {
                wnoutrefresh(stdscr);
                clearok(stdscr, FALSE);
            }
            wnoutrefresh(ui.game);
            wnoutrefresh(ui.game_frame);
            wnoutrefresh(ui.status);
            wnoutrefresh(ui.log);
            wnoutrefresh(ui.cmd);
            doupdate();
        }
    }

    curs_set(1);
    endwin();
    return 0;
#else
    (void)emulator;
    std::fprintf(stderr,
                 "chip8: --tui requires a build with wide ncurses "
                 "(e.g. libncursesw5-dev), then reconfigure CMake.\n");
    return 1;
#endif
}

}  // namespace tui_frontend
