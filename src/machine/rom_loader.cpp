#include <chip8/machine/rom_loader.h>

#include <chip8/machine/memory.h>

#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>

bool loadRomFromFile(Chip8Memory& mem, const char* path, std::string& error,
                       std::optional<chip8::debug_map::DebugMap>* out_debug_map) {
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
    file.seekg(0);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(buf.data()), size);
    if (!file) {
        error = "ROM read failure";
        return false;
    }

    const chip8::debug_map::ContainerProbe probe = chip8::debug_map::probe_container(buf.data(), buf.size());
    const std::size_t rom_len = probe.is_container ? probe.rom_size : buf.size();

    if (out_debug_map) {
        out_debug_map->reset();
    }

    if (probe.is_container && probe.debug_size > 0) {
        std::string parse_err;
        auto parsed = chip8::debug_map::parse_blob(buf.data() + probe.debug_offset, probe.debug_size,
                                                    &parse_err);
        if (!parsed) {
            error = std::string("ROM container has invalid debug map: ") + parse_err;
            return false;
        }
        if (out_debug_map) {
            *out_debug_map = std::move(*parsed);
        }
    }

    if (static_cast<std::size_t>(Chip8Memory::PROGRAM_START) + rom_len > Chip8Memory::MEMORY_SIZE) {
        error = "ROM too big for CHIP-8 memory";
        return false;
    }

    mem.writeBytes(Chip8Memory::PROGRAM_START, buf.data(), rom_len);
    return true;
}
