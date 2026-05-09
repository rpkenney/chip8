#ifndef CHIP8_CPU_H
#define CHIP8_CPU_H

#include <cstdint>

#include "debug.h"

class Chip8Memory;
class Chip8IO;

class Chip8CPU {
public:
    Chip8CPU(Chip8Memory& mem, Chip8IO& io);

    void setTrace(Chip8DebugSink* sink, TraceLevel level);

    std::uint16_t getPC() const { return pc; }

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
