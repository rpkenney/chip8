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

class Chip8DebugSink {
public:
    virtual ~Chip8DebugSink() = default;

    virtual void onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) = 0;
    virtual void onBreakpointHit(std::uint16_t pc) {}
};

class PrintingDebugSink final : public Chip8DebugSink {
public:
    void onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) override;
    void onBreakpointHit(std::uint16_t pc) override {
        std::fprintf(stderr,
                     "BREAK PC=0x%04X (Space=step, Enter=resume run)\n",
                     pc);
    }
};

#endif
