#include <chip8/frontends/tui/tui_input.h>

#include <algorithm>
#include <cctype>
#include <vector>

#include <chip8/machine/cpu.h>
#include <chip8/debug/debugger.h>
#include <chip8/machine/keypad_state.h>
#include <chip8/machine/memory.h>
#include <chip8/app/runner.h>

#include <chip8/frontends/tui/tui_commands.h>
#include <chip8/frontends/tui/tui_constants.h>
#include <chip8/frontends/tui/tui_support.h>

namespace chip8_tui {

void syncChip8Keys(int ch, Chip8KeypadState& kp) {
    static const char kMap[16] = {'x', '1', '2', '3', 'q', 'w', 'e',
                                  'a', 's', 'd', 'z', 'c', '4', 'r', 'f', 'v'};
    for (int i = 0; i < 16; ++i) {
        const char k = kMap[i];
        if (ch == k || ch == static_cast<char>(std::toupper(static_cast<unsigned char>(k)))) {
            kp.setKey(i, true);
            return;
        }
    }
}

void gatherWgetch(std::vector<int>& pending) {
    pending.clear();
    const int ch = wgetch(stdscr);
    if (ch != ERR) {
        pending.push_back(ch);
    }
    wtimeout(stdscr, 0);
    for (;;) {
        const int q = wgetch(stdscr);
        if (q == ERR) {
            break;
        }
        pending.push_back(q);
    }
    wtimeout(stdscr, Chip8Runner::FRAME_INTERVAL_MS);
}

bool handlePausedKey(int ch, std::string& cmd_line, std::vector<std::string>& log, bool& quit,
                     TuiPagerState& pager, int log_view_rows, Chip8Debugger& debugger,
                     Chip8CPU& cpu, Chip8Memory& memory,
                     const chip8::debug_map::DebugMap* debug_map) {
    if (ch == '\r' || ch == '\n' || ch == KEY_ENTER) {
        const std::string t = trimCopy(cmd_line);
        if (t.empty()) {
            pager.active = false;
            debugger.requestResume();
        } else {
            dispatchTuiCommand(t, log, quit, pager, log_view_rows, debugger, cpu, memory,
                               debug_map);
        }
        cmd_line.clear();
        return true;
    }
    if (ch == 27) {
        cmd_line.clear();
        return true;
    }
    // Try pager input first if pager is active
    if (handlePagerInput(ch, pager, log_view_rows, static_cast<int>(log.size()))) {
        return true;
    }
    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == KEY_DC) {
        if (!cmd_line.empty()) {
            cmd_line.pop_back();
        }
        return true;
    }
    if (ch >= 32 && ch <= 126) {
        if (cmd_line.empty() && ch == ' ') {
            debugger.requestStep();
            return true;
        }
        if (cmd_line.size() < kCmdMaxLen) {
            cmd_line.push_back(static_cast<char>(ch));
        }
        return true;
    }
    return false;
}

}  // namespace chip8_tui
