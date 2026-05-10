#pragma once

#include <string>
#include <vector>

#include <curses.h>

#include "tui_support.h"

class Chip8CPU;
class Chip8Debugger;
class Chip8Memory;
class Chip8KeypadState;

namespace chip8_tui {

void gatherWgetch(std::vector<int>& pending);
void syncChip8Keys(int ch, Chip8KeypadState& kp);

bool handlePausedKey(int ch, std::string& cmd_line, std::vector<std::string>& log, bool& quit,
                     TuiPagerState& pager, int log_view_rows, Chip8Debugger& debugger,
                     Chip8CPU& cpu, Chip8Memory& memory);

}  // namespace chip8_tui
