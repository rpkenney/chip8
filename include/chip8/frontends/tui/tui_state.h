#pragma once

#include <string>
#include <vector>

#include <chip8/frontends/tui/tui_support.h>

namespace chip8_tui {

struct TuiState {
    std::vector<std::string> log_lines;
    std::string cmd_line;
    TuiPagerState pager;
    
    TuiState() : pager{} {}
};

}  // namespace chip8_tui
