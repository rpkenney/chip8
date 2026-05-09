#ifndef CHIP8_RUNNER_H
#define CHIP8_RUNNER_H

#include <cstdint>
#include <unordered_set>

class Chip8CPU;
class Chip8IO;
class Chip8Memory;
class Chip8DebugSink;

class Chip8Runner {
public:
    Chip8Runner(Chip8CPU& cpu, Chip8IO& io, Chip8Memory& memory);

    void setStepMode(bool enabled);
    void setDebugSink(Chip8DebugSink* sink);
    void setBreakpoints(std::unordered_set<std::uint16_t> addresses);

    void run();

private:
    Chip8CPU& cpu;
    Chip8IO& io;
    Chip8Memory& memory;
    Chip8DebugSink* debug_sink = nullptr;

    /// Set from `--step`: manual pacing only; Enter never resumes timer-driven run.
    bool step_cli = false;
    /// When false, only Space advances the PC (breakpoint or `--step`). When true, the 2 ms timer runs the CPU (subject to breakpoints).
    bool auto_pacing = true;
    std::unordered_set<std::uint16_t> breakpoints;
    bool skip_breakpoint_once = false;
};

#endif
