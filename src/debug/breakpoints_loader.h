#ifndef CHIP8_BREAKPOINTS_LOADER_H
#define CHIP8_BREAKPOINTS_LOADER_H

#include <cstdint>
#include <string>
#include <unordered_set>

bool loadBreakpointsFile(const char* path, std::unordered_set<std::uint16_t>& out, std::string& error);

#endif
