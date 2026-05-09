#include "rom_loader.h"

#include "memory.h"

#include <cstdint>
#include <fstream>
#include <vector>

bool loadRomFromFile(Chip8Memory& mem, const char* path, std::string& error) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        error = std::string("cannot open ROM: ") + path;
        return false;
    }
    const std::streamsize size = file.tellg();
    if (size < 0) {
        error = "ROM tellg() failed";
        return false;
    }
    if (static_cast<std::size_t>(Chip8Memory::PROGRAM_START) +
            static_cast<std::size_t>(size) > Chip8Memory::MEMORY_SIZE) {
        error = "ROM too big for CHIP-8 memory";
        return false;
    }
    file.seekg(0);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(buf.data()), size);
    if (!file) {
        error = "ROM read failure";
        return false;
    }
    mem.writeBytes(Chip8Memory::PROGRAM_START, buf.data(), buf.size());
    return true;
}
