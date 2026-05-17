#pragma once

#include <string>
#include <vector>

#include <chip8/frontends/tui/tui_constants.h>

namespace chip8_tui {

struct TuiPagerState {
    bool active = false;
    int first_visible = 0;
};

// Pager operations - centralized API
int logMaxFirstIndex(int log_count, int view_rows);
void clampPagerFirst(TuiPagerState& pager, int view_rows, int log_count);
void pagerFollowTailIfAtBottom(TuiPagerState& pager, int view_rows, int old_count, int new_count);

// Input handling for pager: returns true if key was consumed
bool handlePagerInput(int ch, TuiPagerState& pager, int view_rows, int log_count);

// String utilities
std::string trimCopy(std::string s);
void toLowerAscii(std::string& s);
void appendLogLine(std::vector<std::string>& log, std::string line);
void appendLogMultiline(std::vector<std::string>& log, const std::string& text);
bool parseU16(const std::string& s, std::uint16_t& out);

// Initialize color support (call once after initscr)
void initColors();

// Check if colors are available (cached)
bool hasColors();

}  // namespace chip8_tui
