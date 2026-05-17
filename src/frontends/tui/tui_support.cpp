#include <chip8/frontends/tui/tui_support.h>

#include <algorithm>
#include <cctype>
#include <cstdint>

#include <curses.h>

namespace chip8_tui {

static bool g_has_colors = false;

void initColors() {
    g_has_colors = (has_colors() == TRUE);
}

bool hasColors() {
    return g_has_colors;
}

int logMaxFirstIndex(int log_count, int view_rows) {
    if (view_rows <= 0) {
        return 0;
    }
    return std::max(0, log_count - view_rows);
}

void clampPagerFirst(TuiPagerState& pager, int view_rows, int log_count) {
    const int mx = logMaxFirstIndex(log_count, view_rows);
    if (pager.first_visible < 0) {
        pager.first_visible = 0;
    }
    if (pager.first_visible > mx) {
        pager.first_visible = mx;
    }
}

bool handlePagerInput(int ch, TuiPagerState& pager, int view_rows, int log_count) {
    if (!pager.active) {
        return false;
    }
    clampPagerFirst(pager, view_rows, log_count);
    const int mx = logMaxFirstIndex(log_count, view_rows);
    const int page = std::max(1, view_rows);

    if (ch == 'q' || ch == 'Q' || ch == 27) {  // 27 = Esc
        pager.active = false;
        return true;
    }
    switch (ch) {
    case KEY_NPAGE:  // Page Down
        pager.first_visible = std::min(mx, pager.first_visible + page);
        return true;
    case KEY_PPAGE:  // Page Up
        pager.first_visible = std::max(0, pager.first_visible - page);
        return true;
    case KEY_HOME:
        pager.first_visible = 0;
        return true;
    case KEY_END:
        pager.first_visible = mx;
        return true;
    case KEY_DOWN:
    case 'j':
        pager.first_visible = std::min(mx, pager.first_visible + 1);
        return true;
    case KEY_UP:
    case 'k':
        pager.first_visible = std::max(0, pager.first_visible - 1);
        return true;
    case 'g':
        pager.first_visible = 0;
        return true;
    case 'G':
        pager.first_visible = mx;
        return true;
    default:
        return false;
    }
}

void pagerFollowTailIfAtBottom(TuiPagerState& pager, int view_rows, int old_count, int new_count) {
    if (!pager.active || view_rows <= 0 || new_count <= old_count) {
        return;
    }
    const int old_mx = logMaxFirstIndex(old_count, view_rows);
    if (old_count <= view_rows || pager.first_visible >= old_mx) {
        pager.first_visible = logMaxFirstIndex(new_count, view_rows);
    }
}

std::string trimCopy(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0) {
        s.pop_back();
    }
    return s;
}

void toLowerAscii(std::string& s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
}

void appendLogLine(std::vector<std::string>& log, std::string line) {
    log.push_back(std::move(line));
    if (log.size() > kLogMaxLines) {
        const std::size_t drop = log.size() - kLogMaxLines;
        log.erase(log.begin(), log.begin() + static_cast<std::ptrdiff_t>(drop));
    }
}

void appendLogMultiline(std::vector<std::string>& log, const std::string& text) {
    std::string::size_type pos = 0;
    while (pos < text.size()) {
        const auto end = text.find('\n', pos);
        if (end == std::string::npos) {
            appendLogLine(log, text.substr(pos));
            break;
        }
        appendLogLine(log, text.substr(pos, end - pos));
        pos = end + 1;
    }
}

bool parseU16(const std::string& s, std::uint16_t& out) {
    try {
        unsigned long v = 0;
        if (s.size() >= 3 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            v = std::stoul(s, nullptr, 16);
        } else {
            v = std::stoul(s, nullptr, 10);
        }
        if (v > 0xFFFFUL) {
            return false;
        }
        out = static_cast<std::uint16_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace chip8_tui
