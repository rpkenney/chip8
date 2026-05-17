#pragma once

#include <curses.h>

namespace chip8_tui {

struct TuiWindows {
    WINDOW* game_frame = nullptr;
    WINDOW* game = nullptr;
    WINDOW* status = nullptr;
    WINDOW* log = nullptr;
    WINDOW* cmd = nullptr;
    int term_rows = 0;
    int term_cols = 0;
    int panel_w = 0;
    int scale = 1;
    int game_rows = 0;
    int game_cols = 0;

    ~TuiWindows();
    bool layout();
};

bool waitForUsableTerminal(TuiWindows& ui);

}  // namespace chip8_tui
