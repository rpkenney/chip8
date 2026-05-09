#include "io_glfw.h"
#include "memory.h"
#include "cpu.h"
#include "runner.h"
#include "debug.h"

#include <cstring>

int main(int argc, char* argv[]) {
    bool trace = false;
    bool step = false;
    const char* rom_path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--trace") == 0) {
            trace = true;
        } else if (std::strcmp(argv[i], "--step") == 0) {
            step = true;
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

    Chip8Runner runner(cpu, io);
    runner.setStepMode(step);
    runner.run();

    return 0;
}
