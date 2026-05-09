#ifndef CHIP8_RUNNER_H
#define CHIP8_RUNNER_H

class Chip8CPU;
class Chip8IO;
class Chip8Memory;
class Chip8Debugger;

/// Owns the main loop and timing only: 16 ms timer/render cadence and pumping the
/// debugger every iteration. All debug state (pacing, breakpoints, step-over,
/// observer) lives in `Chip8Debugger`.
class Chip8Runner {
public:
    /// CHIP-8 timer/render rate is 60 Hz; 16 ms ≈ 1000/60 (the original
    /// hand-tuned value, kept verbatim so behavior matches prior commits).
    static constexpr int FRAME_INTERVAL_MS = 16;

    Chip8Runner(Chip8CPU& cpu, Chip8IO& io, Chip8Memory& memory, Chip8Debugger& debugger);

    void run();

private:
    Chip8CPU& cpu;
    Chip8IO& io;
    Chip8Memory& memory;
    Chip8Debugger& debugger;
};

#endif
