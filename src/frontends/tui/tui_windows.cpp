#include <chip8/frontends/tui/tui_windows.h>

#include <chip8/app/runner.h>

#include <chip8/frontends/tui/tui_constants.h>

namespace chip8_tui {

TuiWindows::~TuiWindows() {
    if (cmd != nullptr) {
        delwin(cmd);
    }
    if (log != nullptr) {
        delwin(log);
    }
    if (status != nullptr) {
        delwin(status);
    }
    if (game != nullptr) {
        delwin(game);
    }
    if (game_frame != nullptr) {
        delwin(game_frame);
    }
}

bool TuiWindows::layout() {
    getmaxyx(stdscr, term_rows, term_cols);
    const int gh_unit = kHalfRows;  // Always Unicode (16 rows)
    const int log_h = term_rows - kStatusLines - kCmdLines;
    constexpr int kBorder = 2;

    if (log_h < kMinLogRows || term_rows < gh_unit + kBorder) {
        return false;
    }

    const int inner_h_max = term_rows - kBorder;
    const int s_max_h = inner_h_max / gh_unit;
    if (s_max_h < 1) {
        return false;
    }

    int scale_found = 0;
    for (int s = s_max_h; s >= 1; --s) {
        const int game_rows_try = gh_unit * s;
        const int game_cols_try = kFbW * s;
        const int frame_w_try = game_cols_try + kBorder;
        const int frame_h_try = game_rows_try + kBorder;
        const int panel_w_try = term_cols - frame_w_try;
        
        if (panel_w_try < kPanelMinCols) {
            continue;
        }
        if (frame_h_try > term_rows) {
            continue;
        }
        
        const int frame_y_try = (term_rows - frame_h_try) / 2;
        if (frame_y_try < 0 || frame_w_try > term_cols) {
            continue;
        }
        
        scale_found = s;
        break;
    }

    if (scale_found < 1) {
        return false;
    }

    scale = scale_found;
    game_rows = gh_unit * scale;
    game_cols = kFbW * scale;
    const int frame_h = game_rows + kBorder;
    const int frame_w = game_cols + kBorder;
    panel_w = term_cols - frame_w;
    const int frame_y = (term_rows - frame_h) / 2;
    constexpr int frame_x = 0;
    const int panel_x = frame_w;
    if (frame_x + frame_w > term_cols || panel_x + panel_w > term_cols || frame_y < 0) {
        return false;
    }

    const int cmd_y = term_rows - kCmdLines;

    if (game_frame == nullptr) {
        game_frame = newwin(frame_h, frame_w, frame_y, frame_x);
        if (game_frame == nullptr) {
            return false;
        }
        game = derwin(game_frame, game_rows, game_cols, 1, 1);
        status = newwin(kStatusLines, panel_w, 0, panel_x);
        log = newwin(log_h, panel_w, kStatusLines, panel_x);
        cmd = newwin(kCmdLines, panel_w, cmd_y, panel_x);
        if (game == nullptr || status == nullptr || log == nullptr || cmd == nullptr) {
            delwin(game_frame);
            game_frame = nullptr;
            return false;
        }
        box(game_frame, 0, 0);
        keypad(game, TRUE);
        keypad(cmd, TRUE);
    } else {
        if (game != nullptr) {
            delwin(game);
            game = nullptr;
        }
        wresize(game_frame, frame_h, frame_w);
        mvwin(game_frame, frame_y, frame_x);
        werase(game_frame);
        box(game_frame, 0, 0);
        game = derwin(game_frame, game_rows, game_cols, 1, 1);
        if (game == nullptr) {
            return false;
        }
        wresize(status, kStatusLines, panel_w);
        wresize(log, log_h, panel_w);
        wresize(cmd, kCmdLines, panel_w);
        mvwin(status, 0, panel_x);
        mvwin(log, kStatusLines, panel_x);
        mvwin(cmd, cmd_y, panel_x);
        keypad(game, TRUE);
        werase(status);
        werase(log);
        werase(cmd);
    }
    return true;
}

bool waitForUsableTerminal(TuiWindows& ui) {
    wtimeout(stdscr, -1);
    for (;;) {
        werase(stdscr);
        printw("Terminal too small.\n"
               "Resize the window.\n");
        refresh();
        const int ch = wgetch(stdscr);
        if (ch == 'q' || ch == 'Q') {
            wtimeout(stdscr, Chip8Runner::FRAME_INTERVAL_MS);
            return false;
        }
        if (ch == KEY_RESIZE) {
#if defined(NCURSES_VERSION)
            (void)resize_term(0, 0);
#endif
            if (ui.layout()) {
                wtimeout(stdscr, Chip8Runner::FRAME_INTERVAL_MS);
                werase(stdscr);
                refresh();
                return true;
            }
        }
    }
}

}  // namespace chip8_tui
