#include "runner.h"

#include "cpu.h"
#include "debug_frame.h"
#include "debugger.h"
#include "format_debug_frame.h"
#include "io.h"
#include "memory.h"

#include <chrono>
#include <cstdio>

Chip8Runner::Chip8Runner(Chip8CPU& cpu, Chip8IO& io, Chip8Memory& memory,
                         Chip8Debugger& debugger)
    : cpu(cpu), io(io), memory(memory), debugger(debugger) {}

void Chip8Runner::run() {
    auto last_frame = std::chrono::high_resolution_clock::now();
    HostDebugKeys prev{};

    while (!io.shouldClose()) {
        io.pollEvents();

        const HostDebugKeys keys = io.readHostDebugKeys();
        if (keys.space && !prev.space) debugger.requestStep();
        if (keys.enter && !prev.enter) debugger.requestResume();
        if (keys.n && !prev.n) debugger.requestStepOver();
        if (keys.p && !prev.p && debugger.pacing() == PausePacing::Manual) {
            formatDebugFrame(debugger.captureFrame(cpu, memory), stderr);
        }
        prev = keys;

        const auto now = std::chrono::high_resolution_clock::now();
        if (now - last_frame > std::chrono::milliseconds(FRAME_INTERVAL_MS)) {
            cpu.timerTick();
            io.render();
            last_frame = now;
        }

        debugger.tick(cpu, memory);
    }
}
