#include <chip8/app/emulator.h>

#include <chip8/debug/breakpoints_loader.h>
#include <chip8/machine/rom_loader.h>

#include <utility>
#include <unordered_set>

std::unique_ptr<Chip8Emulator> Chip8Emulator::create(const Chip8EmulatorConfig& cfg,
                                                    std::string& err) {
    std::unique_ptr<Chip8Emulator> emu(new Chip8Emulator());
    if (!emu->load(cfg, err)) {
        return nullptr;
    }
    return emu;
}

bool Chip8Emulator::load(const Chip8EmulatorConfig& cfg, std::string& err) {
    debug_map_.reset();
    if (!loadRomFromFile(memory_, cfg.rom_path, err, &debug_map_)) {
        return false;
    }

    std::unordered_set<std::uint16_t> breakpoints;
    if (cfg.breakpoints_path != nullptr) {
        if (!loadBreakpointsFile(cfg.breakpoints_path, breakpoints, err)) {
            return false;
        }
    }

    debugger_.setBreakpoints(std::move(breakpoints));
    return true;
}
