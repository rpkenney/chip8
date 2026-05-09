#include "runner.h"
#include "cpu.h"
#include "debug.h"
#include "io.h"

#include <chrono>

Chip8Runner::Chip8Runner(Chip8CPU& cpu, Chip8IO& io)
    : cpu(cpu), io(io) {}

void Chip8Runner::setStepMode(bool enabled) {
    step_cli = enabled;
    auto_pacing = !enabled;
}

void Chip8Runner::setDebugSink(Chip8DebugSink* sink) {
    debug_sink = sink;
}

void Chip8Runner::setBreakpoints(std::unordered_set<std::uint16_t> addresses) {
    breakpoints = std::move(addresses);
}

void Chip8Runner::run() {
    auto last_frame = std::chrono::high_resolution_clock::now();
    auto last_instruction = last_frame;
    bool prev_step_down = false;
    bool prev_continue_down = false;

    while (!io.shouldClose()) {
        io.pollEvents();
        auto curr_time = std::chrono::high_resolution_clock::now();

        if (curr_time - last_frame > std::chrono::milliseconds(16)) {
            cpu.timerTick();
            io.render();
            last_frame = curr_time;
        }

        const bool step_down = io.isKeyPressed(0x10);   // Space
        const bool continue_down = io.isKeyPressed(0x11);  // Enter

        if (!auto_pacing && !step_cli) {
            if (continue_down && !prev_continue_down) {
                auto_pacing = true;
                skip_breakpoint_once = true;
            }
            prev_continue_down = continue_down;
        } else {
            prev_continue_down = false;
        }

        bool do_execute = false;

        if (!auto_pacing) {
            if (step_down && !prev_step_down) {
                do_execute = true;
            }
        } else if (curr_time - last_instruction >= std::chrono::milliseconds(2)) {
            const std::uint16_t pc = cpu.getPC();
            if (!breakpoints.empty() && breakpoints.count(pc) != 0) {
                if (skip_breakpoint_once) {
                    skip_breakpoint_once = false;
                } else {
                    auto_pacing = false;
                    if (debug_sink != nullptr) {
                        debug_sink->onBreakpointHit(pc);
                    }
                }
            }

            if (auto_pacing) {
                do_execute = true;
            }
        }

        if (do_execute) {
            cpu.executeInstruction();
            last_instruction = curr_time;
        }

        prev_step_down = step_down;
    }
}
