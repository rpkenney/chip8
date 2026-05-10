#include "emulator.h"
#include "glfw_frontend.h"
#include "tui_frontend.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static void printUsage(const char* argv0) {
    std::fprintf(stderr, "usage: %s [--tui] [-b|--breakpoints FILE] ROM\n", argv0);
}

int main(int argc, char* argv[]) {
    Chip8EmulatorConfig cfg;
    bool use_tui = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--tui") == 0) {
            use_tui = true;
        } else if (std::strcmp(argv[i], "--breakpoints") == 0 ||
                   std::strcmp(argv[i], "-b") == 0) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s: %s requires an argument\n", argv[0], argv[i]);
                printUsage(argv[0]);
                return 1;
            }
            cfg.breakpoints_path = argv[++i];
        } else if (argv[i][0] == '-') {
            std::fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
            printUsage(argv[0]);
            return 1;
        } else if (cfg.rom_path == nullptr) {
            cfg.rom_path = argv[i];
        } else {
            std::fprintf(stderr, "%s: unexpected argument %s\n", argv[0], argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    if (cfg.rom_path == nullptr) {
        std::fprintf(stderr, "%s: missing ROM path\n", argv[0]);
        printUsage(argv[0]);
        return 1;
    }

    std::string err;
    std::unique_ptr<Chip8Emulator> emu = Chip8Emulator::create(cfg, err);
    if (emu == nullptr) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }

    if (use_tui) {
        return tui_frontend::run(*emu);
    }
    return glfw_frontend::run(*emu);
}
