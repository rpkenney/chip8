#ifndef CHIP8_ROM_LOADER_H
#define CHIP8_ROM_LOADER_H

#include <string>

class Chip8Memory;

/// Load a CHIP-8 ROM into `mem` starting at `Chip8Memory::PROGRAM_START`. On
/// failure returns false and writes a human-readable reason to `error` (mirrors
/// `loadBreakpointsFile`). Keeps `Chip8Memory` free of file IO.
bool loadRomFromFile(Chip8Memory& mem, const char* path, std::string& error);

#endif
