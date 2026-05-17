#ifndef CHIP8_ROM_LOADER_H
#define CHIP8_ROM_LOADER_H

#include <chip8/debug_map/debug_map.h>

#include <optional>
#include <string>

class Chip8Memory;

/// Load a CHIP-8 ROM into `mem` starting at `Chip8Memory::PROGRAM_START`.
/// Container files (footer `C8DB` + debug blob) load only the ROM slice into RAM;
/// if `out_debug_map` is non-null, parses the blob on success (fails the load if
/// the blob is corrupt). Plain `.bin` leaves `*out_debug_map` empty when provided.
bool loadRomFromFile(Chip8Memory& mem, const char* path, std::string& error,
                     std::optional<chip8::debug_map::DebugMap>* out_debug_map = nullptr);

#endif
