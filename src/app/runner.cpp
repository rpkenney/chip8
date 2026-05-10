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
        // Only advance timers when the debugger is in auto-pacing mode (i.e., running).
        // When paused, timers should not decrement to maintain realistic behavior.
        if (debugger.pacing() == PausePacing::Auto) {
            cpu.timerTick();
        }
        last_frame = now;
        frame_elapsed = true;
        
        // Execute multiple instructions per frame based on speed.
        const int speed_hz = debugger.getInstructionSpeedHz();
        const int instructions_per_frame = std::max(1, speed_hz / 60);
        for (int i = 0; i < instructions_per_frame; ++i) {
            if (!debugger.executeOne(cpu, memory)) {
                break;
            }
        }
    }
    return frame_elapsed;
}
