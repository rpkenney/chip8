#ifndef CHIP8_DEBUG_H
#define CHIP8_DEBUG_H

#include <cstdint>
#include <cstdio>

enum class TraceLevel : std::uint8_t {
    Off = 0,
    Instructions = 1 << 0,
    Timers = 1 << 1,
    IO = 1 << 2,
    Verbose = 1 << 3,
};

constexpr TraceLevel operator|(TraceLevel a, TraceLevel b) {
    return static_cast<TraceLevel>(static_cast<std::uint8_t>(a) |
                                   static_cast<std::uint8_t>(b));
}

constexpr TraceLevel operator&(TraceLevel a, TraceLevel b) {
    return static_cast<TraceLevel>(static_cast<std::uint8_t>(a) &
                                   static_cast<std::uint8_t>(b));
}

inline bool trace_enabled(TraceLevel mask, TraceLevel bit) {
    return (static_cast<std::uint8_t>(mask) & static_cast<std::uint8_t>(bit)) != 0;
}

/// Why we switched to manual pacing. `pc` is the next instruction to run. Only
/// `Breakpoint` and `StepOverComplete` are emitted today; the rest are reserved.
enum class PauseReason {
    Breakpoint,
    StepComplete,
    StepOverComplete,
    UserPause,
};

class Chip8DebugObserver {
public:
    virtual ~Chip8DebugObserver() = default;

    /// Per-instruction trace event. Default: no-op.
    virtual void onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) {}
    /// Unified pause notification. Default: no-op.
    virtual void onPaused(PauseReason reason, std::uint16_t pc) {}
};

class TerminalDebugObserver final : public Chip8DebugObserver {
public:
    void onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) override;
    void onPaused(PauseReason reason, std::uint16_t pc) override {
        switch (reason) {
            case PauseReason::Breakpoint:
                std::fprintf(stderr,
                             "BREAK PC=0x%04X (Space=step, N=step over, Enter=resume run, P=dump when paused)\n",
                             pc);
                break;
            case PauseReason::StepOverComplete:
                std::fprintf(stderr, "STEP OVER -> PC=0x%04X\n", pc);
                break;
            case PauseReason::StepComplete:
            case PauseReason::UserPause:
                break;
        }
    }
};

#endif
