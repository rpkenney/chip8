#include "runner.h"
#include "cpu.h"
#include "io.h"

#include <chrono>

Chip8Runner::Chip8Runner(Chip8CPU& cpu, Chip8IO& io)
    : cpu(cpu), io(io) {}

void Chip8Runner::setStepMode(bool enabled) {
    step_mode = enabled;
}

void Chip8Runner::run() {
    auto last_frame = std::chrono::high_resolution_clock::now();
    auto last_instruction = last_frame;
    bool prev_step_down = false;

    while (!io.shouldClose()) {
        io.pollEvents();
        auto curr_time = std::chrono::high_resolution_clock::now();

        if (curr_time - last_frame > std::chrono::milliseconds(16)) {
            cpu.timerTick();
            io.render();
            last_frame = curr_time;
        }

        if (step_mode) {
            const bool step_down = io.isKeyPressed(0x10);  // runner-only "host" key
            if (step_down && !prev_step_down) {
                cpu.executeInstruction();
            }
            prev_step_down = step_down;
        } else {
            if (curr_time - last_instruction >= std::chrono::milliseconds(2)) {
                cpu.executeInstruction();
                last_instruction = curr_time;
            }
        }
    }
}
