#include "io_glfw.h"
#include "memory.h"
#include "cpu.h"
#include "runner.h"
#include "debug.h"
#include "breakpoints_loader.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>

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
                return 1;
            }
            breakpoints_path = argv[++i];
        } else if (rom_path == nullptr) {
            rom_path = argv[i];
        } else {
            return 1;
        }
    }
    if (rom_path == nullptr) {
        return 1;
    }

    Chip8IO_GLFW io(640, 320, "CHIP-8");
    Chip8Memory memory;
    Chip8CPU cpu(memory, io);

    PrintingDebugSink print_sink;
    if (trace) {
        cpu.setTrace(&print_sink, TraceLevel::Instructions);
    }

    memory.loadRom(rom_path);

    io.clearDisplay();

    std::unordered_set<std::uint16_t> breakpoints;
    if (breakpoints_path != nullptr) {
        std::string err;
        if (!loadBreakpointsFile(breakpoints_path, breakpoints, err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
    }

    Chip8Runner runner(cpu, io, memory);
    runner.setStepMode(step);
    runner.setDebugSink(&print_sink);
    runner.setBreakpoints(std::move(breakpoints));
    runner.run();

    return 0;
}
