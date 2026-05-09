#include "io_glfw.h"
#include "memory.h"
#include "rom_loader.h"
#include "cpu.h"
#include "runner.h"
#include "debug.h"
#include "debugger.h"
#include "breakpoints_loader.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>

static void printUsage(const char* argv0) {
    std::fprintf(stderr,
                 "usage: %s [--trace] [--step] [-b|--breakpoints FILE] ROM\n",
                 argv0);
}

int main(int argc, char* argv[]) {
    bool trace = false;
    bool step = false;
    const char* rom_path = nullptr;
    const char* breakpoints_path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--trace") == 0) {
            trace = true;
        } else if (std::strcmp(argv[i], "--step") == 0) {
            step = true;
        } else if (std::strcmp(argv[i], "--breakpoints") == 0 || std::strcmp(argv[i], "-b") == 0) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s: %s requires an argument\n", argv[0], argv[i]);
                printUsage(argv[0]);
                return 1;
            }
            breakpoints_path = argv[++i];
        } else if (argv[i][0] == '-') {
            std::fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
            printUsage(argv[0]);
            return 1;
        } else if (rom_path == nullptr) {
            rom_path = argv[i];
        } else {
            std::fprintf(stderr, "%s: unexpected argument %s\n", argv[0], argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    if (rom_path == nullptr) {
        std::fprintf(stderr, "%s: missing ROM path\n", argv[0]);
        printUsage(argv[0]);
        return 1;
    }

    Chip8IO_GLFW io(640, 320, "CHIP-8");
    Chip8Memory memory;
    Chip8CPU cpu(memory, io);

    TerminalDebugObserver terminal_observer;

    {
        std::string err;
        if (!loadRomFromFile(memory, rom_path, err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
    }

    io.clearDisplay();

    std::unordered_set<std::uint16_t> breakpoints;
    if (breakpoints_path != nullptr) {
        std::string err;
        if (!loadBreakpointsFile(breakpoints_path, breakpoints, err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
    }

    Chip8Debugger debugger;
    debugger.setObserver(&terminal_observer);
    debugger.setStartPaused(step);
    if (trace) {
        debugger.setTraceLevel(TraceLevel::Instructions);
    }
    debugger.setBreakpoints(std::move(breakpoints));

    Chip8Runner runner(cpu, io, memory, debugger);
    runner.run();

    return 0;
}
