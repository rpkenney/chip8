#include "runner.h"

#include "cpu.h"
#include "debugger.h"
#include "memory.h"

Chip8Runner::Chip8Runner(Chip8CPU& cpu, Chip8Memory& memory, Chip8Debugger& debugger)
    : cpu(cpu), memory(memory), debugger(debugger),
      last_frame(std::chrono::high_resolution_clock::now()) {}

bool Chip8Runner::tick() {
    bool frame_elapsed = false;
    const auto now = std::chrono::high_resolution_clock::now();
    if (now - last_frame > std::chrono::milliseconds(FRAME_INTERVAL_MS)) {
        cpu.timerTick();
        last_frame = now;
        frame_elapsed = true;
    }
    const bool ran_instruction = debugger.tick(cpu, memory);
    return frame_elapsed || ran_instruction;
}
