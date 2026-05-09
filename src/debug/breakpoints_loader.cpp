#include "breakpoints_loader.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>

static void trim_in_place(std::string& s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
}

bool loadBreakpointsFile(const char* path, std::unordered_set<std::uint16_t>& out, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot open breakpoints file";
        return false;
    }

    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        trim_in_place(line);
        if (const auto hash = line.find('#'); hash != std::string::npos) {
            line.resize(hash);
            trim_in_place(line);
        }
        if (line.empty()) {
            continue;
        }

        errno = 0;
        char* end = nullptr;
        const unsigned long value = std::strtoul(line.c_str(), &end, 0);
        if (end == line.c_str() || *end != '\0' || errno == ERANGE || value > 0xFFFF) {
            std::ostringstream oss;
            oss << path << ':' << line_no << ": invalid address \"" << line << '"';
            error = oss.str();
            return false;
        }
        out.insert(static_cast<std::uint16_t>(value));
    }
    return true;
}
