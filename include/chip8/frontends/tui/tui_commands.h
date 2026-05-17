#pragma once

#include <string>
#include <vector>

#include <chip8/frontends/tui/tui_support.h>

class Chip8CPU;
class Chip8Debugger;
class Chip8Memory;

namespace chip8::debug_map {
class DebugMap;
}

namespace chip8_tui {

void dispatchTuiCommand(const std::string& line, std::vector<std::string>& log, bool& quit,
                         TuiPagerState& pager, int log_view_rows, Chip8Debugger& debugger,
                         Chip8CPU& cpu, Chip8Memory& memory,
                         const chip8::debug_map::DebugMap* debug_map);

}  // namespace chip8_tui
