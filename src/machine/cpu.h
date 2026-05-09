#ifndef CHIP8_CPU_H
#define CHIP8_CPU_H

#include <array>
#include <cstdint>

#include "debug.h"

class Chip8Memory;
class Chip8IO;

struct Chip8CpuSnapshot {
    std::array<std::uint8_t, 16> v{};
    std::uint16_t I = 0;
    std::uint16_t pc = 0;
    std::uint8_t sp = 0;
    std::array<std::uint16_t, 16> stack{};
    std::uint8_t dt = 0;
    std::uint8_t st = 0;
};

class Chip8CPU {
public:
    Chip8CPU(Chip8Memory& mem, Chip8IO& io);

    void setTrace(Chip8DebugSink* sink, TraceLevel level);

    std::uint16_t getPC() const { return pc; }

    Chip8CpuSnapshot snapshot() const;

    void executeInstruction();
    void timerTick();

private:
    Chip8Memory& memory;
    Chip8IO& io;

    Chip8DebugSink* debug_sink = nullptr;
    TraceLevel trace_level = TraceLevel::Off;

    uint16_t pc;

    uint16_t stack[16];
    uint8_t sp;


    uint8_t reg[16];


    uint16_t I;

    uint8_t dt;
    uint8_t st;

};
#endif
