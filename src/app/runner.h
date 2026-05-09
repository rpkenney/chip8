#ifndef CHIP8_RUNNER_H
#define CHIP8_RUNNER_H

#include <chrono>

class Chip8CPU;
class Chip8Memory;
class Chip8Debugger;

/// Per-iteration scheduler. Owns the 60 Hz CPU timer cadence and pumps the
/// debugger; the frontend owns the actual `while` loop and decides when to
/// render. All debug state (pacing, breakpoints, step-over, observer) lives
/// in `Chip8Debugger`.
class Chip8Runner {
public:
    static constexpr int FRAME_INTERVAL_MS = 16;  // ~60 Hz

    Chip8Runner(Chip8CPU& cpu, Chip8Memory& memory, Chip8Debugger& debugger);

    /// Runs one loop iteration: advances `cpu.timerTick()` if the 60 Hz
    /// boundary has elapsed and pumps the debugger. Returns true when the
    /// boundary was crossed (the caller renders on true).
    bool tick();

private:
    Chip8CPU& cpu;
    Chip8Memory& memory;
    Chip8Debugger& debugger;
    std::chrono::high_resolution_clock::time_point last_frame;
};

#endif
